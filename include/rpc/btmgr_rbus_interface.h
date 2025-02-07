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
#include "rbus.h"


/**
 * @file  btmgr_rbus_interface.h
 *
 * @defgroup   RBUS Interface
 * This file defines bluetooth manager's rbus interfaces to external BT devices.
 * @ingroup  BTR_MGR
 */
#define TotalEventParams  16

void BTRMgr_BeginRBUSMode();
void BTRMgr_TermRBUSMode();

typedef struct _BTRMGR_RBUSGATTInfo_t {
    unsigned char m_adapterIndex;
    char m_ParentUUID[BTRMGR_MAX_STR_LEN];
    char m_UUID[BTRMGR_MAX_STR_LEN];
    unsigned short m_Flags;
    char m_Value[BTRMGR_MAX_STR_LEN];
    BTRMGR_LeProperty_t m_Element;
}BTRMGR_RBUSGATTInfo_t;

/**
 RBUS elemets
 **/

#define BTRMGR_RBUS_METHOD_SET_ADAPTER_DISCOVERABLE                 "SetAdapterDiscoverable"
#define BTRMGR_RBUS_METHOD_CHANGE_DEVICE_DISCOVERY_STATUS           "SetDeviceDiscoveryStatus"
#define BTRMGR_RBUS_METHOD_GET_DISCOVERED_DEVICES                   "GetDiscoveredDevices"
#define BTRMGR_RBUS_METHOD_PAIR_DEVICE                              "PairDevice"
#define BTRMGR_RBUS_METHOD_UNPAIR_DEVICE                            "UnpairDevice"
#define BTRMGR_RBUS_METHOD_GET_PAIRED_DEVICES                       "GetPairedDevices"
#define BTRMGR_RBUS_METHOD_CONNECT_TO_DEVICE                        "ConnectToDevice"
#define BTRMGR_RBUS_METHOD_DISCONNECT_FROM_DEVICE                   "DisconnectFromDevice"
#define BTRMGR_RBUS_METHOD_GET_CONNECTED_DEVICES                    "GetConnectedDevices"
#define BTRMGR_RBUS_METHOD_GET_DEVICE_PROPERTIES                    "GetDeviceProperties"
#define BTRMGR_RBUS_METHOD_SET_EVENT_RESPONSE                       "SetEventResponse"
#define BTRMGR_RBUS_METHOD_GET_SYS_DIAG_INFO                        "GetSystemDiagnosticsInfo"
#define BTRMGR_RBUS_METHOD_LE_SET_GATT_SERVICE_INFO                 "LeSetGattServiceInfo"
#define BTRMGR_RBUS_METHOD_LE_SET_GATT_CHAR_INFO                    "LeSetGattCharacteristicInfo"
#define BTRMGR_RBUS_METHOD_LE_START_ADVERTISEMENT                   "LeStartAdvertisement"
#define BTRMGR_RBUS_METHOD_LE_STOP_ADVERTISEMENT                    "LeStopAdvertisement"
#define BTRMGR_RBUS_METHOD_TO_SET_LTE_SERVICE_STATE                 "SetLTEServiceState"
#define BTRMGR_RBUS_METHOD_SET_BROADCAST_STATE                      "SetBroadcastState"
#define BTRMGR_RBUS_METHOD_SET_BATTERY_OPS_STATE                    "SetBatteryOpsState"

#define BTRMGR_RBUS_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST            "ExternelPairRequest!"
#define BTRMGR_RBUS_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST         "ExternelConnectRequest!"
#define BTRMGR_RBUS_EVENT_DEVICE_PAIRING_COMPLETE                   "PairingCompletedEvent!"
#define BTRMGR_RBUS_EVENT_DEVICE_PAIRING_FAILED                     "PairingFailedEvent!"
#define BTRMGR_RBUS_EVENT_DEVICE_DISCOVERY_UPDATE                   "DeviceDiscoveryUpdateEvent!"
#define BTRMGR_RBUS_EVENT_DEVICE_UNPAIRING_COMPLETE                 "UnPairingCompletedEvent!"
#define BTRMGR_RBUS_EVENT_DEVICE_UNPAIRING_FAILED                   "UnPairingFailedEvent!"
#define BTRMGR_RBUS_EVENT_DEVICE_CONNECTION_COMPLETE                "ConnectionCompletedEvent!"
#define BTRMGR_RBUS_EVENT_DEVICE_CONNECTION_FAILED                  "ConnectionFailedEvent!"
#define BTRMGR_RBUS_EVENT_DEVICE_DISCONNECT_COMPLETE                "DisConnectionCompletedEvent!"
#define BTRMGR_RBUS_EVENT_DEVICE_DISCONNECT_FAILED                  "DisConnectionFailedEvent!"
#define BTRMGR_RBUS_EVENT_DEVICE_FOUND                              "DeviceFoundEvent!"
#define BTRMGR_RBUS_EVENT_DEVICE_OUT_OF_RANGE                       "DeviceOutOfRange!"
#define BTRMGR_RBUS_EVENT_DEVICE_OP_READY                           "DeviceOPReady!"
#define BTRMGR_RBUS_EVENT_DEVICE_OP_INFORMATION                     "DeviceOPInformation!"
#define BTRMGR_RBUS_EVENT_BATTERY_INFO                              "BatteryInfo!"
/** @} */

