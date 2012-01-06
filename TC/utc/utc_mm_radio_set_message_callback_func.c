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
	{utc_mm_radio_set_message_callback_func_01, 1},
	{utc_mm_radio_set_message_callback_func_02, 2},
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

static int __msg_callback(int message, void *pParam, void *user_param)
{
         MMMessageParamType* param = (MMMessageParamType*)pParam;
         MMHandleType radio = (MMHandleType) user_param;

         printf("incomming message : %d\n", message);
 
         switch(message)
         {
         case MM_MESSAGE_STATE_CHANGED:
 
                 printf("MM_MESSAGE_STATE_CHANGED: current : %d    old : %d\n"
                                 , param->state.current, param->state.previous);
                 break;
         case MM_MESSAGE_RADIO_SCAN_START:
                 printf("MM_MESSAGE_RADIO_SCAN_START\n");
                 break;
         case MM_MESSAGE_RADIO_SCAN_INFO:
                 //assert(param);
                 printf("MM_MESSAGE_RADIO_SCAN_INFO : freq : %d\n", param->radio_scan.frequency);
                 break;
         case MM_MESSAGE_RADIO_SCAN_STOP:
                 printf("MM_MESSAGE_RADIO_SCAN_STOP\n");
                 break;
         case MM_MESSAGE_RADIO_SCAN_FINISH:
                 printf("MM_MESSAGE_RADIO_SCAN_FINISHED\n");
                 mm_radio_scan_stop(radio);
                 break;
         case MM_MESSAGE_RADIO_SEEK_START:
                         printf("MM_MESSAGE_RADIO_SEEK_START\n");
                         break;
         case MM_MESSAGE_RADIO_SEEK_FINISH:
                 printf("MM_MESSAGE_RADIO_SEEK_FINISHED : freq : %d\n", param->radio_scan.frequency);
                 break;
         default:
                 printf("ERROR : unknown message received!\n");
                 break;
         }
 
         return true;
}

void utc_mm_radio_set_message_callback_func_01()
{
	int ret = 0;
	MMHandleType hradio;
	
	tet_infoline( "[[ TET_MSG ]]:: Set Message Callback for the Radio instance" );

	ret = mm_radio_create(&hradio);

	/* Set Message Callback for the instance of the Radio */
	ret = mm_radio_set_message_callback(hradio, (MMMessageCallback)__msg_callback, (void*)hradio );
	dts_check_eq(__func__, ret, MM_ERROR_NONE, "err=%x", ret );

	ret = mm_radio_destroy(hradio);

	return;
}



void utc_mm_radio_set_message_callback_func_02()
{
	int ret = 0;
	MMHandleType hradio;

	ret = mm_radio_create(&hradio);

	tet_infoline( "[[ TET_MSG ]]:: Set Message Callback for the Radio instance" );

	/* Set Message Callback for the instance of the Radio */
	ret = mm_radio_set_message_callback(NULL, (MMMessageCallback)__msg_callback, (void*)hradio );
	dts_check_ne(__func__, ret, MM_ERROR_NONE, "err=%x", ret );

	ret = mm_radio_destroy(hradio);
	
	return;
}


/** @} */




