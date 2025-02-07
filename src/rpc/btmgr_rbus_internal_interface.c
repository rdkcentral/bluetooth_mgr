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
#include <rbus/rbus.h>

#include "btmgr.h"
#include "btrMgr_logger.h"
#include "btmgr_rbus_interface.h"
#include "btrMgr_platform_spec.h"

#define TotalParams  18

/* Callbacks Prototypes */
static BTRMGR_Result_t btrMgr_EventCallback (BTRMGR_EventMessage_t astEventMessage);

static int           subscribed[TotalEventParams];
rbusHandle_t         rbusHandle;
static char          componentName[] = "BTRMgrInternalInterface";
static unsigned char gIsBTRMGR_Internal_Inited = 0;


static rbusError_t
btrMgr_SetAdapterDiscoverable (
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle);

static rbusError_t
btrMgr_ChangeDeviceDiscoveryStatus (
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle);

static rbusError_t
btrMgr_GetDiscoveredDevices (
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle);

static rbusError_t
btrMgr_PairDevice(
    rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts);

static rbusError_t
btrMgr_UnPairDevice(
    rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts);

static rbusError_t
btrMgr_GetPairedDevices(
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle);

static rbusError_t
btrMgr_ConnectToDevice(
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle);

static rbusError_t
btrMgr_DisconnectFromDevice(
    rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts);

static rbusError_t
btrMgr_GetDeviceProperties(
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle);

static rbusError_t
btrMgr_SetEventResponse(
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle);

rbusError_t
eventSubHandler(
    rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish);

static rbusError_t
btrMgr_SysDiagInfo(
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle);

static rbusError_t
btrMgr_LeSetServiceInfo(
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle);

static rbusError_t
btrMgr_LeSetGattInfo(
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle);

static rbusError_t
btrMgr_LeStartAdvertisement(
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle);

static rbusError_t
btrMgr_LeStopAdvertisement(
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle);

static rbusError_t
btrMgr_LeSetLteServiceState(
    rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts);

static rbusError_t
btrMgr_LeSetBroadcastState(
    rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts);

static rbusError_t
btrMgr_LeSetBatteryOpsState(
    rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts);


rbusDataElement_t dataElements[TotalParams] = {
   {BTRMGR_RBUS_METHOD_SET_ADAPTER_DISCOVERABLE, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, NULL, NULL, NULL, NULL, btrMgr_SetAdapterDiscoverable}},
   {BTRMGR_RBUS_METHOD_CHANGE_DEVICE_DISCOVERY_STATUS, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, NULL, NULL, NULL, NULL, btrMgr_ChangeDeviceDiscoveryStatus}},
   {BTRMGR_RBUS_METHOD_GET_DISCOVERED_DEVICES, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, NULL, NULL, NULL, NULL, btrMgr_GetDiscoveredDevices}},
   {BTRMGR_RBUS_METHOD_PAIR_DEVICE, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, btrMgr_PairDevice, NULL, NULL, NULL, NULL}},
   {BTRMGR_RBUS_METHOD_UNPAIR_DEVICE, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, btrMgr_UnPairDevice, NULL, NULL, NULL, NULL}},
   {BTRMGR_RBUS_METHOD_GET_PAIRED_DEVICES, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, NULL, NULL, NULL, NULL, btrMgr_GetPairedDevices}}, 
   {BTRMGR_RBUS_METHOD_CONNECT_TO_DEVICE, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, NULL, NULL, NULL, NULL, btrMgr_ConnectToDevice}},
   {BTRMGR_RBUS_METHOD_DISCONNECT_FROM_DEVICE, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, btrMgr_DisconnectFromDevice, NULL, NULL, NULL, NULL}},
   {BTRMGR_RBUS_METHOD_GET_DEVICE_PROPERTIES, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, NULL, NULL, NULL, NULL, btrMgr_GetDeviceProperties}},
   {BTRMGR_RBUS_METHOD_SET_EVENT_RESPONSE, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, NULL, NULL, NULL, NULL, btrMgr_SetEventResponse}},
   {BTRMGR_RBUS_METHOD_GET_SYS_DIAG_INFO, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, NULL, NULL, NULL, NULL, btrMgr_SysDiagInfo}},
   {BTRMGR_RBUS_METHOD_LE_SET_GATT_SERVICE_INFO, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, NULL, NULL, NULL, NULL, btrMgr_LeSetServiceInfo}},
   {BTRMGR_RBUS_METHOD_LE_SET_GATT_CHAR_INFO, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, NULL, NULL, NULL, NULL, btrMgr_LeSetGattInfo}},
   {BTRMGR_RBUS_METHOD_LE_START_ADVERTISEMENT, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, NULL, NULL, NULL, NULL, btrMgr_LeStartAdvertisement}},
   {BTRMGR_RBUS_METHOD_LE_STOP_ADVERTISEMENT, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, NULL, NULL, NULL, NULL, btrMgr_LeStopAdvertisement}},
   {BTRMGR_RBUS_METHOD_TO_SET_LTE_SERVICE_STATE, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, btrMgr_LeSetLteServiceState, NULL, NULL, NULL, NULL}},
   {BTRMGR_RBUS_METHOD_SET_BROADCAST_STATE, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, btrMgr_LeSetBroadcastState, NULL, NULL, NULL, NULL}},
   {BTRMGR_RBUS_METHOD_SET_BATTERY_OPS_STATE, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, btrMgr_LeSetBatteryOpsState, NULL, NULL, NULL, NULL}},
};

rbusDataElement_t eventDataElements[TotalEventParams] = {
   {BTRMGR_RBUS_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_DEVICE_PAIRING_COMPLETE, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_DEVICE_PAIRING_FAILED, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_DEVICE_DISCOVERY_UPDATE, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_DEVICE_UNPAIRING_COMPLETE, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_DEVICE_UNPAIRING_FAILED, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_DEVICE_CONNECTION_COMPLETE, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_DEVICE_CONNECTION_FAILED, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_DEVICE_DISCONNECT_COMPLETE, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_DEVICE_DISCONNECT_FAILED, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_DEVICE_FOUND, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_DEVICE_OUT_OF_RANGE, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_DEVICE_OP_READY, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_DEVICE_OP_INFORMATION, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
   {BTRMGR_RBUS_EVENT_BATTERY_INFO, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}}
};

static rbusError_t
btrMgr_SetAdapterDiscoverable (
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle)
{
    (void) handle;
    BTRMGRLOG_INFO("Entering...\n");
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t retCode = RBUS_ERROR_SUCCESS;
    rbusValue_t isDiscoverable = NULL, timeOut = NULL;
    unsigned char m_isDiscoverable;
    int m_timeout;

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_INVALID_INPUT;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    isDiscoverable = rbusObject_GetValue(inParams,"setDiscoverable");
    timeOut = rbusObject_GetValue(inParams,"setTimeout");
    BTRMGRLOG_INFO("ParamName = %s and discoverable = %d \n",methodName, rbusValue_GetByte(isDiscoverable));

    m_isDiscoverable = rbusValue_GetByte(isDiscoverable);
    m_timeout = rbusValue_GetInt32(timeOut);

    rc = BTRMGR_SetAdapterDiscoverable(0, m_isDiscoverable, m_timeout);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else
    {
        retCode = RBUS_ERROR_INVALID_OPERATION;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    BTRMGRLOG_INFO("Exit... \n");
    return retCode;
}

static rbusError_t
btrMgr_ChangeDeviceDiscoveryStatus (
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle)
{
    (void) handle;
    BTRMGRLOG_INFO("Entering... \n");
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t retCode = RBUS_ERROR_SUCCESS;
    rbusValue_t disc_flag=NULL, BTRMgrDev_flag = NULL;

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_INVALID_INPUT;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    disc_flag = rbusObject_GetValue(inParams,"setDiscovery");
    BTRMgrDev_flag = rbusObject_GetValue(inParams,"BTRMgrDevOpT");
    BTRMGRLOG_INFO("ParamName = %s and value = %d \n",methodName, rbusValue_GetInt32(disc_flag));

    int32_t m_setDiscovery = rbusValue_GetInt32(disc_flag);
    BTRMGRLOG_INFO("printing m_setDiscovery =%d BTRMgrDev_flag = %d\n", m_setDiscovery,(BTRMGR_DeviceOperationType_t)rbusValue_GetInt32(BTRMgrDev_flag));

    if(m_setDiscovery)
    {
       rc = BTRMGR_StartDeviceDiscovery(0, (BTRMGR_DeviceOperationType_t)rbusValue_GetInt32(BTRMgrDev_flag));
    }
    else
    {
       rc = BTRMGR_StopDeviceDiscovery(0, (BTRMGR_DeviceOperationType_t)rbusValue_GetInt32(BTRMgrDev_flag));
    }
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else
    {
        retCode = RBUS_ERROR_INVALID_OPERATION;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    BTRMGRLOG_INFO("Exit...\n");
    return retCode;
}

/****** get devices list ***************/

static rbusError_t
btrMgr_GetDiscoveredDevices (
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle)
{
    BTRMGRLOG_INFO ("Entering\n");
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_DiscoveredDevicesList_t discoveredDevices;
    //rbusObject_Init(&outParams, NULL);  ///dont use here
    rbusValue_t value;
    rbusObject_t subobj[BTRMGR_LE_DEVICE_COUNT_MAX];

    char object_name[BTRMGR_STR_LEN] = "\0";
    int i=0;
  
    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_INVALID_INPUT;;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    rc = BTRMGR_GetDiscoveredDevices(0, &discoveredDevices);
    BTRMGRLOG_TRACE ("Success\n");
    if (BTRMGR_RESULT_SUCCESS == rc)
    {
        BTRMGRLOG_INFO("Success and setting values, discoveredDevices.m_numOfDevices = %d \n",discoveredDevices.m_numOfDevices);

        rbusValue_Init(&value);
        rbusValue_SetUInt16(value, (unsigned short)discoveredDevices.m_numOfDevices);
        rbusObject_SetValue(outParams, "numofdevices", value);
        rbusValue_Release(value);

        while((i < BTRMGR_LE_DEVICE_COUNT_MAX) && (i < discoveredDevices.m_numOfDevices))
        {
           BTRMGRLOG_INFO ("setting device details... \n");

           BTRMGRLOG_INFO("numofdev= %d deviceHandle = %lld Devicename = %s DeviceAddress = %s \n",
                           discoveredDevices.m_numOfDevices, discoveredDevices.m_deviceProperty[i].m_deviceHandle,
                           discoveredDevices.m_deviceProperty[i].m_name, discoveredDevices.m_deviceProperty[i].m_deviceAddress);
           {
              rbusObject_Init(&subobj[i], NULL);

              rbusValue_Init(&value);
              rbusValue_SetUInt64(value, (uint64_t)discoveredDevices.m_deviceProperty[i].m_deviceHandle);
              rbusObject_SetValue(subobj[i], "DeviceHandle", value);
              rbusValue_Release(value);

              rbusValue_Init(&value);
              rbusValue_SetBytes(value, (uint8_t *)discoveredDevices.m_deviceProperty[i].m_name, strlen(discoveredDevices.m_deviceProperty[i].m_name));
              rbusObject_SetValue(subobj[i], "DeviceName", value);
              rbusValue_Release(value);

              rbusValue_Init(&value);
              rbusValue_SetBytes(value, (uint8_t *)discoveredDevices.m_deviceProperty[i].m_deviceAddress,
                                         strlen(discoveredDevices.m_deviceProperty[i].m_deviceAddress));
              rbusObject_SetValue(subobj[i], "DeviceAddress", value);
              rbusValue_Release(value);

              rbusValue_Init(&value);
              rbusValue_SetByte(value, discoveredDevices.m_deviceProperty[i].m_deviceType);
              rbusObject_SetValue(subobj[i], "DeviceType", value);
              rbusValue_Release(value);

              BTRMGRLOG_INFO("setting subobj[%d] value \n",i);
              rbusValue_Init(&value);
              rbusValue_SetObject(value, subobj[i]);
              rbusObject_Release(subobj[i]);

              sprintf(object_name,"subobject-%u", i);
	      BTRMGRLOG_INFO("object_name == %s\n", object_name);
              rbusObject_SetValue(outParams, object_name, value);
              rbusValue_Release(value);

              BTRMGRLOG_INFO ("Success in setting device details to obj... \n");
         }
         i++;
      }
    }
    else
    {
        retCode = RBUS_ERROR_INVALID_OPERATION;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }
    return retCode;
}

static rbusError_t
btrMgr_PairDevice (
   rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts
) {
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_INVALID_INPUT;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    rbusValue_t value = rbusProperty_GetValue(prop);
    BTRMGRLOG_INFO("Device handle = %llu  \n", rbusValue_GetUInt64(value));

    rc = BTRMGR_PairDevice(0, ((BTRMgrDeviceHandle)( rbusValue_GetUInt64(value)) ));
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = RBUS_ERROR_INVALID_OPERATION ;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

static rbusError_t
btrMgr_UnPairDevice (
   rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts
) {
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_INVALID_INPUT;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    rbusValue_t value = rbusProperty_GetValue(prop);
    BTRMGRLOG_INFO("UnPairDevice handle = %llu \n", rbusValue_GetUInt64(value));
    
    rc = BTRMGR_UnpairDevice(0, ((BTRMgrDeviceHandle)(rbusValue_GetUInt64(value))));
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = RBUS_ERROR_INVALID_OPERATION ;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    BTRMGRLOG_INFO ("Exit ...\n");
    return retCode;
}

/******************************************
  Get Paired devices details
******************************************/
static rbusError_t
btrMgr_GetPairedDevices (
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle)
{
    BTRMGRLOG_INFO("Entering\n");
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    BTRMGR_PairedDevicesList_t  pairedDevices;
    rbusValue_t value;
    rbusObject_t subobj[BTRMGR_LE_DEVICE_COUNT_MAX];
    char object_name[BTRMGR_STR_LEN] = "\0";
    int i=0;

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_INVALID_INPUT;;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    rc = BTRMGR_GetPairedDevices(0, &pairedDevices);
    if (BTRMGR_RESULT_SUCCESS == rc)
    {
        BTRMGRLOG_INFO("Success and setting values...\n");

        BTRMGRLOG_INFO("num of dev %d Handle = %lld name = %s \n",
               pairedDevices.m_numOfDevices, pairedDevices.m_deviceProperty[i].m_deviceHandle,
               pairedDevices.m_deviceProperty[i].m_name);
	BTRMGRLOG_INFO("Address %s Dev Type %d \n",pairedDevices.m_deviceProperty[i].m_deviceAddress,pairedDevices.m_deviceProperty[i].m_deviceType);

        rbusValue_Init(&value);
        rbusValue_SetUInt16(value, (unsigned short)pairedDevices.m_numOfDevices);
        rbusObject_SetValue(outParams, "numofdevices", value);
        rbusValue_Release(value);

        while((i< pairedDevices.m_numOfDevices) && (i < BTRMGR_LE_DEVICE_COUNT_MAX))
        {

            rbusObject_Init(&subobj[i], NULL);

            rbusValue_Init(&value);
            rbusValue_SetUInt64(value, (uint64_t)pairedDevices.m_deviceProperty[i].m_deviceHandle);
            rbusObject_SetValue(subobj[i], "DeviceHandle", value);
            rbusValue_Release(value);

            rbusValue_Init(&value);
            rbusValue_SetBytes(value, (uint8_t *)pairedDevices.m_deviceProperty[i].m_name, strlen(pairedDevices.m_deviceProperty[i].m_name));
            rbusObject_SetValue(subobj[i], "DeviceName", value);
            rbusValue_Release(value);

            rbusValue_Init(&value);
            rbusValue_SetBytes(value, (uint8_t *)pairedDevices.m_deviceProperty[i].m_deviceAddress,
                               strlen(pairedDevices.m_deviceProperty[i].m_deviceAddress));
            rbusObject_SetValue(subobj[i], "DeviceAddress", value);
            rbusValue_Release(value);

            rbusValue_Init(&value);
            rbusValue_SetByte(value, pairedDevices.m_deviceProperty[i].m_deviceType);
            rbusObject_SetValue(subobj[i], "DeviceType", value);
            rbusValue_Release(value);

            BTRMGRLOG_INFO(" setting subobj[%d] values \n",i);
            rbusValue_Init(&value);
            rbusValue_SetObject(value, subobj[i]);
            rbusObject_Release(subobj[i]);

            sprintf(object_name,"subobject-%u", i);
            BTRMGRLOG_INFO("object_name == %s\n", object_name);
            rbusObject_SetValue(outParams, object_name, value);
            rbusValue_Release(value);

            BTRMGRLOG_INFO ("Success in setting device details to obj... \n");
            i++;
        }
    }
    else
    {
        retCode = RBUS_ERROR_INVALID_OPERATION;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    BTRMGRLOG_INFO ("Exit... \n");
    return retCode;
}

static rbusError_t
btrMgr_ConnectToDevice (
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle)
{
    (void) handle;
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    rbusValue_t btrHandle = NULL, deviceType_flag = NULL;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_INVALID_INPUT;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    deviceType_flag = rbusObject_GetValue(inParams,"DeviceType");
    btrHandle = rbusObject_GetValue(inParams,"setHandle");
    BTRMgrDeviceHandle deviceHandle = (BTRMgrDeviceHandle)(rbusValue_GetUInt64(btrHandle));
    
    BTRMGRLOG_INFO("ConnectToDevice handle = %llu Dev_flag = %d \n", deviceHandle, (BTRMGR_DeviceOperationType_t)rbusValue_GetInt32(deviceType_flag));
    rc = BTRMGR_ConnectToDevice(0, deviceHandle, (BTRMGR_DeviceOperationType_t)rbusValue_GetInt32(deviceType_flag));
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = RBUS_ERROR_INVALID_INPUT; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }
    BTRMGRLOG_INFO ("Exit... \n");
    return retCode;
}

static rbusError_t
btrMgr_DisconnectFromDevice (
    rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts
) {
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_INVALID_INPUT;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    rbusValue_t value = rbusProperty_GetValue(prop);
    BTRMGRLOG_INFO("DisConnectToDevice handle = %llu \n", rbusValue_GetUInt64(value));
    rc = BTRMGR_DisconnectFromDevice(0, (BTRMgrDeviceHandle)(rbusValue_GetUInt64(value)));
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = RBUS_ERROR_INVALID_INPUT; /* We do not have other IARM Error code to describe this. */
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

/********************************
 GetDeviceProperties
********************************/

static rbusError_t
btrMgr_GetDeviceProperties (
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle)
{
    BTRMGRLOG_INFO ("Entering\n");
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    BTRMGR_DevicesProperty_t deviceProperty;
    rbusValue_t value, btrHandle = NULL;

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_INVALID_INPUT;;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    btrHandle = rbusObject_GetValue(inParams,"setHandle");
    BTRMgrDeviceHandle deviceHandle = (BTRMgrDeviceHandle)(rbusValue_GetUInt64(btrHandle));
    BTRMGRLOG_INFO("handle = %llu \n", (BTRMgrDeviceHandle)(rbusValue_GetUInt64(btrHandle)));

    rc = BTRMGR_GetDeviceProperties(0, deviceHandle, &deviceProperty);
    if (BTRMGR_RESULT_SUCCESS == rc)
    {
        BTRMGRLOG_INFO("Success and setting values... \n");
        BTRMGRLOG_INFO("<<< deviceHandle = %lld Devicename = %s DeviceAddress = %s >>>\n",
               deviceProperty.m_deviceHandle, deviceProperty.m_name, deviceProperty.m_deviceAddress);

        rbusValue_Init(&value);
        rbusValue_SetUInt64(value, (uint64_t)deviceProperty.m_deviceHandle);
        rbusObject_SetValue(outParams, "DeviceHandle", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)deviceProperty.m_name, strlen(deviceProperty.m_name));
        rbusObject_SetValue(outParams, "DeviceName", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)deviceProperty.m_deviceAddress, strlen(deviceProperty.m_deviceAddress));
        rbusObject_SetValue(outParams, "DeviceAddress", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetByte(value, deviceProperty.m_rssi);
        rbusObject_SetValue(outParams, "DeviceRssi", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetByte(value, deviceProperty.m_isPaired);
        rbusObject_SetValue(outParams, "DeviceIsPaired", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetByte(value, deviceProperty.m_isConnected);
        rbusObject_SetValue(outParams, "DeviceIsConnected", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetUInt16(value, (unsigned short) deviceProperty.m_vendorID);
        rbusObject_SetValue(outParams, "DeviceVendorID", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetUInt16(value, (unsigned short) deviceProperty.m_serviceInfo.m_numOfService);
        rbusObject_SetValue(outParams, "numofservices", value);
        rbusValue_Release(value);

        for(int i =0; i < deviceProperty.m_serviceInfo.m_numOfService; i++)
        {
            rbusValue_Init(&value);
            rbusValue_SetUInt16(value, (unsigned short) deviceProperty.m_serviceInfo.m_profileInfo[i].m_uuid);
            rbusObject_SetValue(outParams, "DeviceProfileID", value);
            rbusValue_Release(value);

            rbusValue_Init(&value);
            rbusValue_SetBytes(value, (uint8_t *)deviceProperty.m_serviceInfo.m_profileInfo[i].m_profile, 
                                      strlen(deviceProperty.m_serviceInfo.m_profileInfo[i].m_profile));
            rbusObject_SetValue(outParams, "DeviceProfileName", value);
            rbusValue_Release(value);

        }
        BTRMGRLOG_TRACE ("Success\n");
    }
    else
    {
        retCode = RBUS_ERROR_INVALID_OPERATION;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }
    return retCode;
}

static rbusError_t
btrMgr_LeSetServiceInfo(
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle)
{
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    rbusValue_t UUID = NULL, opType = NULL;
    char m_UUID[BTRMGR_MAX_STR_LEN]= {};
    unsigned char m_ServiceType;

    BTRMGRLOG_ERROR("Entering......\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_NOT_INITIALIZED;
        BTRMGRLOG_ERROR("BTRMgr is not Inited\n");
    }

    else
    {
       BTRMGRLOG_ERROR("Entering else condition......\n");
       UUID = rbusObject_GetValue(inParams,"MainUUID");
       opType = rbusObject_GetValue(inParams,"OpType");

       strncpy(m_UUID, rbusValue_GetString(UUID,NULL), (BTRMGR_MAX_STR_LEN - 1));

       m_ServiceType = rbusValue_GetByte(opType);
       BTRMGRLOG_ERROR("calling BTRMGR_LE_SetServiceInfo......\n");

        rc = BTRMGR_LE_SetServiceInfo(0, m_UUID, m_ServiceType);

        if (BTRMGR_RESULT_SUCCESS == rc)
        {
            BTRMGRLOG_INFO("Success\n");
        }
        else
        {
            retCode = RBUS_ERROR_INVALID_OPERATION;
            BTRMGRLOG_ERROR("Failed; RetCode = %d\n", rc);
        }
    }
    return retCode;
}

static rbusError_t
btrMgr_LeSetGattInfo(
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle)
{
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    rbusError_t ret = RBUS_ERROR_SUCCESS;

    rbusValue_t parentUUID = NULL, CharUUID = NULL, propertyValue = NULL;
    BTRMGR_RBUSGATTInfo_t lstGattInfo ;

    if (!gIsBTRMGR_Internal_Inited) {
        ret = RBUS_ERROR_NOT_INITIALIZED;
        BTRMGRLOG_ERROR("BTRMgr is not Inited\n");
    }
    else
    {
        CharUUID = rbusObject_GetValue(inParams,"UUID");
        parentUUID = rbusObject_GetValue(inParams,"ParentUUID");
        propertyValue = rbusObject_GetValue(inParams,"PropertyValue");

        lstGattInfo.m_adapterIndex = 0;
        strncpy(lstGattInfo.m_ParentUUID, rbusValue_GetString(parentUUID,NULL), (BTRMGR_MAX_STR_LEN - 1));
        strncpy(lstGattInfo.m_UUID, rbusValue_GetString(CharUUID,NULL), (BTRMGR_MAX_STR_LEN - 1));
        lstGattInfo.m_Flags = 0x1;
        strncpy(lstGattInfo.m_Value, rbusValue_GetString(propertyValue,NULL), (BTRMGR_MAX_STR_LEN - 1));
        lstGattInfo.m_Element = BTRMGR_LE_PROP_CHAR;

        rc = BTRMGR_LE_SetGattInfo(lstGattInfo.m_adapterIndex, lstGattInfo.m_ParentUUID, lstGattInfo.m_UUID, lstGattInfo.m_Flags, lstGattInfo.m_Value, lstGattInfo.m_Element);

        if (BTRMGR_RESULT_SUCCESS == rc)
        {
            BTRMGRLOG_INFO("Success\n");
        }
        else
        {
            ret = RBUS_ERROR_INVALID_OPERATION;
            BTRMGRLOG_ERROR("Failed; RetCode = %d\n", rc);
        }
    }
    return ret;
}

static rbusError_t
btrMgr_SysDiagInfo(
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle)
{
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    //BTRMGR_IARMDiagInfo_t *lDiagInfo = NULL;
    char diagInfo[BTRMGR_MAX_STR_LEN] = {};
    char diagElement[BTRMGR_MAX_STR_LEN] ={};

    rbusValue_t value, m_UUID = NULL, m_OpType = NULL, m_DiagInfo = NULL;
    BTRMGRLOG_INFO("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_NOT_INITIALIZED;
        BTRMGRLOG_ERROR("BTRMgr is not Inited\n");
        return retCode;
    }

    m_UUID = rbusObject_GetValue(inParams,"DiagElement");
    m_OpType = rbusObject_GetValue(inParams,"OpType");

    strncpy(diagElement,rbusValue_GetString(m_UUID,NULL), BTRMGR_MAX_STR_LEN - 1);
    BTRMGR_LeOp_t opType = (BTRMGR_LeOp_t) (rbusValue_GetByte(m_OpType));

    if (BTRMGR_LE_OP_WRITE_VALUE == opType)
    {
        m_DiagInfo = rbusObject_GetValue(inParams,"DiagInfoValue");
        strncpy(diagInfo,rbusValue_GetString(m_DiagInfo,NULL), BTRMGR_MAX_STR_LEN - 1);
    }
    rc = BTRMGR_SysDiagInfo(0, diagElement, diagInfo, opType);
    if (BTRMGR_RESULT_SUCCESS == rc)
    {
        BTRMGRLOG_INFO("Success and setting diagInfo ... \n");
        rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)diagInfo, BTRMGR_MAX_STR_LEN - 1 );
        rbusObject_SetValue(outParams, "DiagInfo", value);
        rbusValue_Release(value);

        BTRMGRLOG_INFO("Success\n");
    }
    else {
        retCode = RBUS_ERROR_INVALID_OPERATION;
        BTRMGRLOG_ERROR("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

static rbusError_t
btrMgr_LeSetLteServiceState (
    rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts
) {
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_INVALID_INPUT;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    rbusValue_t value = rbusProperty_GetValue(prop);
    rc = BTRMGR_SetLTEServiceState(0, (rbusValue_GetByte(value)));
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = RBUS_ERROR_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

static rbusError_t
btrMgr_LeSetBroadcastState(
    rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t *opts)
{
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_INVALID_INPUT;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    rbusValue_t value = rbusProperty_GetValue(prop);
    rc = BTRMGR_SetBroadcastState(0, (rbusValue_GetByte(value)));
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = RBUS_ERROR_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}


static rbusError_t
btrMgr_LeSetBatteryOpsState (
    rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts
) {
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_INVALID_INPUT;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    rbusValue_t value = rbusProperty_GetValue(prop);
    rc = BTRMGR_SetBatteryOpsState(0, (rbusValue_GetByte(value)));
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = RBUS_ERROR_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

static rbusError_t
btrMgr_LeStartAdvertisement(
  rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle)
{
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    char serialNumber[BTRMGR_MAX_STR_LEN] = "";
    BTRMGR_LeCustomAdvertisement_t stCustomAdv =
    {
        0x02 ,
        0x01 ,
        0x06 ,
        0x05 ,
        0x03 ,
        0x0A ,
        0x18 ,
        0xB9 ,
        0xFD ,
        0x11 ,
        0xFF ,
        0xA3 ,
        0x07 ,
        0x0301
    };


    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_NOT_INITIALIZED;
        BTRMGRLOG_ERROR("BTRMgr is not Inited\n");
    }
    else
    {
        BTRMGRLOG_INFO("calling BTRMGR_LE_StartAdvertisement.. \n");

	BTRMGR_SysDiagInfo(0, BTRMGR_SERIAL_NUMBER_UUID, serialNumber, BTRMGR_LE_OP_READ_VALUE);
        strncpy((char*)stCustomAdv.serial_number , serialNumber, BTRMGR_SERIAL_NUM_LEN);
        rc = BTRMGR_LE_StartAdvertisement(0, &stCustomAdv);

        if (BTRMGR_RESULT_SUCCESS == rc)
        {
            BTRMGRLOG_INFO("Success\n");
        }
        else
        {
            retCode = RBUS_ERROR_INVALID_OPERATION;
            BTRMGRLOG_ERROR("Failed; RetCode = %d\n", rc);
        }
    }

    return retCode;
}

static rbusError_t
btrMgr_LeStopAdvertisement(
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle)
{
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_NOT_INITIALIZED;
        BTRMGRLOG_ERROR("BTRMgr is not Inited\n");
    }

    else
    {
        rc = BTRMGR_LE_StopAdvertisement(0);

        if (BTRMGR_RESULT_SUCCESS == rc)
        {
            BTRMGRLOG_INFO("Success\n");
        }
        else
        {
            retCode = RBUS_ERROR_INVALID_OPERATION;
            BTRMGRLOG_ERROR("Failed; RetCode = %d\n", rc);
        }
    }
    return retCode;
}

static rbusError_t
btrMgr_SetEventResponse (
    rbusHandle_t handle, char const *methodName, rbusObject_t inParams, rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle
) {
    rbusError_t   retCode = RBUS_ERROR_SUCCESS;
    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;

    BTRMGR_EventResponse_t  m_stBTRMgrEvtRsp;
    BTRMGRLOG_INFO ("Entering\n");

    if (!gIsBTRMGR_Internal_Inited) {
        retCode = RBUS_ERROR_INVALID_INPUT;
        BTRMGRLOG_ERROR ("BTRMgr is not Inited\n");
        return retCode;
    }

    rbusValue_t deviceHandle = NULL, eventType = NULL, eventResp = NULL;

    deviceHandle = rbusObject_GetValue(inParams,"DeviceHandle");
    eventType    = rbusObject_GetValue(inParams,"DeviceEventType");
    eventResp    = rbusObject_GetValue(inParams,"DeviceEventResp");

    m_stBTRMgrEvtRsp.m_deviceHandle = (BTRMgrDeviceHandle)(rbusValue_GetUInt64(deviceHandle));
    m_stBTRMgrEvtRsp.m_eventType = (BTRMGR_Events_t) (rbusValue_GetByte(eventType));
    m_stBTRMgrEvtRsp.m_eventResp = (unsigned char) (rbusValue_GetByte(eventResp));

    BTRMGRLOG_INFO("deviceHandle = %lld\n",m_stBTRMgrEvtRsp.m_deviceHandle);
    rc = BTRMGR_SetEventResponse(0, &m_stBTRMgrEvtRsp);
    if (BTRMGR_RESULT_SUCCESS == rc) {
        BTRMGRLOG_INFO ("Success\n");
    }
    else {
        retCode = RBUS_ERROR_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Failed; RetCode = %d\n", rc);
    }

    return retCode;
}

/*  Incoming Callbacks */

static BTRMGR_Result_t
btrMgr_EventCallback (
    BTRMGR_EventMessage_t   astEventMessage
)
{
    BTRMGR_EventMessage_t   lstEventMessage;
    BTRMGR_Result_t         lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    rbusEvent_t             event;
    rbusObject_t            data;
    rbusValue_t             value;
    int                     rc = RBUS_ERROR_SUCCESS;

    rbusObject_Init(&data, NULL);
    BTRMGRLOG_INFO ("Entering\n");
    memcpy (&lstEventMessage, &astEventMessage, sizeof(BTRMGR_EventMessage_t));

    rbusValue_Init(&value);
    rbusValue_SetByte(value, lstEventMessage.m_eventType);
    rbusObject_SetValue(data, "DeviceEventType", value);
    rbusValue_Release(value);

    BTRMGRLOG_INFO("printing EventMessage Type =%d \n", lstEventMessage.m_eventType);
    if ((lstEventMessage.m_eventType == BTRMGR_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST) && subscribed[0])
    {
        BTRMGRLOG_INFO ("Post External Device Pair Request event\n");

        BTRMGRLOG_INFO("devicetype = %d deviceName =%s deviceHandle =%lld \n",lstEventMessage.m_pairedDevice.m_deviceType, lstEventMessage.m_pairedDevice.m_name, lstEventMessage.m_externalDevice.m_deviceHandle);

        rbusValue_Init(&value);
        rbusValue_SetUInt64(value, (uint64_t)lstEventMessage.m_externalDevice.m_deviceHandle);
        rbusObject_SetValue(data, "ExDeviceHandle", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_externalDevice.m_name, strlen(lstEventMessage.m_externalDevice.m_name));
        rbusObject_SetValue(data, "ExDeviceName", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_externalDevice.m_deviceAddress,
                                   strlen(lstEventMessage.m_externalDevice.m_deviceAddress));
        rbusObject_SetValue(data, "ExDeviceAddress", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetUInt32(value, lstEventMessage.m_externalDevice.m_externalDevicePIN);
        rbusObject_SetValue(data, "ExternalDevicePIN", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetByte(value, lstEventMessage.m_externalDevice.m_requestConfirmation);
        rbusObject_SetValue(data, "RequestConfirmation", value);
        rbusValue_Release(value);

        event.name = eventDataElements[0].name;
        event.data = data;
        event.type = RBUS_EVENT_GENERAL;

        rc = rbusEvent_Publish(rbusHandle, &event);
        rbusObject_Release(data);

        if(rc != RBUS_ERROR_SUCCESS)
        {
            BTRMGRLOG_INFO("rbusEvent_Publish Event failed: %d\n", rc);
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }
        else
            BTRMGRLOG_INFO("rbusEvent_Publish Event success: %d\n", rc);
    }
    if ((lstEventMessage.m_eventType == BTRMGR_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST) && subscribed[1])
    {
        BTRMGRLOG_INFO("Post External Device Connect Request event\n");

        BTRMGRLOG_INFO("deviceAdd = %s deviceName =%s deviceHandle =%lld \n",lstEventMessage.m_pairedDevice.m_deviceAddress, lstEventMessage.m_pairedDevice.m_name,lstEventMessage.m_externalDevice.m_deviceHandle);	

        rbusValue_Init(&value);
        rbusValue_SetUInt64(value, (uint64_t)lstEventMessage.m_externalDevice.m_deviceHandle);
        rbusObject_SetValue(data, "ExDeviceHandle", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_externalDevice.m_name, strlen(lstEventMessage.m_externalDevice.m_name));
        rbusObject_SetValue(data, "ExDeviceName", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_externalDevice.m_deviceAddress,
                                  strlen(lstEventMessage.m_externalDevice.m_deviceAddress));
        rbusObject_SetValue(data, "ExDeviceAddress", value);
        rbusValue_Release(value);

        event.name = eventDataElements[1].name;
        event.data = data;
        event.type = RBUS_EVENT_GENERAL;

        rc = rbusEvent_Publish(rbusHandle, &event);
        rbusObject_Release(data);

        if(rc != RBUS_ERROR_SUCCESS)
        {
            BTRMGRLOG_INFO("rbusEvent_Publish Event failed: %d\n", rc);
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }
        else
            BTRMGRLOG_INFO("rbusEvent_Publish Event success: %d\n", rc);
    }
    if ( ((lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_PAIRING_COMPLETE) && subscribed[2]) ||
	  ((lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_PAIRING_FAILED) && subscribed[3]) ||
          ((lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_DISCOVERY_UPDATE) && subscribed[4]) )
    {

        BTRMGRLOG_INFO("deviceAdd = %s deviceName =%s deviceHandle =%lld \n",lstEventMessage.m_discoveredDevice.m_deviceAddress, lstEventMessage.m_discoveredDevice.m_name,lstEventMessage.m_discoveredDevice.m_deviceHandle);

        rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_discoveredDevice.m_name, strlen(lstEventMessage.m_discoveredDevice.m_name));
        rbusObject_SetValue(data, "DiscDeviceName", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetByte(value, lstEventMessage.m_discoveredDevice.m_deviceType);
        rbusObject_SetValue(data, "DiscDeviceType", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetUInt64(value, (uint64_t)lstEventMessage.m_discoveredDevice.m_deviceHandle);
        rbusObject_SetValue(data, "DiscDeviceHandle", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_discoveredDevice.m_deviceAddress,
                                  strlen(lstEventMessage.m_discoveredDevice.m_deviceAddress));
        rbusObject_SetValue(data, "DiscDeviceAddress", value);
        rbusValue_Release(value);

        if(lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_PAIRING_COMPLETE)
        {
            event.name = eventDataElements[2].name;
        }
        else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_PAIRING_FAILED)
        {
            event.name = eventDataElements[3].name;
        }
        else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_DISCOVERY_UPDATE)
        {
            event.name = eventDataElements[4].name;
        }       
        else
        {
            BTRMGRLOG_INFO("Invalid Event...\n");
        }

        event.data = data;
        event.type = RBUS_EVENT_GENERAL;

        rc = rbusEvent_Publish(rbusHandle, &event);
        rbusObject_Release(data);

        if(rc != RBUS_ERROR_SUCCESS)
        {
            BTRMGRLOG_INFO("rbusEvent_Publish Event failed: %d\n", rc);
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }
        else
            BTRMGRLOG_INFO("rbusEvent_Publish Event success: %d\n", rc);
    }
    if ( ((lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_UNPAIRING_COMPLETE) && subscribed[5]) ||
          ((lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_UNPAIRING_FAILED) && subscribed[6]) ||
          ((lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_CONNECTION_COMPLETE) && subscribed[7]) ||
          ((lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_CONNECTION_FAILED) && subscribed[8]) ||
          ((lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_DISCONNECT_COMPLETE) && subscribed[9]) ||
	      ((lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_DISCONNECT_FAILED) && subscribed[10]) ||
          ((lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_FOUND) && subscribed[11]) ||
          ((lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_OUT_OF_RANGE) && subscribed[12]) )
    {
        BTRMGRLOG_INFO("devicetype = %d deviceName =%s deviceHandle =%lld \n",lstEventMessage.m_pairedDevice.m_deviceType, lstEventMessage.m_pairedDevice.m_name, lstEventMessage.m_pairedDevice.m_deviceHandle);

        rbusValue_Init(&value);
        rbusValue_SetByte(value, lstEventMessage.m_pairedDevice.m_deviceType);
        rbusObject_SetValue(data, "PairedDeviceType", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_pairedDevice.m_name, strlen(lstEventMessage.m_pairedDevice.m_name));
        rbusObject_SetValue(data, "PairedDeviceName", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetUInt64(value, (uint64_t)lstEventMessage.m_pairedDevice.m_deviceHandle);
        rbusObject_SetValue(data, "PairedDeviceHandle", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_pairedDevice.m_deviceAddress,
                                strlen(lstEventMessage.m_pairedDevice.m_deviceAddress));
        rbusObject_SetValue(data, "PairedDeviceAddress", value);
        rbusValue_Release(value);

	    rbusValue_Init(&value);
        rbusValue_SetByte(value, lstEventMessage.m_pairedDevice.m_isLastConnectedDevice);
        rbusObject_SetValue(data, "LastConnectedDevice", value);
        rbusValue_Release(value);

        if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_UNPAIRING_COMPLETE)
        {
            event.name = eventDataElements[5].name;
        }
        else if(lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_UNPAIRING_FAILED)
        {
            event.name = eventDataElements[6].name;
        }
        else if(lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_CONNECTION_COMPLETE)
        {
            event.name = eventDataElements[7].name;
        }
	    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_CONNECTION_FAILED)
        {
            event.name = eventDataElements[8].name;
        }
        else if(lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_DISCONNECT_COMPLETE)
        {
            event.name = eventDataElements[9].name;
	    }
	    else if(lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_DISCONNECT_FAILED)
        {
            event.name = eventDataElements[10].name;
	    }
	    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_FOUND)
	    {
            event.name = eventDataElements[11].name;
        }
	    else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_OUT_OF_RANGE)
	    {
            event.name = eventDataElements[12].name;
        }
        else
        {
            BTRMGRLOG_INFO("Invalid event...\n");
	    }

        event.data = data;
        event.type = RBUS_EVENT_GENERAL;

        rc = rbusEvent_Publish(rbusHandle, &event);
        rbusObject_Release(data);

        if(rc != RBUS_ERROR_SUCCESS)
        {
            BTRMGRLOG_INFO("rbusEvent_Publish Event failed: %d\n", rc);
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }
        else
            BTRMGRLOG_INFO("rbusEvent_Publish Event success: %d\n", rc);
    }
    if( ((lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_OP_READY) && subscribed[13]) ||
	 ((lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_OP_INFORMATION) && subscribed[14]) )
    {

	     BTRMGRLOG_INFO("devicetype = %d deviceName =%s \n",lstEventMessage.m_deviceOpInfo.m_deviceType, lstEventMessage.m_deviceOpInfo.m_name);

	    rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_deviceOpInfo.m_name, strlen(lstEventMessage.m_deviceOpInfo.m_name));
        rbusObject_SetValue(data, "DeviceOPInfoName", value);
        rbusValue_Release(value);

	    rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_deviceOpInfo.m_uuid, strlen(lstEventMessage.m_deviceOpInfo.m_uuid));
        rbusObject_SetValue(data, "DeviceOPInfoUuid", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetByte(value, lstEventMessage.m_deviceOpInfo.m_deviceType);
        rbusObject_SetValue(data, "DeviceOPInfoType", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_deviceOpInfo.m_notifyData, strlen(lstEventMessage.m_deviceOpInfo.m_notifyData));
        rbusObject_SetValue(data, "DeviceOPInfoNotifyData", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetByte(value, lstEventMessage.m_deviceOpInfo.m_leOpType);
        rbusObject_SetValue(data, "DeviceLeOpType", value);
        rbusValue_Release(value);

	    rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_deviceOpInfo.m_writeData, strlen(lstEventMessage.m_deviceOpInfo.m_writeData));
        rbusObject_SetValue(data, "DeviceOPInfoWriteData", value);
        rbusValue_Release(value);

        if(lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_OP_READY)
        {
            BTRMGRLOG_WARN ("Post  Device OP ready event\n");
            event.name = eventDataElements[13].name;
        }
        else if (lstEventMessage.m_eventType == BTRMGR_EVENT_DEVICE_OP_INFORMATION)
        {
            BTRMGRLOG_WARN ("Post  Device OP INFORMATION event\n");
            event.name = eventDataElements[14].name;
        }

        event.data = data;
        event.type = RBUS_EVENT_GENERAL;

        rc = rbusEvent_Publish(rbusHandle, &event);
        rbusObject_Release(data);

        if(rc != RBUS_ERROR_SUCCESS)
        {
            BTRMGRLOG_INFO("rbusEvent_Publish Event failed: %d\n", rc);
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }
        else
            BTRMGRLOG_INFO("rbusEvent_Publish Event success: %d\n", rc);
    }

    if(((lstEventMessage.m_eventType == BTRMGR_EVENT_BATTERY_INFO) && subscribed[15]))
    {
        BTRMGRLOG_INFO("devicetype = %d deviceName =%s \n",lstEventMessage.m_batteryInfo.m_deviceType, lstEventMessage.m_batteryInfo.m_name);

        rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_batteryInfo.m_name, strlen(lstEventMessage.m_batteryInfo.m_name));
        rbusObject_SetValue(data, "BatteryInfoName", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetByte(value, lstEventMessage.m_batteryInfo.m_deviceType);
        rbusObject_SetValue(data, "BatteryInfoType", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_batteryInfo.m_uuid, strlen(lstEventMessage.m_batteryInfo.m_uuid));
        rbusObject_SetValue(data, "BatteryInfoUuid", value);
        rbusValue_Release(value);

        rbusValue_Init(&value);
        rbusValue_SetUInt64(value, (uint64_t)lstEventMessage.m_batteryInfo.m_deviceHandle);
        rbusObject_SetValue(data, "BatteryInfoDeviceHandle", value);
        rbusValue_Release(value);

	    BTRMGRLOG_INFO("Battery Notification from internal interface : value - %s UUID - %s \n",lstEventMessage.m_batteryInfo.m_notifyData,lstEventMessage.m_batteryInfo.m_uuid);

	    if (!strcmp(lstEventMessage.m_batteryInfo.m_uuid,BTRMGR_UUID_BATTERY_LEVEL)) {
            rbusValue_Init(&value);
            rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_batteryInfo.m_notifyData, strlen(lstEventMessage.m_batteryInfo.m_notifyData));
            rbusObject_SetValue(data, "BatteryInfoBatteryLevel", value);
            rbusValue_Release(value);
        } else if (!strcmp(lstEventMessage.m_batteryInfo.m_uuid,BTRMGR_UUID_BATTERY_FLAGS)) {
            rbusValue_Init(&value);
            rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_batteryInfo.m_notifyData, strlen(lstEventMessage.m_batteryInfo.m_notifyData));
            rbusObject_SetValue(data, "BatteryInfoFlags", value);
            rbusValue_Release(value);
        } else if (!strcmp(lstEventMessage.m_batteryInfo.m_uuid,BTRMGR_UUID_BATTERY_ERROR_VALUES)) {
            rbusValue_Init(&value);
            rbusValue_SetBytes(value, (uint8_t *)lstEventMessage.m_batteryInfo.m_notifyData, strlen(lstEventMessage.m_batteryInfo.m_notifyData));
            rbusObject_SetValue(data, "BatteryInfoErrorValues", value);
            rbusValue_Release(value);
        }

	 rbusValue_Init(&value);
         
         BTRMGRLOG_INFO("Post Battery Info ready event \n");

         event.name = eventDataElements[15].name;
         event.data = data;
         event.type = RBUS_EVENT_GENERAL;

        rc = rbusEvent_Publish(rbusHandle, &event);
        rbusObject_Release(data);

        if(rc != RBUS_ERROR_SUCCESS)
        {
            BTRMGRLOG_INFO("rbusEvent_Publish Event failed: %d\n", rc);
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }
        else
            BTRMGRLOG_INFO("rbusEvent_Publish Event success: %d\n", rc);
    }

    return lenBtrMgrResult;
}

rbusError_t eventSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)autoPublish;
    (void)interval;

    BTRMGRLOG_INFO(
        "eventSubHandler called:\n" \
        "\taction=%s\n" \
        "\teventName=%s\n",
        action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
        eventName);

    if(!strcmp(BTRMGR_RBUS_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST, eventName))
    {
        subscribed[0] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST, eventName))
    {
        subscribed[1] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_DEVICE_PAIRING_COMPLETE, eventName))
    {
        subscribed[2] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_DEVICE_PAIRING_FAILED, eventName))
    {
        subscribed[3] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_DEVICE_DISCOVERY_UPDATE, eventName))
    {
        subscribed[4] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_DEVICE_UNPAIRING_COMPLETE, eventName))
    {
        subscribed[5] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_DEVICE_UNPAIRING_FAILED, eventName))
    {
        subscribed[6] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_DEVICE_CONNECTION_COMPLETE, eventName))
    {
        subscribed[7] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_DEVICE_CONNECTION_FAILED, eventName))
    {
        subscribed[8] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_DEVICE_DISCONNECT_COMPLETE, eventName))
    {
        subscribed[9] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_DEVICE_DISCONNECT_FAILED, eventName))
    {
        subscribed[10] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_DEVICE_FOUND, eventName))
    {
        subscribed[11] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_DEVICE_OUT_OF_RANGE, eventName))
    {
        subscribed[12] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_DEVICE_OP_READY, eventName))
    {
        subscribed[13] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_DEVICE_OP_INFORMATION, eventName))
    {
        subscribed[14] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp(BTRMGR_RBUS_EVENT_BATTERY_INFO, eventName))
    {
        subscribed[15] = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else
    {
        BTRMGRLOG_INFO("provider: eventSubHandler unexpected eventName %s\n", eventName);
    }
    BTRMGRLOG_INFO("eventSubHandler success\n");
    return RBUS_ERROR_SUCCESS;
}

void
BTRMgr_BeginRBUSMode (void)
{
   rbusError_t rc;
   BTRMGRLOG_INFO ("Entering BTRMgr_BeginRBUSMode\n");
   if (!gIsBTRMGR_Internal_Inited)
   {
       gIsBTRMGR_Internal_Inited = 1;

       rc = rbus_open(&rbusHandle, componentName);
       if(rc != RBUS_ERROR_SUCCESS)
       {
          BTRMGRLOG_INFO("BTRMgr_BeginRBUSMode: rbus_open failed: %d\n", rc);
          return;
       }
          BTRMGRLOG_INFO("BTRMgr_BeginRBUSMode: rbus_open sucess: %d\n", rc);

       rc = rbus_regDataElements(rbusHandle, TotalParams, dataElements);
       if(rc == RBUS_ERROR_SUCCESS)
       {
          BTRMGRLOG_INFO("BTRMgr_BeginRBUSMode: rbus_regDataElements Successful:\n");
       }
       else
       {
          BTRMGRLOG_INFO("BTRMgr_BeginRBUSMode: rbus_regDataElements failed: %d\n", rc);
          return;
       }
       rc = rbus_regDataElements(rbusHandle, TotalEventParams, eventDataElements);
       if(rc == RBUS_ERROR_SUCCESS)
       {
          BTRMGRLOG_INFO("BTRMgr_BeginRBUSMode: rbus_regeventDataElements Successful:\n");
       }
       else
       {
          BTRMGRLOG_INFO("BTRMgr_BeginRBUSMode: rbus_regDataElements failed: %d\n", rc);
          return;
       }

   }

   /* Register a callback */
   BTRMGR_RegisterEventCallback(btrMgr_EventCallback);

   BTRMGRLOG_INFO("BTRMgr_BeginRBUSMode: exit\n");
   return;
}

void
BTRMgr_TermRBUSMode (
    void
) {
    BTRMGRLOG_INFO ("Entering\n");
    rbusError_t rc;

    if (gIsBTRMGR_Internal_Inited) {
        BTRMGRLOG_INFO ("RBUS Interface Being terminated\n");
        rc = rbus_unregDataElements(rbusHandle, TotalParams, dataElements);
        if(rc != RBUS_ERROR_SUCCESS)
        {
           BTRMGRLOG_INFO("BTRMgr_TermRBUSMode: rbus_unregDataElements failed: %d\n", rc);
           return;
        }
        rc = rbus_unregDataElements(rbusHandle, TotalEventParams, eventDataElements);
        if(rc != RBUS_ERROR_SUCCESS)
        {
           BTRMGRLOG_INFO("BTRMgr_TermRBUSMode: rbus_unregDataElements failed: %d\n", rc);
           return;
        }
        rc = rbus_close(rbusHandle);
        if(rc != RBUS_ERROR_SUCCESS)
        {
           BTRMGRLOG_INFO("BTRMgr_TermRBUSMode: rbus_close failed: %d\n", rc);
           return;
        }
        else
        {
           rbusHandle = NULL;
        }
    }
    else {
        BTRMGRLOG_INFO ("RBUS Interface Not Inited\n");
    }
}

