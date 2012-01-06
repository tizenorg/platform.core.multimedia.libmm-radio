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
 
#include <assert.h>
#include <mm_debug.h>
#include "mm_radio_asm.h"

static ASM_sound_events_t __mmradio_asm_get_event_type(int type);

int mmradio_asm_register(MMRadioASM* sm, ASM_sound_cb_t callback, void* param)
{
	/* read mm-session type */
	int sessionType = MM_SESSION_TYPE_SHARE;
	int errorcode = MM_ERROR_NONE;
	int asm_handle = -1;
	int event_type = ASM_EVENT_NONE;
	int pid = -1;

	debug_log("\n");

	if ( ! sm )
	{
		debug_error("invalid session handle\n");
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}

	/* read session type */
//	errorcode = __MMSessionUtilReadType(&sessionType);
	errorcode = _mm_session_util_read_type(-1, &sessionType);
	if ( errorcode )
	{
		debug_warning("Read MMSession Type failed. use default \"exclusive\" type\n");
		sessionType = MM_SESSION_TYPE_EXCLUSIVE;

		/* init session */
		errorcode = mm_session_init(sessionType);
		if ( errorcode )
		{
				debug_critical("mm_session_init() failed\n");
				return errorcode;
		}
	}

	/* check if it's CALL */
	if ( sessionType == MM_SESSION_TYPE_CALL || sessionType == MM_SESSION_TYPE_VIDEOCALL)
	{
		debug_log("session type is VOICE or VIDEO CALL (%d)\n", sessionType);
		return MM_ERROR_NONE;
	}

	/* interpret session type */
	event_type = __mmradio_asm_get_event_type(sessionType);

	/* check if it's running on the media_server */
	if ( sm->pid > 0 )
	{
		pid = sm->pid;
		debug_log("mm-radio is running on different process. Just faking pid to [%d]. :-p\n", pid);
	}
	else
	{
		debug_log("no pid has assigned. using default(current) context\n");
	}

	/* register audio-session-manager callback */
	if( ! ASM_register_sound(pid, &asm_handle, event_type, ASM_STATE_NONE, callback, (void*)param, ASM_RESOURCE_NONE, &errorcode))
	{
		debug_critical("ASM_register_sound() failed\n");
		return errorcode;
	}

	/* now succeded to register our callback. take result */
	sm->handle = asm_handle;
	sm->state = ASM_STATE_NONE;

	return MM_ERROR_NONE;
}

int mmradio_asm_deregister(MMRadioASM* sm)
{
	int sessionType = MM_SESSION_TYPE_SHARE;
	int event_type = ASM_EVENT_NONE;
	int errorcode = 0;

	if ( ! sm )
	{
		debug_error("invalid session handle\n");
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}

	/* read session type */
//	errorcode = __MMSessionUtilReadType(&sessionType);
	errorcode = _mm_session_util_read_type(-1, &sessionType);
	if ( errorcode )
	{
		debug_error("MMSessionReadType Fail %s\n",__func__);
		return MM_ERROR_POLICY_INTERNAL;
	}

	/* check if it's CALL */
	if ( sessionType == MM_SESSION_TYPE_CALL || sessionType == MM_SESSION_TYPE_VIDEOCALL)
	{
		debug_log("session type is VOICE or VIDEO CALL (%d)\n", sessionType);
		return MM_ERROR_NONE;
	}

	/* interpret session type */
	event_type = __mmradio_asm_get_event_type(sessionType);

	if( ! ASM_unregister_sound( sm->handle, event_type, &errorcode) )
	{
		debug_error("Unregister sound failed 0x%X\n", errorcode);
		return MM_ERROR_POLICY_INTERNAL;
	}

	return MM_ERROR_NONE;
}

int mmradio_asm_set_state(MMRadioASM* sm, ASM_sound_states_t state, ASM_resource_t resource)
{
	int sessionType = MM_SESSION_TYPE_SHARE;
	int event_type = ASM_EVENT_NONE;
	int errorcode = 0;

	if ( ! sm )
	{
		debug_error("invalid session handle\n");
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}

	/* read session type */
//	errorcode = __MMSessionUtilReadType(&sessionType);
	errorcode = _mm_session_util_read_type(-1, &sessionType);
	if ( errorcode )
	{
		debug_error("MMSessionReadType Fail\n");
		return MM_ERROR_POLICY_INTERNAL;
	}

	/* check if it's CALL */
	if ( sessionType == MM_SESSION_TYPE_CALL || sessionType == MM_SESSION_TYPE_VIDEOCALL )
	{
		debug_log("session type is VOICE or VIDEO CALL (%d)\n", sessionType);
		return MM_ERROR_NONE;
	}

	if ( sm->by_asm_cb == MMRADIO_ASM_CB_NONE ) //|| sm->state == ASM_STATE_PLAYING )
	{
		int ret = 0;

		event_type = __mmradio_asm_get_event_type(sessionType);

		if( ! ASM_set_sound_state( sm->handle, event_type, state, resource, &ret) )
		{
			debug_error("set ASM state to [%d] failed 0x%X\n", state, ret);
			return MM_ERROR_POLICY_BLOCKED;
		}

		sm->state = state;
	}
	else // by asm callback
	{
		sm->by_asm_cb = MMRADIO_ASM_CB_NONE;
		sm->state = state;
	}

	return MM_ERROR_NONE;
}


ASM_sound_events_t __mmradio_asm_get_event_type(int type)
{
	int event_type = ASM_EVENT_NONE;

	/* interpret session type */
	switch(type)
	{
		case MM_SESSION_TYPE_SHARE:
			event_type = ASM_EVENT_SHARE_FMRADIO;
		break;

		case MM_SESSION_TYPE_EXCLUSIVE:
			event_type = ASM_EVENT_EXCLUSIVE_FMRADIO;
		break;

		case MM_SESSION_TYPE_ALARM:
			event_type = ASM_EVENT_ALARM;
		break;

		case MM_SESSION_TYPE_NOTIFY:
		case MM_SESSION_TYPE_CALL:
		case MM_SESSION_TYPE_VIDEOCALL:
		default:
			debug_critical("unexpected case! (%d)\n", type);
			assert(0);
	}

	return event_type;
}
