/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR ee specific language governing permissions and
 * limitations under the License.
*/
#include "btrMgr_Types.h"

#define BTRMGR_UUID_STR_LEN_MAX             64

/* BATTERY SERVICE UUIDs */
#define BTRMGR_UUID_BATTERY_LEVEL               "00002a19-0000-1000-8000-00805f9b34fb"
#define BTRMGR_UUID_BATTERY_ERROR_VALUES        "327e5c2c-a2a2-4510-892c-73015b971fce"
#define BTRMGR_UUID_BATTERY_FLAGS               "1917392e-d7f8-4dbe-985e-4d859122bd16"

typedef unsigned long long int BTRMgrDeviceHandle;
typedef void* tBTRMgrBatteryHdl;

typedef struct _stBTRMgrBatteryNotifyChar {
    char BatteryLevel[BTRMGR_UUID_STR_LEN_MAX];
    char ErrorStatus[BTRMGR_UUID_STR_LEN_MAX];
    char BatteryFlags[BTRMGR_UUID_STR_LEN_MAX];
} stBTRMgrBatteryNotifyChar;

/* TODO:Expand the structure to store all the battery related info */
typedef struct _stBTRMgrBatteryHdl {
    stBTRMgrBatteryNotifyChar         stBtrMgrNotifyUuidList;
    int batteryLevel;
} stBTRMgrBatteryHdl;



eBTRMgrRet BTRMgr_BatteryModInit (tBTRMgrBatteryHdl* phBTRMgrBatteryHdl);
eBTRMgrRet BTRMgr_BatteryStartNotifyChar (stBTRMgrBatteryHdl* BatteryInfo, char *uuid);
eBTRMgrRet BTRMgr_TriggerBatteryStartNotify (BTRMgrDeviceHandle ahBTRMgrDevHdl,stBTRMgrBatteryHdl* BatteryInfo,tBTRCoreHandle ghBTRCoreHdl);
