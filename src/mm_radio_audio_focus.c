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
#include "mm_radio_audio_focus.h"
#include "mm_radio_utils.h"
#include "mm_sound_focus.h"

int mmradio_audio_focus_register(MMRadioAudioFocus* sm, mm_sound_focus_changed_cb callback, void* param)
{
	/* read mm-session information */
	int session_type = MM_SESSION_TYPE_MEDIA;
	int session_flags = 0;
	int errorcode = MM_ERROR_NONE;
	int pid = getpid();
	int handle;

	MMRADIO_LOG_FENTER();

	if ( ! sm )
	{
		MMRADIO_LOG_ERROR("invalid session handle\n");
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}

	/* read session information */
	errorcode = _mm_session_util_read_information(pid, &session_type, &session_flags);
	if ( errorcode == MM_ERROR_NONE )
	{
		debug_warning("Read Session Information success. session_type : %d flags: %d \n", session_type, session_flags);
		sm->sound_focus_register = true;
		sm->asm_session_flags = session_flags;
		session_type = MM_SESSION_TYPE_MEDIA;
	}
	else
	{
		debug_warning("Read Session Information failed. skip sound focus function. errorcode %x \n" ,errorcode);
		sm->sound_focus_register = false;
	}

	/* check if it's MEDIA type */
	if ( session_type != MM_SESSION_TYPE_MEDIA)
	{
		MMRADIO_LOG_DEBUG("session type is not MEDIA (%d)\n", session_type);
		return MM_ERROR_RADIO_INTERNAL;
	}

	/* check if it's running on the media_server */
	if ( pid > 0 )
		sm->pid = pid;
	else
		return MM_ERROR_INVALID_ARGUMENT;

    MMRADIO_LOG_DEBUG("sound register focus pid[%d]", pid);

	mm_sound_focus_get_id(&handle);
	sm->handle = handle;
	if(sm->sound_focus_register)
	{
		if(mm_sound_register_focus_for_session(handle, pid, "radio", callback, param) != MM_ERROR_NONE)
		{
			MMRADIO_LOG_DEBUG("mm_sound_register_focus_for_session is failed\n");
			return MM_ERROR_POLICY_BLOCKED;
		}
		else
			MMRADIO_LOG_DEBUG("mm_sound_register_focus_for_session is success. handle : %d session_type : %d, flags : %x\n", handle, session_type, sm->asm_session_flags);
	}

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}


int mmradio_audio_focus_deregister(MMRadioAudioFocus* sm)
{
	MMRADIO_LOG_FENTER();

	if ( ! sm )
	{
		MMRADIO_LOG_ERROR("invalid session handle\n");
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}

	if(sm->sound_focus_register)
	{
		if (MM_ERROR_NONE != mm_sound_unregister_focus(sm->handle))
			MMRADIO_LOG_ERROR("mm_sound_unregister_focus failed\n");
	}

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

int mmradio_set_audio_focus(MMRadioAudioFocus* sm, mm_sound_focus_changed_watch_cb callback, int bSet, void* param)
{
	int ret = MM_ERROR_NONE;
	MMRADIO_LOG_FENTER();
#if 0    
	if(sm->sound_focus_register == false)
		return ret;
#endif

	MMRADIO_LOG_ERROR("mmradio_set_audio_focus bSet: %d asm_session_flags : %d\n", bSet, sm->asm_session_flags);
	if(sm->asm_session_flags & MM_SESSION_OPTION_PAUSE_OTHERS)
	{
		if(bSet)
			ret = mm_sound_acquire_focus(sm->handle, FOCUS_FOR_BOTH, NULL);
		else
			ret = mm_sound_release_focus(sm->handle, FOCUS_FOR_BOTH, NULL);

		if(ret != MM_ERROR_NONE)
		{
			MMRADIO_LOG_ERROR("mm_sound_set_audio_focus failed set : %d\n", bSet);
			return MM_ERROR_POLICY_BLOCKED;
		}
	}
	else
	{
		if(bSet)
			ret = mm_sound_set_focus_watch_callback_for_session(sm->pid, FOCUS_FOR_BOTH, callback, param, &sm->handle);
		else
			ret = mm_sound_unset_focus_watch_callback(sm->handle);

		if(ret != MM_ERROR_NONE)
		{
			MMRADIO_LOG_ERROR("mm_sound_set_audio_focus failed set : %d\n", bSet);
			return MM_ERROR_POLICY_BLOCKED;
		}

		MMRADIO_LOG_ERROR("mmradio_set_audio_focus ret: %d\n", ret);
	}

	MMRADIO_LOG_FLEAVE();
	return ret ;
}

#define AUDIO_FOCUS_REASON_MAX	128

void mmradio_get_audio_focus_reason(mm_sound_focus_state_e focus_state, const char *reason_for_change, ASM_event_sources_t *event_source, int *postMsg)
{
	MMRADIO_LOG_FENTER();
	MMRADIO_LOG_ERROR("mmradio_get_audio_focus_reason focus_state : %d reason_for_change :%s\n", focus_state, reason_for_change);

	if(0 == strncmp(reason_for_change, "call-voice", AUDIO_FOCUS_REASON_MAX)
		|| (0 == strncmp(reason_for_change, "voip", AUDIO_FOCUS_REASON_MAX))
		|| (0 == strncmp(reason_for_change, "ringtone-voip", AUDIO_FOCUS_REASON_MAX))
		|| (0 == strncmp(reason_for_change, "ringtone-call", AUDIO_FOCUS_REASON_MAX))
		)
	{
		if(focus_state == FOCUS_IS_RELEASED)
			*event_source = ASM_EVENT_SOURCE_CALL_START;
		else if(focus_state == FOCUS_IS_ACQUIRED)
			*event_source = ASM_EVENT_SOURCE_CALL_END;
		*postMsg = true;
	}
	else if(0 == strncmp(reason_for_change, "alarm", AUDIO_FOCUS_REASON_MAX))
	{
		if(focus_state == FOCUS_IS_RELEASED)
			*event_source = ASM_EVENT_SOURCE_ALARM_START;
		else if(focus_state == FOCUS_IS_ACQUIRED)
			*event_source = ASM_EVENT_SOURCE_ALARM_END;
		*postMsg = true;
	}
	else if(0 == strncmp(reason_for_change, "notification", AUDIO_FOCUS_REASON_MAX))
	{
		if(focus_state == FOCUS_IS_RELEASED)
			*event_source = ASM_EVENT_SOURCE_NOTIFY_START;
		else if(focus_state == FOCUS_IS_ACQUIRED)
			*event_source = ASM_EVENT_SOURCE_NOTIFY_END;
		*postMsg = true;
	}
	else if(0 == strncmp(reason_for_change, "emergency", AUDIO_FOCUS_REASON_MAX))
	{
		if(focus_state == FOCUS_IS_RELEASED)
			*event_source = ASM_EVENT_SOURCE_EMERGENCY_START;
		else if(focus_state == FOCUS_IS_ACQUIRED)
			*event_source = ASM_EVENT_SOURCE_EMERGENCY_END;
		*postMsg = false;
	}
	else if(0 == strncmp(reason_for_change, "media", AUDIO_FOCUS_REASON_MAX))
	{
		*event_source = ASM_EVENT_SOURCE_MEDIA;
		*postMsg = false;
	}
	else
	{
		*event_source = ASM_EVENT_SOURCE_MEDIA;
		*postMsg = false;
	}
	MMRADIO_LOG_FLEAVE();
}
