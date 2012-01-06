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

#include "utc_mm_radio_common.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////////////////////
// Declare the global variables and registers and Internal Funntions
//-------------------------------------------------------------------------------------------------

struct tet_testlist tet_testlist[] = {
	{utc_mm_radio_get_sound_path_func_01, 1},
	{utc_mm_radio_get_sound_path_func_02, 2},
	{NULL, 0}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/* Initialize TCM data structures */


/* Start up function for each test purpose */
void
startup ()
{
	tet_infoline("[[ COMMON ]]::Inside startup \n");

	tet_infoline("[[ COMMON ]]::Completing startup \n");

}

/* Clean up function for each test purpose */
void
cleanup ()
{

}

void utc_mm_radio_get_sound_path_func_01()
{
	MMHandleType hradio;
	int ret = 0;	
	MMRadioOutputType soundpath;

	tet_infoline( "[[ TET_MSG ]]:: Get sound path for the Radio instance" );

	UTC_RADIO_START_ALL(hradio, ret);

	/* Get sound path for the instance of the Radio */
	ret = mm_radio_get_sound_path(hradio, &soundpath);

	dts_check_eq(__func__, ret, MM_ERROR_NONE, "err=%x", ret );

	UTC_RADIO_DESTROY_ALL(hradio, ret);

	return;
}

void utc_mm_radio_get_sound_path_func_02()
{
	MMHandleType hradio;
	int ret = 0; 

	tet_infoline( "[[ TET_MSG ]]:: Get sound path for the Radio instance" );

	UTC_RADIO_START_ALL(hradio, ret);

	/* Get sound path for the instance of the Radio */
	ret = mm_radio_get_sound_path(hradio, NULL);
	
	dts_check_ne(__func__, ret, MM_ERROR_NONE, "err=%x", ret );

	UTC_RADIO_DESTROY_ALL(hradio, ret);

	return;
}


/** @} */




