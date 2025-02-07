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
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <rbus.h>
#include "btmgr.h"
#include "btrMgr_logger.h"
#include "btmgr_rbus_interface.h"
#include "btrMgr_platform_spec.h"

static rbusHandle_t  handle;
static unsigned char isBTRMGR_Inited = 0;
static unsigned char isBTRMGR_Rbus_Connected = 0;
static BTRMGR_EventCallback m_eventCallbackFunction = NULL;

#ifdef RDK_LOGGER_ENABLED
int b_rdk_logger_enabled = 0;
#endif

static void
btrMgrdeviceCallback (
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription
);

rbusEventSubscription_t eventSubscriptions[TotalEventParams] = {
                {BTRMGR_RBUS_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_DEVICE_PAIRING_COMPLETE, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_DEVICE_PAIRING_FAILED, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_DEVICE_DISCOVERY_UPDATE, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_DEVICE_UNPAIRING_COMPLETE, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_DEVICE_UNPAIRING_FAILED, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_DEVICE_CONNECTION_COMPLETE, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_DEVICE_CONNECTION_FAILED, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_DEVICE_DISCONNECT_COMPLETE, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_DEVICE_DISCONNECT_FAILED, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_DEVICE_FOUND, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_DEVICE_OUT_OF_RANGE, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_DEVICE_OP_READY, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_DEVICE_OP_INFORMATION, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL},
                {BTRMGR_RBUS_EVENT_BATTERY_INFO, NULL, 0, 0, btrMgrdeviceCallback, NULL, NULL, NULL}

};

const char*
BTRMGR_GetDeviceTypeAsString (
    BTRMGR_DeviceType_t  type
) {
    if (type == BTRMGR_DEVICE_TYPE_WEARABLE_HEADSET)
        return "WEARABLE HEADSET";
    else if (type == BTRMGR_DEVICE_TYPE_HANDSFREE)
        return "HANDSFREE";
    else if (type == BTRMGR_DEVICE_TYPE_MICROPHONE)
        return "MICROPHONE";
    else if (type == BTRMGR_DEVICE_TYPE_LOUDSPEAKER)
        return "LOUDSPEAKER";
    else if (type == BTRMGR_DEVICE_TYPE_HEADPHONES)
        return "HEADPHONES";
    else if (type == BTRMGR_DEVICE_TYPE_PORTABLE_AUDIO)
        return "PORTABLE AUDIO DEVICE";
    else if (type == BTRMGR_DEVICE_TYPE_CAR_AUDIO)
        return "CAR AUDIO";
    else if (type == BTRMGR_DEVICE_TYPE_STB)
        return "STB";
    else if (type == BTRMGR_DEVICE_TYPE_HIFI_AUDIO_DEVICE)
        return "HIFI AUDIO DEVICE";
    else if (type == BTRMGR_DEVICE_TYPE_VCR)
        return "VCR";
    else if (type == BTRMGR_DEVICE_TYPE_VIDEO_CAMERA)
        return "VIDEO CAMERA";
    else if (type == BTRMGR_DEVICE_TYPE_CAMCODER)
        return "CAMCODER";
    else if (type == BTRMGR_DEVICE_TYPE_VIDEO_MONITOR)
        return "VIDEO MONITOR";
    else if (type == BTRMGR_DEVICE_TYPE_TV)
        return "TV";
    else if (type == BTRMGR_DEVICE_TYPE_VIDEO_CONFERENCE)
        return "VIDEO CONFERENCING";
    else if (type == BTRMGR_DEVICE_TYPE_SMARTPHONE)
        return "SMARTPHONE";
    else if (type == BTRMGR_DEVICE_TYPE_TABLET)
        return "TABLET";
    else if (type == BTRMGR_DEVICE_TYPE_TILE)
        return "LE TILE";
    else if ((type == BTRMGR_DEVICE_TYPE_HID) || (type == BTRMGR_DEVICE_TYPE_HID_GAMEPAD))
        return "HUMAN INTERFACE DEVICE";
    else if (type == BTRMGR_DEVICE_TYPE_XBB)
        return "BATTERY DEVICE";
    else
        return "UNKNOWN DEVICE";
}

BTRMGR_Result_t
BTRMGR_Init (
    void
) {
    rbusError_t ret;

    if (!isBTRMGR_Inited)
        isBTRMGR_Inited = 1;
#ifdef RDK_LOGGER_ENABLED
    const char* pDebugConfig = NULL;
    const char* BTRMGR_RBUS_DEBUG_ACTUAL_PATH    = "/etc/debug.ini";
    const char* BTRMGR_RBUS_DEBUG_OVERRIDE_PATH  = "/opt/debug.ini";

    /* Init the logger */
    if (access(BTRMGR_RBUS_DEBUG_OVERRIDE_PATH, F_OK) != -1)
        pDebugConfig = BTRMGR_RBUS_DEBUG_OVERRIDE_PATH;
    else
        pDebugConfig = BTRMGR_RBUS_DEBUG_ACTUAL_PATH;

    if (0 == rdk_logger_init(pDebugConfig))
        b_rdk_logger_enabled = 1;
#endif


    char   componentName[] = "BTRMgrExternalInterface";

    ret = rbus_open(&handle, componentName);
    if(ret != RBUS_ERROR_SUCCESS)
    {
        BTRMGRLOG_INFO("BTRMGR_Init: rbus_open failed and exit: %d\n", ret);
        return ret;
    }
    isBTRMGR_Rbus_Connected = 1;
    BTRMGRLOG_INFO("BTRMGR_Init: rbus_open success \n");

    BTRMGR_RegisterForCallbacks(NULL);
    return BTRMGR_RESULT_SUCCESS;
}

BTRMGR_Result_t
BTRMGR_RegisterForCallbacks (
    const char *apcProcessName
)  {

     if (!apcProcessName) {
        BTRMGRLOG_INFO ("BTRMGR_INIT has called\n");
     }
     rbusError_t retCode = RBUS_ERROR_SUCCESS;

     retCode = rbusEvent_SubscribeEx(handle, eventSubscriptions, TotalEventParams, 0);
   /* retCode = rbusEvent_Subscribe(
              handle,
              BTRMGR_RBUS_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST,
              btrMgrdeviceCallback,
              NULL,
              0);
     if(retCode != RBUS_ERROR_SUCCESS)
    {
        BTRMGRLOG_INFO(": rbusEvent_Subscribe  failed for pair: %d\n", retCode);
        return retCode;
    }
    retCode = rbusEvent_Subscribe(
              handle,
              BTRMGR_RBUS_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST,
              btrMgrdeviceCallback,
              NULL,
              0);
    */
    if(retCode != RBUS_ERROR_SUCCESS)
    {
        BTRMGRLOG_INFO(": rbusEvent_Subscribe  failed for connect: %d\n", retCode);
        return retCode;
    }
    BTRMGRLOG_INFO ("RBUS Interface Inited Register Event Successfully\n");
    return BTRMGR_RESULT_SUCCESS;
}

BTRMGR_Result_t
BTRMGR_DeInit (
    void
)
{
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t retCode = RBUS_ERROR_SUCCESS;

    if (m_eventCallbackFunction)
        m_eventCallbackFunction = NULL;

    if (isBTRMGR_Inited) {
        BTRMGR_UnRegisterFromCallbacks(NULL);

        if (isBTRMGR_Rbus_Connected)  {
            if (RBUS_ERROR_SUCCESS == (retCode = rbus_close(handle))) {
                isBTRMGR_Rbus_Connected = 0;
            }
            else {
                BTRMGRLOG_ERROR ("rbus_close Failed; RetCode = %d\n", retCode);
            }
        }

        BTRMGRLOG_INFO ("RBUS Interface termination Successfully \n");
    }
    else {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_INFO ("RBUS Interface for BTRMgr is Not Inited Yet..\n");
    }

    return rc;
}

BTRMGR_Result_t
BTRMGR_UnRegisterFromCallbacks (
    const char *apcProcessName
)  {

    if (!apcProcessName) {
        BTRMGRLOG_INFO ("apcProcessName is NULL\n");
    }

    rbusError_t retCode = RBUS_ERROR_SUCCESS;

    retCode = rbusEvent_UnsubscribeEx(handle, eventSubscriptions, TotalEventParams);

    if(retCode != RBUS_ERROR_SUCCESS)
    {
        BTRMGRLOG_INFO(": rbusEvent_UnsubscribeEx  failed: %d\n", retCode);
        return retCode;
    }

    BTRMGRLOG_INFO ("RBUS Interface UnRegister Event Successfully\n");
    return BTRMGR_RESULT_SUCCESS;
}

/**************************************
  SetAdapterDiscoverable
***************************************/
BTRMGR_Result_t
BTRMGR_SetAdapterDiscoverable (
    unsigned char   index_of_adapter,
    unsigned char   discoverable,
    int  timeout
) {
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    rbusObject_t inParams;
    rbusObject_t outParams;
    rbusValue_t value;

   if ((BTRMGR_ADAPTER_COUNT_MAX < index_of_adapter) || (discoverable > 1)) {
        rc = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return rc;
    }

    rbusObject_Init(&inParams, NULL);

    rbusValue_Init(&value);
    rbusValue_SetByte(value, discoverable);
    rbusObject_SetValue(inParams, "setDiscoverable", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetInt32(value, timeout);
    rbusObject_SetValue(inParams, "setTimeout", value);
    rbusValue_Release(value);

    ret = rbusMethod_Invoke(handle, BTRMGR_RBUS_METHOD_SET_ADAPTER_DISCOVERABLE, inParams, &outParams);

    if (ret != RBUS_ERROR_SUCCESS) {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_INFO ("rbusMethod_Invoke Failed with error [%d]\n", ret);
        return rc;
    }
    rbusObject_Release(inParams);
    BTRMGRLOG_INFO("success [%d] and Exit BTRMGR_SetAdapterDiscoverable \n", ret);
    return rc;
}


/*********************************************
      Start DeviceDiscovery
**********************************************/

BTRMGR_Result_t
BTRMGR_StartDeviceDiscovery (
    unsigned char                index_of_adapter,
    BTRMGR_DeviceOperationType_t aenBTRMgrDevOpT
) {
     BTRMGRLOG_INFO("In BTRMGR_StartDeviceDiscovery \n");

     BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
     rbusError_t ret = RBUS_ERROR_SUCCESS;
     rbusObject_t inParams;
     rbusObject_t outParams;
     rbusValue_t value;
     int32_t m_setDiscovery =1;
     if (BTRMGR_ADAPTER_COUNT_MAX < index_of_adapter) {
         rc = BTRMGR_RESULT_INVALID_INPUT;
         BTRMGRLOG_ERROR ("Input is invalid\n");
         return rc;
     }

    rbusObject_Init(&inParams, NULL);
    rbusValue_Init(&value);
    rbusValue_SetInt32(value, m_setDiscovery);
    rbusObject_SetValue(inParams, "setDiscovery", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetInt32(value, (int32_t) aenBTRMgrDevOpT);
    rbusObject_SetValue(inParams, "BTRMgrDevOpT", value);
    rbusValue_Release(value);

    ret = rbusMethod_Invoke(handle, BTRMGR_RBUS_METHOD_CHANGE_DEVICE_DISCOVERY_STATUS, inParams, &outParams);

    if (ret != RBUS_ERROR_SUCCESS) {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_INFO ("rbus_set Failed with error [%d]\n", ret);
        return rc;
    }
    rbusObject_Release(inParams);
    BTRMGRLOG_INFO ("rbusMethod_Invoke success [%d] and Exit..\n", ret);
    return rc;
}

/*********************************************
        Stop DeviceDiscovery
**********************************************/

BTRMGR_Result_t
BTRMGR_StopDeviceDiscovery (
    unsigned char                index_of_adapter,
    BTRMGR_DeviceOperationType_t aenBTRMgrDevOpT
) {
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    rbusObject_t inParams;
    rbusObject_t outParams;
    rbusValue_t value;
    int32_t m_setDiscovery =0;

    if (BTRMGR_ADAPTER_COUNT_MAX < index_of_adapter) {
        rc = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return rc;
    }

    rbusObject_Init(&inParams, NULL);

    rbusValue_Init(&value);
    rbusValue_SetInt32(value, m_setDiscovery);
    rbusObject_SetValue(inParams, "setDiscovery", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetInt32(value, (int) aenBTRMgrDevOpT);
    rbusObject_SetValue(inParams, "BTRMgrDevOpT", value);
    rbusValue_Release(value);

    ret = rbusMethod_Invoke(handle, BTRMGR_RBUS_METHOD_CHANGE_DEVICE_DISCOVERY_STATUS, inParams, &outParams);
    if (ret != RBUS_ERROR_SUCCESS) {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_INFO ("rbus_set Failed with error [%d]\n", ret);
    }
    else {
        BTRMGRLOG_ERROR ("Rbus_set Success RetCode = %d\n", ret);
        rbusObject_Release(inParams);
    }

    return rc;
}

/**************************************************
 Get list of sccanned devices
**************************************************/

BTRMGR_Result_t
BTRMGR_GetDiscoveredDevices (
    unsigned char                   index_of_adapter,
    BTRMGR_DiscoveredDevicesList_t*  pDiscoveredDevices
) {
    BTRMGRLOG_INFO("Enter BTRMGR_GetDiscoveredDevices \n");
    char object_name[BTRMGR_STR_LEN] = "\0";
    int i = 0,NameLen,AddLen;
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    rbusObject_t inParams ;
    rbusObject_t outParams ;
    rbusObject_Init(&inParams, NULL);
    rbusObject_Init(&outParams, NULL);
    rbusValue_t numOfDevice = NULL;
    rbusValue_t subObject[BTRMGR_LE_DEVICE_COUNT_MAX];
    rbusObject_t obj[BTRMGR_LE_DEVICE_COUNT_MAX];
    rbusValue_t deviceHandle[BTRMGR_LE_DEVICE_COUNT_MAX], deviceName[BTRMGR_LE_DEVICE_COUNT_MAX], deviceAdd[BTRMGR_LE_DEVICE_COUNT_MAX], deviceType[BTRMGR_LE_DEVICE_COUNT_MAX];

    if ((BTRMGR_ADAPTER_COUNT_MAX < index_of_adapter) || (NULL == pDiscoveredDevices)) {
        rc = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return rc;
    }

    ret = rbusMethod_Invoke(handle, BTRMGR_RBUS_METHOD_GET_DISCOVERED_DEVICES, inParams, &outParams);

    rbusObject_Release(inParams);
    if (ret == RBUS_ERROR_SUCCESS) {
        numOfDevice  = rbusObject_GetValue(outParams, "numofdevices");
        pDiscoveredDevices->m_numOfDevices = rbusValue_GetUInt16(numOfDevice);

        BTRMGRLOG_INFO("rbusMethod_Invoke success and Number of devices - %d \n",pDiscoveredDevices->m_numOfDevices );
        while((i < pDiscoveredDevices->m_numOfDevices) && (i < BTRMGR_LE_DEVICE_COUNT_MAX))
        {
          BTRMGRLOG_INFO("filling device details..\n");
          {
             sprintf(object_name,"subobject-%u", i);

             subObject[i]    = rbusObject_GetValue(outParams, object_name);
             obj[i]          = rbusValue_GetObject(subObject[i]);
             deviceAdd[i]    = rbusObject_GetValue(obj[i], "DeviceAddress");
             deviceType[i]   = rbusObject_GetValue(obj[i], "DeviceType");
             deviceHandle[i] = rbusObject_GetValue(obj[i], "DeviceHandle");
             deviceName[i]   = rbusObject_GetValue(obj[i], "DeviceName");

             if(!outParams || !deviceAdd[i] || !deviceName[i] || !deviceHandle[i] || !deviceType[i])
             {
                BTRMGRLOG_INFO("outParams is NULL\n");
                rc = BTRMGR_RESULT_INVALID_INPUT;
                return rc;
             }

             BTRMGRLOG_INFO("copying data to local structure\n");
             pDiscoveredDevices->m_deviceProperty[i].m_deviceHandle = (BTRMgrDeviceHandle)(rbusValue_GetUInt64(deviceHandle[i]));
             pDiscoveredDevices->m_deviceProperty[i].m_deviceType = rbusValue_GetByte(deviceType[i]);
             snprintf(pDiscoveredDevices->m_deviceProperty[i].m_name,(BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(deviceName[i], &NameLen));
             pDiscoveredDevices->m_deviceProperty[i].m_name[NameLen] = '\0';
             snprintf(pDiscoveredDevices->m_deviceProperty[i].m_deviceAddress,(BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(deviceAdd[i], &AddLen));
             pDiscoveredDevices->m_deviceProperty[i].m_deviceAddress[AddLen] = '\0';
             pDiscoveredDevices->m_deviceProperty[i].m_deviceAddress[17] = '\0';
             BTRMGRLOG_INFO("Handle - %llu Name - %s Address - %s\n",pDiscoveredDevices->m_deviceProperty[i].m_deviceHandle, pDiscoveredDevices->m_deviceProperty[i].m_name,pDiscoveredDevices->m_deviceProperty[i].m_deviceAddress);
          }
          i++;
       }

    }
    else
    {
        BTRMGRLOG_ERROR ("rbusMethod_Invoke failed RetCode = %d\n", ret);
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    rbusObject_Release(outParams);

    return rc;
}

/*********** *******************
       Pair device
*******************************/

BTRMGR_Result_t
BTRMGR_PairDevice (
    unsigned char       index_of_adapter,
    BTRMgrDeviceHandle   btrHandle
) {
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    BTRMGRLOG_INFO("In BTRMGR_PairDevice \n");
    rbusSetOptions_t opts;
    opts.commit = true;

    if ((BTRMGR_ADAPTER_COUNT_MAX < index_of_adapter) || (0 == btrHandle)) {
        rc = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return rc;
    }

    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetUInt64(value, (uint64_t)btrHandle);
    ret = rbus_set(handle, BTRMGR_RBUS_METHOD_PAIR_DEVICE, value, &opts);
    if (ret != RBUS_ERROR_SUCCESS) {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_INFO ("rbus_set Failed with error [%d]\n", ret);
    }
    else {
        BTRMGRLOG_INFO ("rbus_set success and end of BTRMGR_PairDevice = %d\n", ret);
    }
    rbusValue_Release(value);
    return rc;
}

/*********** *******************
         UnPair device
*******************************/

BTRMGR_Result_t
BTRMGR_UnpairDevice (
    unsigned char       index_of_adapter,
    BTRMgrDeviceHandle  btrHandle
) {
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t retCode = RBUS_ERROR_SUCCESS;
    rbusSetOptions_t opts;
    opts.commit = true;

    if ((BTRMGR_ADAPTER_COUNT_MAX < index_of_adapter) || (0 == btrHandle)) {
        rc = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return rc;
    }
    BTRMGRLOG_INFO("in BTRMGR_UnpairDevice before rbus_set calling \n");
    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetUInt64(value, (uint64_t)btrHandle);
    retCode = rbus_set(handle, BTRMGR_RBUS_METHOD_UNPAIR_DEVICE, value, &opts);
    if (retCode != RBUS_ERROR_SUCCESS) {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_INFO ("rbus_set Failed with error [%d]\n", retCode);
    }
    else {
        BTRMGRLOG_INFO ("rbus_set success and end of BTRMGR_UnPairDevice = %d\n", retCode);
    }
    rbusValue_Release(value);

    return rc;
}

/*********** *******************
  GetPairedDevices list
*******************************/

BTRMGR_Result_t
BTRMGR_GetPairedDevices (
    unsigned char               index_of_adapter,
    BTRMGR_PairedDevicesList_t*  pPairedDevices
) {
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    rbusObject_t inParams , outParams;
    rbusValue_t numOfDevice = NULL;
    rbusObject_Init(&inParams, NULL);
    rbusObject_Init(&outParams, NULL);
    char object_name[BTRMGR_STR_LEN] = "\0";
    int i = 0,NameLen,AddLen;

    rbusValue_t deviceHandle[BTRMGR_LE_DEVICE_COUNT_MAX],deviceName[BTRMGR_LE_DEVICE_COUNT_MAX], deviceAdd[BTRMGR_LE_DEVICE_COUNT_MAX], deviceType[BTRMGR_LE_DEVICE_COUNT_MAX];
    rbusValue_t subObject[BTRMGR_LE_DEVICE_COUNT_MAX];
    rbusObject_t obj[BTRMGR_LE_DEVICE_COUNT_MAX];

    if ((BTRMGR_ADAPTER_COUNT_MAX < index_of_adapter) || (NULL == pPairedDevices)) {
        rc = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return rc;
    }

    ret = rbusMethod_Invoke(handle, BTRMGR_RBUS_METHOD_GET_PAIRED_DEVICES, inParams, &outParams);

    rbusObject_Release(inParams);
    if (ret == RBUS_ERROR_SUCCESS)
    {
        numOfDevice  = rbusObject_GetValue(outParams, "numofdevices");
        pPairedDevices->m_numOfDevices = rbusValue_GetUInt16(numOfDevice);

        while((i < pPairedDevices->m_numOfDevices) && (i < BTRMGR_LE_DEVICE_COUNT_MAX))
        {
             sprintf(object_name,"subobject-%u", i);
             subObject[i]    = rbusObject_GetValue(outParams, object_name);
             obj[i]          = rbusValue_GetObject(subObject[i]);
             deviceAdd[i]    = rbusObject_GetValue(obj[i], "DeviceAddress");
             deviceType[i]   = rbusObject_GetValue(obj[i], "DeviceType");
             deviceHandle[i] = rbusObject_GetValue(obj[i], "DeviceHandle");
             deviceName[i]   = rbusObject_GetValue(obj[i], "DeviceName");

             if(!deviceAdd[i] || !deviceName[i] || !deviceHandle[i] || !deviceType[i])
             {
                BTRMGRLOG_INFO("outParams is NULL\n");
                rc = BTRMGR_RESULT_INVALID_INPUT;
                return rc;
             }
             pPairedDevices->m_deviceProperty[i].m_deviceHandle = (BTRMgrDeviceHandle)(rbusValue_GetUInt64(deviceHandle[i]));
             pPairedDevices->m_deviceProperty[i].m_deviceType = rbusValue_GetByte(deviceType[i]);
             snprintf(pPairedDevices->m_deviceProperty[i].m_name,(BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(deviceName[i], &NameLen));
             pPairedDevices->m_deviceProperty[i].m_name[NameLen] = '\0';
             snprintf(pPairedDevices->m_deviceProperty[i].m_deviceAddress,(BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(deviceAdd[i], &AddLen));
             pPairedDevices->m_deviceProperty[i].m_deviceAddress[AddLen] = '\0';
             pPairedDevices->m_deviceProperty[i].m_deviceAddress[17] = '\0';
             BTRMGRLOG_INFO("<<Add %s >> Name %s Type %d \n",pPairedDevices->m_deviceProperty[i].m_deviceAddress,pPairedDevices->m_deviceProperty[i].m_name,pPairedDevices->m_deviceProperty[i].m_deviceType);

             i++;
       }
    }
    else
    {
        BTRMGRLOG_ERROR ("rbusMethod_Invoke failed RetCode = %d\n", ret);
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    rbusObject_Release(outParams);

    return rc;
}

/*********** *******************
  ConnectToDevice
*******************************/

BTRMGR_Result_t
BTRMGR_ConnectToDevice (
    unsigned char                index_of_adapter,
    BTRMgrDeviceHandle           btrHandle,
    BTRMGR_DeviceOperationType_t connectAs
) {
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t retCode = RBUS_ERROR_SUCCESS;
    rbusObject_t inParams;
    rbusObject_t outParams;
    rbusValue_t value;

    if ((BTRMGR_ADAPTER_COUNT_MAX < index_of_adapter) || (0 == btrHandle)) {
        rc = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return rc;
    }

    BTRMGRLOG_INFO(" in BTRMGR_ConnectToDevice and invoking rbus_method\n");
    rbusObject_Init(&inParams, NULL);

    rbusValue_Init(&value);
    rbusValue_SetUInt64(value, (uint64_t)btrHandle);
    rbusObject_SetValue(inParams, "setHandle", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetInt32(value, (int32_t)connectAs);
    rbusObject_SetValue(inParams, "DeviceType", value);
    rbusValue_Release(value);

    retCode = rbusMethod_Invoke(handle, BTRMGR_RBUS_METHOD_CONNECT_TO_DEVICE, inParams, &outParams);
    if (retCode != RBUS_ERROR_SUCCESS) {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_INFO ("rbus_set Failed with error [%d]\n", retCode);
    }
    else {
        BTRMGRLOG_INFO ("rbusMethod_Invoke success and end of BTRMGR_connectDevice = %d\n", retCode);
    }
    rbusObject_Release(inParams);

    return rc;
}

/*********** *******************
  DisConnect From Device
*******************************/

BTRMGR_Result_t
BTRMGR_DisconnectFromDevice (
    unsigned char       index_of_adapter,
    BTRMgrDeviceHandle  btrHandle
) {
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t retCode = RBUS_ERROR_SUCCESS;
    rbusSetOptions_t opts;
    opts.commit = true;

    if ((BTRMGR_ADAPTER_COUNT_MAX < index_of_adapter) || (0 == btrHandle)) {
        rc = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return rc;
    }

    BTRMGRLOG_INFO("in BTRMGR_DisconnectFromDevice before rbus_set calling \n");
    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetUInt64(value, (uint64_t)btrHandle);
    retCode = rbus_set(handle, BTRMGR_RBUS_METHOD_DISCONNECT_FROM_DEVICE, value, &opts);
    if (retCode != RBUS_ERROR_SUCCESS) {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_INFO ("rbus_set Failed with error [%d]\n", retCode);
    }
    else {
        BTRMGRLOG_INFO ("rbus_set success and end of BTRMGR_DisconnectFromDevice= %d\n", retCode);
    }
    rbusValue_Release(value);
    return rc;
}

/***************************************
 Get device properities
***************************************/
BTRMGR_Result_t
BTRMGR_GetDeviceProperties (
    unsigned char               index_of_adapter,
    BTRMgrDeviceHandle           btrHandle,
    BTRMGR_DevicesProperty_t*    pDeviceProperty
) {
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    int NameLen,AddLen,UuidLen;

    rbusObject_t inParams, outParams;
    rbusObject_Init(&inParams, NULL);
    rbusObject_Init(&outParams, NULL);

    rbusValue_t numOfService = NULL;
    rbusValue_t deviceHandle = NULL, deviceName = NULL, deviceAdd = NULL, device_rssi = NULL, isPaired = NULL, isConnected = NULL, vendorID = NULL, profileID = NULL, profileName = NULL;

    if ((BTRMGR_ADAPTER_COUNT_MAX < index_of_adapter) || (0 == btrHandle) || (NULL == pDeviceProperty)) {
        rc = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return rc;
    }

    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetUInt64(value, (uint64_t)btrHandle);
    rbusObject_SetValue(inParams, "setHandle", value);
    rbusValue_Release(value);

    BTRMGRLOG_INFO("calling rbusMethod_Invoke ...\n");
    ret = rbusMethod_Invoke(handle, BTRMGR_RBUS_METHOD_GET_DEVICE_PROPERTIES, inParams, &outParams);
    rbusObject_Release(inParams);
    if (ret == RBUS_ERROR_SUCCESS)
    {
        numOfService  = rbusObject_GetValue(outParams, "numofservices");
        deviceAdd    = rbusObject_GetValue(outParams, "DeviceAddress");
        deviceHandle = rbusObject_GetValue(outParams, "DeviceHandle");
        deviceName   = rbusObject_GetValue(outParams, "DeviceName");
        device_rssi  = rbusObject_GetValue(outParams, "DeviceRssi");
        isPaired     = rbusObject_GetValue(outParams, "DeviceIsPaired");
        isConnected  = rbusObject_GetValue(outParams, "DeviceIsConnected");
        vendorID     = rbusObject_GetValue(outParams, "DeviceVendorID");
        profileID    = rbusObject_GetValue(outParams, "DeviceProfileID");
        profileName  = rbusObject_GetValue(outParams, "DeviceProfileName");

        BTRMGRLOG_INFO("setting values to structure \n");

        pDeviceProperty->m_serviceInfo.m_numOfService= rbusValue_GetUInt16(numOfService);
        pDeviceProperty->m_deviceHandle = (BTRMgrDeviceHandle)(rbusValue_GetUInt64(deviceHandle));
        pDeviceProperty->m_rssi         = rbusValue_GetByte(device_rssi);
        pDeviceProperty->m_isPaired     = rbusValue_GetByte(isPaired);
        pDeviceProperty->m_isConnected  = rbusValue_GetByte(isConnected);
        pDeviceProperty->m_vendorID     = rbusValue_GetUInt16(vendorID);
        snprintf(pDeviceProperty->m_name, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(deviceName, &NameLen));
        pDeviceProperty->m_name[NameLen] = '\0';
        snprintf(pDeviceProperty->m_deviceAddress,(BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(deviceAdd, &AddLen));
        pDeviceProperty->m_deviceAddress[AddLen] = '\0';

        for (int i = 0; i < pDeviceProperty->m_serviceInfo.m_numOfService; i++)
        {
             pDeviceProperty->m_serviceInfo.m_profileInfo[i].m_uuid = rbusValue_GetUInt16(profileID);
             snprintf(pDeviceProperty->m_serviceInfo.m_profileInfo[i].m_profile, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(profileName,&UuidLen));
             pDeviceProperty->m_serviceInfo.m_profileInfo[i].m_profile[UuidLen] = '\0';
        }
    }
    else
    {
        BTRMGRLOG_ERROR ("rbusMethod_Invoke failed RetCode = %d\n", ret);
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    rbusObject_Release(outParams);

    return rc;
}


BTRMGR_Result_t BTRMGR_LE_SetServiceInfo(unsigned char aui8AdapterIdx, char *aUUID, unsigned char aServiceType)
{
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    rbusValue_t value;
    rbusObject_t inParams , outParams;

    BTRMGRLOG_INFO("Enter BTRMGR_LE_SetServiceInfo\n");
    rbusObject_Init(&inParams, NULL);

    rbusValue_Init(&value);
    rbusValue_SetString(value, aUUID);
    rbusObject_SetValue(inParams, "MainUUID", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetByte(value, aServiceType);
    rbusObject_SetValue(inParams, "OpType", value);
    rbusValue_Release(value);

    BTRMGRLOG_INFO("calling BTRMGR_RBUS_METHOD_LE_SET_GATT_SERVICE_INFO\n");
    ret = rbusMethod_Invoke(handle, BTRMGR_RBUS_METHOD_LE_SET_GATT_SERVICE_INFO, inParams, &outParams);

    if (RBUS_ERROR_SUCCESS == ret)
    {
        BTRMGRLOG_INFO("Success; \n");
    }
    else
    {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_ERROR("Failed; RetCode = %d\n", ret);
    }
    rbusObject_Release(inParams);

    return rc;
}

BTRMGR_Result_t BTRMGR_LE_SetGattInfo(unsigned char aui8AdapterIdx, char *aParentUUID, char *aUUID, unsigned short aFlags, char *aValue, BTRMGR_LeProperty_t aElement)
{
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;
    rbusValue_t value;
    rbusObject_t inParams , outParams;


    if ((NULL != aParentUUID) && (NULL != aUUID))
    {
        rbusObject_Init(&inParams, NULL);

        rbusValue_Init(&value);
        rbusValue_SetString(value, aParentUUID);
        rbusObject_SetValue(inParams, "ParentUUID", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetString(value, aUUID);
        rbusObject_SetValue(inParams, "UUID", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetString(value, aValue);
        rbusObject_SetValue(inParams, "PropertyValue", value);
        rbusValue_Release(value);

        //lGattCharInfo.m_Element = aElement;

        retCode = rbusMethod_Invoke(handle, BTRMGR_RBUS_METHOD_LE_SET_GATT_CHAR_INFO, inParams, &outParams);
        if (RBUS_ERROR_SUCCESS == retCode)
        {
            BTRMGRLOG_INFO("Success; \n");
        }
        else
        {
            rc = BTRMGR_RESULT_GENERIC_FAILURE;
            BTRMGRLOG_ERROR("Failed; RetCode = %d\n", retCode);
        }
        rbusObject_Release(inParams);
    }
    return rc;
}

/**** SysDiagInfo ****/
BTRMGR_Result_t
BTRMGR_SysDiagInfo(
    unsigned char aui8AdapterIdx,
    char *apDiagElement,
    char *apValue,
    BTRMGR_LeOp_t aOpType
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

    rbusError_t ret = RBUS_ERROR_SUCCESS;
    rbusObject_t inParams , outParams;
    rbusValue_t value;
    rbusValue_t diagInfo = NULL;
    int InfoLen;

    if (BTRMGR_ADAPTER_COUNT_MAX < aui8AdapterIdx) {
        lenBtrMgrResult = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR("Input is invalid\n");
        return lenBtrMgrResult;
    }

    rbusObject_Init(&inParams, NULL);
    rbusObject_Init(&outParams, NULL);

    rbusValue_Init(&value);
    rbusValue_SetString(value, apDiagElement);
    rbusObject_SetValue(inParams, "DiagElement", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetByte(value, aOpType);
    rbusObject_SetValue(inParams, "OpType", value);
    rbusValue_Release(value);

    if (BTRMGR_LE_OP_WRITE_VALUE == aOpType)
    {
        rbusValue_Init(&value);
        rbusValue_SetString(value, apValue);
        rbusObject_SetValue(inParams, "DiagInfoValue", value);
        rbusValue_Release(value);
    }
    BTRMGRLOG_INFO("calling  BTRMGR_RBUS_METHOD_GET_SYS_DIAG_INFO \n");
    ret = rbusMethod_Invoke(handle, BTRMGR_RBUS_METHOD_GET_SYS_DIAG_INFO, inParams, &outParams);

    if ( ret == RBUS_ERROR_SUCCESS)
    {
        if (BTRMGR_LE_OP_READ_VALUE == aOpType)
        {
            diagInfo = rbusObject_GetValue(outParams, "DiagInfo");
            strncpy(apValue, (const char *)rbusValue_GetBytes(diagInfo, &InfoLen), BTRMGR_MAX_STR_LEN - 1);
            apValue[InfoLen] = '\0';
        }
        BTRMGRLOG_INFO("Success and printing diagInfo = %s and =%s\n", apValue, rbusValue_GetBytes(diagInfo, NULL));
    }
    else {
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
            BTRMGRLOG_INFO ("Failed with error [%d]\n", ret);
    }
    rbusObject_Release(inParams);
    rbusObject_Release(outParams);

    return lenBtrMgrResult;
}

/***********************************
 Set LTE Service State
***********************************/
BTRMGR_Result_t
BTRMGR_SetLTEServiceState (
    unsigned char   aui8AdapterIdx,
    unsigned char   aui8State
) {
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t retCode = RBUS_ERROR_SUCCESS;

    rbusValue_t value;
    rbusValue_Init(&value);

    rbusSetOptions_t opts;
    opts.commit = true;

    if (BTRMGR_ADAPTER_COUNT_MAX < aui8AdapterIdx) {
        rc = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return rc;
    }

    rbusValue_SetByte(value, aui8State);
    retCode = rbus_set(handle, BTRMGR_RBUS_METHOD_TO_SET_LTE_SERVICE_STATE, value, &opts);
    if (retCode != RBUS_ERROR_SUCCESS) {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_INFO ("rbus_set Failed with error [%d]\n", retCode);
    }
    else {
        BTRMGRLOG_INFO ("rbus_set success and end of BTRMGR_SetLTEServiceState = %d\n", retCode);
    }
    rbusValue_Release(value);
    return rc;
}

/***********************************
 Set Broadcast State
***********************************/
BTRMGR_Result_t
BTRMGR_SetBroadcastState (
    unsigned char   aui8AdapterIdx,
    unsigned char   aui8State
) {
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t retCode = RBUS_ERROR_SUCCESS;

    rbusValue_t value;
    rbusValue_Init(&value);

    rbusSetOptions_t opts;
    opts.commit = true;

    if (BTRMGR_ADAPTER_COUNT_MAX < aui8AdapterIdx) {
        rc = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return rc;
    }

    rbusValue_SetByte(value, aui8State);
    retCode = rbus_set(handle, BTRMGR_RBUS_METHOD_SET_BROADCAST_STATE, value, &opts);
    if (retCode != RBUS_ERROR_SUCCESS) {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_INFO ("rbus_set Failed with error [%d]\n", retCode);
    }
    else {
        BTRMGRLOG_INFO("rbus_set success and end of BTRMGR_SetBroadcastState = %d\n", retCode);
    }
    rbusValue_Release(value);
    return rc;
}

/***********************************
 Set Battery Operations State
***********************************/
BTRMGR_Result_t
BTRMGR_SetBatteryOpsState (
    unsigned char   aui8AdapterIdx,
    unsigned char   aui8State
) {
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t retCode = RBUS_ERROR_SUCCESS;

    rbusValue_t value;
    rbusValue_Init(&value);

    rbusSetOptions_t opts;
    opts.commit = true;

    if (BTRMGR_ADAPTER_COUNT_MAX < aui8AdapterIdx) {
        rc = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return rc;
    }

    rbusValue_SetByte(value, aui8State);
    retCode = rbus_set(handle, BTRMGR_RBUS_METHOD_SET_BATTERY_OPS_STATE, value, &opts);
    if (retCode != RBUS_ERROR_SUCCESS) {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_INFO ("rbus_set Failed with error [%d]\n", retCode);
    }
    else {
        BTRMGRLOG_INFO ("rbus_set success and end of BTRMGR_SetBatteryOpsState = %d\n", retCode);
    }
    rbusValue_Release(value);
    return rc;
}


/***********************************
   Start Advertisement
***********************************/
BTRMGR_Result_t BTRMGR_LE_StartAdvertisement(unsigned char aui8AdapterIdx, BTRMGR_LeCustomAdvertisement_t *pstBTMGR_LeCustomAdvt)
{
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t retCode = RBUS_ERROR_SUCCESS;
    rbusObject_t inParams , outParams;

    BTRMGRLOG_INFO("calling  BTRMGR_RBUS_METHOD_LE_START_ADVERTISEMENT \n");

    //Just invoking BTRMGR_RBUS_METHOD_LE_START_ADVERTISEMENT , not passing/getting any values from or to internel to externel so initilization is not required.
    rbusObject_Init(&inParams, NULL);

    retCode = rbusMethod_Invoke(handle, BTRMGR_RBUS_METHOD_LE_START_ADVERTISEMENT, inParams, &outParams);
    if (RBUS_ERROR_SUCCESS == retCode)
    {
        BTRMGRLOG_INFO("Success; Device is now advertising\n");
    }
    else {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_ERROR("Failed; RetCode = %d\n", retCode);
    }
    rbusObject_Release(inParams);

    return rc;
}

/***********************************
   Stop Advertisement
***********************************/
BTRMGR_Result_t BTRMGR_LE_StopAdvertisement(unsigned char aui8AdapterIdx)
{
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t retCode = RBUS_ERROR_SUCCESS;
    rbusObject_t inParams, outParams;

    BTRMGRLOG_INFO("calling  BTRMGR_RBUS_METHOD_LE_STOP_ADVERTISEMENT \n");
    //TODO: Initilization is not required, just invoking method to call stop advertisement fun
    rbusObject_Init(&inParams, NULL);

    retCode = rbusMethod_Invoke(handle, BTRMGR_RBUS_METHOD_LE_STOP_ADVERTISEMENT, inParams, &outParams);

    if (RBUS_ERROR_SUCCESS == retCode)
    {
        BTRMGRLOG_INFO("Success; Device has stopped advertising\n");
    }
    else {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_ERROR("Failed; RetCode = %d\n", retCode);
    }
    rbusObject_Release(inParams);

    return rc;
}

/*********** *******************
   Set Event Response
*******************************/

BTRMGR_Result_t
BTRMGR_SetEventResponse (
    unsigned char           index_of_adapter,
    BTRMGR_EventResponse_t* apstBTRMgrEvtRsp
) {
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t retCode = RBUS_ERROR_SUCCESS;
    rbusObject_t inParams;
    rbusObject_t outParams;
    rbusValue_t value;

    if ((BTRMGR_ADAPTER_COUNT_MAX < index_of_adapter) || (apstBTRMgrEvtRsp == NULL)) {
        rc = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return rc;
    }
    rbusObject_Init(&inParams, NULL);

    rbusValue_Init(&value);
    rbusValue_SetUInt64(value, (uint64_t)(apstBTRMgrEvtRsp->m_deviceHandle));
    rbusObject_SetValue(inParams, "DeviceHandle", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetByte(value, apstBTRMgrEvtRsp->m_eventType);
    rbusObject_SetValue(inParams, "DeviceEventType", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetByte(value, apstBTRMgrEvtRsp->m_eventResp);
    rbusObject_SetValue(inParams, "DeviceEventResp", value);
    rbusValue_Release(value);

    BTRMGRLOG_INFO("<<< in BTRMGR_SetEventResponse before rbus_set calling and handle = %lld >>>\n", apstBTRMgrEvtRsp->m_deviceHandle);

    retCode = rbusMethod_Invoke(handle, BTRMGR_RBUS_METHOD_SET_EVENT_RESPONSE, inParams, &outParams);

    if (retCode != RBUS_ERROR_SUCCESS) {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        BTRMGRLOG_INFO ("rbusMethod_Invoke Failed with error [%d]\n", retCode);
    }
    else {
        BTRMGRLOG_INFO ("rbusMethod_Invoke success and end of BTRMGR_SetEventResponse= %d\n", retCode);
    }
    rbusObject_Release(inParams);

    return rc;
}

/*********** *******************
   Event Registration callback
*******************************/

BTRMGR_Result_t
BTRMGR_RegisterEventCallback (
    BTRMGR_EventCallback eventCallback
) {
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;

    if (!eventCallback) {
        rc = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
    }
    else {
        m_eventCallbackFunction = eventCallback;
        BTRMGRLOG_INFO ("Success\n");
    }

    return rc;
}


/**********************/
/* Incoming Callbacks */
/**********************/

static void
btrMgrdeviceCallback (
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription
)
{
    BTRMGRLOG_INFO("<< in btrMgrdeviceCallback >>\n");
    BTRMGR_EventMessage_t newEvent;
    int NameLen,AddLen,UuidLen,NotifyDataLen,WriteDataLen;
    memset(&newEvent, 0, sizeof(newEvent));
    BTRMGRLOG_INFO("Setting eventCallbackValues and even name =%s \n", event->name);

    rbusValue_t eventType = NULL;
    eventType = rbusObject_GetValue(event->data,"DeviceEventType");
    newEvent.m_eventType = (BTRMGR_Events_t)rbusValue_GetByte(eventType);

    if(strcmp(BTRMGR_RBUS_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST, event->name) == 0)
    {
        BTRMGRLOG_INFO("recived event name = %s\n", event->name);
        rbusValue_t exDevicePIN =NULL, requestConfirm = NULL, exDeviceAdd =NULL, exdeviceHandle = NULL, exDeviceName = NULL;

        exdeviceHandle = rbusObject_GetValue(event->data,"ExDeviceHandle");
        exDeviceName = rbusObject_GetValue(event->data,"ExDeviceName");
        exDeviceAdd = rbusObject_GetValue(event->data,"ExDeviceAddress");
        exDevicePIN = rbusObject_GetValue(event->data,"ExternalDevicePIN");
        requestConfirm = rbusObject_GetValue(event->data,"RequestConfirmation");

        newEvent.m_externalDevice.m_deviceHandle = (BTRMgrDeviceHandle)(rbusValue_GetUInt64(exdeviceHandle));
        snprintf(newEvent.m_externalDevice.m_name, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(exDeviceName, &NameLen));
        newEvent.m_externalDevice.m_name[NameLen] = '\0';
        snprintf(newEvent.m_externalDevice.m_deviceAddress, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(exDeviceAdd, &AddLen));
        newEvent.m_externalDevice.m_deviceAddress[AddLen] = '\0';
        newEvent.m_externalDevice.m_deviceAddress[17] = '\0';
        newEvent.m_externalDevice.m_externalDevicePIN = rbusValue_GetUInt32(exDevicePIN);
        newEvent.m_externalDevice.m_requestConfirmation = (unsigned char) rbusValue_GetByte(requestConfirm);

        BTRMGRLOG_INFO("setting newEvent values sucess for %s \n",event->name);
    }
    else if(strcmp(BTRMGR_RBUS_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST, event->name) == 0)
    {
        BTRMGRLOG_INFO("recived event name = %s\n", event->name);

        rbusValue_t exDeviceName =NULL, exDeviceAdd =NULL, exdeviceHandle = NULL;

        exdeviceHandle = rbusObject_GetValue(event->data,"ExDeviceHandle");
        exDeviceName = rbusObject_GetValue(event->data,"ExDeviceName");
        exDeviceAdd = rbusObject_GetValue(event->data,"ExDeviceAddress");

        newEvent.m_externalDevice.m_deviceHandle = (BTRMgrDeviceHandle)(rbusValue_GetUInt64(exdeviceHandle));
        snprintf(newEvent.m_externalDevice.m_name, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(exDeviceName, &NameLen));
        newEvent.m_externalDevice.m_name[NameLen] = '\0';
        snprintf(newEvent.m_externalDevice.m_deviceAddress, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(exDeviceAdd, &AddLen));
        newEvent.m_externalDevice.m_deviceAddress[AddLen] = '\0';
        newEvent.m_externalDevice.m_deviceAddress[17] = '\0';

        BTRMGRLOG_INFO("setting newEvent values sucess for %s \n",event->name);
    }
    else if((strcmp(BTRMGR_RBUS_EVENT_DEVICE_PAIRING_COMPLETE, event->name) == 0) ||
            (strcmp(BTRMGR_RBUS_EVENT_DEVICE_PAIRING_FAILED, event->name) == 0)   ||
            (strcmp(BTRMGR_RBUS_EVENT_DEVICE_DISCOVERY_UPDATE, event->name) == 0) )
    {
        BTRMGRLOG_INFO("recived event name = %s\n", event->name);

        rbusValue_t discDeviceName =NULL, discDeviceAdd =NULL, discdeviceHandle = NULL, discDeviceType = NULL;

        discdeviceHandle = rbusObject_GetValue(event->data,"DiscDeviceHandle");
        discDeviceName = rbusObject_GetValue(event->data,"DiscDeviceName");
        discDeviceAdd = rbusObject_GetValue(event->data,"DiscDeviceAddress");
        discDeviceType = rbusObject_GetValue(event->data,"DeviceEventType");

        newEvent.m_discoveredDevice.m_deviceHandle = (BTRMgrDeviceHandle)(rbusValue_GetUInt64(discdeviceHandle));
        snprintf(newEvent.m_discoveredDevice.m_name, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(discDeviceName,&NameLen));
        newEvent.m_discoveredDevice.m_name[NameLen] = '\0';
        snprintf(newEvent.m_discoveredDevice.m_deviceAddress, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(discDeviceAdd, &AddLen));
        newEvent.m_discoveredDevice.m_deviceAddress[AddLen] = '\0';
        newEvent.m_discoveredDevice.m_deviceAddress[17] = '\0';
        newEvent.m_discoveredDevice.m_deviceType = rbusValue_GetByte(discDeviceType);

        BTRMGRLOG_INFO("setting newEvent values sucess for %s \n",event->name);
    }
    else if((strcmp(BTRMGR_RBUS_EVENT_DEVICE_UNPAIRING_COMPLETE, event->name) == 0) ||
            (strcmp(BTRMGR_RBUS_EVENT_DEVICE_UNPAIRING_FAILED, event->name) == 0)   ||
            (strcmp(BTRMGR_RBUS_EVENT_DEVICE_CONNECTION_COMPLETE, event->name) == 0)||
            (strcmp(BTRMGR_RBUS_EVENT_DEVICE_CONNECTION_FAILED, event->name) == 0)  ||
            (strcmp(BTRMGR_RBUS_EVENT_DEVICE_DISCONNECT_COMPLETE, event->name) == 0)||
            (strcmp(BTRMGR_RBUS_EVENT_DEVICE_DISCONNECT_FAILED, event->name) == 0)  ||
            (strcmp(BTRMGR_RBUS_EVENT_DEVICE_FOUND, event->name) == 0)              ||
            (strcmp(BTRMGR_RBUS_EVENT_DEVICE_OUT_OF_RANGE, event->name) == 0))
    {
        BTRMGRLOG_INFO("recived event name = %s\n", event->name);
 
        rbusValue_t pairedDeviceName =NULL, pairedDeviceAdd =NULL, paireddeviceHandle = NULL, pairedDeviceType = NULL, lastConnectedDevice = NULL;

        paireddeviceHandle = rbusObject_GetValue(event->data,"PairedDeviceHandle");
        pairedDeviceName = rbusObject_GetValue(event->data,"PairedDeviceName");
        pairedDeviceAdd = rbusObject_GetValue(event->data,"PairedDeviceAddress");
        pairedDeviceType = rbusObject_GetValue(event->data,"PairedDeviceType");
        lastConnectedDevice = rbusObject_GetValue(event->data,"LastConnectedDevice");

        newEvent.m_pairedDevice.m_deviceHandle = (BTRMgrDeviceHandle)(rbusValue_GetUInt64(paireddeviceHandle));
        snprintf(newEvent.m_pairedDevice.m_name, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(pairedDeviceName, &NameLen));
        newEvent.m_pairedDevice.m_name[NameLen] = '\0';
        snprintf(newEvent.m_pairedDevice.m_deviceAddress, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(pairedDeviceAdd, &AddLen));
        newEvent.m_pairedDevice.m_deviceAddress[AddLen] = '\0';
        newEvent.m_pairedDevice.m_deviceAddress[17] = '\0';
        newEvent.m_pairedDevice.m_deviceType = rbusValue_GetByte(pairedDeviceType);
        newEvent.m_pairedDevice.m_isLastConnectedDevice = rbusValue_GetByte(lastConnectedDevice);

        BTRMGRLOG_INFO("setting newEvent values sucess for %s \n",event->name);
    }
    else if( (strcmp(BTRMGR_RBUS_EVENT_DEVICE_OP_READY, event->name) == 0) ||
             (strcmp(BTRMGR_RBUS_EVENT_DEVICE_OP_INFORMATION, event->name) == 0) )
    {
        BTRMGRLOG_INFO("recived event name = %s\n", event->name);

        rbusValue_t deviceOPInfoName =NULL, deviceOPInfoUuid =NULL, deviceOPInfoType = NULL, deviceOPInfoNotifyData = NULL, deviceLeOpType = NULL, deviceOPInfoWriteData = NULL;

        deviceOPInfoName = rbusObject_GetValue(event->data,"DeviceOPInfoName");
        deviceOPInfoUuid = rbusObject_GetValue(event->data,"DeviceOPInfoUuid");
        deviceOPInfoType = rbusObject_GetValue(event->data,"DeviceOPInfoType");
        deviceLeOpType   = rbusObject_GetValue(event->data,"DeviceLeOpType");
        deviceOPInfoNotifyData = rbusObject_GetValue(event->data,"DeviceOPInfoNotifyData");
        deviceOPInfoWriteData = rbusObject_GetValue(event->data,"DeviceOPInfoWriteData");

        snprintf(newEvent.m_deviceOpInfo.m_name, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(deviceOPInfoName, &NameLen));
        newEvent.m_deviceOpInfo.m_name[NameLen] = '\0';
        snprintf(newEvent.m_deviceOpInfo.m_uuid, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(deviceOPInfoUuid, &AddLen));
        newEvent.m_deviceOpInfo.m_uuid[AddLen] = '\0';
        newEvent.m_deviceOpInfo.m_deviceType = rbusValue_GetByte(deviceOPInfoType);
        newEvent.m_deviceOpInfo.m_leOpType = rbusValue_GetByte(deviceLeOpType);
        snprintf(newEvent.m_deviceOpInfo.m_notifyData, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(deviceOPInfoNotifyData, &NotifyDataLen));
        newEvent.m_deviceOpInfo.m_notifyData[NotifyDataLen] = '\0';
        snprintf(newEvent.m_deviceOpInfo.m_writeData, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(deviceOPInfoWriteData, &WriteDataLen));
        newEvent.m_deviceOpInfo.m_writeData[WriteDataLen] = '\0';

        BTRMGRLOG_INFO("setting newEvent values sucess for %s \n",event->name);
    }
    else if (strcmp(BTRMGR_RBUS_EVENT_BATTERY_INFO, event->name) == 0)
    {
        BTRMGRLOG_INFO("recived event name = %s\n", event->name);
        rbusValue_t BatteryInfoName = NULL, BatteryInfoUuid = NULL, BatteryInfoType = NULL, BatteryInfoBatteryLevel = NULL, BatteryInfoErrorValues = NULL,BatteryInfoFlags = NULL,BatteryInfoDeviceHandle = NULL;

        BatteryInfoName = rbusObject_GetValue(event->data,"BatteryInfoName");
        snprintf(newEvent.m_batteryInfo.m_name, (BTRMGR_NAME_LEN_MAX - 1), "%s", rbusValue_GetBytes(BatteryInfoName, &NameLen));
        newEvent.m_batteryInfo.m_name[NameLen] = '\0';
        BTRMGRLOG_INFO("Device Name - %s \n",newEvent.m_batteryInfo.m_name);

        BatteryInfoType = rbusObject_GetValue(event->data,"BatteryInfoType");
        newEvent.m_batteryInfo.m_deviceType = rbusValue_GetByte(BatteryInfoType);
        BTRMGRLOG_INFO("Device Type - %d \n",newEvent.m_batteryInfo.m_deviceType);

        BatteryInfoUuid = rbusObject_GetValue(event->data,"BatteryInfoUuid");
        snprintf(newEvent.m_batteryInfo.m_uuid, (BTRMGR_UUID_STR_LEN_MAX - 1), "%s", rbusValue_GetBytes(BatteryInfoUuid, &UuidLen));
        newEvent.m_batteryInfo.m_uuid[UuidLen] = '\0';
        BTRMGRLOG_INFO("UUID - %s \n",newEvent.m_batteryInfo.m_uuid);

        BatteryInfoDeviceHandle = rbusObject_GetValue(event->data,"BatteryInfoDeviceHandle");
        newEvent.m_batteryInfo.m_deviceHandle = (BTRMgrDeviceHandle)(rbusValue_GetUInt64(BatteryInfoDeviceHandle));
        BTRMGRLOG_INFO("DeviceHandle - %lld \n",newEvent.m_batteryInfo.m_deviceHandle);

        if (!strcmp(newEvent.m_batteryInfo.m_uuid,BTRMGR_UUID_BATTERY_LEVEL)) {
            BatteryInfoBatteryLevel = rbusObject_GetValue(event->data,"BatteryInfoBatteryLevel");
            snprintf(newEvent.m_batteryInfo.m_notifyData, (BTRMGR_MAX_DEV_OP_DATA_LEN - 1), "%s", rbusValue_GetBytes(BatteryInfoBatteryLevel, &NotifyDataLen));
            newEvent.m_batteryInfo.m_notifyData[NotifyDataLen] = '\0';
            BTRMGRLOG_INFO("Notification value : Battery Level - %s\n",newEvent.m_batteryInfo.m_notifyData);
        } else if (!strcmp(newEvent.m_batteryInfo.m_uuid,BTRMGR_UUID_BATTERY_ERROR_VALUES)) {
            BatteryInfoErrorValues = rbusObject_GetValue(event->data,"BatteryInfoErrorValues");
            snprintf(newEvent.m_batteryInfo.m_notifyData, (BTRMGR_MAX_DEV_OP_DATA_LEN - 1), "%s", rbusValue_GetBytes(BatteryInfoErrorValues, &NotifyDataLen));
            newEvent.m_batteryInfo.m_notifyData[NotifyDataLen] = '\0';
            BTRMGRLOG_INFO("Notification value : Battery Error Values - %s\n",newEvent.m_batteryInfo.m_notifyData);
        } else if (!strcmp(newEvent.m_batteryInfo.m_uuid,BTRMGR_UUID_BATTERY_FLAGS)) {
            BatteryInfoFlags = rbusObject_GetValue(event->data,"BatteryInfoFlags");
            snprintf(newEvent.m_batteryInfo.m_notifyData, (BTRMGR_MAX_DEV_OP_DATA_LEN - 1), "%s", rbusValue_GetBytes(BatteryInfoFlags, &NotifyDataLen));
            newEvent.m_batteryInfo.m_notifyData[NotifyDataLen] = '\0';
            BTRMGRLOG_INFO("Notification value : Battery Info Flags - %s\n",newEvent.m_batteryInfo.m_notifyData);
        }
    }
    else {
        BTRMGRLOG_ERROR ("Event is invalid\n");
        return;
    }

    if (m_eventCallbackFunction)
        m_eventCallbackFunction (newEvent);

    BTRMGRLOG_INFO ("posted event(%d) to listener successfully\n", newEvent.m_eventType);
    return;
}

