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
 * @file btrMgr_main.c
 *
 * @description This file defines bluetooth manager's Controller functionality
 *
 */
#include <stdio.h>
#include <stdbool.h>

#include <signal.h>
#include <string.h>

#include <time.h>
#include <unistd.h>

#if defined(ENABLE_SD_NOTIFY)
#include <systemd/sd-daemon.h>
#endif

#include "btmgr.h"

#ifdef IARM_RPC_ENABLED
#include "btrMgr_IarmInternalIfce.h"
#else
#include "btmgr_rbus_interface.h"
#endif

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif

static bool gbExitBTRMgr = false;

#define BT_MAX_HCICONFIG_OUTPUT_SIZE    1024
#define BT_HCI0_TIMEOUT 30
static void
btrMgr_SignalHandler (
    int i32SignalNumber
) {
    time_t curr = 0;

    time(&curr);
    printf ("Received SIGNAL %d = %s - %s\n", i32SignalNumber, strsignal(i32SignalNumber), ctime(&curr));
    fflush(stdout);

    if (i32SignalNumber == SIGTERM)
        gbExitBTRMgr = true;
}

static unsigned char btrMgr_systemReady() {

    char output[BT_MAX_HCICONFIG_OUTPUT_SIZE] = {0};
    FILE* fp;
    int output_length = 0;

    // Execute hciconfig -a command and capture its output
    fp = popen("hciconfig hci0", "r");
    if (fp == NULL) {
            printf("Failed to execute hciconfig\n");
            return -1;
    }

    output_length = fread(output, sizeof(char), BT_MAX_HCICONFIG_OUTPUT_SIZE - 1, fp);
    output[output_length] = '\0';
    pclose(fp);

    if (strstr(output, "UP RUNNING"))
    {
        printf("Hci0 is up\n");
        return 1;
    }
    else 
    {
        printf("Hci0 is not yet up\n");
        return 0;
    }
    return 0;
}

int
main (
    void
) {
    time_t curr = 0;
    BTRMGR_Result_t lenBtrMgrResult     = BTRMGR_RESULT_SUCCESS;
#ifdef LE_MODE
    BTRMGR_Result_t lenBtrMgrLeResult   = BTRMGR_RESULT_SUCCESS;
#else
    BTRMGR_Result_t lenBtrMgrSoResult   = BTRMGR_RESULT_SUCCESS;
#endif

    //verify that the hci interface is up or fail after 30 seconds
    unsigned char timeout = 0;
    while (!btrMgr_systemReady() && timeout < BT_HCI0_TIMEOUT)
    {
        timeout++;
        sleep(1);
    }
    if (timeout == BT_HCI0_TIMEOUT)
    {
        printf("HCI is not up, btmgr will most likely fail to start\n");
    }
    
    if ((lenBtrMgrResult = BTRMGR_Init()) == BTRMGR_RESULT_SUCCESS) {

        signal(SIGTERM, btrMgr_SignalHandler);

#if defined(ENABLE_SD_NOTIFY)
        sd_notify(0, "READY=1");
#endif

        time(&curr);

#ifdef IARM_RPC_ENABLED
        printf ("I-ARM BTMgr Bus: BTRMgr_BeginIARMMode %s\n", ctime(&curr));
        BTRMgr_BeginIARMMode();
#else
        printf ("BTMgr Bus: BTRMgr_BeginRBUSMode %s\n", ctime(&curr));
        BTRMgr_BeginRBUSMode();
#endif
        fflush(stdout);

#ifdef INCLUDE_BREAKPAD
        breakpad_ExceptionHandler();
#endif

#ifdef LE_MODE
        lenBtrMgrLeResult = BTRMGR_LEDeviceActivation();
        printf ("BTRMGR_StartLEDeviceActivation - %d\n", lenBtrMgrLeResult);
        fflush(stdout);
#else
        lenBtrMgrSoResult = BTRMGR_StartAudioStreamingOut_StartUp(0, BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT);
        printf ("BTRMGR_StartAudioStreamingOut_StartUp - %d\n", lenBtrMgrSoResult);
        fflush(stdout);

        lenBtrMgrSoResult = BTRMGR_ConnectGamepads_StartUp(0, BTRMGR_DEVICE_OP_TYPE_HID);
        printf ("BTRMGR_ConnectGamepads_StartUp - %d\n", lenBtrMgrSoResult);
        fflush(stdout);
#endif

#if defined(ENABLE_SD_NOTIFY)
        sd_notifyf(0, "READY=1\n"
                      "STATUS=BTRMgr Successfully Initialized  - Processing requests…\n"
                      "MAINPID=%lu", (unsigned long) getpid());
#endif

        while (gbExitBTRMgr == false) {
            time(&curr);
            //printf ("BTMgr Bus: HeartBeat at %s\n", ctime(&curr));
            fflush(stdout);
            sleep(10);
        }

#if defined(ENABLE_SD_NOTIFY)
        sd_notify(0, "STOPPING=1");
#endif

    }


    if (lenBtrMgrResult != BTRMGR_RESULT_SUCCESS) {
#if defined(ENABLE_SD_NOTIFY)
        sd_notifyf(0, SD_EMERG "STATUS=BTRMgr Init Failed %d\n", lenBtrMgrResult);
#endif
        printf ("BTRMGR_Init Failed\n");
        fflush(stdout);
    }


    lenBtrMgrResult = BTRMGR_DeInit();

    time(&curr);
    printf ("BTRMGR_DeInit %d - %s\n", lenBtrMgrResult, ctime(&curr));
    fflush(stdout);

    /* Since the IARM events will be generated during the deinit
     * always terminate the IARM interface after Deinit.
     */

#ifdef IARM_RPC_ENABLED
        BTRMgr_TermIARMMode();
#else
        BTRMgr_TermRBUSMode();
#endif
        time(&curr);
        printf (" BTMgr Bus: BTRMgr_TermIARMMode %s\n", ctime(&curr));
        fflush(stdout);

#if defined(ENABLE_SD_NOTIFY)
    sd_notifyf(0, "STOPPING=1\n"
                  "STATUS=BTRMgr Successfully DeInitialized\n"
                  "MAINPID=%lu", (unsigned long) getpid());
#endif

    return lenBtrMgrResult;
}
