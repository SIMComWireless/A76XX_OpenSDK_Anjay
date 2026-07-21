/**
  ******************************************************************************
  * @file    AppMain.c
  * @author  SIMCom OpenSDK Team
  * @brief   Source code for SIMCom OpenSDK application, void userSpace_Main(void * arg) is the app entry for OpenSDK application,customer should start application from this call.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 SIMCom Wireless.
  * All rights reserved.
  *
  ******************************************************************************
  */

/*----------------------------------------------------------------------
*  Include files
*----------------------------------------------------------------------*/
#include "include.h"

#include "userspaceConfig.h"
#include "api_map.h"
#include "simcom_debug.h"
#include "simcom_os.h"

/*----------------------------------------------------------------------
*  Function Definitions
*----------------------------------------------------------------------*/
/**
  * @brief  OpenSDK app entry.
  * @param  Pointer arg
  * @note   This is the app entry,like main(),all functions must start from here!!!!!!
  * @retval void
  */
void userSpace_Main(void *arg)
{

    //SIMCom api init. Do not modify! 
    ApiMapInit(arg);
    //catch dump log 
    sAPI_enableDUMP();

}

void abort(void)
{
    printf("abort!!!");
    while (1);
}

#define _appRegTable_attr_ __attribute__((unused, section(".userSpaceRegTable")))

#define appMainStackSize (1024*10)

#ifndef APP_VERSION
#define APP_VERSION "Anjay_SDK_1.0.0"
#endif
userSpaceEntry_t userSpaceEntry _appRegTable_attr_ = {NULL, NULL, appMainStackSize, 30, "userSpaceMain", userSpace_Main, APP_VERSION };

