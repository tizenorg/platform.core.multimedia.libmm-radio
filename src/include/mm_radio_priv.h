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
 
#ifndef __MM_Radio_INTERNAL_H__
#define	__MM_Radio_INTERNAL_H__

/*===========================================================================================
  INCLUDE FILES
========================================================================================== */
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>
#include <signal.h>

#include <mm_types.h>
#include <mm_message.h>

#include "mm_radio_asm.h"
#include "mm_radio.h"
#include "mm_radio_utils.h"
#include <linux/videodev2.h>
 
#ifdef __cplusplus
	extern "C" {
#endif

/*===========================================================================================
  GLOBAL DEFINITIONS AND DECLARATIONS FOR MODULE
========================================================================================== */

/*---------------------------------------------------------------------------
    GLOBAL #defines:
---------------------------------------------------------------------------*/
#define SAMPLEDELAY     	15000

/*---------------------------------------------------------------------------
    GLOBAL CONSTANT DEFINITIONS:
---------------------------------------------------------------------------*/
typedef enum
{
	MMRADIO_COMMAND_CREATE,
	MMRADIO_COMMAND_DESTROY,
	MMRADIO_COMMAND_REALIZE,
	MMRADIO_COMMAND_UNREALIZE,
	MMRADIO_COMMAND_START,
	MMRADIO_COMMAND_STOP,
	MMRADIO_COMMAND_START_SCAN,
	MMRADIO_COMMAND_STOP_SCAN,
	MMRADIO_COMMAND_SET_VOLUME,
	MMRADIO_COMMAND_GET_VOLUME,
	MMRADIO_COMMAND_SET_FREQ,
	MMRADIO_COMMAND_GET_FREQ,
	MMRADIO_COMMAND_SET_SOUND_PATH,
	MMRADIO_COMMAND_GET_SOUND_PATH,
	MMRADIO_COMMAND_MUTE,
	MMRADIO_COMMAND_UNMUTE,
	MMRADIO_COMMAND_SEEK,

	MMPLAYER_COMMAND_NUM
} MMRadioCommand;

/*---------------------------------------------------------------------------
    GLOBAL DATA TYPE DEFINITIONS:
---------------------------------------------------------------------------*/
 
typedef struct {
	/* radio state */
	int current_state;
	int old_state;
	int pending_state;

	int cmd;

	/* command lock */
	pthread_mutex_t cmd_lock;

	/* radio attributes */
	MMHandleType* attrs;

	/* message callback */
	MMMessageCallback msg_cb;
	void* msg_cb_param;

	/* radio device fd */
	int radio_fd;

	/* device control */
	struct v4l2_capability vc;
	struct v4l2_tuner vt;
	struct v4l2_control vctrl;
	struct v4l2_frequency vf;

	/* hw debug */
	struct v4l2_dbg_register reg;

	/* scan */
	pthread_t	scan_thread;
	bool	stop_scan;

	/* seek */
	pthread_t seek_thread;
	int prev_seek_freq;
	MMRadioSeekDirectionType seek_direction;

	/* sound path */
	MMRadioOutputType path;

	/* ASM */
	MMRadioASM sm;

	int freq;
	unsigned int volume;
} mm_radio_t;

/*===========================================================================================
  GLOBAL FUNCTION PROTOTYPES
========================================================================================== */
int _mmradio_create_radio(mm_radio_t* radio);
int _mmradio_destroy(mm_radio_t* radio);
int _mmradio_realize(mm_radio_t* radio);
int _mmradio_unrealize(mm_radio_t* radio);
int _mmradio_set_message_callback(mm_radio_t* radio, MMMessageCallback callback, void *user_param);
int _mmradio_get_state(mm_radio_t* radio, int* pState);
int _mmradio_set_volume(mm_radio_t* radio, int volume);
int _mmradio_get_volume(mm_radio_t* radio, int* pVolume);
int _mmradio_set_frequency(mm_radio_t* radio, int freq);
int _mmradio_get_frequency(mm_radio_t* radio, int* pFreq);
int _mmradio_mute(mm_radio_t* radio);
int _mmradio_unmute(mm_radio_t* radio);
int _mmradio_start(mm_radio_t* radio);
int _mmradio_stop(mm_radio_t* radio);
int _mmradio_seek(mm_radio_t* radio, MMRadioSeekDirectionType direction);
int _mmradio_start_scan(mm_radio_t* radio);
int _mmradio_stop_scan(mm_radio_t* radio);
int _mmradio_set_sound_path(mm_radio_t* radio, MMRadioOutputType path );
int _mmradio_get_sound_path(mm_radio_t* radio, MMRadioOutputType* pPath );
int _mmradio_release_sound_path(void);
int _mmradio_audio_state_update(bool onoff);

#ifdef __cplusplus
	}
#endif

#endif	/* __MM_Radio_INTERNAL_H__ */
