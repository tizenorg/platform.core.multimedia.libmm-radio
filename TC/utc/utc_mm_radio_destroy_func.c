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
	{utc_mm_radio_destroy_func_01, 1},
	{utc_mm_radio_destroy_func_02, 2},
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

void utc_mm_radio_destroy_func_01()
{
	int ret = 0;
	MMHandleType hradio;

	tet_infoline( "[[ TET_MSG ]]:: Destroy the Radio instance" );

	/* Create the instance of the Radio */
	ret = mm_radio_create(&hradio);

	dts_check_eq(__func__, ret, MM_ERROR_NONE, "err=%x", ret );

	/* Destroy the instance of the Radio */
	ret = mm_radio_destroy(hradio);

	dts_check_eq(__func__, ret, MM_ERROR_NONE, "err=%x", ret );

	return;
}


void utc_mm_radio_destroy_func_02()
{
	int ret = 0;

	tet_infoline( "[[ TET_MSG ]]:: Destroy the Radio instance" );

	/* Destroy the instance of the Radio */
	ret = mm_radio_destroy(NULL);

	dts_check_ne(__func__, ret, MM_ERROR_NONE, "err=%x", ret );

	return;
}

/** @} */




