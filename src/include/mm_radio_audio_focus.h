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

#ifndef MM_RADIO_AUDIO_FOCUS_H_
#define MM_RADIO_AUDIO_FOCUS_H_

#include <mm_types.h>
#include <mm_error.h>

#include <mm_session.h>
#include <mm_session_private.h>
#include <audio-session-manager.h>
#include <mm_sound_focus.h>

enum {
	MMRADIO_ASM_CB_NONE,
	MMRADIO_ASM_CB_POSTMSG,
	MMRADIO_ASM_CB_SKIP_POSTMSG
};
typedef struct {
	int handle;
	int pid;
	int by_asm_cb;
	int event_src;
	int sound_focus_register;
	int asm_session_flags;
} MMRadioAudioFocus;

int mmradio_audio_focus_register(MMRadioAudioFocus* sm, mm_sound_focus_changed_cb callback, void* param);
int mmradio_audio_focus_deregister(MMRadioAudioFocus* sm);
int mmradio_set_audio_focus(MMRadioAudioFocus* sm, mm_sound_focus_changed_watch_cb callback, void* param);
int mmradio_unset_audio_focus(MMRadioAudioFocus* sm, void* param);
void mmradio_get_audio_focus_reason(mm_sound_focus_state_e focus_state, const char *reason_for_change, ASM_event_sources_t *event_source, int *postMsg);

#endif /* MM_RADIO_AUDIO_FOCUS_H_ */
