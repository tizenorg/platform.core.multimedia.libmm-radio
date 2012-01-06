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

#include <string.h>
#include "mm_radio.h"
#include "mm_radio_priv.h"
#include "mm_radio_utils.h"
#include <mm_types.h>
#include <mm_message.h>
#include "mm_debug.h"
#include <mm_ta.h>

/*===========================================================================================
|																							|
|  LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE											|
|  																							|
========================================================================================== */

/*---------------------------------------------------------------------------
|    GLOBAL CONSTANT DEFINITIONS:											|
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    IMPORTED VARIABLE DECLARATIONS:										|
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    IMPORTED FUNCTION DECLARATIONS:										|
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    LOCAL #defines:														|
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    LOCAL CONSTANT DEFINITIONS:											|
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    LOCAL DATA TYPE DEFINITIONS:											|
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS:											|
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS:											|
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:												|
---------------------------------------------------------------------------*/

/*===========================================================================
|																			|
|  FUNCTION DEFINITIONS														|
|  																			|
========================================================================== */

int mm_radio_create(MMHandleType *hradio)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* new_radio = NULL;
	
	debug_log("\n");

	return_val_if_fail(hradio, MM_ERROR_RADIO_NOT_INITIALIZED);

	MMTA_INIT();

	/* alloc radio structure */
	new_radio = (mm_radio_t*) malloc(sizeof(mm_radio_t));
	if ( ! new_radio )
	{
		debug_critical("cannot allocate memory for radio\n");
		goto ERROR;
	}
	memset(new_radio, 0, sizeof(mm_radio_t));

	/* internal creation stuffs */
	result = _mmradio_create_radio( new_radio );

	if(result != MM_ERROR_NONE)
		goto ERROR;

	*hradio = (MMHandleType)new_radio;

	return result;

ERROR:

	if ( new_radio )
	{
		MMRADIO_FREEIF( new_radio );
	}

	*hradio = (MMHandleType)0;
	
	/* FIXIT : need to specify more error case */
	return MM_ERROR_RADIO_NO_FREE_SPACE; 
}

int  mm_radio_destroy(MMHandleType hradio)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;

	debug_log("\n");
	
	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);

	result = _mmradio_destroy( radio ); 
	
	if ( result != MM_ERROR_NONE )
	{
		debug_error("failed to destroy radio\n");
		/* FIXIT : add more code for handling error */
	}
	
	/* free radio */
	MMRADIO_FREEIF( radio );

	MMTA_ACUM_ITEM_SHOW_RESULT_TO(MMTA_SHOW_FILE);
	//MMTA_ACUM_ITEM_SHOW_RESULT_TO(MMTA_SHOW_STDOUT);

	MMTA_RELEASE();
	
	return result;
}

int mm_radio_realize(MMHandleType hradio)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;
	
	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);

	MMRADIO_CMD_LOCK( radio );

	__ta__("[KPI] initialize media radio service", 
		   result = _mmradio_realize( radio );
	)

	MMRADIO_CMD_UNLOCK( radio );

	return result;
}

int mm_radio_unrealize(MMHandleType hradio)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;
	
	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);

	MMRADIO_CMD_LOCK( radio );

	__ta__("[KPI] cleanup media radio service", 
		   result = _mmradio_unrealize( radio );
	)
	
	MMRADIO_CMD_UNLOCK( radio );
	
	return result;
}

int mm_radio_set_message_callback(MMHandleType hradio, MMMessageCallback callback, void *user_param)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;
	
	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);

	MMRADIO_CMD_LOCK( radio );

	result = _mmradio_set_message_callback( radio, callback, user_param );

	MMRADIO_CMD_UNLOCK( radio );
	
	return result;
}

int mm_radio_get_state(MMHandleType hradio, MMRadioStateType* pState)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;
	int state = 0;
	
	debug_log("\n");
	
	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);
	return_val_if_fail(pState, MM_ERROR_COMMON_INVALID_ARGUMENT);

	MMRADIO_CMD_LOCK( radio );

	result = _mmradio_get_state( radio, &state );

	*pState = state;

	MMRADIO_CMD_UNLOCK( radio );

	return result;
}

int mm_radio_start(MMHandleType hradio)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;
	MMRadioOutputType path;
	
	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);

	MMRADIO_CMD_LOCK( radio );

	MMTA_ACUM_ITEM_BEGIN("[KPI] start media radio service", false);
	result = _mmradio_start( radio );

	MMRADIO_CMD_UNLOCK( radio );

	return result;
}

int  mm_radio_stop(MMHandleType hradio)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;
	
	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);

	MMRADIO_CMD_LOCK( radio );

	__ta__("[KPI] stop media radio service",
		   result = _mmradio_stop( radio ); 
	)

	MMRADIO_CMD_UNLOCK( radio );

	return result;
}

int mm_radio_seek(MMHandleType hradio, MMRadioSeekDirectionType direction)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;

	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);
	return_val_if_fail(direction >= MM_RADIO_SEEK_UP && direction <= MM_RADIO_SEEK_DOWN, MM_ERROR_INVALID_ARGUMENT);	

	MMRADIO_CMD_LOCK( radio );

	result = _mmradio_seek( radio, direction );

	MMRADIO_CMD_UNLOCK( radio );

	return result;
}

int mm_radio_set_frequency(MMHandleType hradio, int freq)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;

	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);

	MMRADIO_CMD_LOCK( radio );

	result = _mmradio_set_frequency( radio, freq );

	MMRADIO_CMD_UNLOCK( radio );

	return result;
}

int mm_radio_get_frequency(MMHandleType hradio, int* pFreq)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;
	int freq = 0;

	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);
	return_val_if_fail(pFreq, MM_ERROR_INVALID_ARGUMENT);

	MMRADIO_CMD_LOCK( radio );

	result = _mmradio_get_frequency( radio, &freq );

	*pFreq = freq;

	MMRADIO_CMD_UNLOCK( radio );

	return result;
}

int mm_radio_scan_start(MMHandleType hradio)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;

	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);

	MMRADIO_CMD_LOCK( radio );

	result = _mmradio_start_scan( radio );

	MMRADIO_CMD_UNLOCK( radio );

	return result;
}

int mm_radio_scan_stop(MMHandleType hradio)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;

	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);

	MMRADIO_CMD_LOCK( radio );

	result = _mmradio_stop_scan( radio );

	MMRADIO_CMD_UNLOCK( radio );

	return result;
}

int mm_radio_set_sound_path(MMHandleType hradio, MMRadioOutputType path)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;
	int radio_state = MM_RADIO_STATE_NULL;

	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);
	return_val_if_fail(path >= MM_RADIO_OUTPUT_AUTO && path <= MM_RADIO_OUTPUT_HEADSET, MM_ERROR_INVALID_ARGUMENT);

#if 0
	MMRADIO_CMD_LOCK( radio );

	_mmradio_get_state(radio, &radio_state);
	switch(radio_state)
	{
	case MM_RADIO_STATE_READY:
		//cache sound path
		radio->path = path;
		break;
	case MM_RADIO_STATE_PLAYING:
	case MM_RADIO_STATE_SCANNING:
		//immediate set sound path
		result = _mmradio_set_sound_path( radio, path );
		break;
	case MM_RADIO_STATE_NULL:
	default:
		debug_error("Invalid state of mm_radio handle\n");
		result = MM_ERROR_RADIO_INVALID_STATE;
	}

	MMRADIO_CMD_UNLOCK( radio );
#endif

	return result;
}

int mm_radio_get_sound_path(MMHandleType hradio, MMRadioOutputType* pPath)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;
	MMRadioOutputType path = 0;

	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);
	return_val_if_fail(pPath, MM_ERROR_INVALID_ARGUMENT);

#if 0
	MMRADIO_CMD_LOCK( radio );

	result = _mmradio_get_sound_path( radio, &path );

	*pPath = path;

	MMRADIO_CMD_UNLOCK( radio );
#endif

	return result;
}

int mm_radio_set_mute(MMHandleType hradio, bool muted)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;

	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);

	MMRADIO_CMD_LOCK(radio);

	if (muted) 
	{
		result = _mmradio_mute(radio);
	}
	else
	{
		result = _mmradio_unmute(radio);
	}

	MMRADIO_CMD_UNLOCK(radio);

	return result;
}

#if 0
int MMRadioGetAttrs(MMHandleType hradio, MMPlayerAttrsType type, MMHandleType *attrs)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;

	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);


	if ( type >=MM_RADIO_ATTRS_NUM )
	{
		debug_error("type(%d) is invalid\n", type);
		result = MM_ERROR_COMMON_INVALID_ARGUMENT;
		goto RETURN;
	}

	if ( !attrs )
	{
		debug_error("attrs is NULL\n");
		result = MM_ERROR_COMMON_INVALID_ARGUMENT;
		goto RETURN;
	}

	*attrs = (MMHandleType)mmradio_get_attrs( radio, type );

RETURN:

	
	return result;
}


int MMRadioSetAttrs(MMHandleType hradio, MMPlayerAttrsType type, MMHandleType attrs)
{
	int result = MM_ERROR_NONE;
	mm_radio_t* radio = (mm_radio_t*)hradio;

	debug_log("\n");

	return_val_if_fail(radio, MM_ERROR_RADIO_NOT_INITIALIZED);

	MMRADIO_CMD_LOCK( radio );

	if ( type >=MM_RADIO_ATTRS_NUM )
	{
		debug_error("type(%d) is invalid\n", type);
		result = MM_ERROR_COMMON_INVALID_ARGUMENT;
		goto RETURN;
	}

	result = mmradio_set_attrs( radio, type, attrs );

RETURN:

	MMRADIO_CMD_UNLOCK( radio );

	return result;
}
#endif
