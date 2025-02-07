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
 * @file btrMgr_LeOnboarding.c
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

#ifdef BTRTEST_LE_ONBRDG_ENABLE
#include "libIBus.h"
#include "libIARM.h"

#include "sysMgr.h"
#endif

#include "safec_lib.h"

#include "cJSON.h"
//#include "sysUtils.h"


#include "ecdh.h"
#include "btrMgr_Types.h"
#include "btrMgr_logger.h"
#include "btrMgr_LEOnboarding.h"

#ifdef LE_MODE
#include "btrMgr_platform_spec.h"
#endif

#ifdef BTRTEST_LE_ONBRDG_ENABLE
#include <curl/curl.h>
#include "cJSON.h"
#include "urlHelper.h"
#include "deviceutils.h"
#include "json_parse.h"
#endif
#define get_value_string(obj,str,size) strncpy(str, obj->valuestring, size);
      

#define get_value_number(obj, num) num = obj->valueint;

#define kPrivateKeyPath "/tmp/bootstrap_private.pem"
#define kPublicKeyPath "/tmp/bootstrap_public.pem"

#ifndef LE_MODE
#define BTRMGR_LE_MODELNUMBER        ""
#define BTRMGR_LE_SWREVISION         ""
#define BTRMGR_LE_UUID_QR_CODE       ""
#define BTRMGR_LE_MFGRNAME           ""
#endif

/* router security mode */
enum SECURITY_MODE
{
  SECURITY_MODE_NONE = 1,    /* none */
  SECURITY_MODE_WPA2_PSK,    /* WPA2-PSK (AES) */
  SECURITY_MODE_RESERVED1,
  SECURITY_MODE_WPA_WPA2_PSK  /* WPAWPA2 PSK (TKIP/AES) */
};


int gLeOnboardingState = BTRMGR_LE_ONBRDG_UNDEFINED;
short int gUuidProvisionStatus = 0;
char gWifiPayload[MAX_PAYLOAD_LEN];
int gWifiPayloadLen = 0;
int gDataLenRxd = 0;
bool gWifiPayloadDecodeSuccess = false;
bool gWifiConnectSuccess = false;
bool gWifiPayloadRxd = false;
typedef struct wifi_credentials wifi_creds_t;
wifi_creds_t WifiCreds;

eBTRMgrRet BTRMGR_LeWifi_CheckWifiConnSuccess(char* aSSID);
static void BTRMGR_LeLoadDatatoBuffer(char *aData);
static int get_wifi_creds(cJSON* wifi_settings, wifi_creds_t *creds, int index, int *ptotalEntries);
void BTRMGR_LeDecodeRxdWifiPayload(char * agWifiPayload);
eBTRMgrRet BTRMGR_LeWifi_ConnectToWifi(char* aSSID, char* aPassword, int aSecurityMode);

//Function to get publickey
static void
get_publicKey (
    char* key
) {
    FILE *file = NULL;
    int nread;
    char header[] = "-----BEGIN PUBLIC KEY-----";
    char footer[] = "-----END PUBLIC KEY-----";
    size_t len = 0;
    char *line = NULL;

    file = fopen(kPublicKeyPath, "r");
    if (file) {
        while ((nread = getline(&line, &len, file)) != -1) {
            if ((strncmp(line, header, nread - 1) != 0) && (strncmp(line, footer, nread - 1) != 0)) {
                strncpy(&key[strlen(key)], line, nread - 1);
            }
        }

        free(line);
        fclose(file);
    }
}

static void
BTRMGR_LeLoadDatatoBuffer (
    char*   aData
) {
    if (gWifiPayloadLen == 0) {
        sscanf(aData, "%4x", &gWifiPayloadLen);
        BTRMGRLOG_DEBUG("Data length is %d", gWifiPayloadLen);
        aData += 4;
    }
    /* TODO: CID 135218: Out-of-bounds access (OVERRUN)*/
    if (gDataLenRxd + strlen(aData) < MAX_PAYLOAD_LEN) {
        gDataLenRxd += snprintf(&gWifiPayload[gDataLenRxd], MAX_PAYLOAD_LEN - gDataLenRxd, "%s", aData);
    }
    else {
        BTRMGRLOG_WARN("The string is too long");
    }

    BTRMGRLOG_DEBUG("\nLength received is %d\n", gDataLenRxd);

    if (gDataLenRxd == gWifiPayloadLen) {
        BTRMGRLOG_DEBUG("Data is %s", gWifiPayload);
    }
}


//Function to parse cjson wifi credentials string
static int
get_wifi_creds (
    cJSON*          wifi_settings,
    wifi_creds_t*   creds,
    int             index,
    int*            ptotalEntries
) {
    int noofEntries = 0;
    int ret = -1;
    //PRVMGR_ASSERT_NOT_NULL(wifi_settings);

    noofEntries = cJSON_GetArraySize(wifi_settings);
    *ptotalEntries = noofEntries;
    BTRMGRLOG_DEBUG("noofEntries in ssid json %d and current index %d", *ptotalEntries, index);

    if (noofEntries >= index) {
        cJSON* cur_ssid = cJSON_GetArrayItem(wifi_settings, (index - 1));
        //PRVMGR_ASSERT_NOT_NULL(cur_ssid);

        get_value_string(cJSON_GetObjectItem(cur_ssid, "ssid"), creds->ssid, SSID_MAX_LEN);
        BTRMGRLOG_INFO("creds->ssid (%s)", creds->ssid);

        get_value_string(cJSON_GetObjectItem(cur_ssid, "password"), creds->passphrase, PASS_PHRASE_LEN);
        BTRMGRLOG_DEBUG("creds->passphrase (%s)", creds->passphrase);

        if (!cJSON_GetObjectItem(cur_ssid, "frequency")) {
            BTRMGRLOG_WARN("json doesn't have frequency... using default");
        }
        else {
            get_value_string(cJSON_GetObjectItem(cur_ssid, "frequency"), creds->frequency, MAX_FREQ_STR_LEN);
        }
        BTRMGRLOG_INFO("creds->frequency (%s)", creds->frequency);

        if (!cJSON_GetObjectItem(cur_ssid, "securitymode")) {
            BTRMGRLOG_WARN("json doesn't have securitymode... using default");
            creds->securitymode = SECURITY_MODE_WPA_WPA2_PSK;
        }
        else {
            get_value_number(cJSON_GetObjectItem(cur_ssid, "securitymode"), creds->securitymode);
        }
        BTRMGRLOG_INFO("creds->securitymode (%d)", creds->securitymode);
        ret = 0;
    }
    else {
        BTRMGRLOG_ERROR("Error index %d greater than available entries %d", index, noofEntries);
        ret = -1;
    }

    return ret;
}

void
BTRMGR_LeDecodeRxdWifiPayload (
    char* agWifiPayload
) {
    cJSON*  psrv_payload = NULL;
    cJSON*  wifi_settings = NULL;
    int     noofEntries;
    int     entry = 1;
    
    psrv_payload = cJSON_Parse((char const*)agWifiPayload);
    if (!psrv_payload) {
        BTRMGRLOG_INFO("wifi_payload parse error!!");
    }
    else {
        //decrypt wifi settings
        
        BTRMGRLOG_DEBUG("decrypt wifi settings");
        int sts = ECDH_DecryptWiFiSettings(psrv_payload, &wifi_settings);
        if (sts != 1) {
            BTRMGRLOG_ERROR("Unable to decrypt wifi settings");
        }
        else {
            MEMSET_S(&WifiCreds, sizeof(WifiCreds), 0, sizeof(WifiCreds));
            int ret = get_wifi_creds(wifi_settings, &WifiCreds, entry, &noofEntries);
            if (ret == 0) {
                BTRMGRLOG_DEBUG("wifi creds for %s radio: ssid (%s) password (%s)",
                    WifiCreds.frequency, WifiCreds.ssid, WifiCreds.passphrase);
                gWifiPayloadDecodeSuccess = true;
            }
        }
    }
}

/* Interfaces */
eBTRMgrRet
BTRMGR_LeOnboarding_GetData (
    BTRMGR_LeOnboardingChar_t aenLeOnboardingChar,
    char* aData
) {
    eBTRMgrRet      rc = eBTRMgrSuccess;
    //IARM_Result_t lIARMStatus = IARM_RESULT_SUCCESS;

    switch (aenLeOnboardingChar) {
    case BTRMGR_LE_ONBRDG_SYSTEMID: {
        snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "%s", "3C9872F8DA9F");
    }
        break;
    case BTRMGR_LE_ONBRDG_HWREVISION:
    case BTRMGR_LE_ONBRDG_MODELNUMBER: {
        snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "%s", BTRMGR_LE_MODELNUMBER);
    }
        break; 
    case BTRMGR_LE_ONBRDG_SERIALNUMBER: {
        snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "%s", "S1807CKZ000113");
    }
        break;
    case BTRMGR_LE_ONBRDG_FWREVISION: {
        snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "%s", "V.0.0.1");
    }
        break;
    case BTRMGR_LE_ONBRDG_SWREVISION: {
        snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "%s", BTRMGR_LE_SWREVISION);
    }
        break;
    case BTRMGR_LE_ONBRDG_MFGRNAME: {
        snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "%s", BTRMGR_LE_MFGRNAME);
    }
        break;
    case BTRMGR_LE_ONBRDG_UUID_QR_CODE: {
        snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "%s", BTRMGR_LE_UUID_QR_CODE);
    }
        break;
    case BTRMGR_LE_ONBRDG_UUID_PROVISION_STATUS: {
        switch (gLeOnboardingState) {
        case BTRMGR_LE_ONBRDG_UNDEFINED: {
            gWifiPayloadDecodeSuccess = false;
            gLeOnboardingState = BTRMGR_LE_ONBRDG_ADVERTISE;
        }
            break;
        case BTRMGR_LE_ONBRDG_ADVERTISE: {
            gLeOnboardingState = BTRMGR_LE_ONBRDG_BT_PAIRING;
            snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "0x%x", 0);
        }
            break;
        case BTRMGR_LE_ONBRDG_BT_PAIRING: {
            gLeOnboardingState = BTRMGR_LE_ONBRDG_INPROGRESS;
            snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "0x%x", BTRMGR_LE_ONBRDG_AWAITING_WIFI_CONFIG);
        }
            break;
        case BTRMGR_LE_ONBRDG_INPROGRESS: {
            gLeOnboardingState = BTRMGR_LE_ONBRDG_GET_WIFI_CREDS;
            snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "0x%x", BTRMGR_LE_ONBRDG_AWAITING_WIFI_CONFIG);
        }
            break;
        case BTRMGR_LE_ONBRDG_GET_WIFI_CREDS: {
            if(true == gWifiPayloadRxd) {
                gLeOnboardingState = BTRMGR_LE_ONBRDG_CONNECT_WIFI;
                snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "0x%x", BTRMGR_LE_ONBRDG_PROCESSING_WIFI_CONFIG);
            }
            else {
                snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "0x%x", BTRMGR_LE_ONBRDG_AWAITING_WIFI_CONFIG);
            }
        }
            break;
        case BTRMGR_LE_ONBRDG_CONNECT_WIFI: {
            if ((true == gWifiConnectSuccess) && (eBTRMgrSuccess == BTRMGR_LeWifi_CheckWifiConnSuccess(WifiCreds.ssid))) {
                BTRMGRLOG_INFO("Wifi is connected\n");
                snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "0x%x", BTRMGR_LE_ONBRDG_WIFI_CONNECT_SUCCESS);
                gLeOnboardingState = BTRMGR_LE_ONBRDG_COMPLETE;
            }
            else if (true == gWifiPayloadDecodeSuccess) {
                snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "0x%x", BTRMGR_LE_ONBRDG_CONNECTING_TO_WIFI);
                WifiCreds.securitymode = 6;
                if (eBTRMgrSuccess == BTRMGR_LeWifi_ConnectToWifi(WifiCreds.ssid, WifiCreds.passphrase, WifiCreds.securitymode)) {
                    gWifiConnectSuccess = true;
                }
            }
        }
            break;
        case BTRMGR_LE_ONBRDG_COMPLETE: {
            snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "0x%x", BTRMGR_LE_ONBRDG_COMPLETE_SUCCESS);
        }
            break;
        default:
            break;
        }

        BTRMGRLOG_INFO("Onboarding status is %d\n", gLeOnboardingState);
    }
        break;
    case BTRMGR_LE_ONBRDG_UUID_PUBLIC_KEY: {
        char lPublicKey[MAX_LEN_PUBLIC_KEY] = "";
        get_publicKey(lPublicKey);
        int ret =snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "%s", lPublicKey);
        if (ret > (BTRMGR_LE_STR_LEN_MAX - 1)) {
            BTRMGRLOG_DEBUG("BTRMGR_LE_ONBRDG_UUID_PUBLIC_KEY truncated\n");
        }

    }
        break;
    case BTRMGR_LE_ONBRDG_UUID_WIFI_CONFIG: {
    }
        break;
    case BTRMGR_LE_ONBRDG_UUID_SSID_LIST: {
        snprintf(aData, (BTRMGR_LE_STR_LEN_MAX - 1), "%s", " ");
    }
        break;
    default:
        rc = eBTRMgrFailure;
        break;
    }

    return rc;
}

eBTRMgrRet
BTRMGR_LeOnboarding_SetData (
    BTRMGR_LeOnboardingChar_t   aenLeOnboardingChar,
    char*                       payload
) {
    eBTRMgrRet      rc = eBTRMgrSuccess;

    switch(aenLeOnboardingChar) {
    case BTRMGR_LE_ONBRDG_UUID_WIFI_CONFIG: {
        char tempBuffer[BTRMGR_LE_STR_LEN_MAX] = "\0";
        int datatxd = 0;
        char *ptrPayload = payload;
        int index = 0;
        BTRMGRLOG_DEBUG("Length of payload is %u\n", (unsigned int)strlen(payload));
        BTRMGRLOG_INFO("Payload is %s\n", payload);
        for (index = 0; index < strlen(payload);) {
            datatxd = snprintf(tempBuffer, BTRMGR_LE_STR_LEN_MAX, "%s", ptrPayload);
            BTRMGRLOG_DEBUG("data txd is %d\n", datatxd);
            if (datatxd > 0) {
                if (datatxd > BTRMGR_LE_STR_LEN_MAX) {
                    tempBuffer[BTRMGR_LE_STR_LEN_MAX-1] = '\0';
                    BTRMGRLOG_DEBUG("Tx buffer is %s\n", tempBuffer);
                    BTRMGR_LeLoadDatatoBuffer(tempBuffer);
                    index += strlen(tempBuffer);
                    ptrPayload = &payload[index];
                }
                else {
                    BTRMGR_LeLoadDatatoBuffer(tempBuffer);
                    index += datatxd;
                }
                BTRMGRLOG_DEBUG("index is %d\n", index);
            }
        }

        if (gDataLenRxd == gWifiPayloadLen) {
            gWifiPayloadRxd = true;
            BTRMGRLOG_DEBUG("Data is %s", gWifiPayload);
            gWifiPayloadLen = 0;
            /* Decode received data */
            BTRMGR_LeDecodeRxdWifiPayload(gWifiPayload);
        }
    }
        break;
    default:
        break;
    }

    return rc;
}

eBTRMgrRet
BTRMGR_LeWifi_CheckWifiConnSuccess (
    char* aSSID
) {
    eBTRMgrRet lRetCode = eBTRMgrFailure;

#ifdef BTRTEST_LE_ONBRDG_ENABLE
    DownloadData DwnLoc;
    char post_data[] = "{\"jsonrpc\":\"2.0\",\"id\":\"42\",\"method\": \"org.rdk.NetworkManager.GetConnectedSSID\"}";
    if (MemDLAlloc(&DwnLoc, DEFAULT_DL_ALLOC) == 0) {
        if (0 != getJsonRpc(post_data, &DwnLoc)) {
            BTRMGRLOG_ERROR("Failed to get connected SSID using JSON-RPC\n");
            return eBTRMgrFailure;
        } 
        else {
            cJSON *pJson = cJSON_Parse((char *)DwnLoc.pvOut);
            if (pJson != NULL) {
                cJSON *pItem = cJSON_GetObjectItem(pJson, "result");
                if (pItem != NULL) {
                    cJSON *connectedSSID = cJSON_GetObjectItem(pItem, "ssid");
                    if (connectedSSID != NULL && connectedSSID->valuestring != NULL) {
                        BTRMGRLOG_DEBUG("Retrieved SSID: %s\n", connectedSSID->valuestring);
                        if ( strcmp(connectedSSID->valuestring, aSSID) == 0) {
                            BTRMGRLOG_DEBUG("Wifi has been successfully connected\n");
                            lRetCode = eBTRMgrSuccess;
                        }
                        else {
                            BTRMGRLOG_ERROR("SSID does not match the expected value\n");
                        }
                    }
                    else {
                        BTRMGRLOG_ERROR("SSID is NULL or SSID value string is NULL\n");
                    }  
                }
                cJSON_Delete(pJson);
            }
        }
        if (DwnLoc.pvOut != NULL) {
            free(DwnLoc.pvOut);
        }
    }
#else
    BTRMGRLOG_DEBUG("Wifi not available\n");
#endif /* #ifdef BTRTEST_LE_ONBRDG_ENABLE */

    return lRetCode;
}

eBTRMgrRet
BTRMGR_LeWifi_ConnectToWifi (
    char* aSSID,
    char* aPassword,
    int     aSecurityMode
) {
    eBTRMgrRet lRetCode = eBTRMgrSuccess;

#ifdef BTRTEST_LE_ONBRDG_ENABLE
    char post_data[512];
    DownloadData DwnLoc;
    snprintf(post_data, sizeof(post_data), "{\"jsonrpc\":\"2.0\",\"id\":\"42\",\"method\": \"org.rdk.NetworkManager.WiFiConnect\", \"params\": {\"ssid\": \"%s\", \"passphrase\": \"%s\", \"securityMode\": %d}}", aSSID, aPassword, aSecurityMode);

    BTRMGRLOG_DEBUG("Wifi ssid is : %s\n", aSSID);
    BTRMGRLOG_DEBUG("Wifi pwd is : %s\n", aPassword);
    BTRMGRLOG_DEBUG("Wifi sec mode is : %d\n", aSecurityMode);

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
                    lRetCode = eBTRMgrFailure;
                }
                cJSON_Delete(pJson);
            }
        }
        if (DwnLoc.pvOut != NULL) {
            free(DwnLoc.pvOut);
        }
    }
#else
    BTRMGRLOG_INFO("Wifi not available\n");
#endif /* #ifdef BTRTEST_LE_ONBRDG_ENABLE */

    return lRetCode;
}

