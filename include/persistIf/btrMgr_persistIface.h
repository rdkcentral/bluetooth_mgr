/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2017 RDK Management
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
*/
/**
 * @file btrMgr_persistIface.h
 *
 * @defgroup Persistant_Interface Persistant Interface
 * This file defines bluetooth manager's Persistent storage interfaces.
 * @ingroup  BTR_MGR
 *
 */
#ifndef __BTR_MGR_PERSIST_IFCE_H__
#define __BTR_MGR_PERSIST_IFCE_H__

#ifdef UNIT_TEST
#include "btrMgr_Types.h"
#endif

/**
 * @addtogroup  Persistant_Interface
 * @{
 *
 */

#define BTRMGR_NAME_LEN_MAX            64
#define BTRMGR_MAX_PERSISTENT_PROFILE_COUNT 5
#define BTRMGR_MAX_PERSISTENT_DEVICE_COUNT	5
#ifdef UNIT_TEST
#define BTRMGR_PERSISTENT_DATA_PATH JSON_PATH_UNIT_TEST
#else
#define BTRMGR_PERSISTENT_DATA_PATH "/opt/lib/bluetooth/btmgrPersist.json"
#endif
#define BTRMGR_A2DP_SRC_PROFILE_ID  "0x110a"
#define BTRMGR_A2DP_SINK_PROFILE_ID "0x110b"

#ifdef BUILD_RDKTV
#define RDKTV_PERSIST_VOLUME 1
#endif

typedef void* tBTRMgrPIHdl;

/**
 * @brief  This API initializes bluetooth manager's persistent storage interface.
 */
typedef struct _stBTRMgrPersistDevice {
    unsigned long long deviceId;
    int isConnected;
    int lastConnected;
#ifdef RDKTV_PERSIST_VOLUME
    int Volume;
#endif
} stBTRMgrPersistDevice;

typedef struct _stBTRMgrPersistProfile {
    unsigned char numOfDevices;
    char profileId[BTRMGR_NAME_LEN_MAX];
    stBTRMgrPersistDevice deviceList[BTRMGR_MAX_PERSISTENT_DEVICE_COUNT];
} stBTRMgrPersistProfile;

typedef struct _BTRMGR_Beacon_PersistentData_t {
    char limitBeaconDetection[BTRMGR_NAME_LEN_MAX];
} BTRMGR_Beacon_PersistentData_t;

#ifdef RDKTV_PERSIST_VOLUME
typedef struct _BTRMGR_Volume_PersistentData_t {
    unsigned  char Volume;
} BTRMGR_Volume_PersistentData_t;

typedef struct _BTRMGR_Mute_PersistentData_t {
    char Mute[BTRMGR_NAME_LEN_MAX];
} BTRMGR_Mute_PersistentData_t;
#endif

typedef struct _BTRMGR_ConStatus_PersistentData_t {
    int ConStatus;
} BTRMGR_ConStatus_PersistentData_t;

typedef struct _BTRMGR_PersistentData_t {
    char adapterId[BTRMGR_NAME_LEN_MAX];
    unsigned short numOfProfiles;
    stBTRMgrPersistProfile profileList[BTRMGR_NAME_LEN_MAX];
    char limitBeaconDetection[BTRMGR_NAME_LEN_MAX];
} BTRMGR_PersistentData_t;


typedef struct _BTRMGR_Profile_t {
    char adapterId[BTRMGR_NAME_LEN_MAX];
    char profileId[BTRMGR_NAME_LEN_MAX];
    unsigned long long deviceId;
    int isConnect;
    int lastConnected;
    int Volume;
} BTRMGR_Profile_t;

eBTRMgrRet BTRMgr_PI_GetLEBeaconLimitingStatus (BTRMGR_Beacon_PersistentData_t*    persistentData);
eBTRMgrRet BTRMgr_PI_SetLEBeaconLimitingStatus (BTRMGR_Beacon_PersistentData_t*    persistentData);
eBTRMgrRet BTRMgr_PI_SetConnectionStatus (BTRMGR_ConStatus_PersistentData_t*    persistentData,const char *ProfileStr,unsigned long long int deviceID);
#ifdef RDKTV_PERSIST_VOLUME
eBTRMgrRet BTRMgr_PI_GetVolume (BTRMGR_Volume_PersistentData_t*    persistentData,const char *ProfileStr,unsigned long long int deviceID);
eBTRMgrRet BTRMgr_PI_SetVolume (BTRMGR_Volume_PersistentData_t*    persistentData,const char *ProfileStr,unsigned long long int deviceID);
eBTRMgrRet BTRMgr_PI_GetMute (BTRMGR_Mute_PersistentData_t*    persistentData);
eBTRMgrRet BTRMgr_PI_SetMute (BTRMGR_Mute_PersistentData_t*    persistentData);
#endif

/**
 * @brief  This API initializes bluetooth manager's persistent storage interface.
 *
 * @param[in] hBTRMgrPiHdl      Handle to the audio capture interface.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSuccess on success, appropriate error code otherwise.
 */
eBTRMgrRet BTRMgr_PI_Init (tBTRMgrPIHdl* hBTRMgrPiHdl);


/**
* @brief  This API deinitializes bluetooth manager's persistent storage interface.
*
* @param[in] hBTRMgrPiHdl      Handle to the audio capture interface.
*
* @return Returns the status of the operation.
* @retval eBTRMgrSuccess on success, appropriate error code otherwise.
*/
eBTRMgrRet BTRMgr_PI_DeInit (tBTRMgrPIHdl hBTRMgrPiHdl);

/**
 * @brief  This API reads all bluetooth profiles from json file and saves to BTRMGR_PersistentData_t structure.
 *
 * @param[in] hBTRMgrPiHdl     Handle to the audio capture interface.
 * @param[out] persistentData  Structure that fetches persistent profiles data.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSuccess on success, appropriate error code otherwise.
 */
eBTRMgrRet BTRMgr_PI_GetAllProfiles(tBTRMgrPIHdl hBTRMgrPiHdl,BTRMGR_PersistentData_t* persistentData);


/**
 * @brief  This API sets all bluetooth profiles.
 *
 * @param[in] hBTRMgrPiHdl     Handle to the audio capture interface.
 * @param[in] persistentData   Structure that is used to set the profiles data.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSuccess on success, appropriate error code otherwise.
 */
eBTRMgrRet BTRMgr_PI_SetAllProfiles (tBTRMgrPIHdl hBTRMgrPiHdl,BTRMGR_PersistentData_t* persistentData);

/**
 * @brief  This API adds a single bluetooth profile to json file.
 *
 * @param[in] hBTRMgrPiHdl    Handle to the audio capture interface.
 * @param[in] persistProfile  Profile to be added.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSuccess on success, appropriate error code otherwise.
 */
eBTRMgrRet BTRMgr_PI_AddProfile (tBTRMgrPIHdl hBTRMgrPiHdl,BTRMGR_Profile_t* persistProfile);

/**
 * @brief  This API removes a single bluetooth profile from json file.
 *
 * @param[in] hBTRMgrPiHdl     Handle to the persistant storage interface.
 * @param[in] persistProfile   Profile to be removed.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSuccess on success, appropriate error code otherwise.
 */
eBTRMgrRet BTRMgr_PI_RemoveProfile (tBTRMgrPIHdl hBTRMgrPiHdl,BTRMGR_Profile_t* persistProfile);
/** @} */

#endif /* __BTR_MGR_PERSIST_IFCE_H__ */
