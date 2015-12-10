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
#include "unistd.h"

int mmradio_audio_focus_register(MMRadioAudioFocus *sm, mm_sound_focus_changed_cb callback, void *param)
{
	/* read mm-session information */
	int session_type = MM_SESSION_TYPE_MEDIA;
	int session_flags = 0;
	int errorcode = MM_ERROR_NONE;
	int pid = getpid();
	int handle;

	MMRADIO_LOG_FENTER();

	if (!sm) {
		MMRADIO_LOG_ERROR("invalid session handle\n");
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}
	sm->cur_focus_type = FOCUS_NONE;

	/* read session information */
	errorcode = _mm_session_util_read_information(pid, &session_type, &session_flags);
	if (errorcode == MM_ERROR_NONE) {
		debug_warning("Read Session Information success. session_type : %d flags: %d \n", session_type, session_flags);
		sm->snd_session_flags = session_flags;
		session_type = MM_SESSION_TYPE_MEDIA;
	} else {
		debug_warning("Read Session Information failed. skip sound focus register function. errorcode %x \n", errorcode);
	}

	/* check if it's MEDIA type */
	if (session_type != MM_SESSION_TYPE_MEDIA) {
		MMRADIO_LOG_DEBUG("session type is not MEDIA (%d)\n", session_type);
		return MM_ERROR_RADIO_INTERNAL;
	}

	/* check if it's running on the media_server */
	sm->pid = pid;

	MMRADIO_LOG_DEBUG("sound register focus pid[%d]", pid);

	mm_sound_focus_get_id(&handle);
	sm->handle = handle;

	if (mm_sound_register_focus_for_session(handle, pid, "radio", callback, param) != MM_ERROR_NONE) {
		MMRADIO_LOG_DEBUG("mm_sound_register_focus_for_session is failed\n");
		return MM_ERROR_POLICY_BLOCKED;
	}

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

int mmradio_audio_focus_deregister(MMRadioAudioFocus *sm)
{
	MMRADIO_LOG_FENTER();

	if (!sm) {
		MMRADIO_LOG_ERROR("invalid session handle\n");
		return MM_ERROR_RADIO_NOT_INITIALIZED;
	}

	if (MM_ERROR_NONE != mm_sound_unregister_focus(sm->handle))
		MMRADIO_LOG_ERROR("mm_sound_unregister_focus failed\n");

	MMRADIO_LOG_FLEAVE();

	return MM_ERROR_NONE;
}

int mmradio_acquire_audio_focus(MMRadioAudioFocus *sm)
{
	int ret = MM_ERROR_NONE;
	mm_sound_focus_type_e focus_type = FOCUS_NONE;
	MMRADIO_LOG_FENTER();

	MMRADIO_LOG_ERROR("mmradio_acquire_audio_focus sm->cur_focus_type : %d\n", sm->cur_focus_type);

	focus_type = FOCUS_FOR_BOTH & ~(sm->cur_focus_type);
	if (focus_type != FOCUS_NONE) {
		ret = mm_sound_acquire_focus(sm->handle, focus_type, NULL);
		if (ret != MM_ERROR_NONE) {
			MMRADIO_LOG_ERROR("mm_sound_acquire_focus is failed\n");
			return MM_ERROR_POLICY_BLOCKED;
		}
		sm->cur_focus_type = FOCUS_FOR_BOTH;
	}

	MMRADIO_LOG_FLEAVE();
	return ret;
}

int mmradio_release_audio_focus(MMRadioAudioFocus *sm)
{
	int ret = MM_ERROR_NONE;
	MMRADIO_LOG_FENTER();

	MMRADIO_LOG_ERROR("mmradio_release_audio_focus sm->cur_focus_type : %d\n", sm->cur_focus_type);
	if (sm->cur_focus_type != FOCUS_NONE) {
		ret = mm_sound_release_focus(sm->handle, sm->cur_focus_type, NULL);
		if (ret != MM_ERROR_NONE) {
			MMRADIO_LOG_ERROR("mm_sound_release_focus is failed\n");
			return MM_ERROR_POLICY_BLOCKED;
		}
		sm->cur_focus_type = FOCUS_NONE;
	}

	MMRADIO_LOG_FLEAVE();
	return ret;
}

#define AUDIO_FOCUS_REASON_MAX	128

void mmradio_get_audio_focus_reason(mm_sound_focus_state_e focus_state, const char *reason_for_change, enum MMMessageInterruptedCode *event_source, int *postMsg)
{
	MMRADIO_LOG_FENTER();
	MMRADIO_LOG_ERROR("mmradio_get_audio_focus_reason focus_state : %d reason_for_change :%s\n", focus_state, reason_for_change);

	if (0 == strncmp(reason_for_change, "call-voice", AUDIO_FOCUS_REASON_MAX)
		|| (0 == strncmp(reason_for_change, "voip", AUDIO_FOCUS_REASON_MAX))
		|| (0 == strncmp(reason_for_change, "ringtone-voip", AUDIO_FOCUS_REASON_MAX))
		|| (0 == strncmp(reason_for_change, "ringtone-call", AUDIO_FOCUS_REASON_MAX))
		) {
		if (focus_state == FOCUS_IS_RELEASED)
			*event_source = MM_MSG_CODE_INTERRUPTED_BY_CALL_START;
		else if (focus_state == FOCUS_IS_ACQUIRED)
			*event_source = MM_MSG_CODE_INTERRUPTED_BY_CALL_END;
		*postMsg = true;
	} else if (0 == strncmp(reason_for_change, "alarm", AUDIO_FOCUS_REASON_MAX)) {
		if (focus_state == FOCUS_IS_RELEASED)
			*event_source = MM_MSG_CODE_INTERRUPTED_BY_ALARM_START;
		else if (focus_state == FOCUS_IS_ACQUIRED)
			*event_source = MM_MSG_CODE_INTERRUPTED_BY_ALARM_END;
		*postMsg = true;
	} else if (0 == strncmp(reason_for_change, "notification", AUDIO_FOCUS_REASON_MAX)) {
		if (focus_state == FOCUS_IS_RELEASED)
			*event_source = MM_MSG_CODE_INTERRUPTED_BY_NOTIFICATION_START;
		else if (focus_state == FOCUS_IS_ACQUIRED)
			*event_source = MM_MSG_CODE_INTERRUPTED_BY_NOTIFICATION_END;
		*postMsg = true;
	} else if (0 == strncmp(reason_for_change, "emergency", AUDIO_FOCUS_REASON_MAX)) {
		if (focus_state == FOCUS_IS_RELEASED)
			*event_source = MM_MSG_CODE_INTERRUPTED_BY_EMERGENCY_START;
		else if (focus_state == FOCUS_IS_ACQUIRED)
			*event_source = MM_MSG_CODE_INTERRUPTED_BY_EMERGENCY_END;
		*postMsg = false;
	} else if (0 == strncmp(reason_for_change, "media", AUDIO_FOCUS_REASON_MAX)) {
		*event_source = MM_MSG_CODE_INTERRUPTED_BY_MEDIA;
		*postMsg = false;
	} else {
		*event_source = MM_MSG_CODE_INTERRUPTED_BY_MEDIA;
		*postMsg = false;
	}
	MMRADIO_LOG_FLEAVE();
}
