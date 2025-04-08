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
/**
 * @file btrMgr_SysDiag.c
 *
 * @description This file implements bluetooth manager's 
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* System Headers */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* Ext lib Headers */
#ifdef BTR_SYS_DIAG_IARM_ENABLE
#include "libIBus.h"
#include "libIARM.h"
#include "sysMgr.h"
#include "power_controller.h"
#endif

#ifdef BTR_SYS_DIAG_RBUS_ENABLE
#include "rbus.h"
#include "syscfg/syscfg.h"
#endif

#include "safec_lib.h"

#ifdef BTR_SYS_DIAG_IARM_ENABLE
#include <curl/curl.h>
#include "cJSON.h"
#include "urlHelper.h"
#include "btrMgr_DeviceUtils.h"
#include "json_parse.h"
#endif
/* Interface lib Headers */
#include "btrMgr_logger.h"

/* Local Headers */
#include "btrMgr_Types.h"
#include "btrMgr_SysDiag.h"

#ifdef BTR_SYS_DIAG_RBUS_ENABLE
#define DEVICE_QR_CODE                     "WN18A0C0044.WN12345"
#define DEVICE_MESH_BAKHAUL_STAT           "radio:5g"
#define RDK_SETUP_MESH_BACKHAUL_WIFI    0x0100
#define RDK_SETUP_MESH_BACKHAUL_ETH     0x0101
#define RDK_SETUP_MESH_BACKHAUL_ERROR   0x01FF
extern rbusHandle_t    rbusHandle;
#endif

typedef struct _stBTRMgrSDHdl {
#ifdef BTR_SYS_DIAG_IARM_ENABLE
    PowerController_PowerState_t    _powerState;
#endif
    stBTRMgrSysDiagStatus           lstBtrMgrSysDiagStat;
    fPtr_BTRMgr_SD_StatusCb         fpcBSdStatus;
    void*                           pvcBUserData;
} stBTRMgrSDHdl;

stBTRMgrSDHdl* gpstSDHandle = NULL;

#ifndef UNIT_TEST
#define STATIC static
#else 
#define STATIC 
#endif 


/* STATIC Function Prototypes */
STATIC int btrMgr_SysDiag_getDeviceMAC(char* aFileName, unsigned char* aData);
#ifndef BTR_SYS_DIAG_RBUS_ENABLE
STATIC int btrMgr_SysDiag_getDiagInfoFromFile(char* aFileName, char* aData);
#endif
STATIC int btrMgr_SysDiag_getDiagInfoFromPipe(char* aCmd, char* aData);

/* Incoming Callbacks Prototypes */
#ifdef BTR_SYS_DIAG_IARM_ENABLE
STATIC void btrMgr_SysDiag_powerModeChangeCb (
                const PowerController_PowerState_t currentState,
                const PowerController_PowerState_t newState,
                void *userData);
#endif

/* STATIC Function Definitions */
STATIC int
btrMgr_SysDiag_getDeviceMAC (
    char*           aFileName,
    unsigned char*  aData
) {
    FILE *fPtr;
    int lElement;
    int index = 0;
    char temp[6];
    unsigned int lDeviceMac[16];
    int count = 0;
    int lDataLen = 0;
    int ch = 0;
    int leBtrMgrAcRet = 0;
    
    fPtr = fopen(aFileName, "r");
    if (NULL == fPtr) {
        printf("File cannot be opened\n");
    }
    else {
        while ((lElement = fgetc(fPtr)) != '\n') {
            if (lElement != ':') {
                snprintf(temp, sizeof(temp), "%c", lElement);
                sscanf(temp, "%x", &ch);
                lDeviceMac[index] = ch;
                index++;
            }
        }
    
        lDataLen = index;
        index = 0;
        while (index < lDataLen) {
            aData[count] = lDeviceMac[index] << 4;
            index++;
            aData[count] |= lDeviceMac[index++];
            count++;
        }

        fclose(fPtr);
    }
    printf("device mac addr is %s\n", aData);
    return leBtrMgrAcRet;
}

#ifndef BTR_SYS_DIAG_RBUS_ENABLE
STATIC int
btrMgr_SysDiag_getDiagInfoFromFile (
    char* aFileName,
    char* aData
) {
    FILE* fPtr = NULL;
    int leBtrMgrAcRet = 0;

    fPtr = fopen(aFileName, "r");
    if (NULL == fPtr) {
        leBtrMgrAcRet = -1;
        printf("File cannot be opened\n");
    }
    else {
        if (NULL == fgets(aData, BTRMGR_STR_LEN_MAX, fPtr)) {
            BTRMGRLOG_INFO("Could not parse output of <%s>\n", aFileName);
        }
        else {
            if ('\n' == aData[strlen(aData) - 1]) {
                aData[strlen(aData) - 1] = '\0';
            }
        }
        fclose(fPtr);   //CID:115333 - Alloc free mismatch
    }

    return leBtrMgrAcRet;
}
#endif

STATIC int
btrMgr_SysDiag_getDiagInfoFromPipe (
    char* aCmd,
    char* aData
) {
    FILE *fPipe;
    int leBtrMgrAcRet = 0;
    
    fPipe = popen(aCmd, "r");
    if (NULL == fPipe) {    /* check for errors */
        leBtrMgrAcRet = -1;
        BTRMGRLOG_INFO("Pipe failed to open\n");
    }
    else {
        if (NULL == fgets(aData, BTRMGR_STR_LEN_MAX, fPipe)) {
            BTRMGRLOG_INFO("Could not parse output of <%s>\n", aCmd);
        }
        else {
            if ('\n' == aData[strlen(aData) - 1]) {
                aData[strlen(aData) - 1] = '\0';
            }
        }

        pclose(fPipe);
    }

    return leBtrMgrAcRet;
}


/* Interfaces - Public Functions */
eBTRMgrRet
BTRMgr_SD_Init (
    tBTRMgrSDHdl*           hBTRMgrSdHdl,
    fPtr_BTRMgr_SD_StatusCb afpcBSdStatus,
    void*                   apvUserData
) {
    stBTRMgrSDHdl* sDHandle = NULL;

    if ((sDHandle = (stBTRMgrSDHdl*)malloc (sizeof(stBTRMgrSDHdl))) == NULL) {
        BTRMGRLOG_ERROR ("BTRMgr_SD_Init FAILED\n");
        return eBTRMgrFailure;
    }

    MEMSET_S(sDHandle, sizeof(stBTRMgrSDHdl), 0, sizeof(stBTRMgrSDHdl));
    sDHandle->lstBtrMgrSysDiagStat.enSysDiagChar = BTRMGR_SYS_DIAG_UNKNOWN;
#ifdef BTR_SYS_DIAG_IARM_ENABLE
    sDHandle->_powerState = POWER_STATE_OFF;
#endif
    sDHandle->fpcBSdStatus= afpcBSdStatus;
    sDHandle->pvcBUserData= apvUserData;

    gpstSDHandle = sDHandle;
    *hBTRMgrSdHdl = (tBTRMgrSDHdl)sDHandle;
    return eBTRMgrSuccess;
}


eBTRMgrRet
BTRMgr_SD_DeInit (
    tBTRMgrSDHdl hBTRMgrSdHdl
) {
    stBTRMgrSDHdl*  pstBtrMgrSdHdl = (stBTRMgrSDHdl*)hBTRMgrSdHdl;

    if (NULL != pstBtrMgrSdHdl) {
        gpstSDHandle = NULL;
        free((void*)pstBtrMgrSdHdl);
        pstBtrMgrSdHdl = NULL;
        BTRMGRLOG_INFO ("BTRMgr_SD_DeInit SUCCESS\n");
        return eBTRMgrSuccess;
    }
    else {
        BTRMGRLOG_WARN ("BTRMgr SD handle is not Inited(NULL)\n");
        return eBTRMgrFailure;
    }
}

eBTRMgrRet
BTRMGR_SD_GetData (
    tBTRMgrSDHdl         hBTRMgrSdHdl,
    BTRMGR_SysDiagChar_t aenSysDiagChar,
    char*                aData
) {
    stBTRMgrSDHdl*  pstBtrMgrSdHdl = (stBTRMgrSDHdl*)hBTRMgrSdHdl;
    eBTRMgrRet      rc = eBTRMgrSuccess;
#ifdef BTR_SYS_DIAG_IARM_ENABLE
    IARM_Result_t   lIARMStatus = IARM_RESULT_SUCCESS;
#endif

#ifdef BTR_SYS_DIAG_RBUS_ENABLE
    int ret = 0;
    rbusError_t retCode = RBUS_ERROR_BUS_ERROR;
    rbusValue_t value = NULL;
    printf("<<<in BTR_SYS_DIAG_RBUS_ENABLE >>>\n");

    char buff[16] = {0};
    bool mesh_type;

    (void)pstBtrMgrSdHdl;
#endif

    if (NULL == pstBtrMgrSdHdl)
        return eBTRMgrFailure;

    switch (aenSysDiagChar) {
        case BTRMGR_SYS_DIAG_DEVICEMAC: {
            unsigned char lData[BTRMGR_STR_LEN_MAX] = "\0";

            btrMgr_SysDiag_getDeviceMAC("/tmp/.estb_mac", lData);
            int ret = snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", lData);
            if (ret > (BTRMGR_STR_LEN_MAX - 1)) {
                BTRMGRLOG_DEBUG("BTRMGR_SYS_DIAG_DEVICEMAC truncated\n");
            }
        }
        break;
        case BTRMGR_SYS_DIAG_BTRADDRESS: {
            btrMgr_SysDiag_getDiagInfoFromPipe("hcitool dev |grep hci |cut -d$'\t' -f3", aData);
        }
        break;
        case BTRMGR_SYS_DIAG_SYSTEMID: {
#ifdef BTR_SYS_DIAG_RBUS_ENABLE
            retCode = rbus_get(rbusHandle, "Device.DeviceInfo.X_COMCAST-COM_CM_MAC", &value);
            if ((retCode == RBUS_ERROR_SUCCESS) && (value != NULL)) {
                printf("DeviceMac  = [%s]\n", rbusValue_GetString(value, NULL));
                ret = snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", rbusValue_GetString(value, NULL));
                if (ret > (BTRMGR_STR_LEN_MAX - 1)) {
                    BTRMGRLOG_DEBUG("BTRMGR_SYS_DIAG_DEVICEMAC truncated\n");
                }
            }
            else {
                rc = eBTRMgrFailure;
            }
#else
            btrMgr_SysDiag_getDiagInfoFromFile("/tmp/.model_number", aData);
#endif
        }
        break;
        case BTRMGR_SYS_DIAG_HWREVISION: {
#ifdef BTR_SYS_DIAG_RBUS_ENABLE
            retCode = rbus_get(rbusHandle, "Device.DeviceInfo.HardwareVersion", &value);
            if ((retCode == RBUS_ERROR_SUCCESS) && (value != NULL)) {
                printf("HWVersion  = [%s]\n", rbusValue_GetString(value, NULL));
                snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", rbusValue_GetString(value, NULL));
            }
            else {
                rc = eBTRMgrFailure;
            }
#else
            btrMgr_SysDiag_getDiagInfoFromFile("/tmp/.model_number", aData);
#endif
        }
        break;
        case BTRMGR_SYS_DIAG_MODELNUMBER: {
#ifdef BTR_SYS_DIAG_RBUS_ENABLE
            retCode = rbus_get(rbusHandle, "Device.DeviceInfo.ModelName", &value);
            if ((retCode == RBUS_ERROR_SUCCESS) && (value != NULL)) {
                printf("ModelNumber  = [%s]\n", rbusValue_GetString(value, NULL));
                snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", rbusValue_GetString(value, NULL));
            }
            else {
                rc = eBTRMgrFailure;
            }
#else
            btrMgr_SysDiag_getDiagInfoFromFile("/tmp/.model_number", aData);
#endif
        }
        break;
        case BTRMGR_SYS_DIAG_SERIALNUMBER: {
#ifdef BTR_SYS_DIAG_RBUS_ENABLE
            retCode = rbus_get(rbusHandle, "Device.DeviceInfo.SerialNumber", &value);
            if ((retCode == RBUS_ERROR_SUCCESS) && (value != NULL)) {
                printf("Serial Number  = [%s]\n", rbusValue_GetString(value, NULL));
                snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", rbusValue_GetString(value, NULL));
            }
            else {
                rc = eBTRMgrFailure;
            }
#else
            btrMgr_SysDiag_getDiagInfoFromPipe("grep Serial /proc/cpuinfo | cut -d ' ' -f2 | tr '[:lower:]' '[:upper:]'", aData);
#endif
        }
        break;
        case BTRMGR_SYS_DIAG_FWREVISION:
        case BTRMGR_SYS_DIAG_SWREVISION: {
#ifdef BTR_SYS_DIAG_RBUS_ENABLE
            retCode = rbus_get(rbusHandle, "Device.DeviceInfo.SoftwareVersion", &value);
            if ((retCode == RBUS_ERROR_SUCCESS) && (value != NULL)) {
                printf("Software/firmware Version  = [%s]\n", rbusValue_GetString(value, NULL));
                snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", rbusValue_GetString(value, NULL));
            }
            else {
                rc = eBTRMgrFailure;
            }
#else
            btrMgr_SysDiag_getDiagInfoFromFile("/tmp/.imageVersion", aData);
#endif
        }
        break;
        case BTRMGR_SYS_DIAG_MFGRNAME: {
#ifdef BTR_SYS_DIAG_RBUS_ENABLE
            retCode = rbus_get(rbusHandle, "Device.DeviceInfo.Manufacturer", &value);
            if ((retCode == RBUS_ERROR_SUCCESS) && (value != NULL)) {
                printf("Manufacturer  = [%s]\n", rbusValue_GetString(value, NULL));
                snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", rbusValue_GetString(value, NULL));
            }
            else {
                rc = eBTRMgrFailure;
            }
#else
            btrMgr_SysDiag_getDiagInfoFromPipe("grep MFG_NAME /etc/device.properties | cut -d'=' -f2", aData);
#endif
        }
        break;
#ifdef BTR_SYS_DIAG_RBUS_ENABLE
        case BTRMGR_SYS_DIAG_PROVISION_STATUS: {
            syscfg_get(NULL, "unit_activated", buff, sizeof(buff));
            snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", buff);
        }
        break;
        case BTRMGR_SYS_DIAG_SIM_ICCID: {
            retCode = rbus_get(rbusHandle, "Device.Cellular.Interface.1.X_RDK_Identification.Iccid", &value);
            if ((retCode == RBUS_ERROR_SUCCESS) && (value != NULL)) {
                printf("ICCID  = [%s]\n", rbusValue_GetString(value, NULL));
                snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", rbusValue_GetString(value, NULL));
            }
            else {
                rc = eBTRMgrFailure;
            }
        }
        break;
        case BTRMGR_SYS_DIAG_MODEM_IMEI: {
            retCode = rbus_get(rbusHandle, "Device.Cellular.Interface.1.X_RDK_Identification.Imei", &value);
            if ((retCode == RBUS_ERROR_SUCCESS) && (value != NULL)) {
                printf("IMEI number  = [%s]\n", rbusValue_GetString(value, NULL));
                snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", rbusValue_GetString(value, NULL));
            }
            else {
                rc = eBTRMgrFailure;
            }
        }
        break;
        case BTRMGR_SYS_DIAG_CELLULAR_SIGNAL_STRENGTH: {
            retCode = rbus_get(rbusHandle, "Device.Cellular.Interface.1.X_RDK_RadioSignal.Rsrp", &value);
            if ((retCode == RBUS_ERROR_SUCCESS) && (value != NULL)) {
                printf("RSRp  = [%d]\n", rbusValue_GetInt32(value));
                snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%d", rbusValue_GetInt32(value));
            }
            else {
                rc = eBTRMgrFailure;
            }
        }
        break;
        case BTRMGR_SYS_DIAG_MESH_BACKHAUL_TYPE: {
            retCode = rbus_get(rbusHandle, "Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.Mesh.PodEthernetBackhaulEnable", &value);
            if ((retCode == RBUS_ERROR_SUCCESS) && (value != NULL)) {
                printf("Back_haul_type  = [%d]\n", rbusValue_GetBoolean(value));
                mesh_type = rbusValue_GetBoolean(value);
                if(mesh_type) {
                    snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%x", RDK_SETUP_MESH_BACKHAUL_ETH);
                }
                else {
                    snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%x", RDK_SETUP_MESH_BACKHAUL_WIFI);
                }
            }
            else {
                rc = eBTRMgrFailure;
            }
        }
        break;
        case BTRMGR_SYS_DIAG_WIFI_BACKHAUL_STATS: {
             snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", DEVICE_MESH_BAKHAUL_STAT);
        }
        break;
        case BTRMGR_SYS_DIAG_BLE_BROADCAST_STATUS: {
            if (syscfg_get(NULL, "BLEBroadcast", buff, sizeof(buff)) != 0) {
                BTRMGRLOG_INFO("Error getting BLE.Broadcast RFC \n");
                rc = eBTRMgrFailure;
            }
            else {
                snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", buff);
            }
        }
        break;
#endif
        case BTRMGR_SYS_DIAG_DEVICESTATUS: {
        }
        break;
        case BTRMGR_SYS_DIAG_FWDOWNLOADSTATUS: {
            char lValue[BTRMGR_STR_LEN_MAX] = "";
#ifdef BTR_SYS_DIAG_IARM_ENABLE
            IARM_Bus_SYSMgr_GetSystemStates_Param_t param = { 0 };
            MEMSET_S(&param, sizeof(param), 0, sizeof(param));
#endif

            btrMgr_SysDiag_getDiagInfoFromPipe("grep BOX_TYPE /etc/device.properties | cut -d'=' -f2", lValue);
            if (!strcmp(lValue, "pi")) {
                BTRMGRLOG_DEBUG("Box is PI \n");
                snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", "COMPLETED");
            }
            else {
#ifdef BTR_SYS_DIAG_IARM_ENABLE
                lIARMStatus = IARM_Bus_Call(IARM_BUS_SYSMGR_NAME, IARM_BUS_SYSMGR_API_GetSystemStates, (void *)&param, sizeof(param));
                if (IARM_RESULT_SUCCESS != lIARMStatus) {
                    BTRMGRLOG_INFO("Failure : Return code is %d\n", lIARMStatus);
                    rc = eBTRMgrFailure;
                }
                else {
                    BTRMGRLOG_DEBUG("Iarm call fw state :%d\n", param.firmware_download.state);

                    if (IARM_BUS_SYSMGR_IMAGE_FWDNLD_DOWNLOAD_INPROGRESS == param.firmware_download.state) {
                        snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", "IN PROGRESS");
                    }
                    else {
                        snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", "COMPLETED");
                    }
                }
#endif
            }
        }
        break;
        case BTRMGR_SYS_DIAG_WEBPASTATUS: {
            char lValue[BTRMGR_STR_LEN_MAX] = "";
            char* lCmd = "tr181Set -g Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TR069support.Enable 2>&1 1>/dev/null";

            btrMgr_SysDiag_getDiagInfoFromPipe(lCmd, lValue);
            if (0 == strcmp(lValue, "true")) {
                snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", "UP");
            }
            else {
                snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", "DOWN");
            }
            BTRMGRLOG_DEBUG("Webpa status:%s\n", aData);
        }
        break;
        case BTRMGR_SYS_DIAG_WIFIRADIO1STATUS:
        case BTRMGR_SYS_DIAG_WIFIRADIO2STATUS: {
#ifdef BTR_SYS_DIAG_IARM_ENABLE
               DownloadData DwnLoc;
               char post_data[] = "{\"jsonrpc\":\"2.0\",\"id\":\"42\",\"method\": \"org.rdk.NetworkManager.GetAvailableInterfaces\"}";
	       if (MemDLAlloc(&DwnLoc, DEFAULT_DL_ALLOC) == 0) {
		   if (0 != getJsonRpc(post_data, &DwnLoc)) {
		       return eBTRMgrFailure;
	           }
		   else {
		       cJSON *pJson = cJSON_Parse((char *)DwnLoc.pvOut);
		       if (pJson != NULL) {
			   cJSON *pItem = cJSON_GetObjectItem(pJson, "result");
			   if (pItem != NULL) {
			       cJSON *interfaces = cJSON_GetObjectItem(pItem, "interfaces");
                               cJSON *interface=NULL, *interfaceType =NULL;
                               for (int i = 0; i < cJSON_GetArraySize(interfaces); i++) {
                                    interface = cJSON_GetArrayItem(interfaces, i);
                                    interfaceType = cJSON_GetObjectItem(interface, "type");
                                    if (strcmp(interfaceType->valuestring, "WIFI") == 0)
                                        break;
                               }
			       cJSON *WiFi_isEnabled = cJSON_GetObjectItem(interface, "isEnabled");
                               bool Enabled = false;
                               if (WiFi_isEnabled != NULL) {
                                   Enabled = cJSON_IsTrue(WiFi_isEnabled);
                               } 
			       else {
                                   BTRMGRLOG_ERROR("WiFi_isEnabled is NULL\n");
                               }

                               cJSON *WiFi_isConnected = cJSON_GetObjectItem(interface, "isConnected");
                               bool Connected = false;
                               if (WiFi_isConnected != NULL) {
                                   Connected = cJSON_IsTrue(WiFi_isConnected);
                               }
			       else {
                                   BTRMGRLOG_ERROR("WiFi_isConnected is NULL\n");
                               }

                               BTRMGRLOG_INFO("Print Enabled = %d and Connected = %d \n", Enabled, Connected);

                               if (Enabled && Connected) {
                                   BTRMGRLOG_INFO("We can set WIFI status as UP \n");
                                   snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", "UP");
                               }
			       else {
                                   BTRMGRLOG_ERROR("Wifi is not connected  \n");
                                   snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", "DOWN");
                               }
                           }
                       }
                       cJSON_Delete(pJson);
                   }
		   if (DwnLoc.pvOut != NULL) {
                       free(DwnLoc.pvOut);
		   }
               }
#else
            BTRMGRLOG_DEBUG("Wifi diagnostics is not available\n");
            snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", "DOWN");
#endif /* #ifdef BTR_SYS_DIAG_IARM_ENABLE */
        }
        break;
        case BTRMGR_SYS_DIAG_RFSTATUS: {
            char lValue[BTRMGR_STR_LEN_MAX] = "";
#ifdef BTR_SYS_DIAG_IARM_ENABLE
            IARM_Bus_SYSMgr_GetSystemStates_Param_t param = { 0 };
            MEMSET_S(&param, sizeof(param), 0, sizeof(param));
#endif

            btrMgr_SysDiag_getDiagInfoFromPipe("grep GATEWAY_DEVICE /etc/device.properties | cut -d'=' -f2", lValue);
            BTRMGRLOG_DEBUG("Is Gateway device:%s\n", lValue);

            if (!strcmp(lValue, "false")) {
                snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", "NOT CONNECTED");
            }
            else {
#ifdef BTR_SYS_DIAG_IARM_ENABLE
                lIARMStatus = IARM_Bus_Call(IARM_BUS_SYSMGR_NAME, IARM_BUS_SYSMGR_API_GetSystemStates, (void *)&param, sizeof(param));
                if (IARM_RESULT_SUCCESS != lIARMStatus) {
                    BTRMGRLOG_INFO("Failure : Return code is %d\n", lIARMStatus);
                    rc = eBTRMgrFailure;
                }
                else {
                    BTRMGRLOG_DEBUG(" Iarm call fw state :%d\n", param.rf_connected.state);

                    if (0 == param.rf_connected.state) {
                        snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", "NOT CONNECTED");
                    }
                    else {
                        snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", "CONNECTED");
                    }
                }
#endif
            }
        }
        break;
        case BTRMGR_SYS_DIAG_POWERSTATE: {
#ifdef BTR_SYS_DIAG_IARM_ENABLE
            int res = -1;
	    PowerController_PowerState_t curState = POWER_STATE_UNKNOWN, previousState = POWER_STATE_UNKNOWN;
	    res = PowerController_GetPowerState(&curState, &previousState);

            snprintf(aData, (BTRMGR_STR_LEN_MAX - 1), "%s", BTRMGR_SYS_DIAG_PWRST_UNKNOWN);
            if (res == POWER_CONTROLLER_ERROR_NONE) {

                if (param.curState == POWER_STATE_ON)
                    snprintf(aData, BTRMGR_STR_LEN_MAX - 1, "%s", BTRMGR_SYS_DIAG_PWRST_ON);
                else if (param.curState == POWER_STATE_STANDBY)
                    snprintf(aData, BTRMGR_STR_LEN_MAX - 1, "%s", BTRMGR_SYS_DIAG_PWRST_STANDBY);
                else if (param.curState == POWER_STATE_STANDBY_LIGHT_SLEEP)
                    snprintf(aData, BTRMGR_STR_LEN_MAX - 1, "%s", BTRMGR_SYS_DIAG_PWRST_STDBY_LIGHT_SLEEP);
                else if (param.curState == POWER_STATE_STANDBY_DEEP_SLEEP)
                    snprintf(aData, BTRMGR_STR_LEN_MAX - 1, "%s", BTRMGR_SYS_DIAG_PWRST_STDBY_DEEP_SLEEP);
                else if (param.curState == POWER_STATE_OFF)
                    snprintf(aData, BTRMGR_STR_LEN_MAX - 1, "%s", BTRMGR_SYS_DIAG_PWRST_OFF);
                

                pstBtrMgrSdHdl->lstBtrMgrSysDiagStat.enSysDiagChar = BTRMGR_SYS_DIAG_POWERSTATE;
                strncpy(pstBtrMgrSdHdl->lstBtrMgrSysDiagStat.pcSysDiagRes, aData, BTRMGR_STR_LEN_MAX - 1);
                pstBtrMgrSdHdl->_powerState = curState;
		    
                if (curState != POWER_STATE_ON) {
                    BTRMGRLOG_WARN("BTRMGR_SYS_DIAG_POWERSTATE PWRMGR :%d - %s\n", curState, aData);
		    BTRMGRLOG_ERROR("Preethi: PowerInit started\n");
                    PowerController_Init();
		    BTRMGRLOG_ERROR("Preethi: PowerInit ended\n");
		    BTRMGRLOG_ERROR("Preethi: PowerController_RegisterPowerModeChangedCallback started\n");
		    PowerController_RegisterPowerModeChangedCallback(btrMgr_SysDiag_powerModeChangeCb, NULL);
		    BTRMGRLOG_ERROR("Preethi: PowerController_RegisterPowerModeChangedCallback ended\n");
                }
            }
            else {
                BTRMGRLOG_DEBUG("BTRMGR_SYS_DIAG_POWERSTATE Failure : Return code is %d\n", res);
                /* In case of Failure to call GetPowerState registet the event handler anyway */
		    BTRMGRLOG_ERROR("Preethi: PowerInit started in else\n");
                    PowerController_Init();
		    BTRMGRLOG_ERROR("Preethi: PowerInit ended in else\n");
		    BTRMGRLOG_ERROR("Preethi: PowerController_RegisterPowerModeChangedCallback started in else\n");
		    PowerController_RegisterPowerModeChangedCallback(btrMgr_SysDiag_powerModeChangeCb, NULL);
		    BTRMGRLOG_ERROR("Preethi: PowerController_RegisterPowerModeChangedCallback ended in else\n");
		    rc = eBTRMgrFailure;
            }
#else
            rc = eBTRMgrFailure;
#endif
        }
        break;
        default: {
            rc = eBTRMgrFailure;
        }
        break;
    }

    return rc;
}

eBTRMgrRet
BTRMGR_SD_SetData (
    tBTRMgrSDHdl         hBTRMgrSdHdl,
    BTRMGR_SysDiagChar_t aenSysDiagChar,
    unsigned char*       aData
) {
    stBTRMgrSDHdl*  pstBtrMgrSdHdl = (stBTRMgrSDHdl*)hBTRMgrSdHdl;
    eBTRMgrRet      rc = eBTRMgrSuccess;

    if ((NULL == pstBtrMgrSdHdl) || (aData == NULL))
        return eBTRMgrFailure;

    switch (aenSysDiagChar) {
    case BTRMGR_SYS_DIAG_BLE_BROADCAST_STATUS:
#ifdef BTR_SYS_DIAG_RBUS_ENABLE
        syscfg_set_commit(NULL, "BLEBroadcast", *aData ? "true" : "false");
#endif
        break;
    default:
        rc = eBTRMgrFailure;
        break;
    }

    return rc;
}

eBTRMgrRet
BTRMGR_SD_ConnectToWifi (
    tBTRMgrSDHdl    hBTRMgrSdHdl,
    char*           aSSID,
    char*           aPassword,
    int             aSecurityMode
) {
    stBTRMgrSDHdl*  pstBtrMgrSdHdl = (stBTRMgrSDHdl*)hBTRMgrSdHdl;
    eBTRMgrRet      rc = eBTRMgrSuccess;

    if (NULL == pstBtrMgrSdHdl)
        return eBTRMgrFailure;


#ifdef BTR_SYS_DIAG_IARM_ENABLE
    char post_data[512];
    DownloadData DwnLoc;
    snprintf(post_data, sizeof(post_data), "{\"jsonrpc\":\"2.0\",\"id\":\"42\",\"method\": \"org.rdk.NetworkManager.WiFiConnect\", \"params\": {\"ssid\": \"%s\", \"passphrase\": \"%s\", \"securityMode\": %d}}", aSSID, aPassword, aSecurityMode);
    if (MemDLAlloc(&DwnLoc, DEFAULT_DL_ALLOC) == 0) {
        if (0 != getJsonRpc(post_data, &DwnLoc)) {
            BTRMGRLOG_ERROR("Failed to connect to Wi-Fi using JSON-RPC\n");
            return eBTRMgrFailure;
	    } 
	else {
            cJSON *pJson = cJSON_Parse((char *)DwnLoc.pvOut);
            if (pJson != NULL) {
                cJSON *pItem = cJSON_GetObjectItem(pJson, "result");
                if (pItem != NULL && cJSON_IsTrue(cJSON_GetObjectItem(pItem, "success"))) {
                    BTRMGRLOG_DEBUG("\"%s\", status: \"Success\"\n", "org.rdk.NetworkManager.WiFiConnect");
                }
                else {
                    BTRMGRLOG_ERROR("\"%s\", status: \"Failure\"\n", "org.rdk.NetworkManager.WiFiConnect");
                    rc = eBTRMgrFailure;
                }
                cJSON_Delete(pJson);
            }
	    else {
                BTRMGRLOG_ERROR("Failed to parse JSON response\n");
                rc = eBTRMgrFailure;
            }
        }
        if (DwnLoc.pvOut != NULL) {
            free(DwnLoc.pvOut);
        }
    }
    else {
        BTRMGRLOG_ERROR("Failed to allocate memory for download\n");
        rc = eBTRMgrFailure;
    }
#else
    BTRMGRLOG_DEBUG("Wifi not available\n");
#endif /* #ifdef BTR_SYS_DIAG_IARM_ENABLE */

    return rc;
}

#ifdef LE_MODE
eBTRMgrRet
BTRMGR_SD_Check_Cellularmanager_ISOnline (
    char *lte_wan_status,
    bool *lte_enable
) {
    BTRMGRLOG_INFO ("Entering ....\n");
    eBTRMgrRet  rc = eBTRMgrSuccess;

#ifdef BTR_SYS_DIAG_RBUS_ENABLE
    rbusError_t retCode = RBUS_ERROR_BUS_ERROR;
    rbusValue_t value = NULL;

    BTRMGRLOG_INFO ("Getting X_RDK_status and X_RDK_enabled values ....\n");
    retCode = rbus_get(rbusHandle, "Device.Cellular.X_RDK_Status", &value);
    if ((retCode == RBUS_ERROR_SUCCESS) && (value != NULL)) {
        BTRMGRLOG_INFO ("Print X_RDK_status = %s ....\n",rbusValue_GetString(value, NULL));
        snprintf(lte_wan_status, (BTRMGR_STR_LEN_MAX - 1), "%s", rbusValue_GetString(value, NULL));
        rbusValue_Release(value);
    }

    retCode = rbus_get(rbusHandle, "Device.Cellular.X_RDK_Enable", &value);
    if ((retCode == RBUS_ERROR_SUCCESS) && (value != NULL)) {
        BTRMGRLOG_INFO ("Print X_RDK_Enable = %d ....\n", rbusValue_GetBoolean(value));
        *lte_enable = rbusValue_GetBoolean(value);
        rbusValue_Release(value);
    }
#endif

    BTRMGRLOG_INFO ("After setting X_RDK_status and X_RDK_enabled values through RBUS....\n");
    if (strlen(lte_wan_status) && *lte_enable == 1) {
        rc = eBTRMgrSuccess;
    }
    else {
        rc = eBTRMgrFailure;
    }
    BTRMGRLOG_INFO ("Exit .....\n");

    return rc;
}
#endif

/*  Incoming Callbacks */
#ifdef BTR_SYS_DIAG_IARM_ENABLE
STATIC void
btrMgr_SysDiag_powerModeChangeCb (
    const PowerController_PowerState_t currentState,
    const PowerController_PowerState_t newState,
    void *userData
) {
            IARM_Bus_PWRMgr_EventData_t *param = (IARM_Bus_PWRMgr_EventData_t *)data;
            BTRMGRLOG_WARN("BTRMGR_SYS_DIAG_POWERSTATE Event IARM_BUS_PWRMGR_EVENT_MODECHANGED: new State: %d\n", newState);

            if (gpstSDHandle != NULL) {

                if (newState == IARM_BUS_PWRMGR_POWERSTATE_ON)
                    snprintf(gpstSDHandle->lstBtrMgrSysDiagStat.pcSysDiagRes, BTRMGR_STR_LEN_MAX - 1, "%s", BTRMGR_SYS_DIAG_PWRST_ON);
                else if (newState == IARM_BUS_PWRMGR_POWERSTATE_STANDBY)
                    snprintf(gpstSDHandle->lstBtrMgrSysDiagStat.pcSysDiagRes, BTRMGR_STR_LEN_MAX - 1, "%s", BTRMGR_SYS_DIAG_PWRST_STANDBY);
                else if (newState == IARM_BUS_PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP)
                    snprintf(gpstSDHandle->lstBtrMgrSysDiagStat.pcSysDiagRes, BTRMGR_STR_LEN_MAX - 1, "%s", BTRMGR_SYS_DIAG_PWRST_STDBY_LIGHT_SLEEP);
                else if (newState == IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP)
                    snprintf(gpstSDHandle->lstBtrMgrSysDiagStat.pcSysDiagRes, BTRMGR_STR_LEN_MAX - 1, "%s", BTRMGR_SYS_DIAG_PWRST_STDBY_DEEP_SLEEP);
                else if (newState == IARM_BUS_PWRMGR_POWERSTATE_OFF)
                    snprintf(gpstSDHandle->lstBtrMgrSysDiagStat.pcSysDiagRes, BTRMGR_STR_LEN_MAX - 1, "%s", BTRMGR_SYS_DIAG_PWRST_OFF);
                else
                    snprintf(gpstSDHandle->lstBtrMgrSysDiagStat.pcSysDiagRes, BTRMGR_STR_LEN_MAX - 1, "%s", BTRMGR_SYS_DIAG_PWRST_UNKNOWN);

                gpstSDHandle->lstBtrMgrSysDiagStat.enSysDiagChar = BTRMGR_SYS_DIAG_POWERSTATE;


                if (gpstSDHandle->_powerState != newState && (newState != POWER_STATE_ON && newState != POWER_STATE_STANDBY_LIGHT_SLEEP)) {
                    BTRMGRLOG_WARN("BTRMGR_SYS_DIAG_POWERSTATE - Device is being suspended\n");
                }

                if (gpstSDHandle->_powerState != param->data.state.newState && param->data.state.newState == POWER_STATE_ON) {
                    BTRMGRLOG_WARN("BTRMGR_SYS_DIAG_POWERSTATE - Device just woke up\n");
                    if (gpstSDHandle->fpcBSdStatus) {
                        stBTRMgrSysDiagStatus   lstBtrMgrSysDiagStat;
                        eBTRMgrRet              leBtrMgrSdRet = eBTRMgrSuccess;

                        MEMCPY_S(&lstBtrMgrSysDiagStat, sizeof(stBTRMgrSysDiagStatus), &gpstSDHandle->lstBtrMgrSysDiagStat, sizeof(stBTRMgrSysDiagStatus));
                        if ((leBtrMgrSdRet = gpstSDHandle->fpcBSdStatus(&lstBtrMgrSysDiagStat, gpstSDHandle->pvcBUserData)) != eBTRMgrSuccess) {
                            BTRMGRLOG_ERROR("BTRMGR_SYS_DIAG_POWERSTATE - Device woke up - NOT PROCESSED\n");
                        }
                    }
                }

                gpstSDHandle->_powerState = newState;
            }
}
#endif /* #ifdef BTR_SYS_DIAG_IARM_ENABLE */
