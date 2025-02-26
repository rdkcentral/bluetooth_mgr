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

#ifdef IARM_RPC_ENABLED
        BTRMgr_TermIARMMode();
#else
        BTRMgr_TermRBUSMode();
#endif

        time(&curr);
        printf (" BTMgr Bus: BTRMgr_TermIARMMode %s\n", ctime(&curr));
        fflush(stdout);

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

#if defined(ENABLE_SD_NOTIFY)
    sd_notifyf(0, "STOPPING=1\n"
                  "STATUS=BTRMgr Successfully DeInitialized\n"
                  "MAINPID=%lu", (unsigned long) getpid());
#endif

    return lenBtrMgrResult;
}
