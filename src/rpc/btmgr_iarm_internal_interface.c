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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <string.h>


#include "btmgr.h"
#include "btrMgr_logger.h"
#include "btmgr_iarm_interface.h"
#include "btrMgr_IarmInternalIfce.h"

#include "safec_lib.h"

#ifndef UNIT_TEST
#define STATIC static
#else 
#define STATIC 
#endif 

/* STATIC Function Prototypes */
STATIC IARM_Result_t btrMgr_GetNumberOfAdapters (void* arg);
STATIC IARM_Result_t btrMgr_SetAdapterName (void* arg);
STATIC IARM_Result_t btrMgr_GetAdapterName (void* arg);
STATIC IARM_Result_t btrMgr_SetAdapterPowerStatus (void* arg); 
STATIC IARM_Result_t btrMgr_GetAdapterPowerStatus (void* arg); 
STATIC IARM_Result_t btrMgr_SetAdapterDiscoverable (void* arg); 
STATIC IARM_Result_t btrMgr_IsAdapterDiscoverable (void* arg);
STATIC IARM_Result_t btrMgr_ChangeDeviceDiscoveryStatus (void* arg);
STATIC IARM_Result_t btrMgr_GetDeviceDiscoveryStatus (void* arg);
STATIC IARM_Result_t btrMgr_GetDiscoveredDevices (void* arg);
STATIC IARM_Result_t btrMgr_PairDevice (void* arg); 
STATIC IARM_Result_t btrMgr_UnpairDevice (void* arg); 
STATIC IARM_Result_t btrMgr_GetPairedDevices (void* arg);
STATIC IARM_Result_t btrMgr_ConnectToDevice (void* arg);
STATIC IARM_Result_t btrMgr_DisconnectFromDevice (void* arg);
STATIC IARM_Result_t btrMgr_GetConnectedDevices (void* arg); 
STATIC IARM_Result_t btrMgr_GetDeviceProperties (void* arg); 
STATIC IARM_Result_t btrMgr_GetDeviceBatteryLevel (void* arg);
STATIC IARM_Result_t btrMgr_StartAudioStreamingOut (void* arg); 
STATIC IARM_Result_t btrMgr_StopAudioStreamingOut (void* arg); 
STATIC IARM_Result_t btrMgr_IsAudioStreamingOut (void* arg);
STATIC IARM_Result_t btrMgr_SetAudioStreamOutType (void* arg); 
STATIC IARM_Result_t btrMgr_StartAudioStreamingIn (void* arg); 
STATIC IARM_Result_t btrMgr_StopAudioStreamingIn (void* arg); 
STATIC IARM_Result_t btrMgr_IsAudioStreamingIn (void* arg);
STATIC IARM_Result_t btrMgr_SetEventResponse (void* arg);
STATIC IARM_Result_t btrMgr_ResetAdapter (void* arg);
STATIC IARM_Result_t btrMgr_MediaControl (void* arg);
STATIC IARM_Result_t btrMgr_SetDeviceVolumeMute (void* arg);
STATIC IARM_Result_t btrMgr_GetDeviceVolumeMute (void* arg);
STATIC IARM_Result_t btrMgr_GetMediaTrackInfo (void* arg);
STATIC IARM_Result_t btrMgr_GetMediaElementTrackInfo (void* arg);
STATIC IARM_Result_t btrMgr_GetMediaCurrentPosition (void* arg);
STATIC IARM_Result_t btrMgr_SetMediaElementActive (void* arg);
STATIC IARM_Result_t btrMgr_GetMediaElementList (void* arg);
STATIC IARM_Result_t btrMgr_SelectMediaElement (void* arg);
STATIC IARM_Result_t btrMgr_GetLeProperty (void* arg);
STATIC IARM_Result_t btrMgr_PerformLeOp (void* arg);
STATIC IARM_Result_t btrMgr_SetAudioInServiceState (void* arg);
STATIC IARM_Result_t btrMgr_SysDiagInfo(void* arg);
STATIC IARM_Result_t btrMgr_ConnectToWifi(void* arg);
STATIC IARM_Result_t btrMgr_GetLimitBeaconDetection (void* arg);
STATIC IARM_Result_t btrMgr_SetLimitBeaconDetection (void* arg);
STATIC IARM_Result_t btrMgr_DeInit (void* arg);
STATIC IARM_Result_t btrMgr_LeStartAdvertisement(void* arg);
STATIC IARM_Result_t btrMgr_LeStopAdvertisement(void *arg);
STATIC IARM_Result_t btrMgr_LeGetPropertyValue(void* arg);
STATIC IARM_Result_t btrMgr_LeSetServiceInfo(void* arg);
STATIC IARM_Result_t btrMgr_LeSetGattInfo(void* arg);
STATIC IARM_Result_t btrMgr_LeSetGattPropertyValue(void* arg);

/* Callbacks Prototypes */
STATIC BTRMGR_Result_t btrMgr_EventCallback (BTRMGR_EventMessage_t astEventMessage); 

STATIC unsigned char gIsBTRMGR_Internal_Inited = 0;


/* STATIC Function Definition */
STATIC IARM_Result_t
btrMgr_GetNumberOfAdapters (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    unsigned char   numOfAdapters = 0;
    unsigned char*  pNumberOfAdapters = (unsigned char*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pNumberOfAdapters) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_GetNumberOfAdapters(&numOfAdapters);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        *pNumberOfAdapters = numOfAdapters;
        BTRMGRLOG_INFO ("Success; Number of Adapters = %d\n", numOfAdapters);
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_SetAdapterName (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMAdapterName_t* pName = (BTRMGR_IARMAdapterName_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pName) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_SetAdapterName(pName->m_adapterIndex, pName->m_name);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_GetAdapterName (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMAdapterName_t* pName = (BTRMGR_IARMAdapterName_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pName) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_GetAdapterName(pName->m_adapterIndex, pName->m_name);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success ; Adapter name is %s\n", pName->m_name);
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_SetAdapterPowerStatus (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMAdapterPower_t* pPowerStatus = (BTRMGR_IARMAdapterPower_t*) arg;

    BTRMGRLOG_DEBUG ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pPowerStatus) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_SetAdapterPowerStatus(pPowerStatus->m_adapterIndex, pPowerStatus->m_powerStatus);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_DEBUG ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_GetAdapterPowerStatus (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMAdapterPower_t* pPowerStatus = (BTRMGR_IARMAdapterPower_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pPowerStatus) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_GetAdapterPowerStatus(pPowerStatus->m_adapterIndex, &pPowerStatus->m_powerStatus);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_SetAdapterDiscoverable (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMAdapterDiscoverable_t* pDiscoverable = (BTRMGR_IARMAdapterDiscoverable_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pDiscoverable) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_SetAdapterDiscoverable(pDiscoverable->m_adapterIndex, pDiscoverable->m_isDiscoverable, pDiscoverable->m_timeout);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_IsAdapterDiscoverable (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMAdapterDiscoverable_t* pDiscoverable = (BTRMGR_IARMAdapterDiscoverable_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pDiscoverable) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_IsAdapterDiscoverable(pDiscoverable->m_adapterIndex, &pDiscoverable->m_isDiscoverable);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_ChangeDeviceDiscoveryStatus (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMAdapterDiscover_t* pAdapterIndex = (BTRMGR_IARMAdapterDiscover_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pAdapterIndex) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    if (pAdapterIndex->m_setDiscovery)
        rc = BTRMGR_StartDeviceDiscovery(pAdapterIndex->m_adapterIndex, pAdapterIndex->m_enBTRMgrDevOpT);
    else
        rc = BTRMGR_StopDeviceDiscovery(pAdapterIndex->m_adapterIndex, pAdapterIndex->m_enBTRMgrDevOpT);

    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_GetDeviceDiscoveryStatus (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMDiscoveryStatus_t* pDiscoverStatus = (BTRMGR_IARMDiscoveryStatus_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pDiscoverStatus) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_GetDiscoveryStatus(pDiscoverStatus->m_adapterIndex, &pDiscoverStatus->m_discoveryInProgress, &pDiscoverStatus->m_discoveryType);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }
    return retCode;
}

STATIC IARM_Result_t
btrMgr_GetDiscoveredDevices (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMDiscoveredDevices_t* pDiscoveredDevices = (BTRMGR_IARMDiscoveredDevices_t*) arg;

    BTRMGRLOG_TRACE ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pDiscoveredDevices) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_GetDiscoveredDevices(pDiscoveredDevices->m_adapterIndex, &pDiscoveredDevices->m_devices);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_TRACE ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_PairDevice (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMPairDevice_t* pPairDevice = (BTRMGR_IARMPairDevice_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pPairDevice) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_PairDevice(pPairDevice->m_adapterIndex, pPairDevice->m_deviceHandle);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_DEBUG ("Success\n");
    }
    else {
#if 0
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
#endif
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_UnpairDevice (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMPairDevice_t* pUnPairDevice = (BTRMGR_IARMPairDevice_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pUnPairDevice) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_UnpairDevice(pUnPairDevice->m_adapterIndex, pUnPairDevice->m_deviceHandle);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
#if 0
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
#endif
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_GetPairedDevices (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMPairedDevices_t* pPairedDevices = (BTRMGR_IARMPairedDevices_t*) arg;

    BTRMGRLOG_TRACE ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pPairedDevices) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_GetPairedDevices(pPairedDevices->m_adapterIndex, &pPairedDevices->m_devices);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_TRACE ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t 
btrMgr_ConnectToDevice (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMConnectDevice_t* pConnect = (BTRMGR_IARMConnectDevice_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pConnect) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_ConnectToDevice(pConnect->m_adapterIndex, pConnect->m_deviceHandle, pConnect->m_connectAs);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_DisconnectFromDevice (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMConnectDevice_t* pConnect = (BTRMGR_IARMConnectDevice_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pConnect) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_DisconnectFromDevice(pConnect->m_adapterIndex, pConnect->m_deviceHandle);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_GetConnectedDevices (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMConnectedDevices_t* pConnectedDevices = (BTRMGR_IARMConnectedDevices_t*) arg;

    BTRMGRLOG_TRACE ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pConnectedDevices) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_GetConnectedDevices(pConnectedDevices->m_adapterIndex, &pConnectedDevices->m_devices);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_TRACE ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_GetDeviceProperties (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMDDeviceProperty_t* pDeviceProperty = (BTRMGR_IARMDDeviceProperty_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pDeviceProperty) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_GetDeviceProperties(pDeviceProperty->m_adapterIndex, pDeviceProperty->m_deviceHandle, &pDeviceProperty->m_deviceProperty);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}
STATIC IARM_Result_t
btrMgr_GetDeviceBatteryLevel (
    void * arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMDeviceBatteryLevel_t* pDeviceBattLevelInfo = (BTRMGR_IARMDeviceBatteryLevel_t*) arg;
    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pDeviceBattLevelInfo) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_GetDeviceBatteryLevel(pDeviceBattLevelInfo->m_adapterIndex, pDeviceBattLevelInfo->m_deviceHandle, &pDeviceBattLevelInfo->deviceBatteryLevel);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_StartAudioStreamingOut (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMStreaming_t*  pStartStream = (BTRMGR_IARMStreaming_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pStartStream) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_StartAudioStreamingOut(pStartStream->m_adapterIndex, pStartStream->m_deviceHandle, pStartStream->m_audioPref);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_StopAudioStreamingOut (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMStreaming_t* pStopStream = (BTRMGR_IARMStreaming_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pStopStream) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_StopAudioStreamingOut(pStopStream->m_adapterIndex, pStopStream->m_deviceHandle);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t 
btrMgr_IsAudioStreamingOut (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMStreamingStatus_t* pStreamStatus = (BTRMGR_IARMStreamingStatus_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pStreamStatus) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_IsAudioStreamingOut(pStreamStatus->m_adapterIndex, &pStreamStatus->m_streamingStatus);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_SetAudioStreamOutType (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMStreamingType_t* pStartStream = (BTRMGR_IARMStreamingType_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pStartStream) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_SetAudioStreamingOutType(pStartStream->m_adapterIndex, pStartStream->m_audioOutType);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_StartAudioStreamingIn (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMStreaming_t*  pStartStream = (BTRMGR_IARMStreaming_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pStartStream) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_StartAudioStreamingIn(pStartStream->m_adapterIndex, pStartStream->m_deviceHandle, pStartStream->m_audioPref);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t
btrMgr_StopAudioStreamingIn (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMStreaming_t* pStopStream = (BTRMGR_IARMStreaming_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pStopStream) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_StopAudioStreamingIn(pStopStream->m_adapterIndex, pStopStream->m_deviceHandle);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }


    return retCode;
}

STATIC IARM_Result_t 
btrMgr_IsAudioStreamingIn (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMStreamingStatus_t* pStreamStatus = (BTRMGR_IARMStreamingStatus_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pStreamStatus) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_IsAudioStreamingIn(pStreamStatus->m_adapterIndex, &pStreamStatus->m_streamingStatus);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}


STATIC IARM_Result_t
btrMgr_SetEventResponse (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMEventResp_t* pIArmEvtResp = (BTRMGR_IARMEventResp_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pIArmEvtResp) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_SetEventResponse(pIArmEvtResp->m_adapterIndex, &pIArmEvtResp->m_stBTRMgrEvtRsp);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

STATIC IARM_Result_t
btrMgr_MediaControl (
    void*   arg 
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMMediaProperty_t *pMediaProperty = (BTRMGR_IARMMediaProperty_t*) arg;
 
    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pMediaProperty) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_MediaControl(pMediaProperty->m_adapterIndex, pMediaProperty->m_deviceHandle, pMediaProperty->m_mediaControlCmd);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}
   

STATIC IARM_Result_t
btrMgr_GetMediaCurrentPosition (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMMediaProperty_t *pMediaProperty = (BTRMGR_IARMMediaProperty_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pMediaProperty) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_GetMediaCurrentPosition(pMediaProperty->m_adapterIndex, pMediaProperty->m_deviceHandle, &pMediaProperty->m_mediaPositionInfo);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

static IARM_Result_t
btrMgr_GetDataPathConfInfo (
    void *arg
)
{
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMDeviceStreamingConfig_t *pdevStreamConfig = (BTRMGR_IARMDeviceStreamingConfig_t*)arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pdevStreamConfig) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_GetDataPathAndConfigurationForStreamOut(pdevStreamConfig->m_adapterIndex, pdevStreamConfig->m_deviceHandle, &pdevStreamConfig->m_pi32dataPath, &pdevStreamConfig->m_pi32Readmtu , &pdevStreamConfig->m_pi32Writemtu, &pdevStreamConfig->m_pi32Delay, &pdevStreamConfig->m_pstCodecInformation);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

static IARM_Result_t
btrMgr_ReleaseDataPath
(
    void * arg
)
{
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMDeviceStreamingRelease_t *pdevStreamRelease = (BTRMGR_IARMDeviceStreamingRelease_t*)arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pdevStreamRelease) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_ReleaseDataPathForStreamOut(pdevStreamRelease->m_adapterIndex, pdevStreamRelease->m_deviceHandle);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

static IARM_Result_t
btrMgr_StartSendingAudioFromFile (
    void *arg
)
{
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMDeviceFileStreamingConfig_t *pdevFileStreamConfig = (BTRMGR_IARMDeviceFileStreamingConfig_t*)arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pdevFileStreamConfig) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_StartSendingAudioFromFile(pdevFileStreamConfig->m_adapterIndex, pdevFileStreamConfig->m_deviceHandle, &pdevFileStreamConfig->m_audioConfigOut, &pdevFileStreamConfig->m_audioConfigIn , pdevFileStreamConfig->m_outputFd, pdevFileStreamConfig->m_outMtu, pdevFileStreamConfig->m_outDevDelay, pdevFileStreamConfig->m_filePath);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

static IARM_Result_t
btrMgr_StopSendingAudioFromFile (
    void *arg
)
{
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    rc = BTRMGR_StopSendingAudioFromFile();
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}


STATIC IARM_Result_t
btrMgr_GetDeviceVolumeMute (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMDeviceVolumeMute_t *pdevvolmut = (BTRMGR_IARMDeviceVolumeMute_t*)arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pdevvolmut) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_GetDeviceVolumeMute(pdevvolmut->m_adapterIndex, pdevvolmut->m_deviceHandle, pdevvolmut->m_deviceOpType , &pdevvolmut->m_volume, &pdevvolmut->m_mute);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

STATIC IARM_Result_t
btrMgr_SetDeviceVolumeMute (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMDeviceVolumeMute_t *pdevvolmut = (BTRMGR_IARMDeviceVolumeMute_t*)arg;

    BTRMGRLOG_TRACE ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
	retCode = IARM_RESULT_INVALID_STATE;
	BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
	return retCode;
    }

    if (!pdevvolmut) {
	retCode = IARM_RESULT_INVALID_PARAM;
	BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
	return retCode;
    }

    rc = BTRMGR_SetDeviceVolumeMute(pdevvolmut->m_adapterIndex, pdevvolmut->m_deviceHandle, pdevvolmut->m_deviceOpType , pdevvolmut->m_volume, pdevvolmut->m_mute);
    if (BTRMGR_RESULT_SUCCESS == rc) {
	BTRMGRLOG_TRACE ("Success\n");
    }
    else {
	retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
	BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

STATIC IARM_Result_t
btrMgr_SetDeviceDelay (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMDeviceDelay_t *pdevdelay = (BTRMGR_IARMDeviceDelay_t*)arg;

    BTRMGRLOG_TRACE ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
	retCode = IARM_RESULT_INVALID_STATE;
	BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
	return retCode;
    }

    if (!pdevdelay) {
	retCode = IARM_RESULT_INVALID_PARAM;
	BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
	return retCode;
    }

    rc = BTRMGR_SetDeviceDelay(pdevdelay->m_adapterIndex, pdevdelay->m_deviceHandle, pdevdelay->m_deviceOpType , pdevdelay->m_delay);
    if (BTRMGR_RESULT_SUCCESS == rc) {
	BTRMGRLOG_TRACE ("Success\n");
    }
    else {
	retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
	BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

STATIC IARM_Result_t
btrMgr_GetDeviceDelay (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMDeviceDelay_t *pdevdelay = (BTRMGR_IARMDeviceDelay_t*)arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pdevdelay) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_GetDeviceDelay(pdevdelay->m_adapterIndex, pdevdelay->m_deviceHandle, pdevdelay->m_deviceOpType , &pdevdelay->m_delay, &pdevdelay->m_msinqueue);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

STATIC IARM_Result_t
btrMgr_GetMediaTrackInfo (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMMediaProperty_t *pMediaProperty = (BTRMGR_IARMMediaProperty_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pMediaProperty) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_GetMediaTrackInfo(pMediaProperty->m_adapterIndex, pMediaProperty->m_deviceHandle, &pMediaProperty->m_mediaTrackInfo);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}


STATIC IARM_Result_t
btrMgr_GetMediaElementTrackInfo (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMMediaProperty_t *pMediaProperty = (BTRMGR_IARMMediaProperty_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pMediaProperty) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_GetMediaElementTrackInfo(pMediaProperty->m_adapterIndex, pMediaProperty->m_deviceHandle, pMediaProperty->m_mediaElementHandle, &pMediaProperty->m_mediaTrackInfo);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

STATIC IARM_Result_t
btrMgr_SetMediaElementActive (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMMediaElementListInfo_t *pMedElementList = (BTRMGR_IARMMediaElementListInfo_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;     
    }
    
    if (!pMedElementList) {  
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_SetMediaElementActive (pMedElementList->m_adapterIndex,
                                       pMedElementList->m_deviceHandle,
                                       pMedElementList->m_mediaElementHandle,
                                       pMedElementList->m_mediaElementType);
    if (BTRMGR_RESULT_SUCCESS == rc) { 
        BTRMGRLOG_INFO ("Success\n");
    }   
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }   

    return retCode;
}


STATIC IARM_Result_t
btrMgr_GetMediaElementList (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMMediaElementListInfo_t *pMedElementList = (BTRMGR_IARMMediaElementListInfo_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pMedElementList) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_GetMediaElementList (pMedElementList->m_adapterIndex,
                                     pMedElementList->m_deviceHandle,
                                     pMedElementList->m_mediaElementHandle,
                                     pMedElementList->m_mediaElementStartIdx,
                                     pMedElementList->m_mediaElementEndIdx,
                                     pMedElementList->m_mediaElementListDepth,
                                     pMedElementList->m_mediaElementType,
                                     &pMedElementList->m_mediaTrackListInfo);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

STATIC IARM_Result_t
btrMgr_SelectMediaElement (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMMediaElementListInfo_t *pMedElementList = (BTRMGR_IARMMediaElementListInfo_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pMedElementList) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_SelectMediaElement (pMedElementList->m_adapterIndex,
                                    pMedElementList->m_deviceHandle,
                                    pMedElementList->m_mediaElementHandle,
                                    pMedElementList->m_mediaElementType);

    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}


STATIC IARM_Result_t
btrMgr_GetLeProperty (
    void* arg
) {
    BTRMGR_Result_t rc    = BTRMGR_RESULT_SUCCESS;
    IARM_Result_t retCode = IARM_RESULT_SUCCESS;
    BTRMGR_IARMLeProperty_t *leProperty = (BTRMGR_IARMLeProperty_t*) arg;
    void* propValue = NULL;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!leProperty) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    switch (leProperty->m_enLeProperty) {
    case BTRMGR_LE_PROP_UUID:
        propValue = &leProperty->m_uuidList;
        break;
    case BTRMGR_LE_PROP_DEVICE:
        propValue = &leProperty->m_devicePath;
        break;
    case BTRMGR_LE_PROP_SERVICE:
        propValue = &leProperty->m_servicePath;
        break;
    case BTRMGR_LE_PROP_CHAR:
        propValue = &leProperty->m_characteristicPath;
        break;
    case BTRMGR_LE_PROP_VALUE:
        propValue = &leProperty->m_value;
        break;
    case BTRMGR_LE_PROP_PRIMARY:
        propValue = &leProperty->m_primary;
        break;
    case BTRMGR_LE_PROP_NOTIFY:
        propValue = &leProperty->m_notifying;
        break;
    case BTRMGR_LE_PROP_FLAGS:
        propValue = &leProperty->m_flags;
        break;
    default:
        break;
    }

    if(propValue) {
       rc = BTRMGR_GetLeProperty (leProperty->m_adapterIndex, leProperty->m_deviceHandle, leProperty->m_propUuid, leProperty->m_enLeProperty, propValue);
       if (BTRMGR_RESULT_SUCCESS == rc) {
           BTRMGRLOG_INFO ("Success\n");
       }
       else {
           retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
           BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
      }
    }
    else {
       BTRMGRLOG_ERROR ("propValue is NULL ..\n");
    }

    return retCode;
}



STATIC IARM_Result_t
btrMgr_PerformLeOp (
    void* arg
) {
    BTRMGR_Result_t rc      = BTRMGR_RESULT_SUCCESS;
    IARM_Result_t retCode   = IARM_RESULT_SUCCESS;
    BTRMGR_IARMLeOp_t *leOp = (BTRMGR_IARMLeOp_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!leOp) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_PerformLeOp (leOp->m_adapterIndex, leOp->m_deviceHandle, leOp->m_uuid, leOp->m_leOpType, leOp->m_opArg, leOp->m_opRes);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

STATIC IARM_Result_t
btrMgr_GetLimitBeaconDetection (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMBeaconDetection_t* pBeaconDetection = (BTRMGR_IARMBeaconDetection_t*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pBeaconDetection) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_GetLimitBeaconDetection(pBeaconDetection->m_adapterIndex, &pBeaconDetection->m_limitBeaconDetection);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Get Value : Beacon Detection Limited ?  %s\n",  pBeaconDetection->m_limitBeaconDetection ? "true" : "false");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }
    return retCode;
}


STATIC IARM_Result_t
btrMgr_SetLimitBeaconDetection (
    void* arg
) {
    IARM_Result_t    retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMBeaconDetection_t* pBeaconDetection = (BTRMGR_IARMBeaconDetection_t*)arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pBeaconDetection) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_SetLimitBeaconDetection (pBeaconDetection->m_adapterIndex, pBeaconDetection->m_limitBeaconDetection);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Value set success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

STATIC IARM_Result_t btrMgr_LeStartAdvertisement(void* arg)
{
    IARM_Result_t    retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    BTRMGR_IARMAdvtInfo_t* lpstAdvtInfo = (BTRMGR_IARMAdvtInfo_t *)arg;

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR("BTRMgr is not Inited\n");
    }
    else if (NULL == lpstAdvtInfo)
    {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR("Failed; RetCode = %d\n", retCode);
    }
    else
    {
        rc = BTRMGR_LE_StartAdvertisement(lpstAdvtInfo->m_adapterIndex, &lpstAdvtInfo->m_CustAdvt);

        if (BTRMGR_RESULT_SUCCESS == rc)
        {
            BTRMGRLOG_INFO("Success\n");
        }
        else
        {
            retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
            BTRMGRLOG_ERROR("Failed; RetCode = %d\n", rc);
        }
    }

    return retCode;
}

STATIC IARM_Result_t btrMgr_LeStopAdvertisement(void *arg)
{
    IARM_Result_t    retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    unsigned char* lpAdapterIndex = (unsigned char*)arg;

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR("BTRMgr is not Inited\n");
    }
    else if (NULL == lpAdapterIndex)
    {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR("Failed; RetCode = %d\n", retCode);
    }
    else
    {
        rc = BTRMGR_LE_StopAdvertisement(*lpAdapterIndex);

        if (BTRMGR_RESULT_SUCCESS == rc)
        {
            BTRMGRLOG_INFO("Success\n");
        }
        else
        {
            retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
            BTRMGRLOG_ERROR("Failed; RetCode = %d\n", rc);
        }
    }
    return retCode;
}

STATIC IARM_Result_t btrMgr_LeGetPropertyValue(void* arg)
{
    IARM_Result_t    retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    BTRMGR_IARMGATTValue_t* lpstGattInfo = (BTRMGR_IARMGATTValue_t *)arg;

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR("BTRMgr is not Inited\n");
    }
    else if (NULL == lpstGattInfo)
    {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR("Failed; RetCode = %d\n", retCode);
    }
    else
    {
        rc = BTRMGR_LE_GetPropertyValue(lpstGattInfo->m_adapterIndex, lpstGattInfo->m_UUID, lpstGattInfo->m_Value, lpstGattInfo->aElement);

        if (BTRMGR_RESULT_SUCCESS == rc)
        {
            BTRMGRLOG_INFO("Success\n");
        }
        else
        {
            retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
            BTRMGRLOG_ERROR("Failed; RetCode = %d\n", rc);
        }
    }
    return retCode;
}

STATIC IARM_Result_t btrMgr_LeSetServiceInfo(void* arg)
{
    IARM_Result_t    retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    BTRMGR_IARMGATTServiceInfo_t* lpstGattServiceInfo = (BTRMGR_IARMGATTServiceInfo_t *)arg;

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR("BTRMgr is not Inited\n");
    }
    else if (NULL == lpstGattServiceInfo)
    {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR("Failed; RetCode = %d\n", retCode);
    }
    else
    {
        rc = BTRMGR_LE_SetServiceInfo(lpstGattServiceInfo->m_adapterIndex, lpstGattServiceInfo->m_UUID, lpstGattServiceInfo->m_ServiceType);

        if (BTRMGR_RESULT_SUCCESS == rc)
        {
            BTRMGRLOG_INFO("Success\n");
        }
        else
        {
            retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
            BTRMGRLOG_ERROR("Failed; RetCode = %d\n", rc);
        }
    }
    return retCode;
}

STATIC IARM_Result_t btrMgr_LeSetGattInfo(void* arg)
{
    IARM_Result_t    retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    BTRMGR_IARMGATTInfo_t* lpstGattInfo = (BTRMGR_IARMGATTInfo_t *)arg;

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR("BTRMgr is not Inited\n");
    }
    else if (NULL == lpstGattInfo)
    {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR("Failed; RetCode = %d\n", retCode);
    }
    else
    {
        rc = BTRMGR_LE_SetGattInfo(lpstGattInfo->m_adapterIndex, lpstGattInfo->m_ParentUUID, lpstGattInfo->m_UUID, lpstGattInfo->m_Flags, lpstGattInfo->m_Value, lpstGattInfo->m_Element);

        if (BTRMGR_RESULT_SUCCESS == rc)
        {
            BTRMGRLOG_INFO("Success\n");
        }
        else
        {
            retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
            BTRMGRLOG_ERROR("Failed; RetCode = %d\n", rc);
        }
    }
    return retCode;
}

STATIC IARM_Result_t btrMgr_LeSetGattPropertyValue(void* arg) {
    IARM_Result_t    retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    BTRMGR_IARMGATTValue_t* lpstGattInfo = (BTRMGR_IARMGATTValue_t *)arg;

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR("BTRMgr is not Inited\n");
    }
    else if (NULL == lpstGattInfo)
    {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR("Failed; RetCode = %d\n", retCode);
    }
    else
    {
        rc = BTRMGR_LE_SetGattPropertyValue(lpstGattInfo->m_adapterIndex, lpstGattInfo->m_UUID, lpstGattInfo->m_Value, lpstGattInfo->aElement);

        if (BTRMGR_RESULT_SUCCESS == rc)
        {
            BTRMGRLOG_INFO("Success\n");
        }
        else
        {
            retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
            BTRMGRLOG_ERROR("Failed; RetCode = %d\n", rc);
        }
    }
    return retCode;
}
 
STATIC IARM_Result_t
btrMgr_SetAudioInServiceState (
    void* arg
) {
    IARM_Result_t    retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMAudioInServiceState_t* audioInSerivceState = (BTRMGR_IARMAudioInServiceState_t*)arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!audioInSerivceState) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_SetAudioInServiceState (audioInSerivceState->m_adapterIndex, audioInSerivceState->m_serviceState);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

STATIC IARM_Result_t
btrMgr_SetBtmgrDebugModeState (
    void* arg
) {
    IARM_Result_t    retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    unsigned char * ui8DebugModeState = (unsigned char *) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!ui8DebugModeState) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_SetBtmgrDebugModeState (*ui8DebugModeState);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}
STATIC IARM_Result_t
btrMgr_SetHidGamePadServiceState (
    void* arg
) {
    IARM_Result_t    retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMHidGamePadServiceState_t* hidGamePadSerivceState = (BTRMGR_IARMHidGamePadServiceState_t*)arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!hidGamePadSerivceState) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }

    rc = BTRMGR_SetHidGamePadServiceState (hidGamePadSerivceState->m_adapterIndex, hidGamePadSerivceState->m_serviceState);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

STATIC IARM_Result_t
btrMgr_SysDiagInfo(
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMDiagInfo_t *lDiagInfo = (BTRMGR_IARMDiagInfo_t*)arg;

    BTRMGRLOG_INFO("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR("BTRMgr is not Inited\n");
        return retCode;
    }

    rc = BTRMGR_SysDiagInfo(lDiagInfo->m_adapterIndex, lDiagInfo->m_UUID, lDiagInfo->m_DiagInfo, lDiagInfo->m_OpType);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

STATIC IARM_Result_t
btrMgr_ConnectToWifi(
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_IARMWifiConnectInfo_t* lWifiInfo = (BTRMGR_IARMWifiConnectInfo_t*)arg;

    BTRMGRLOG_INFO("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR("BTRMgr is not Inited\n");
        return retCode;
    }

    rc = BTRMGR_ConnectToWifi(lWifiInfo->m_adapterIndex, lWifiInfo->m_SSID, lWifiInfo->m_Password, lWifiInfo->m_SecMode);

    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}


STATIC IARM_Result_t
btrMgr_ResetAdapter (
    void*   arg
) {
    IARM_Result_t   retCode = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    unsigned char*  pAdapterIndex =  (unsigned char*) arg;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = IARM_RESULT_INVALID_STATE;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    if (!pAdapterIndex) {
        retCode = IARM_RESULT_INVALID_PARAM;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", retCode);
        return retCode;
    }


    rc = BTRMGR_ResetAdapter (*pAdapterIndex);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = IARM_RESULT_IPCCORE_FAIL; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}


STATIC IARM_Result_t
btrMgr_DeInit (
    void*   arg
) {
    if (gIsBTRMGR_Internal_Inited)
        BTRMGR_DeInit();

    return IARM_RESULT_SUCCESS;
}


/* Public Functions */
void
BTRMgr_BeginIARMMode (
    void
) {

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        gIsBTRMGR_Internal_Inited = 1;
        IARM_Bus_Init(IARM_BUS_BTRMGR_NAME);
        IARM_Bus_Connect();

        BTRMGRLOG_INFO ("IARM Interface Initializing\n");

        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_NUMBER_OF_ADAPTERS, btrMgr_GetNumberOfAdapters);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_SET_ADAPTER_NAME, btrMgr_SetAdapterName);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_ADAPTER_NAME, btrMgr_GetAdapterName);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_SET_ADAPTER_POWERSTATUS, btrMgr_SetAdapterPowerStatus);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_ADAPTER_POWERSTATUS, btrMgr_GetAdapterPowerStatus);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_SET_ADAPTER_DISCOVERABLE, btrMgr_SetAdapterDiscoverable);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_IS_ADAPTER_DISCOVERABLE, btrMgr_IsAdapterDiscoverable);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_CHANGE_DEVICE_DISCOVERY_STATUS, btrMgr_ChangeDeviceDiscoveryStatus);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_DISCOVERY_STATUS, btrMgr_GetDeviceDiscoveryStatus);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_DISCOVERED_DEVICES, btrMgr_GetDiscoveredDevices);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_PAIR_DEVICE, btrMgr_PairDevice);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_UNPAIR_DEVICE, btrMgr_UnpairDevice);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_PAIRED_DEVICES, btrMgr_GetPairedDevices);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_CONNECT_TO_DEVICE, btrMgr_ConnectToDevice);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_DISCONNECT_FROM_DEVICE, btrMgr_DisconnectFromDevice);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_CONNECTED_DEVICES, btrMgr_GetConnectedDevices);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_DEVICE_PROPERTIES, btrMgr_GetDeviceProperties);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_BATTERY_LEVEL, btrMgr_GetDeviceBatteryLevel);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_START_AUDIO_STREAMING_OUT, btrMgr_StartAudioStreamingOut);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_STOP_AUDIO_STREAMING_OUT, btrMgr_StopAudioStreamingOut);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_IS_AUDIO_STREAMING_OUT, btrMgr_IsAudioStreamingOut);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_SET_AUDIO_STREAM_OUT_TYPE, btrMgr_SetAudioStreamOutType);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_START_AUDIO_STREAMING_IN, btrMgr_StartAudioStreamingIn);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_STOP_AUDIO_STREAMING_IN, btrMgr_StopAudioStreamingIn);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_IS_AUDIO_STREAMING_IN, btrMgr_IsAudioStreamingIn);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_SET_EVENT_RESPONSE, btrMgr_SetEventResponse);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_RESET_ADAPTER, btrMgr_ResetAdapter);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_MEDIA_CONTROL, btrMgr_MediaControl);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_DEVICE_DATA_PATH_CONF_INFO, btrMgr_GetDataPathConfInfo);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_RELEASE_DATA_PATH, btrMgr_ReleaseDataPath);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_START_STREAMING_AUDIO_FROM_FILE,btrMgr_StartSendingAudioFromFile);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_STOP_STREAMING_AUDIO_FROM_FILE,btrMgr_StopSendingAudioFromFile);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_STOP_STREAMING_AUDIO_FROM_FILE,btrMgr_StopSendingAudioFromFile);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_SET_DEVICE_VOLUME_MUTE_INFO, btrMgr_SetDeviceVolumeMute);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_DEVICE_VOLUME_MUTE_INFO, btrMgr_GetDeviceVolumeMute);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_SET_DEVICE_DELAY_INFO, btrMgr_SetDeviceDelay);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_DEVICE_DELAY_INFO, btrMgr_GetDeviceDelay);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_MEDIA_TRACK_INFO, btrMgr_GetMediaTrackInfo);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_MEDIA_ELEMENT_TRACK_INFO, btrMgr_GetMediaElementTrackInfo);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_MEDIA_CURRENT_POSITION, btrMgr_GetMediaCurrentPosition);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_SET_MEDIA_ELEMENT_ACTIVE, btrMgr_SetMediaElementActive);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_MEDIA_ELEMENT_LIST, btrMgr_GetMediaElementList);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_SELECT_MEDIA_ELEMENT, btrMgr_SelectMediaElement);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_LE_PROPERTY, btrMgr_GetLeProperty);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_PERFORM_LE_OP, btrMgr_PerformLeOp);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_SET_AUDIO_IN_SERVICE_STATE, btrMgr_SetAudioInServiceState);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_SET_HID_GAMEPAD_SERVICE_STATE, btrMgr_SetHidGamePadServiceState);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_SET_BTMGR_DEBUG_MODE_STATE, btrMgr_SetBtmgrDebugModeState);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_SET_LIMIT_BEACON_DETECTION, btrMgr_SetLimitBeaconDetection);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_LIMIT_BEACON_DETECTION, btrMgr_GetLimitBeaconDetection);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_LE_START_ADVERTISEMENT, btrMgr_LeStartAdvertisement);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_LE_STOP_ADVERTISEMENT, btrMgr_LeStopAdvertisement);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_LE_GET_PROP_VALUE, btrMgr_LeGetPropertyValue);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_LE_SET_GATT_SERVICE_INFO, btrMgr_LeSetServiceInfo);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_LE_SET_GATT_CHAR_INFO, btrMgr_LeSetGattInfo);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_LE_SET_GATT_PROPERTY_VALUE, btrMgr_LeSetGattPropertyValue);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_GET_SYS_DIAG_INFO, btrMgr_SysDiagInfo);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_WIFI_CONNECT_INFO, btrMgr_ConnectToWifi);
        IARM_Bus_RegisterCall(BTRMGR_IARM_METHOD_DEINIT, btrMgr_DeInit);

        IARM_Bus_RegisterEvent(BTRMGR_IARM_EVENT_MAX);

        /* Register a callback */
        BTRMGR_RegisterEventCallback(btrMgr_EventCallback);

        BTRMGRLOG_INFO ("IARM Interface Inited Successfully\n");
    }
    else {
        BTRMGRLOG_INFO ("IARM Interface Already Inited\n");
    }

    return;
}

void
BTRMgr_TermIARMMode (
    void
) {
    BTRMGRLOG_INFO ("Entering\n");

    if (gIsBTRMGR_Internal_Inited) {
        BTRMGRLOG_INFO ("IARM Interface Being terminated\n");
        IARM_Bus_Disconnect();
        IARM_Bus_Term();
    }
    else {
        BTRMGRLOG_INFO ("IARM Interface Not Inited\n");
    }
}


/*  Incoming Callbacks */
STATIC BTRMGR_Result_t
btrMgr_EventCallback (
    BTRMGR_EventMessage_t   astEventMessage
) {
    BTRMGR_EventMessage_t   lstEventMessage;
    IARM_Result_t           lenIarmResult   = IARM_RESULT_SUCCESS;
    BTRMGR_Result_t         lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

    BTRMGRLOG_TRACE ("Entering\n");

    MEMCPY_S(&lstEventMessage,sizeof(BTRMGR_EventMessage_t), &astEventMessage, sizeof(BTRMGR_EventMessage_t));

    if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_OUT_OF_RANGE) {
        BTRMGRLOG_WARN ("Post Device Out of Range event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_OUT_OF_RANGE, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_DISCOVERY_STARTED) {
        BTRMGRLOG_WARN ("Post Discovery Started event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_DISCOVERY_STARTED, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_DISCOVERY_UPDATE) {
        BTRMGRLOG_WARN ("Post Discovery Status update\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_DISCOVERY_UPDATE, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_DISCOVERY_COMPLETE) {
        BTRMGRLOG_WARN ("Post Discovery Complete event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_DISCOVERY_COMPLETE, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_PAIRING_COMPLETE) {
        BTRMGRLOG_WARN ("Post Device Pairing Complete event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_PAIRING_COMPLETE, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_UNPAIRING_COMPLETE) {
        BTRMGRLOG_WARN ("Post Device UnPairing Complete event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_UNPAIRING_COMPLETE, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_CONNECTION_COMPLETE) {
        BTRMGRLOG_WARN ("Post Device Connection Complete event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_CONNECTION_COMPLETE, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_DISCONNECT_COMPLETE) {
        BTRMGRLOG_WARN ("Post Device Disconnected event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_DISCONNECT_COMPLETE, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_PAIRING_FAILED) {
        BTRMGRLOG_WARN ("Post Device Pairing Failed event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_PAIRING_FAILED, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_UNPAIRING_FAILED) {
        BTRMGRLOG_WARN ("Post Device UnPairing Failed event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_UNPAIRING_FAILED, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_CONNECTION_FAILED) {
        BTRMGRLOG_WARN ("Post Device Connection Failed event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_CONNECTION_FAILED, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_DISCONNECT_FAILED) {
        BTRMGRLOG_WARN ("Post Device Disconnect Failed event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_DISCONNECT_FAILED, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST) {
        BTRMGRLOG_WARN ("Post External Device Pair Request event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST) {
        BTRMGRLOG_WARN ("Post External Device Connect Request event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_RECEIVED_EXTERNAL_PLAYBACK_REQUEST) {
        BTRMGRLOG_WARN ("Post External Device Playback Request event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_RECEIVED_EXTERNAL_PLAYBACK_REQUEST, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_FOUND) {
        BTRMGRLOG_WARN ("Post External Device Found Back event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_FOUND, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_TRACK_STARTED) {
        BTRMGRLOG_WARN ("Post Media Track Started event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_TRACK_STARTED, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_TRACK_PLAYING) {
        BTRMGRLOG_WARN ("Post Media Track Playing event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_TRACK_PLAYING, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_TRACK_PAUSED) {
        BTRMGRLOG_WARN ("Post Media Track Paused event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_TRACK_PAUSED, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_TRACK_STOPPED) {
        BTRMGRLOG_WARN ("Post Media Track Stopped event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_TRACK_STOPPED, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_TRACK_POSITION) {
        BTRMGRLOG_WARN ("Post Media Track Position event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_TRACK_POSITION, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_TRACK_CHANGED) {
        BTRMGRLOG_WARN ("Post Media Track Changed event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_TRACK_CHANGED, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYBACK_ENDED) {
        BTRMGRLOG_WARN ("Post Media Playback Ended event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYBACK_ENDED, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_OP_READY) {
        BTRMGRLOG_WARN ("Post External Device Op Ready\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_OP_READY, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_OP_INFORMATION) {
        BTRMGRLOG_WARN ("Post External Device Op Information\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_OP_INFORMATION, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYER_NAME) {
        BTRMGRLOG_WARN ("Post Media Player Name event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYER_NAME, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYER_VOLUME) {
        BTRMGRLOG_WARN ("Post Media Player Volume event\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYER_VOLUME, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYER_EQUALIZER_OFF) {
        BTRMGRLOG_WARN ("Post Media Player Equalizer Off\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYER_EQUALIZER_OFF, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYER_EQUALIZER_ON) {
        BTRMGRLOG_WARN ("Post Media Player Equalizer On\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYER_EQUALIZER_ON, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYER_SHUFFLE_OFF) {
        BTRMGRLOG_WARN ("Post Media Player Shuffle Off\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYER_SHUFFLE_OFF, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYER_SHUFFLE_ALLTRACKS) {
        BTRMGRLOG_WARN ("Post Media Player Shuffle All Tracks\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYER_SHUFFLE_ALLTRACKS, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYER_SHUFFLE_GROUP) {
        BTRMGRLOG_WARN ("Post Media Player Shuffle Group\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYER_SHUFFLE_GROUP, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYER_REPEAT_OFF) {
        BTRMGRLOG_WARN ("Post Media Player Repeat Off\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYER_REPEAT_OFF, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYER_REPEAT_SINGLETRACK) {
        BTRMGRLOG_WARN ("Post Media Player Repeat Single Track\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYER_REPEAT_SINGLETRACK, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYER_REPEAT_ALLTRACKS) {
        BTRMGRLOG_WARN ("Post Media Player Repeat All Tracks\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYER_REPEAT_ALLTRACKS, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYER_REPEAT_GROUP) {
        BTRMGRLOG_WARN ("Post Media Player Repeat Group\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYER_REPEAT_GROUP, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_ALBUM_INFO) {
        BTRMGRLOG_WARN ("Post Media Album Info\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_ALBUM_INFO, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_ARTIST_INFO) {
        BTRMGRLOG_WARN ("Post Media Artist Info\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_ARTIST_INFO, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_GENRE_INFO) {
        BTRMGRLOG_WARN ("Post Media Genre Info\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_GENRE_INFO, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_COMPILATION_INFO) {
        BTRMGRLOG_WARN ("Post Media Compilation Info\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_COMPILATION_INFO, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYLIST_INFO) {
        BTRMGRLOG_WARN ("Post Media PlayList Info\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYLIST_INFO, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_TRACKLIST_INFO) {
        BTRMGRLOG_WARN ("Post Media TrackList Info\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_TRACKLIST_INFO, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_TRACK_INFO) {
        BTRMGRLOG_WARN ("Post Media Track Info\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_TRACK_INFO, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYER_MUTE) {
        BTRMGRLOG_WARN ("Post Media Mute Info\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYER_MUTE, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_MEDIA_PLAYER_UNMUTE) {
        BTRMGRLOG_WARN ("Post Media UnMute Info\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_MEDIA_PLAYER_UNMUTE, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }
    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_MEDIA_STATUS) {
        BTRMGRLOG_WARN ("Post Device Media Status Info\n");
        lenIarmResult = IARM_Bus_BroadcastEvent(IARM_BUS_BTRMGR_NAME, (IARM_EventId_t) BTRMGR_IARM_EVENT_DEVICE_MEDIA_STATUS, (void *)&lstEventMessage, sizeof(lstEventMessage));
    }

    if (lenIarmResult != IARM_RESULT_SUCCESS) {
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    
    return lenBtrMgrResult;
}

