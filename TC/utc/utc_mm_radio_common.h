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


#ifndef UTS_MM_FRAMEWORK_RADIO_COMMON_H
#define UTS_MM_FRAMEWORK_RADIO_COMMON_H


#include <mm_radio.h>
#include <mm_message.h>
#include <mm_error.h>
#include <mm_types.h>
#include <stdio.h>
#include <string.h>
#include <tet_api.h>
#include <unistd.h>
#include <glib.h>

#define UTC_RADIO_START_ALL(x_handle, x_ret) \
do \
{ \
	x_ret = mm_radio_create(&x_handle); \
	dts_check_eq("mm_radio_create", ret, MM_ERROR_NONE, "err=%x", ret ); \
	x_ret = mm_radio_realize(x_handle); \
	dts_check_eq("mm_radio_realize", ret, MM_ERROR_NONE, "err=%x", ret ); \
	x_ret = mm_radio_set_frequency(x_handle, 1077); \
	dts_check_eq("mm_radio_set_frequency", ret, MM_ERROR_NONE, "err=%x", ret ); \
	x_ret = mm_radio_start(x_handle); \
	dts_check_eq("mm_radio_start", ret, MM_ERROR_NONE, "err=%x", ret ); \
} while (0)

#define UTC_RADIO_DESTROY_ALL(x_handle, x_ret) \
do \
{ \
	x_ret = mm_radio_stop(x_handle);\
	dts_check_eq("mm_radio_stop", ret, MM_ERROR_NONE, "err=%x", ret ); \
	x_ret = mm_radio_unrealize(x_handle);\
	dts_check_eq("mm_radio_unrealize", ret, MM_ERROR_NONE, "err=%x", ret ); \
	x_ret = mm_radio_destroy(x_handle);\
	dts_check_eq("mm_radio_destroy", ret, MM_ERROR_NONE, "err=%x", ret ); \
} while (0)

void startup();
void cleanup();

void (*tet_startup)() = startup;
void (*tet_cleanup)() = cleanup;

void utc_mm_radio_create_func_01 ();
void utc_mm_radio_create_func_02 ();

void utc_mm_radio_destroy_func_01 ();
void utc_mm_radio_destroy_func_02 ();

void utc_mm_radio_realize_func_01 ();
void utc_mm_radio_realize_func_02 ();

void utc_mm_radio_unrealize_func_01 ();
void utc_mm_radio_unrealize_func_02 ();

void utc_mm_radio_set_message_callback_func_01 ();
void utc_mm_radio_set_message_callback_func_02 ();

void utc_mm_radio_get_state_func_01 ();
void utc_mm_radio_get_state_func_02 ();
void utc_mm_radio_get_state_func_03 ();

void utc_mm_radio_start_func_01 ();
void utc_mm_radio_start_func_02 ();

void utc_mm_radio_stop_func_01 ();
void utc_mm_radio_stop_func_02 ();
void utc_mm_radio_stop_func_03 ();

void utc_mm_radio_seek_func_01 ();
void utc_mm_radio_seek_func_02 ();
void utc_mm_radio_seek_func_03 ();
void utc_mm_radio_seek_func_04 ();

//Frequency range 875-1080
void utc_mm_radio_set_frequency_func_01 ();
void utc_mm_radio_set_frequency_func_02 ();
void utc_mm_radio_set_frequency_func_03 ();
void utc_mm_radio_set_frequency_func_04 ();

void utc_mm_radio_get_frequency_func_01 ();
void utc_mm_radio_get_frequency_func_02 ();

void utc_mm_radio_scan_start_func_01 ();
void utc_mm_radio_scan_start_func_02 ();

void utc_mm_radio_scan_stop_func_01 ();
void utc_mm_radio_scan_stop_func_02 ();

#endif /* UTS_MM_FRAMEWORK_RADIO_COMMON_H */
