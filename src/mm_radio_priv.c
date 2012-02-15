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
|  																							|
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
#include <mm_sound.h>
#include <mm_ta.h>

#include <mm_error.h>
#include <mm_debug.h>
#include <mm_message.h>
#include <avsys-audio.h>

#include "mm_radio_priv.h"

/*===========================================================================================
  LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE
========================================================================================== */
#define DEFAULT_DEVICE			"/dev/radio0"

#define TUNER_INDEX				0
#define FREQUENCY_MIN			87500
#define FREQUENCY_MAX			108000
#define DEFAULT_FREQ				107700
#define FREQUENCY_STEP			50
#define FREQ_FRAC				16	
#define DEFAULT_WRAP_AROUND 	1	//If non-zero, wrap around when at the end of the frequency range, else stop seeking

#define RADIO_VOLUME_TYPE 		VOLUME_TYPE_MEDIA

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

/*---------------------------------------------------------------------------
    LOCAL FUNCTION PROTOTYPES:
---------------------------------------------------------------------------*/

static bool	__mmradio_post_message(mm_radio_t* radio, enum MMMessageType msgtype, MMMessageParamType* param);
static int  		__mmradio_check_state(mm_radio_t* radio, MMRadioCommand command);
static int		__mmradio_get_state(mm_radio_t* radio);
static bool	__mmradio_set_state(mm_radio_t* radio, int new_state);
static void 	__mmradio_seek_thread(mm_radio_t* radio);
static void	__mmradio_scan_thread(mm_radio_t* radio);
static int 		__mmradio_set_sound_path(MMRadioOutputType path);
ASM_cb_result_t	__mmradio_asm_callback(int handle, ASM_event_sources_t sound_event, ASM_sound_commands_t command, unsigned int sound_status, void* cb_data);
static int 		__mmradio_register_media_volume_callback(mm_radio_t* radio);
static bool 	__is_tunable_frequency(int freq);
static int 		__mmradio_get_media_volume(unsigned int* value);
/*===========================================================================
  FUNCTION DEFINITIONS
========================================================================== */

int
_mmradio_create_radio(mm_radio_t* radio)
{
	int ret  = 0;

	debug_log("\n");
		
	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_CREATE );

	/* set default value */
	radio->radio_fd = -1;
	radio->path = MM_RADIO_OUTPUT_HEADSET;
	radio->freq = DEFAULT_FREQ;

	/* set default volume */
	__mmradio_get_media_volume(&radio->volume);

	/* create command lock */
	ret = pthread_mutex_init( &radio->cmd_lock, NULL );
	if ( ret )
	{
		debug_error("mutex creation failed\n");
		return MM_ERROR_RADIO_INTERNAL;
	}

	MMRADIO_SET_STATE( radio, MM_RADIO_STATE_NULL );

	/* register to ASM */
	ret = mmradio_asm_register(&radio->sm, __mmradio_asm_callback, (void*)radio);
	if ( ret )
	{
		/* NOTE : we are dealing it as an error since we cannot expect it's behavior */
		debug_error("failed to register asm server\n");
		return MM_ERROR_RADIO_INTERNAL;
	}

	return MM_ERROR_NONE;
}

int
_mmradio_realize(mm_radio_t* radio)
{
	int val = 0;

	debug_log("\n");

	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_REALIZE );

	/* open radio device */
	if(radio->radio_fd == -1)
	{
		/* prevent noise - don't need to set */
		//__mmradio_set_sound_path(MM_RADIO_OUTPUT_NONE);

		/* open device */
		radio->radio_fd = open(DEFAULT_DEVICE, O_RDONLY);
		if (radio->radio_fd < 0)
		{
			debug_error("failed to open radio device[%s] because of %s(%d)\n", 
						DEFAULT_DEVICE, strerror(errno), errno);
			
			/* check error */
			switch (errno)
			{
				case ENOENT:
					return MM_ERROR_RADIO_DEVICE_NOT_FOUND;
				case EACCES:					
					return MM_ERROR_RADIO_PERMISSION_DENIED;
				default:
					return MM_ERROR_RADIO_DEVICE_NOT_OPENED;
			}
		}
		debug_log("radio device fd : %d\n", radio->radio_fd);

		/* Query Radio device capabilities. */
		if (ioctl(radio->radio_fd, VIDIOC_QUERYCAP, &(radio->vc)) < 0)
		{
			debug_error("VIDIOC_QUERYCAP failed!\n");
			goto error;
		}

		if ( ! ( radio->vc.capabilities & V4L2_CAP_TUNER) )
		{
			debug_error("this system can't support fm-radio!\n");
			goto error;
		}

		/* set tuner audio mode */
		ioctl(radio->radio_fd, VIDIOC_G_TUNER, &(radio->vt));

		if ( ! ( (radio->vt).capability & V4L2_TUNER_CAP_STEREO) )
		{
			debug_error("this system can support mono!\n");
			(radio->vt).audmode = V4L2_TUNER_MODE_MONO;
		}
		else
		{
			(radio->vt).audmode = V4L2_TUNER_MODE_STEREO;
		}

		/* Set tuner index. Must be 0. */
		(radio->vt).index = TUNER_INDEX;
		ioctl(radio->radio_fd, VIDIOC_S_TUNER, &(radio->vt));
	}

	/* ready but nosound */
	if( _mmradio_mute(radio) != MM_ERROR_NONE)
		goto error;

	MMRADIO_SET_STATE( radio, MM_RADIO_STATE_READY );

	return MM_ERROR_NONE;

error:
	if (radio->radio_fd >= 0)
	{
		close(radio->radio_fd);
		radio->radio_fd = -1;
	}

	return MM_ERROR_RADIO_INTERNAL;
}

int
_mmradio_unrealize(mm_radio_t* radio)
{
	int res = MM_ERROR_NONE;
	debug_log("\n");

	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_UNREALIZE );

	if( _mmradio_mute(radio) != MM_ERROR_NONE)
		return MM_ERROR_RADIO_NOT_INITIALIZED;

	/* close radio device here !!!! */
	if (radio->radio_fd >= 0)
	{
		close(radio->radio_fd);
		radio->radio_fd = -1;
	}

	MMRADIO_SET_STATE( radio, MM_RADIO_STATE_NULL );

	res = mmradio_asm_set_state(&radio->sm, ASM_STATE_STOP, ASM_RESOURCE_NONE);
	if ( res )
	{
			debug_error("failed to set asm state to STOP\n");
			return res;
	}

	return MM_ERROR_NONE;
}

int
_mmradio_destroy(mm_radio_t* radio)
{
	int ret = 0;
	debug_log("\n");
	
	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_DESTROY );
	
	ret = mmradio_asm_deregister(&radio->sm);
	if ( ret )
	{
		debug_error("failed to deregister asm server\n");
		return MM_ERROR_RADIO_INTERNAL;
	}

	_mmradio_unrealize( radio );

	return MM_ERROR_NONE;
}

int
_mmradio_set_volume(mm_radio_t* radio, int volume)
{
	int err = 0;
	debug_log("\n");
	
	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_SET_VOLUME );

	//remove white noise when volume zero playback
	if(!volume && radio->volume) //amp on
		err = avsys_audio_ext_device_ampon(AVSYS_AUDIO_EXT_DEVICE_FMRADIO);
	else if(volume && !radio->volume) //amp off
		err = avsys_audio_ext_device_ampoff(AVSYS_AUDIO_EXT_DEVICE_FMRADIO);

	if(AVSYS_FAIL(err))
		debug_error("codec mute or unmute failed\n");

	radio->volume = volume;

	if (radio->radio_fd < 0)
	{
		debug_log("radio is still not opened. just store given volume\n");
		return MM_ERROR_NONE;
	}

	(radio->vctrl).id = V4L2_CID_AUDIO_VOLUME;
	(radio->vctrl).value = volume;

	if (ioctl(radio->radio_fd, VIDIOC_S_CTRL, &(radio->vctrl))< 0)
	{
		return MM_ERROR_RADIO_INTERNAL;
	}

	return MM_ERROR_NONE;
}

int
_mmradio_get_volume(mm_radio_t* radio, int* pVolume)
{
	debug_log("\n");
	
	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_GET_VOLUME );

	return_val_if_fail(pVolume, MM_ERROR_INVALID_ARGUMENT);

	if ( radio->radio_fd < 0 )
	{
		debug_log("radio is not opened yet. returning stored volume [%d]\n", radio->volume);
		*pVolume = radio->volume;
		return MM_ERROR_NONE;
	}

	if (ioctl(radio->radio_fd, VIDIOC_G_CTRL, &(radio->vctrl)) < 0)
	{
		return MM_ERROR_RADIO_INTERNAL;
	}	

	radio->volume = (int)(radio->vctrl).value;
	*pVolume = radio->volume;

	debug_log("volume : %d\n", *pVolume);

	return MM_ERROR_NONE;
}

int
_mmradio_set_frequency(mm_radio_t* radio, int freq) // 87500 ~ 108000 KHz
{
	debug_log("\n");

	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_SET_FREQ );

	radio->freq = freq;

	if (radio->radio_fd < 0)
	{
		debug_log("radio device is not opened yet. just store given frequency [%d]\n", freq);
		return MM_ERROR_NONE;
	}

	if ( freq < FREQUENCY_MIN || freq > FREQUENCY_MAX )
		return MM_ERROR_INVALID_ARGUMENT;

	(radio->vf).tuner = 0;
	(radio->vf).frequency = (freq * FREQ_FRAC);

	if(ioctl(radio->radio_fd, VIDIOC_S_FREQUENCY, &(radio->vf))< 0)
	{
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}
	
	return MM_ERROR_NONE;
	
}

int
_mmradio_get_frequency(mm_radio_t* radio, int* pFreq)
{
	int freq = 0;
	debug_log("\n");

	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_GET_FREQ );

	return_val_if_fail( pFreq, MM_ERROR_INVALID_ARGUMENT );

	/* just return stored frequency if radio device is not ready */
	if ( radio->radio_fd < 0 )
	{
		debug_log("freq : %d\n", radio->freq);
		*pFreq = radio->freq;
		return MM_ERROR_NONE;
	}

	if (ioctl(radio->radio_fd, VIDIOC_G_FREQUENCY, &(radio->vf)) < 0)
	{
		debug_error("failed to do VIDIOC_G_FREQUENCY\n");
		return MM_ERROR_RADIO_INTERNAL;
	}	

	/* (87500 ~ 108000) */
	freq = (radio->vf).frequency;
	freq /= FREQ_FRAC;

	/* update freq in handle */
	radio->freq = freq;

	*pFreq = radio->freq;

	return MM_ERROR_NONE;
}

int
_mmradio_mute(mm_radio_t* radio)
{
	debug_log("\n");
	
	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_MUTE );

	if (radio->radio_fd < 0)
	{
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}	

	(radio->vctrl).id = V4L2_CID_AUDIO_MUTE;
	(radio->vctrl).value = 1; //mute

	if (ioctl(radio->radio_fd, VIDIOC_S_CTRL, &(radio->vctrl)) < 0)
	{
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}
	
	return MM_ERROR_NONE;
	
}

int
_mmradio_unmute(mm_radio_t* radio)
{
	debug_log("\n");
	
	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_UNMUTE );

	if (radio->radio_fd < 0)
	{
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}	

	(radio->vctrl).id = V4L2_CID_AUDIO_MUTE;
	(radio->vctrl).value = 0; //unmute

	if (ioctl(radio->radio_fd, VIDIOC_S_CTRL, &(radio->vctrl)) < 0)
	{
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}
	return MM_ERROR_NONE;
	
}
	
int
_mmradio_set_message_callback(mm_radio_t* radio, MMMessageCallback callback, void *user_param)
{
	debug_log("\n");
	
	MMRADIO_CHECK_INSTANCE( radio );

	radio->msg_cb = callback;
	radio->msg_cb_param = user_param;

	debug_log("msg_cb : 0x%x msg_cb_param : 0x%x\n", (unsigned int)callback, (unsigned int)user_param);

	return MM_ERROR_NONE;
}
		
int
_mmradio_get_state(mm_radio_t* radio, int* pState)
{
	debug_log("\n");
	int state = 0;
	
	MMRADIO_CHECK_INSTANCE( radio );
	return_val_if_fail( pState, MM_ERROR_INVALID_ARGUMENT );
		
	state = __mmradio_get_state( radio );

	*pState = state;

	return MM_ERROR_NONE;
}

int
__mmradio_get_media_volume(unsigned int* value)
{
	int res = MM_ERROR_NONE;

	debug_log("\n");
	
	/* get volume value from sound module */
	res = mm_sound_volume_get_value(RADIO_VOLUME_TYPE, value);
	MMRADIO_CHECK_RETURN_IF_FAIL(res, "get volume");

	return res;
}

void 
__media_volume_changed(void* param)
{
	mm_radio_t *radio = (mm_radio_t *)param;
	int res = MM_ERROR_NONE;
	unsigned int value = 0;	

	debug_log("\n");

	if ( __mmradio_get_media_volume(&value) == MM_ERROR_NONE )
	{
		res = _mmradio_set_volume(radio, value);
		MMRADIO_CHECK_RETURN_IF_FAIL(res, "set chip volume");

		debug_log("radio volume is changed as (%d) by volume app\n", value);
	}
	
	return;
}

int 
__mmradio_register_media_volume_callback(mm_radio_t* radio)
{
	int res = MM_ERROR_NONE;
	
	debug_log("\n");

	MMRADIO_CHECK_INSTANCE( radio );

	res = mm_sound_volume_add_callback(RADIO_VOLUME_TYPE, __media_volume_changed, (void* )radio);
	MMRADIO_CHECK_RETURN_IF_FAIL(res, "add volume");
	
	return res;
}

int
_mmradio_start(mm_radio_t* radio)
{
	int res = MM_ERROR_NONE;
	system_audio_route_t policy = SYSTEM_AUDIO_ROUTE_POLICY_DEFAULT;
	
	debug_log("\n");
	
	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_START );

	debug_log("now tune to frequency : %d\n", radio->freq);

	/* chip volume should also be applied whenever media volume is changed. */
	__mmradio_register_media_volume_callback(radio);
	
	res = mmradio_asm_set_state(&radio->sm, ASM_STATE_PLAYING, ASM_RESOURCE_RADIO_TUNNER);
	if ( res )
	{
		debug_error("failed to set asm state to PLAYING\n");
		return res;
	}

	/* set stored frequency */
	_mmradio_set_frequency( radio, radio->freq );

	/* set stored volume */
	_mmradio_set_volume( radio, radio->volume );

	/* set sound path with mm-sound policy */
	res = mm_sound_route_get_system_policy(&policy);

	if (res != MM_ERROR_NONE)
	{
		debug_error("failed to get sound policy\n");
		return res;
	}

	debug_log("current sound path policy : %d\n", policy);

	if (policy == SYSTEM_AUDIO_ROUTE_POLICY_HANDSET_ONLY)
		__mmradio_set_sound_path(MM_RADIO_OUTPUT_SPEAKER);
	else
		__mmradio_set_sound_path(MM_RADIO_OUTPUT_HEADSET);
	
	/* unmute */
	if( _mmradio_unmute(radio) != MM_ERROR_NONE)
		return MM_ERROR_RADIO_NOT_INITIALIZED;

	/* set audio state */
	_mmradio_audio_state_update( true );

	MMRADIO_SET_STATE( radio, MM_RADIO_STATE_PLAYING );

	return res;
}

int
_mmradio_stop(mm_radio_t* radio)
{
	int res = MM_ERROR_NONE;

	debug_log("\n");
	
	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_STOP );

	_mmradio_audio_state_update( false );
	
	if( _mmradio_mute(radio) != MM_ERROR_NONE)
		return MM_ERROR_RADIO_NOT_INITIALIZED;

	__mmradio_set_sound_path(MM_RADIO_OUTPUT_NONE);	

	MMRADIO_SET_STATE( radio, MM_RADIO_STATE_READY );

	res = mmradio_asm_set_state(&radio->sm, ASM_STATE_STOP, ASM_RESOURCE_NONE);
	if ( res )
	{
		debug_error("failed to set asm state to PLAYING\n");
		return res;
	}

	return MM_ERROR_NONE;
}

int
_mmradio_seek(mm_radio_t* radio, MMRadioSeekDirectionType direction)
{
	debug_log("\n");
	
	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_SEEK );

 	int ret = 0;
 	
	if( _mmradio_mute(radio) != MM_ERROR_NONE)
		return MM_ERROR_RADIO_NOT_INITIALIZED;
		
	debug_log("trying to seek. direction[0:UP/1:DOWN) %d\n", direction);
	radio->seek_direction = direction;

	ret = pthread_create(&radio->seek_thread, NULL,
		(void *)__mmradio_seek_thread, (void *)radio);

	if ( ret )
	{
		debug_log("failed create thread\n");
		return MM_ERROR_RADIO_INTERNAL;
	}

	return MM_ERROR_NONE;
}

int
_mmradio_start_scan(mm_radio_t* radio)
{
	debug_log("\n");
	
	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_START_SCAN );

	int scan_tr_id = 0;
 	
	radio->stop_scan = false;

 	scan_tr_id = pthread_create(&radio->scan_thread, NULL,
		(void *)__mmradio_scan_thread, (void *)radio);

	if (scan_tr_id != 0) 
	{
		debug_log("failed to create thread : scan\n");
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}

	pthread_detach(radio->scan_thread);

	MMRADIO_SET_STATE( radio, MM_RADIO_STATE_SCANNING );

	return MM_ERROR_NONE;
}

int
_mmradio_stop_scan(mm_radio_t* radio)
{
	debug_log("\n");
	
	MMRADIO_CHECK_INSTANCE( radio );
	MMRADIO_CHECK_STATE_RETURN_IF_FAIL( radio, MMRADIO_COMMAND_STOP_SCAN );
		
	radio->stop_scan = true;

 	if( radio->scan_thread > 0 )
	{	
		pthread_join(radio->scan_thread, NULL);
	 	radio->scan_thread = 0;
	}

 	MMRADIO_SET_STATE( radio, MM_RADIO_STATE_READY );
	MMRADIO_POST_MSG(radio, MM_MESSAGE_RADIO_SCAN_STOP, NULL);
	
	return MM_ERROR_NONE;
}

int
_mmradio_set_sound_path(mm_radio_t* radio, MMRadioOutputType path )
{
	int res = MM_ERROR_NONE;

	debug_log("\n");

	MMRADIO_CHECK_INSTANCE( radio );

	/* set sound path */
	radio->path = path;
	debug_log("path : %d\n", radio->path);

	/* apply sound path */
	res = __mmradio_set_sound_path( radio->path );

	return res;
}

int
_mmradio_release_sound_path()
{
	debug_log("\n");

	return __mmradio_set_sound_path( MM_RADIO_OUTPUT_NONE );
}

int
_mmradio_audio_state_update(bool onoff)
{
	debug_log("\n");

	return avsys_audio_set_ext_device_status(AVSYS_AUDIO_EXT_DEVICE_FMRADIO, onoff);
}

int
_mmradio_get_sound_path(mm_radio_t* radio, MMRadioOutputType* pPath )
{
	debug_log("\n");

	MMRADIO_CHECK_INSTANCE( radio );
	return_val_if_fail(pPath, MM_ERROR_INVALID_ARGUMENT);

	*pPath = radio->path;
	debug_log("sound path : %d\n", *pPath);

	return MM_ERROR_NONE;
}

int
__mmradio_set_sound_path(MMRadioOutputType path)
{
	int res = MM_ERROR_NONE;
	int output_path  = 0;
	int input_path = AVSYS_AUDIO_PATH_EX_FMINPUT; //MM_SOUND_PATH_FMINPUT;

	debug_log("\n");
	

	switch(path)
	{
		case MM_RADIO_OUTPUT_NONE:
			output_path = AVSYS_AUDIO_PATH_EX_NONE; //MM_SOUND_PATH_NONE;
			input_path = AVSYS_AUDIO_PATH_EX_NONE; //MM_SOUND_PATH_NONE;
		break;

		case MM_RADIO_OUTPUT_SPEAKER:
			output_path = AVSYS_AUDIO_PATH_EX_SPK; //MM_SOUND_PATH_SPK;
		break;

		case MM_RADIO_OUTPUT_HEADSET:
			output_path = AVSYS_AUDIO_PATH_EX_HEADSET; //MM_SOUND_PATH_HEADSET;
		break;

		case MM_RADIO_OUTPUT_AUTO:
		{
			debug_error("automatic sound path is not implemented yet\n");
			debug_error("this option can be valid only if hw supports internal antenna\n");
			return MM_ERROR_RADIO_INTERNAL;
		}
		break;

		default:
			return MM_ERROR_RADIO_INTERNAL;
		break;
	}

	res = avsys_audio_set_path_ex( AVSYS_AUDIO_GAIN_EX_FMRADIO,
									output_path,
									input_path,
									AVSYS_AUDIO_PATH_OPTION_NONE );

	if ( AVSYS_FAIL(res) )
	{
		debug_warning("failed to set sound path\n");
		return res;
	}

	debug_log("setting sound path finished\n");

	return MM_ERROR_NONE;
}

void
__mmradio_scan_thread(mm_radio_t* radio)
{
	int ret = 0;
	int prev_freq = 0;

	struct v4l2_hw_freq_seek vs = {0,};
	vs.tuner = TUNER_INDEX;
	vs.type = V4L2_TUNER_RADIO;
	vs.wrap_around = 0; /* around:1 not around:0 */
	vs.seek_upward = 1; /* up : 1	------- down : 0 */

	if( _mmradio_mute(radio) != MM_ERROR_NONE)
		goto FINISHED;

	if( _mmradio_set_frequency(radio, FREQUENCY_MIN ) != MM_ERROR_NONE)
		goto FINISHED;

	MMRADIO_POST_MSG(radio, MM_MESSAGE_RADIO_SCAN_START, NULL);
	MMRADIO_SET_STATE( radio, MM_RADIO_STATE_SCANNING );

	while( ! radio->stop_scan )
	{
		int freq = 0;
		MMMessageParamType param = {0,};

		debug_log("scanning....\n");
		ret = ioctl(radio->radio_fd, VIDIOC_S_HW_FREQ_SEEK, &vs);

		if( ret == -1 )
		{
			if ( errno == EAGAIN )
			{
				debug_error("scanning timeout\n");
				continue;
			}
			else if ( errno == EINVAL )
			{
				debug_error("The tuner index is out of bounds or the value in the type field is wrong.");
				break;
			}
			else
			{
				debug_error("Error: %s, %d\n", strerror(errno), errno);
				break;
			}
		}

		/* now we can get new frequency from radio device */

		if ( radio->stop_scan ) break;

		ret = _mmradio_get_frequency(radio, &freq);
		if ( ret )
		{
			debug_error("failed to get current frequency\n");
		}
		else
		{
			if ( freq < prev_freq )
			{
				debug_log("scanning wrapped around. stopping scan\n");
				break;
			}

			if ( freq == prev_freq)
				continue;

			prev_freq = param.radio_scan.frequency = freq;
			debug_log("scanning : new frequency : [%d]\n", param.radio_scan.frequency);
			
			/* drop if max freq is scanned */
			if (param.radio_scan.frequency == FREQUENCY_MAX )
			{
				debug_log("%d freq is dropping...and stopping scan\n", param.radio_scan.frequency);
				break;
			}

			if ( radio->stop_scan ) break; // don't need to post

			MMRADIO_POST_MSG(radio, MM_MESSAGE_RADIO_SCAN_INFO, &param);
		}
	}

FINISHED:
	radio->scan_thread = 0;
	
	MMRADIO_SET_STATE( radio, MM_RADIO_STATE_READY );

	if ( ! radio->stop_scan )
	{
		MMRADIO_POST_MSG(radio, MM_MESSAGE_RADIO_SCAN_FINISH, NULL);
	}

	pthread_exit(NULL);
	return;
}

bool 
__is_tunable_frequency(int freq)
{
	if ( freq == FREQUENCY_MAX || freq == FREQUENCY_MIN )
		return false;

	return true;
}

void
__mmradio_seek_thread(mm_radio_t* radio)
{
	int ret = 0;
	int freq = 0;
	bool seek_stop = false;
	MMMessageParamType param = {0,};
	struct v4l2_hw_freq_seek vs = {0,};

	vs.tuner = TUNER_INDEX;
	vs.type = V4L2_TUNER_RADIO;
	vs.wrap_around = DEFAULT_WRAP_AROUND;

	/* check direction */
	switch( radio->seek_direction )
	{
		case MM_RADIO_SEEK_UP:
			vs.seek_upward = 1;
			break;
		default:
			vs.seek_upward = 0;
			break;
	}

	MMRADIO_POST_MSG(radio, MM_MESSAGE_RADIO_SEEK_START, NULL);

	debug_log("seeking....\n");
	
	while (  ! seek_stop )
	{	
		ret = ioctl( radio->radio_fd, VIDIOC_S_HW_FREQ_SEEK, &vs );

		if( ret == -1 )
		{
			if ( errno == EAGAIN )
			{
				/* FIXIT : we need retrying code here */
				debug_error("scanning timeout\n");
				goto SEEK_FAILED;
			}
			else if ( errno == EINVAL )
			{
				debug_error("The tuner index is out of bounds or the value in the type field is wrong.");
				goto SEEK_FAILED;
			}
			else
			{
				debug_error("Error: %s, %d\n", strerror(errno), errno);
				goto SEEK_FAILED;
			}
		}

		/* now we can get new frequency from radio device */
		ret = _mmradio_get_frequency(radio, &freq);
		if ( ret )
		{
			debug_error("failed to get current frequency\n");
			goto SEEK_FAILED;
		}

		debug_log("found frequency = %d\n", freq);

		/* if same freq is found, ignore it and search next one. */
		if ( freq == radio->prev_seek_freq )
		{
			debug_log("It's same with previous one(%d). So, trying next one. \n", freq);
			continue;
		}

		if ( __is_tunable_frequency(freq) )
		{
			/* now tune to new frequency */
			ret = _mmradio_set_frequency(radio, freq);
			if ( ret )
			{
				debug_error("failed to tune to new frequency\n");
				goto SEEK_FAILED;
			}
		}

		/* now turn on radio 
		  * In the case of limit freq, tuner should be unmuted. 
		  * Otherwise, sound can't output even though application set new frequency. 
		  */
		ret = _mmradio_unmute(radio);
		if ( ret )
		{
			debug_error("failed to tune to new frequency\n");
			goto SEEK_FAILED;
		}

		param.radio_scan.frequency = radio->prev_seek_freq = freq;
		debug_log("seeking : new frequency : [%d]\n", param.radio_scan.frequency);
		MMRADIO_POST_MSG(radio, MM_MESSAGE_RADIO_SEEK_FINISH, &param);
		seek_stop = true;
	}

	radio->seek_thread = 0;
	pthread_exit(NULL);
	return;

SEEK_FAILED:
	/* freq -1 means it's failed to seek */
	param.radio_scan.frequency = -1;
	MMRADIO_POST_MSG(radio, MM_MESSAGE_RADIO_SEEK_FINISH, &param);
	pthread_exit(NULL);
	return;
}

static bool
__mmradio_post_message(mm_radio_t* radio, enum MMMessageType msgtype, MMMessageParamType* param)
{
	return_val_if_fail( radio, false );

	if ( !radio->msg_cb )
	{
		debug_warning("failed to post a message\n");
		return false;
	}

	debug_log("address of msg_cb : %d\n", radio->msg_cb);

	radio->msg_cb(msgtype, param, radio->msg_cb_param);
	return true;
}

static int
 __mmradio_check_state(mm_radio_t* radio, MMRadioCommand command)
 {
	 MMRadioStateType radio_state = MM_RADIO_STATE_NUM;

 	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);

 	radio_state = __mmradio_get_state( radio );

 	debug_log("incomming command : %d  current state : %d\n", command, radio_state);

 	switch( command )
 	{
 		case MMRADIO_COMMAND_CREATE:
 		{
 			if ( radio_state != 0 )
 				goto NO_OP;
 		}
 		break;

 		case MMRADIO_COMMAND_REALIZE:
 		{
			if ( radio_state == MM_RADIO_STATE_READY ||
					radio_state == MM_RADIO_STATE_PLAYING ||
					radio_state == MM_RADIO_STATE_SCANNING )
				goto NO_OP;

			if ( radio_state == 0 )
				goto INVALID_STATE;
 		}
 		break;

 		case MMRADIO_COMMAND_UNREALIZE:
 		{
			if ( radio_state == MM_RADIO_STATE_NULL )
				goto NO_OP;

			/* we can call unrealize at any higher state */
 		}
 		break;

 		case MMRADIO_COMMAND_START:
 		{
 			if ( radio_state == MM_RADIO_STATE_PLAYING )
 				goto NO_OP;

 			if ( radio_state != MM_RADIO_STATE_READY )
 				goto INVALID_STATE;
 		}
 		break;

 		case MMRADIO_COMMAND_STOP:
 		{
 			if ( radio_state == MM_RADIO_STATE_READY )
 				goto NO_OP;

 			if ( radio_state != MM_RADIO_STATE_PLAYING )
 				goto INVALID_STATE;
 		}
 		break;

 		case MMRADIO_COMMAND_START_SCAN:
 		{
 			if ( radio_state == MM_RADIO_STATE_SCANNING )
 				goto NO_OP;

 			if ( radio_state != MM_RADIO_STATE_READY )
 				goto INVALID_STATE;
 		}
 		break;

 		case MMRADIO_COMMAND_STOP_SCAN:
 		{
 			if ( radio_state == MM_RADIO_STATE_READY )
 				goto NO_OP;

 			if ( radio_state != MM_RADIO_STATE_SCANNING )
 				goto INVALID_STATE;
 		}
 		break;

 		case MMRADIO_COMMAND_DESTROY:
 		case MMRADIO_COMMAND_SET_VOLUME:
 		case MMRADIO_COMMAND_GET_VOLUME:
 		case MMRADIO_COMMAND_MUTE:
 		case MMRADIO_COMMAND_UNMUTE:
 		case MMRADIO_COMMAND_SET_FREQ:
 		case MMRADIO_COMMAND_GET_FREQ:
 		case MMRADIO_COMMAND_SET_SOUND_PATH:
 		case MMRADIO_COMMAND_GET_SOUND_PATH:
 		{
			/* we can do it at any state */
 		}
 		break;

 		case MMRADIO_COMMAND_SEEK:
 		{
			if ( radio_state != MM_RADIO_STATE_PLAYING )
				goto INVALID_STATE;
 		}
 		break;

 		default:
 			debug_log("not handled in FSM. don't care it\n");
 		break;
 	}

 	debug_log("status OK\n");

 	radio->cmd = command;

 	return MM_ERROR_NONE;


 INVALID_STATE:
 	debug_warning("invalid state. current : %d  command : %d\n",
 			radio_state, command);
 	return MM_ERROR_RADIO_INVALID_STATE;


 NO_OP:
 	debug_warning("mm-radio is in the desired state(%d). doing noting\n", radio_state);
 	return MM_ERROR_RADIO_NO_OP;

 }

static bool
__mmradio_set_state(mm_radio_t* radio, int new_state)
{
	MMMessageParamType msg = {0, };
	int msg_type = MM_MESSAGE_UNKNOWN;
	
	if ( ! radio )
	{
		debug_warning("calling set_state with invalid radio handle\n");
		return false;
	}

	if ( radio->current_state == new_state && radio->pending_state == 0 )
	{
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
	switch( radio->sm.by_asm_cb )
	{
		case MMRADIO_ASM_CB_NONE:
		{
			msg_type = MM_MESSAGE_STATE_CHANGED;
			MMRADIO_POST_MSG( radio, msg_type, &msg );
		}
		break;

		case MMRADIO_ASM_CB_POSTMSG:
		{
			msg_type = MM_MESSAGE_STATE_INTERRUPTED;
			msg.union_type = MM_MSG_UNION_CODE;
			msg.code = radio->sm.event_src;
			MMRADIO_POST_MSG( radio, msg_type, &msg );
		}
		break;

		case MMRADIO_ASM_CB_SKIP_POSTMSG:
		default:
		break;
	}

	return true;
}

static int
__mmradio_get_state(mm_radio_t* radio)
{
	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);

	debug_log("radio state : current : [%d]   old : [%d]   pending : [%d]\n",
			radio->current_state, radio->old_state, radio->pending_state );

	return radio->current_state;
}

ASM_cb_result_t
__mmradio_asm_callback(int handle, ASM_event_sources_t event_source, ASM_sound_commands_t command, unsigned int sound_status, void* cb_data)
{
	mm_radio_t* radio = (mm_radio_t*) cb_data;
	int result = MM_ERROR_NONE;
	ASM_cb_result_t	cb_res = ASM_CB_RES_NONE;

	radio->sm.event_src = event_source;

	switch(command)
	{
		case ASM_COMMAND_STOP:
		case ASM_COMMAND_PAUSE:
		{
			debug_log("ASM asked me to stop. cmd : %d\n", command);
			switch(event_source)
			{
				case ASM_EVENT_SOURCE_CALL_START:
				case ASM_EVENT_SOURCE_ALARM_START:
				{
					radio->sm.by_asm_cb = MMRADIO_ASM_CB_POSTMSG;					
					result = _mmradio_stop(radio);
					if( result ) 
					{
						debug_error("failed to stop radio\n");
					}

					debug_log("skip unrealize in asm callback")
				}
				break;

				case ASM_EVENT_SOURCE_OTHER_APP:
				case ASM_EVENT_SOURCE_RESOURCE_CONFLICT:
				default:
				{
					radio->sm.by_asm_cb = MMRADIO_ASM_CB_SKIP_POSTMSG;					
					result = _mmradio_stop(radio);
					if( result ) 
					{
						debug_error("failed to stop radio\n");
					}

					radio->sm.by_asm_cb = MMRADIO_ASM_CB_POSTMSG;
					result = _mmradio_unrealize(radio);
					if ( result ) 
					{
						debug_error("failed to unrealize radio\n");
					}
				}
				break;
			}
			cb_res = ASM_CB_RES_STOP;
		}
		break;

		case ASM_COMMAND_PLAY:
		case ASM_COMMAND_RESUME:
		{
			MMMessageParamType msg = {0,};
			msg.union_type = MM_MSG_UNION_CODE;
			msg.code = event_source;

			debug_log("Got ASM resume message by %d\n", msg.code);
			MMRADIO_POST_MSG(radio, MM_MESSAGE_READY_TO_RESUME, &msg);

			cb_res = ASM_CB_RES_IGNORE;
			radio->sm.by_asm_cb = MMRADIO_ASM_CB_NONE;
		}
		break;

		default:
		break;
	}

	return cb_res;
}