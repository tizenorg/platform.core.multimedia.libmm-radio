/*
 * libmm-radio
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: JongHyuk Choi <jhchoi.choi@samsung.com>, YoungHwan An <younghwan_.an@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/*===========================================================================================
|																							|
|  INCLUDE FILES																			|
|																							|
========================================================================================== */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#include <mm_error.h>
#include <mm_debug.h>
#include <mm_message.h>
#include <time.h>

#include <mm_sound.h>
#include <mm_sound_focus.h>
#include <mm_sound_device.h>

#include "mm_radio_priv.h"

/*===========================================================================================
  LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE
========================================================================================== */
/*---------------------------------------------------------------------------
    GLOBAL CONSTANT DEFINITIONS:
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    IMPORTED VARIABLE DECLARATIONS:
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    IMPORTED FUNCTION DECLARATIONS:
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    LOCAL #defines:
---------------------------------------------------------------------------*/
#define TUNER_INDEX				0

#define DEFAULT_FREQ				107700

#define FREQ_FRAC				16
#define RADIO_FREQ_FORMAT_SET(x_freq)		((x_freq) * FREQ_FRAC)
#define RADIO_FREQ_FORMAT_GET(x_freq)		((x_freq) / FREQ_FRAC)
/* If non-zero, wrap around when at the end of the frequency range, else stop seeking */
#define DEFAULT_WRAP_AROUND		1

#define RADIO_DEFAULT_REGION	MM_RADIO_REGION_GROUP_USA
#define EMULATOR_FREQ_MAX		5

/*---------------------------------------------------------------------------
    LOCAL CONSTANT DEFINITIONS:
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    LOCAL DATA TYPE DEFINITIONS:
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    GLOBAL VARIABLE DEFINITIONS:
---------------------------------------------------------------------------*/
extern int errno;

/*---------------------------------------------------------------------------
    LOCAL VARIABLE DEFINITIONS:
---------------------------------------------------------------------------*/
/* radio region configuration table */
static const MMRadioRegion_t region_table[] = {
	{							/* Notrh America, South America, South Korea, Taiwan, Australia */
	 MM_RADIO_REGION_GROUP_USA,	/* region type */
	 MM_RADIO_DEEMPHASIS_75_US,	/* de-emphasis */
	 MM_RADIO_FREQ_MIN_87500_KHZ,	/* min freq. */
	 MM_RADIO_FREQ_MAX_108000_KHZ,	/* max freq. */
	 50,
	 },
	{							/* China, Europe, Africa, Middle East, Hong Kong, India, Indonesia, Russia, Singapore */
	 MM_RADIO_REGION_GROUP_EUROPE,
	 MM_RADIO_DEEMPHASIS_50_US,
	 MM_RADIO_FREQ_MIN_87500_KHZ,
	 MM_RADIO_FREQ_MAX_108000_KHZ,
	 50,
	 },
	{
	 MM_RADIO_REGION_GROUP_JAPAN,
	 MM_RADIO_DEEMPHASIS_50_US,
	 MM_RADIO_FREQ_MIN_76100_KHZ,
	 MM_RADIO_FREQ_MAX_89900_KHZ,
	 50,
	 },
};

static int MMRadioEmulatorFreq[EMULATOR_FREQ_MAX] = {
	89100, 89900, 91900, 99900, 107700
};

static int EmultatorIdx = 0;

/*---------------------------------------------------------------------------
    LOCAL FUNCTION PROTOTYPES:
---------------------------------------------------------------------------*/
static bool __mmradio_post_message(mm_radio_t * radio, enum MMMessageType msgtype, MMMessageParamType * param);
static int __mmradio_check_state(mm_radio_t * radio, MMRadioCommand command);
static int __mmradio_get_state(mm_radio_t * radio);
static bool __mmradio_set_state(mm_radio_t * radio, int new_state);
static void __mmradio_seek_thread(mm_radio_t * radio);
static void __mmradio_scan_thread(mm_radio_t * radio);
static bool __is_tunable_frequency(mm_radio_t * radio, int freq);
static int __mmradio_set_deemphasis(mm_radio_t * radio);
static int __mmradio_set_band_range(mm_radio_t * radio);
static int __mmradio_get_wave_num(mm_radio_t * radio);
static void __mmradio_sound_focus_cb(int id, mm_sound_focus_type_e focus_type,
	mm_sound_focus_state_e focus_state, const char *reason_for_change,
	const char *additional_info, void *user_data);
static void __mmradio_device_connected_cb(MMSoundDevice_t device, bool is_connected, void *user_data);

/*===========================================================================
  FUNCTION DEFINITIONS
========================================================================== */
/* --------------------------------------------------------------------------
 * Name   : _mmradio_apply_region()
 * Desc   : update radio region information and set values to device
 * Param  :
 *		[in] radio : radio handle
 *		[in] region : region type
 *		[in] update : update region values or not
 * Return : zero on success, or negative value with error code
 *---------------------------------------------------------------------------*/
int _mmradio_apply_region(mm_radio_t * radio, MMRadioRegionType region, bool update)
{
	int ret = MM_ERROR_NONE;
	int count = 0;
	int index = 0;

	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_SET_REGION);

	/* if needed, radio region must be updated.
	 * Otherwise, just applying settings to device without it.
	 */
	if (update) {
		count = ARRAY_SIZE(region_table);

		/* TODO: if auto is supported...get the region info. here */

		/* update radio region settings */
		for (index = 0; index < count; index++) {
			/* find the region from pre-defined table */
			if (region_table[index].country == region) {
				radio->region_setting.country = region_table[index].country;
				radio->region_setting.deemphasis = region_table[index].deemphasis;
				radio->region_setting.band_min = region_table[index].band_min;
				radio->region_setting.band_max = region_table[index].band_max;
				radio->region_setting.channel_spacing = region_table[index].channel_spacing;
			}
		}
	}

	/* chech device is opened or not. if it's not ready, skip to apply region to device now */
	if (radio->radio_fd < 0) {
		MMRADIO_LOG_DEBUG("not opened device. just updating region info. \n");
		return MM_ERROR_NONE;
	}

	MMRADIO_SLOG_DEBUG("setting region - country: %d, de-emphasis: %d, band range: %d ~ %d KHz\n",
		radio->region_setting.country, radio->region_setting.deemphasis,
		radio->region_setting.band_min, radio->region_setting.band_max);

	/* set de-emphsasis to device */
	ret = __mmradio_set_deemphasis(radio);

	MMRADIO_CHECK_RETURN_IF_FAIL(ret, "set de-emphasis");

	/* set band range to device */
	ret = __mmradio_set_band_range(radio);

	MMRADIO_CHECK_RETURN_IF_FAIL(ret, "set band range");

	MMRADIO_LOG_FLEAVE();

	return ret;
}

int _mmradio_create_radio(mm_radio_t * radio)
{
	int ret = 0;

	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_CREATE);

	/* set default value */
	radio->radio_fd = -1;
	radio->freq = DEFAULT_FREQ;
	memset(&radio->region_setting, 0, sizeof(MMRadioRegion_t));
	radio->subs_id = 0;

	/* create command lock */
	ret = pthread_mutex_init(&radio->cmd_lock, NULL);
	if (ret) {
		MMRADIO_LOG_ERROR("mutex creation failed\n");
		return MM_ERROR_RADIO_INTERNAL;
	}

	MMRADIO_SET_STATE(radio, MM_RADIO_STATE_NULL);

	/* add device conneted callback */
	ret = mm_sound_add_device_connected_callback(MM_SOUND_DEVICE_STATE_ACTIVATED_FLAG,
		(mm_sound_device_connected_cb)__mmradio_device_connected_cb,
		(void *)radio, &radio->subs_id);
	if (ret) {
		MMRADIO_LOG_ERROR("mm_sound_add_device_connected_callback is failed\n");
		return MM_ERROR_RADIO_INTERNAL;
	}

	/* register to audio focus */
	ret = mmradio_audio_focus_register(&radio->sm, __mmradio_sound_focus_cb, (void *)radio);
	if (ret) {
		/* NOTE : we are dealing it as an error since we cannot expect it's behavior */
		MMRADIO_LOG_ERROR("mmradio_audio_focus_register is failed\n");
		return MM_ERROR_RADIO_INTERNAL;
	}

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

int _mmradio_realize(mm_radio_t * radio)
{
	int ret = MM_ERROR_NONE;

	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_REALIZE);

	/* open radio device */
	if (radio->radio_fd == -1) {
		MMRadioRegionType region = MM_RADIO_REGION_GROUP_NONE;
		bool update = false;

		/* open device */
		radio->radio_fd = 11;

		MMRADIO_LOG_DEBUG("radio device fd : %d\n", radio->radio_fd);

		/* check region country type if it's updated or not */
		if (radio->region_setting.country == MM_RADIO_REGION_GROUP_NONE) {
			/* not initialized  yet. set it with default region */
			region = RADIO_DEFAULT_REGION;
			update = true;
		} else {
			/* already initialized by application */
			region = radio->region_setting.country;
		}

		ret = _mmradio_apply_region(radio, region, update);

		MMRADIO_CHECK_RETURN_IF_FAIL(ret, "update region info");
	}

	/* ready but nosound */
	/*  if( _mmradio_mute(radio) != MM_ERROR_NONE) */
	/*      goto error; */

	MMRADIO_SET_STATE(radio, MM_RADIO_STATE_READY);
#ifdef USE_GST_PIPELINE
	ret = _mmradio_realize_pipeline(radio);
	if (ret) {
		debug_error("_mmradio_realize_pipeline is failed\n");
		return ret;
	}
#endif
	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;

/* error:
	if (radio->radio_fd >= 0) {
		close(radio->radio_fd);
		radio->radio_fd = -1;
	}

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_RADIO_INTERNAL;
*/
}

int _mmradio_unrealize(mm_radio_t * radio)
{
	int ret = MM_ERROR_NONE;

	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_UNREALIZE);

	/*  if( _mmradio_mute(radio) != MM_ERROR_NONE) */
	/*      return MM_ERROR_RADIO_NOT_INITIALIZED; */

	/* close radio device here !!!! */
	if (radio->radio_fd >= 0) {
		close(radio->radio_fd);
		radio->radio_fd = -1;
	}

	MMRADIO_SET_STATE(radio, MM_RADIO_STATE_NULL);
	ret = mmradio_release_audio_focus(&radio->sm);
	if (ret) {
		MMRADIO_LOG_ERROR("mmradio_release_audio_focus is failed\n");
		return ret;
	}
#ifdef USE_GST_PIPELINE
	ret = _mmradio_destroy_pipeline(radio);
	if (ret) {
		MMRADIO_LOG_ERROR("_mmradio_destroy_pipeline is failed\n");
		return ret;
	}
#endif

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

int _mmradio_destroy(mm_radio_t * radio)
{
	int ret = 0;
	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_DESTROY);

	ret = mmradio_audio_focus_deregister(&radio->sm);
	if (ret) {
		MMRADIO_LOG_ERROR("failed to deregister audio focus\n");
		return MM_ERROR_RADIO_INTERNAL;
	}

	ret = mm_sound_remove_device_connected_callback(radio->subs_id);
	if (ret) {
		MMRADIO_LOG_ERROR("mm_sound_remove_device_connected_callback error %d\n", ret);
		return MM_ERROR_RADIO_INTERNAL;
	}
	_mmradio_unrealize(radio);

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

/* unit should be KHz */
int _mmradio_set_frequency(mm_radio_t * radio, int freq)
{
	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_SET_FREQ);

	MMRADIO_SLOG_DEBUG("Setting %d frequency\n", freq);
	MMRADIO_LOG_DEBUG("radio->freq: %d freq: %d\n", radio->freq, freq);

	if (radio->radio_fd < 0) {
		MMRADIO_LOG_DEBUG("radio device is not opened yet\n");
		return MM_ERROR_NONE;
	}

	/* check frequency range */
	if (freq < radio->region_setting.band_min || freq > radio->region_setting.band_max) {
		MMRADIO_LOG_ERROR("out of frequency range\n", freq);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	radio->freq = freq;

#ifdef USE_GST_PIPELINE
	if (radio->pGstreamer_s) {
		int val = 0;
		val = __mmradio_get_wave_num(radio);
		g_object_set(radio->pGstreamer_s->audiosrc, "wave", val, NULL);
	}
#endif
	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;

}

int _mmradio_get_frequency(mm_radio_t * radio, int *pFreq)
{

	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_GET_FREQ);

	return_val_if_fail(pFreq, MM_ERROR_INVALID_ARGUMENT);

	/* just return stored frequency if radio device is not ready */
	if (radio->radio_fd < 0) {
		MMRADIO_SLOG_DEBUG("freq : %d\n", radio->freq);
		*pFreq = radio->freq;
		return MM_ERROR_NONE;
	}
	/* update freq in handle */

	*pFreq = radio->freq;

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

int _mmradio_mute(mm_radio_t * radio)
{
	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_MUTE);

	if (radio->radio_fd < 0)
		return MM_ERROR_RADIO_NOT_INITIALIZED;

#ifdef USE_GST_PIPELINE
	if (radio->pGstreamer_s) {
		g_object_set(radio->pGstreamer_s->volume, "mute", 1, NULL);
		MMRADIO_LOG_DEBUG("g_object set mute\n");
	}
#endif
	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;

}

int _mmradio_unmute(mm_radio_t * radio)
{
	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_UNMUTE);
	MMRADIO_CHECK_DEVICE_STATE(radio);

#ifdef USE_GST_PIPELINE
	if (radio->pGstreamer_s) {
		g_object_set(radio->pGstreamer_s->volume, "mute", 0, NULL);
		MMRADIO_LOG_DEBUG("g_object set  un-mute\n");
	}
#endif

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

/* --------------------------------------------------------------------------
 * Name   : __mmradio_set_deemphasis
 * Desc   : apply de-emphasis value to device
 * Param  :
 *	    [in] radio : radio handle
 * Return : zero on success, or negative value with error code
 *---------------------------------------------------------------------------*/
int __mmradio_set_deemphasis(mm_radio_t * radio)
{
	int value = 0;

	MMRADIO_LOG_FENTER();
	return MM_ERROR_NONE;
	MMRADIO_CHECK_INSTANCE(radio);

	/* get de-emphasis */
	switch (radio->region_setting.deemphasis) {
	case MM_RADIO_DEEMPHASIS_50_US:
		/* V4L2_DEEMPHASIS_50_uS; */
		value = 1;
		break;

	case MM_RADIO_DEEMPHASIS_75_US:
		/* V4L2_DEEMPHASIS_75_uS; */
		value = 2;
		break;

	default:
		MMRADIO_LOG_ERROR("not availabe de-emphasis value\n");
		return MM_ERROR_COMMON_INVALID_ARGUMENT;
	}

	/* set it to device */
	/* V4L2_CID_TUNE_DEEMPHASIS; */
	(radio->vctrl).id = (0x009d0000 | 0x900) + 1;
	(radio->vctrl).value = value;

	if (ioctl(radio->radio_fd, VIDIOC_S_CTRL, &(radio->vctrl)) < 0) {
		MMRADIO_LOG_ERROR("failed to set de-emphasis\n");
		return MM_ERROR_RADIO_INTERNAL;
	}

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

/* --------------------------------------------------------------------------
 * Name   : __mmradio_set_band_range
 * Desc   : apply max and min frequency to device
 * Param  :
 *	    [in] radio : radio handle
 * Return : zero on success, or negative value with error code
 *---------------------------------------------------------------------------*/
int __mmradio_set_band_range(mm_radio_t * radio)
{
	MMRADIO_LOG_FENTER();
	return MM_ERROR_NONE;
	MMRADIO_CHECK_INSTANCE(radio);

	/* get min and max freq. */
	(radio->vt).rangelow = RADIO_FREQ_FORMAT_SET(radio->region_setting.band_min);
	(radio->vt).rangehigh = RADIO_FREQ_FORMAT_SET(radio->region_setting.band_max);

	/* set it to device */
	if (ioctl(radio->radio_fd, VIDIOC_S_TUNER, &(radio->vt)) < 0) {
		MMRADIO_LOG_ERROR("failed to set band range\n");
		return MM_ERROR_RADIO_INTERNAL;
	}

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

int _mmradio_set_message_callback(mm_radio_t * radio, MMMessageCallback callback, void *user_param)
{
	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);

	radio->msg_cb = callback;
	radio->msg_cb_param = user_param;

	MMRADIO_LOG_DEBUG("msg_cb : 0x%x msg_cb_param : 0x%x\n", (unsigned int)callback, (unsigned int)user_param);

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

int _mmradio_get_state(mm_radio_t * radio, int *pState)
{
	int state = 0;

	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	return_val_if_fail(pState, MM_ERROR_INVALID_ARGUMENT);

	state = __mmradio_get_state(radio);

	*pState = state;

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

int _mmradio_start(mm_radio_t * radio)
{
	int ret = MM_ERROR_NONE;

	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_START);

	MMRADIO_SLOG_DEBUG("now tune to frequency : %d\n", radio->freq);

	ret = mmradio_acquire_audio_focus(&radio->sm);
	if (ret) {
		MMRADIO_LOG_ERROR("failed to set audio focus\n");
		return ret;
	}

	/* set stored frequency */
	_mmradio_set_frequency(radio, radio->freq);

	/* unmute */
	/*  if( _mmradio_unmute(radio) != MM_ERROR_NONE) */
	/*      return MM_ERROR_RADIO_NOT_INITIALIZED; */

	MMRADIO_SET_STATE(radio, MM_RADIO_STATE_PLAYING);
#ifdef USE_GST_PIPELINE
	ret = _mmradio_start_pipeline(radio);
	if (ret) {
		debug_error("_mmradio_start_pipeline is failed\n");
		return ret;
	}
#endif

	MMRADIO_LOG_FLEAVE();

	return ret;
}

int _mmradio_stop(mm_radio_t * radio)
{
	int ret = MM_ERROR_NONE;

	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_STOP);

	/*  if( _mmradio_mute(radio) != MM_ERROR_NONE) */
	/*      return MM_ERROR_RADIO_NOT_INITIALIZED; */

	MMRADIO_SET_STATE(radio, MM_RADIO_STATE_READY);

#ifdef USE_GST_PIPELINE
	ret = _mmradio_stop_pipeline(radio);
	if (ret) {
		debug_error("_mmradio_stop_pipeline is failed\n");
		return ret;
	}
#endif

	MMRADIO_LOG_FLEAVE();

	return ret;
}

#ifdef USE_GST_PIPELINE
int _mmradio_realize_pipeline(mm_radio_t * radio)
{
	int ret = MM_ERROR_NONE;
	int val = 0;
	MMRADIO_LOG_FENTER();
	gst_init(NULL, NULL);
	radio->pGstreamer_s = g_new0(mm_radio_gstreamer_s, 1);

	radio->pGstreamer_s->pipeline = gst_pipeline_new("fmradio");

	radio->pGstreamer_s->audiosrc = gst_element_factory_make("audiotestsrc", "fm audio src");
	radio->pGstreamer_s->queue2 = gst_element_factory_make("queue2", "queue2");
	radio->pGstreamer_s->volume = gst_element_factory_make("volume", "volume");
	radio->pGstreamer_s->audiosink = gst_element_factory_make("pulsesink", "audio sink");

	val = __mmradio_get_wave_num(radio);
	g_object_set(radio->pGstreamer_s->audiosrc, "wave", val, "volume", 0.8, NULL);

	if (!radio->pGstreamer_s->pipeline || !radio->pGstreamer_s->audiosrc || !radio->pGstreamer_s->queue2 || !radio->pGstreamer_s->volume || !radio->pGstreamer_s->audiosink) {
		MMRADIO_LOG_DEBUG("[%s][%05d] One element could not be created. Exiting.\n", __func__, __LINE__);
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}

	gst_bin_add_many(GST_BIN(radio->pGstreamer_s->pipeline),
	                 radio->pGstreamer_s->audiosrc,
	                 radio->pGstreamer_s->queue2,
	                 radio->pGstreamer_s->volume,
	                 radio->pGstreamer_s->audiosink,
	                 NULL);
	if (!gst_element_link_many(
	        radio->pGstreamer_s->audiosrc,
	        radio->pGstreamer_s->queue2,
	        radio->pGstreamer_s->volume,
	        radio->pGstreamer_s->audiosink,
	        NULL)) {
		MMRADIO_LOG_DEBUG(, "[%s][%05d] Fail to link b/w appsrc and ffmpeg in rotate\n", __func__, __LINE__);
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}
	MMRADIO_LOG_FLEAVE();
	return ret;
}

int _mmradio_start_pipeline(mm_radio_t * radio)
{
	int ret = MM_ERROR_NONE;
	GstStateChangeReturn ret_state;

	MMRADIO_LOG_FENTER();

	if (gst_element_set_state(radio->pGstreamer_s->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
		MMRADIO_LOG_DEBUG("Fail to change pipeline state");
		gst_object_unref(radio->pGstreamer_s->pipeline);
		g_free(radio->pGstreamer_s);
		return MM_ERROR_RADIO_INVALID_STATE;
	}

	ret_state = gst_element_get_state(radio->pGstreamer_s->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
	if (ret_state == GST_STATE_CHANGE_FAILURE) {
		MMRADIO_LOG_DEBUG("GST_STATE_CHANGE_FAILURE");
		gst_object_unref(radio->pGstreamer_s->pipeline);
		g_free(radio->pGstreamer_s);
		return MM_ERROR_RADIO_INVALID_STATE;
	} else {
		MMRADIO_LOG_DEBUG("[%s][%05d] GST_STATE_NULL ret_state = %d (GST_STATE_CHANGE_SUCCESS)\n", __func__, __LINE__, ret_state);
	}

	MMRADIO_LOG_FLEAVE();
	return ret;
}

int _mmradio_stop_pipeline(mm_radio_t * radio)
{
	int ret = MM_ERROR_NONE;
	GstStateChangeReturn ret_state;

	MMRADIO_LOG_FENTER();
	if (gst_element_set_state(radio->pGstreamer_s->pipeline, GST_STATE_READY) == GST_STATE_CHANGE_FAILURE) {
		MMRADIO_LOG_DEBUG("Fail to change pipeline state");
		gst_object_unref(radio->pGstreamer_s->pipeline);
		g_free(radio->pGstreamer_s);
		return MM_ERROR_RADIO_INVALID_STATE;
	}

	ret_state = gst_element_get_state(radio->pGstreamer_s->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
	if (ret_state == GST_STATE_CHANGE_FAILURE) {
		MMRADIO_LOG_DEBUG("GST_STATE_CHANGE_FAILURE");
		gst_object_unref(radio->pGstreamer_s->pipeline);
		g_free(radio->pGstreamer_s);
		return MM_ERROR_RADIO_INVALID_STATE;
	} else {
		MMRADIO_LOG_DEBUG("[%s][%05d] GST_STATE_NULL ret_state = %d (GST_STATE_CHANGE_SUCCESS)\n", __func__, __LINE__, ret_state);
	}
	MMRADIO_LOG_FLEAVE();
	return ret;
}

int _mmradio_destroy_pipeline(mm_radio_t * radio)
{
	int ret = 0;
	GstStateChangeReturn ret_state;
	MMRADIO_LOG_FENTER();

	if (gst_element_set_state(radio->pGstreamer_s->pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
		MMRADIO_LOG_DEBUG("Fail to change pipeline state");
		gst_object_unref(radio->pGstreamer_s->pipeline);
		g_free(radio->pGstreamer_s);
		return MM_ERROR_RADIO_INVALID_STATE;
	}

	ret_state = gst_element_get_state(radio->pGstreamer_s->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
	if (ret_state == GST_STATE_CHANGE_FAILURE) {
		MMRADIO_LOG_DEBUG("GST_STATE_CHANGE_FAILURE");
		gst_object_unref(radio->pGstreamer_s->pipeline);
		g_free(radio->pGstreamer_s);
		return MM_ERROR_RADIO_INVALID_STATE;
	} else {
		MMRADIO_LOG_DEBUG("[%s][%05d] GST_STATE_NULL ret_state = %d (GST_STATE_CHANGE_SUCCESS)\n", __func__, __LINE__, ret_state);
	}

	gst_object_unref(radio->pGstreamer_s->pipeline);
	g_free(radio->pGstreamer_s);
	MMRADIO_LOG_FLEAVE();
	return ret;
}
#endif

int _mmradio_seek(mm_radio_t * radio, MMRadioSeekDirectionType direction)
{
	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_SEEK);

	int ret = 0;

	/*  if( _mmradio_mute(radio) != MM_ERROR_NONE) */
	/*      return MM_ERROR_RADIO_NOT_INITIALIZED; */

	MMRADIO_SLOG_DEBUG("trying to seek. direction[0:UP/1:DOWN) %d\n", direction);
	radio->seek_direction = direction;

	ret = pthread_create(&radio->seek_thread, NULL, (void *)__mmradio_seek_thread, (void *)radio);

	if (ret) {
		MMRADIO_LOG_DEBUG("failed create thread\n");
		return MM_ERROR_RADIO_INTERNAL;
	}

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

int _mmradio_start_scan(mm_radio_t * radio)
{
	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_START_SCAN);

	int scan_tr_id = 0;

	radio->stop_scan = false;

	scan_tr_id = pthread_create(&radio->scan_thread, NULL, (void *)__mmradio_scan_thread, (void *)radio);

	if (scan_tr_id != 0) {
		MMRADIO_LOG_DEBUG("failed to create thread : scan\n");
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}

	MMRADIO_SET_STATE(radio, MM_RADIO_STATE_SCANNING);

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

int _mmradio_stop_scan(mm_radio_t * radio)
{
	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_STOP_SCAN);

	radio->stop_scan = true;

	if (radio->scan_thread > 0) {
		pthread_cancel(radio->scan_thread);
		pthread_join(radio->scan_thread, NULL);
		radio->scan_thread = 0;
	}

	MMRADIO_SET_STATE(radio, MM_RADIO_STATE_READY);
	MMRADIO_POST_MSG(radio, MM_MESSAGE_RADIO_SCAN_STOP, NULL);

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

int _mm_radio_get_signal_strength(mm_radio_t * radio, int *value)
{
	MMRADIO_LOG_FENTER();
	MMRADIO_CHECK_INSTANCE(radio);

	return_val_if_fail(value, MM_ERROR_INVALID_ARGUMENT);

	/* just return stored frequency if radio device is not ready */
	if (radio->radio_fd < 0) {
		MMRADIO_SLOG_DEBUG("Device not ready so sending 0\n");
		*value = 0;
		return MM_ERROR_NONE;
	}

	unsigned int seed = (unsigned)time(NULL);
	*value = 0 - ((rand_r(&seed) % 20 + 1) + 80);
	MMRADIO_LOG_FLEAVE();
	return MM_ERROR_NONE;
}

void __mmradio_scan_thread(mm_radio_t * radio)
{

	int prev_freq = 0;
	EmultatorIdx = 0;

	MMRADIO_LOG_FENTER();
	MMRADIO_CHECK_INSTANCE_RETURN_VOID(radio);
	/*  if( _mmradio_mute(radio) != MM_ERROR_NONE) */
	/*      goto FINISHED; */

	if (_mmradio_set_frequency(radio, radio->region_setting.band_min) != MM_ERROR_NONE)
		goto FINISHED;

	MMRADIO_POST_MSG(radio, MM_MESSAGE_RADIO_SCAN_START, NULL);
	MMRADIO_SET_STATE(radio, MM_RADIO_STATE_SCANNING);

	while (!radio->stop_scan) {
		int freq = 0;
		MMMessageParamType param = { 0, };

		MMRADIO_LOG_DEBUG("scanning....\n");

		/* now we can get new frequency from radio device */

		if (radio->stop_scan)
			break;
		{
			usleep(1000 * 1000);
			freq = MMRadioEmulatorFreq[EmultatorIdx];
			MMRADIO_LOG_DEBUG("freq: %d", freq);

			if (freq < prev_freq) {
				MMRADIO_LOG_DEBUG("scanning wrapped around. stopping scan\n");
				break;
			}

			if (freq == prev_freq)
				continue;

			prev_freq = param.radio_scan.frequency = freq;
			MMRADIO_SLOG_DEBUG("scanning : new frequency : [%d]\n", param.radio_scan.frequency);

			/* drop if max freq is scanned */
			if (param.radio_scan.frequency == radio->region_setting.band_max
			    || param.radio_scan.frequency > radio->region_setting.band_max
			    || param.radio_scan.frequency < radio->region_setting.band_min) {
				MMRADIO_LOG_DEBUG("%d freq is dropping...and stopping scan\n", param.radio_scan.frequency);
				break;
			}

			if (radio->stop_scan)
				break;			/* doesn't need to post */

			MMRADIO_POST_MSG(radio, MM_MESSAGE_RADIO_SCAN_INFO, &param);
			EmultatorIdx++;
		}
	}
 FINISHED:
	radio->scan_thread = 0;

	MMRADIO_SET_STATE(radio, MM_RADIO_STATE_READY);

	if (!radio->stop_scan)
		MMRADIO_POST_MSG(radio, MM_MESSAGE_RADIO_SCAN_FINISH, NULL);

	MMRADIO_LOG_FLEAVE();

	pthread_exit(NULL);

	return;
}

bool __is_tunable_frequency(mm_radio_t * radio, int freq)
{
	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);

	if (freq == radio->region_setting.band_max || freq == radio->region_setting.band_min)
		return false;

	MMRADIO_LOG_FLEAVE();

	return true;
}

void __mmradio_seek_thread(mm_radio_t * radio)
{
	int ret = 0;
	int freq = 0;
	bool seek_stop = false;
	MMMessageParamType param = { 0, };
	struct v4l2_hw_freq_seek vs = { 0, };

	vs.tuner = TUNER_INDEX;
	vs.type = V4L2_TUNER_RADIO;
	vs.wrap_around = DEFAULT_WRAP_AROUND;

	MMRADIO_LOG_FENTER();
	MMRADIO_CHECK_INSTANCE_RETURN_VOID(radio);

	/* check direction */
	switch (radio->seek_direction) {
	case MM_RADIO_SEEK_UP:
		vs.seek_upward = 1;
		break;
	default:
		vs.seek_upward = 0;
		break;
	}

	MMRADIO_POST_MSG(radio, MM_MESSAGE_RADIO_SEEK_START, NULL);

	MMRADIO_LOG_DEBUG("seeking....\n");

	EmultatorIdx = 0;
	while (!seek_stop) {
		/* now we can get new frequency from radio device */
		{
			MMRADIO_LOG_DEBUG("start radio->freq: %d", radio->freq);

			int i = 0;
			for (i = 0; i < EMULATOR_FREQ_MAX; i++)
				if (MMRadioEmulatorFreq[i] == radio->freq)
					EmultatorIdx = i;

			if (vs.seek_upward == 1) {
				if (EmultatorIdx == EMULATOR_FREQ_MAX - 1)
					EmultatorIdx = -1;
				freq = MMRadioEmulatorFreq[EmultatorIdx + 1];
			} else {
				if (EmultatorIdx == 0)
					EmultatorIdx = EMULATOR_FREQ_MAX;
				freq = MMRadioEmulatorFreq[EmultatorIdx - 1];
			}

			radio->freq = freq;
			MMRADIO_LOG_DEBUG("radio->freq: %d EmultatorIdx: %d", radio->freq, EmultatorIdx);
		}

		MMRADIO_LOG_DEBUG("found frequency\n");

		/* if same freq is found, ignore it and search next one. */
		if (freq == radio->prev_seek_freq) {
			MMRADIO_LOG_DEBUG("It's same with previous found one. So, trying next one. \n");
			continue;
		}

		/* check if it's limit freq or not */
		if (__is_tunable_frequency(radio, freq)) {
			/* now tune to new frequency */
			ret = _mmradio_set_frequency(radio, freq);
			if (ret) {
				MMRADIO_LOG_ERROR("failed to tune to new frequency\n");
				goto SEEK_FAILED;
			}
		}

		/* now turn on radio
		 * In the case of limit freq, tuner should be unmuted.
		 * Otherwise, sound can't output even though application set new frequency.
		 */
#if 0
		ret = _mmradio_unmute(radio);
		if (ret) {
			MMRADIO_LOG_ERROR("failed to tune to new frequency\n");
			goto SEEK_FAILED;
		}
#endif
		param.radio_scan.frequency = radio->prev_seek_freq = freq;
		MMRADIO_SLOG_DEBUG("seeking : new frequency : [%d]\n", param.radio_scan.frequency);
		MMRADIO_POST_MSG(radio, MM_MESSAGE_RADIO_SEEK_FINISH, &param);
		seek_stop = true;
	}

	radio->seek_thread = 0;

	MMRADIO_LOG_FLEAVE();

	pthread_exit(NULL);
	return;

 SEEK_FAILED:
	/* freq -1 means it's failed to seek */
	param.radio_scan.frequency = -1;
	MMRADIO_POST_MSG(radio, MM_MESSAGE_RADIO_SEEK_FINISH, &param);
	pthread_exit(NULL);
	return;
}

static bool __mmradio_post_message(mm_radio_t * radio, enum MMMessageType msgtype, MMMessageParamType * param)
{
	MMRADIO_CHECK_INSTANCE(radio);

	MMRADIO_LOG_FENTER();

	if (!radio->msg_cb) {
		debug_warning("failed to post a message\n");
		return false;
	}

	MMRADIO_LOG_DEBUG("address of msg_cb : %d\n", radio->msg_cb);

	radio->msg_cb(msgtype, param, radio->msg_cb_param);

	MMRADIO_LOG_FLEAVE();

	return true;
}

static int __mmradio_check_state(mm_radio_t * radio, MMRadioCommand command)
{
	MMRadioStateType radio_state = MM_RADIO_STATE_NUM;

	MMRADIO_LOG_FENTER();

	MMRADIO_CHECK_INSTANCE(radio);

	radio_state = __mmradio_get_state(radio);

	MMRADIO_LOG_DEBUG("incomming command : %d  current state : %d\n", command, radio_state);

	switch (command) {
	case MMRADIO_COMMAND_CREATE:
		{
			if (radio_state != 0)
				goto NO_OP;
		}
		break;

	case MMRADIO_COMMAND_REALIZE:
		{
			if (radio_state == MM_RADIO_STATE_READY ||
				radio_state == MM_RADIO_STATE_PLAYING ||
				radio_state == MM_RADIO_STATE_SCANNING)
				goto NO_OP;

			if (radio_state == 0)
				goto INVALID_STATE;
		}
		break;

	case MMRADIO_COMMAND_UNREALIZE:
		{
			if (radio_state == MM_RADIO_STATE_NULL)
				goto NO_OP;

			/* we can call unrealize at any higher state */
		}
		break;

	case MMRADIO_COMMAND_START:
		{
			if (radio_state == MM_RADIO_STATE_PLAYING)
				goto NO_OP;

			if (radio_state != MM_RADIO_STATE_READY)
				goto INVALID_STATE;
		}
		break;

	case MMRADIO_COMMAND_STOP:
		{
			if (radio_state == MM_RADIO_STATE_READY)
				goto NO_OP;

			if (radio_state != MM_RADIO_STATE_PLAYING)
				goto INVALID_STATE;
		}
		break;

	case MMRADIO_COMMAND_START_SCAN:
		{
			if (radio_state == MM_RADIO_STATE_SCANNING)
				goto NO_OP;

			if (radio_state != MM_RADIO_STATE_READY)
				goto INVALID_STATE;
		}
		break;

	case MMRADIO_COMMAND_STOP_SCAN:
		{
			if (radio_state == MM_RADIO_STATE_READY)
				goto NO_OP;

			if (radio_state != MM_RADIO_STATE_SCANNING)
				goto INVALID_STATE;
		}
		break;

	case MMRADIO_COMMAND_DESTROY:
	case MMRADIO_COMMAND_MUTE:
	case MMRADIO_COMMAND_UNMUTE:
	case MMRADIO_COMMAND_SET_FREQ:
	case MMRADIO_COMMAND_GET_FREQ:
	case MMRADIO_COMMAND_SET_REGION:
		{
			/* we can do it at any state */
		}
		break;

	case MMRADIO_COMMAND_SEEK:
		{
			if (radio_state != MM_RADIO_STATE_PLAYING)
				goto INVALID_STATE;
		}
		break;

	case MMRADIO_COMMAND_GET_REGION:
		{
			if (radio_state == MM_RADIO_STATE_NULL)
				goto INVALID_STATE;
		}
		break;

	default:
		MMRADIO_LOG_DEBUG("not handled in FSM. don't care it\n");
		break;
	}

	MMRADIO_LOG_DEBUG("status OK\n");

	radio->cmd = command;

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;

 INVALID_STATE:
	debug_warning("invalid state. current : %d  command : %d\n", radio_state, command);
	MMRADIO_LOG_FLEAVE();
	return MM_ERROR_RADIO_INVALID_STATE;

 NO_OP:
	debug_warning("mm-radio is in the desired state(%d). doing noting\n", radio_state);
	MMRADIO_LOG_FLEAVE();
	return MM_ERROR_RADIO_NO_OP;

}

static bool __mmradio_set_state(mm_radio_t * radio, int new_state)
{
	MMMessageParamType msg = { 0, };
	int msg_type = MM_MESSAGE_UNKNOWN;

	MMRADIO_LOG_FENTER();
	MMRADIO_CHECK_INSTANCE(radio);

	if (!radio) {
		debug_warning("calling set_state with invalid radio handle\n");
		return false;
	}

	if (radio->current_state == new_state && radio->pending_state == 0) {
		debug_warning("we are in same state\n");
		return true;
	}

	/* set state */
	radio->old_state = radio->current_state;
	radio->current_state = new_state;

	/* fill message param */
	msg.state.previous = radio->old_state;
	msg.state.current = radio->current_state;

	/* post message to application */
	switch (radio->sm.by_focus_cb) {
	case MMRADIO_FOCUS_CB_NONE:
		{
			msg_type = MM_MESSAGE_STATE_CHANGED;
			MMRADIO_POST_MSG(radio, msg_type, &msg);
		}
		break;

	case MMRADIO_FOCUS_CB_POSTMSG:
		{
			msg_type = MM_MESSAGE_STATE_INTERRUPTED;
			msg.union_type = MM_MSG_UNION_CODE;
			msg.code = radio->sm.event_src;
			MMRADIO_POST_MSG(radio, msg_type, &msg);
		}
		break;

	case MMRADIO_FOCUS_CB_SKIP_POSTMSG:
	default:
		break;
	}

	MMRADIO_LOG_FLEAVE();

	return true;
}

static int __mmradio_get_state(mm_radio_t * radio)
{
	MMRADIO_CHECK_INSTANCE(radio);

	MMRADIO_LOG_DEBUG("radio state : current : [%d]   old : [%d]   pending : [%d]\n",
	                  radio->current_state, radio->old_state, radio->pending_state);

	return radio->current_state;
}

static void __mmradio_sound_focus_cb(int id, mm_sound_focus_type_e focus_type,
	mm_sound_focus_state_e focus_state, const char *reason_for_change,
	const char *additional_info, void *user_data)
{
	mm_radio_t *radio = (mm_radio_t *) user_data;
	enum MMMessageInterruptedCode event_source;
	int result = MM_ERROR_NONE;
	int postMsg = false;

	MMRADIO_LOG_FENTER();
	MMRADIO_CHECK_INSTANCE_RETURN_VOID(radio);

	mmradio_get_audio_focus_reason(focus_state, reason_for_change, &event_source, &postMsg);
	radio->sm.event_src = event_source;

	switch (focus_state) {
	case FOCUS_IS_RELEASED:{
			radio->sm.cur_focus_type &= ~focus_type;
			radio->sm.by_focus_cb = MMRADIO_FOCUS_CB_POSTMSG;

			result = _mmradio_stop(radio);
			if (result)
				MMRADIO_LOG_ERROR("failed to stop radio\n");

			MMRADIO_LOG_DEBUG("FOCUS_IS_RELEASED cur_focus_type : %d\n", radio->sm.cur_focus_type);
		}
		break;

	case FOCUS_IS_ACQUIRED:{
			MMMessageParamType msg = { 0, };
			msg.union_type = MM_MSG_UNION_CODE;
			msg.code = event_source;

			radio->sm.cur_focus_type |= focus_type;

			if ((postMsg) && (FOCUS_FOR_BOTH == radio->sm.cur_focus_type))
				MMRADIO_POST_MSG(radio, MM_MESSAGE_READY_TO_RESUME, &msg);

			radio->sm.by_focus_cb = MMRADIO_FOCUS_CB_NONE;

			MMRADIO_LOG_DEBUG("FOCUS_IS_ACQUIRED cur_focus_type : %d\n", radio->sm.cur_focus_type);
		}
		break;

	default:
		MMRADIO_LOG_DEBUG("Unknown focus_state\n");
		break;
	}

	MMRADIO_LOG_FLEAVE();
}

static void __mmradio_device_connected_cb(MMSoundDevice_t device, bool is_connected, void *user_data)
{
	mm_radio_t *radio = (mm_radio_t *) user_data;
	int result = MM_ERROR_NONE;
	mm_sound_device_type_e type;

	MMRADIO_LOG_FENTER();
	MMRADIO_CHECK_INSTANCE_RETURN_VOID(radio);

	if (mm_sound_get_device_type(device, &type) != MM_ERROR_NONE) {
		debug_error("getting device type failed");
	} else {
		switch (type) {
		case MM_SOUND_DEVICE_TYPE_AUDIOJACK:
		case MM_SOUND_DEVICE_TYPE_BLUETOOTH:
		case MM_SOUND_DEVICE_TYPE_HDMI:
		case MM_SOUND_DEVICE_TYPE_MIRRORING:
		case MM_SOUND_DEVICE_TYPE_USB_AUDIO:
			if (!is_connected) {
				MMRADIO_LOG_ERROR("sound device unplugged");
				radio->sm.by_focus_cb = MMRADIO_FOCUS_CB_POSTMSG;
				radio->sm.event_src = MM_MSG_CODE_INTERRUPTED_BY_EARJACK_UNPLUG;

				result = _mmradio_stop(radio);
				if (result != MM_ERROR_NONE)
					MMRADIO_LOG_ERROR("failed to stop radio\n");
			}
			break;
		default:
			break;
		}
	}
	MMRADIO_LOG_FLEAVE();
}

int _mmradio_get_region_type(mm_radio_t * radio, MMRadioRegionType * type)
{
	MMRADIO_LOG_FENTER();
	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_GET_REGION);

	return_val_if_fail(type, MM_ERROR_INVALID_ARGUMENT);

	*type = radio->region_setting.country;

	MMRADIO_LOG_FLEAVE();
	return MM_ERROR_NONE;
}

int _mmradio_get_region_frequency_range(mm_radio_t * radio, unsigned int *min_freq, unsigned int *max_freq)
{
	MMRADIO_LOG_FENTER();
	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_GET_REGION);

	return_val_if_fail(min_freq && max_freq, MM_ERROR_INVALID_ARGUMENT);

	*min_freq = radio->region_setting.band_min;
	*max_freq = radio->region_setting.band_max;

	MMRADIO_LOG_FLEAVE();
	return MM_ERROR_NONE;
}

int _mmradio_get_channel_spacing(mm_radio_t * radio, unsigned int *ch_spacing)
{
	MMRADIO_LOG_FENTER();
	MMRADIO_CHECK_INSTANCE(radio);
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL(radio, MMRADIO_COMMAND_GET_REGION);

	return_val_if_fail(ch_spacing, MM_ERROR_INVALID_ARGUMENT);

	*ch_spacing = radio->region_setting.channel_spacing;

	MMRADIO_LOG_FLEAVE();
	return MM_ERROR_NONE;
}

static int __mmradio_get_wave_num(mm_radio_t * radio)
{
	int val = 0;
	MMRADIO_LOG_FENTER();
	switch (radio->freq) {
	case 89100:
		val = 1;
		break;

	case 89900:
		val = 5;
		break;

	case 91900:
		val = 6;
		break;

	case 99900:
		val = 8;
		break;

	case 107700:
		val = 9;
		break;

	default:
		val = 9;
		break;
	}
	MMRADIO_LOG_DEBUG("freq: %d, val : %d", radio->freq, val);
	MMRADIO_LOG_FLEAVE();
	return val;
}
