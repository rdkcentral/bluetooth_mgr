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
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include <inttypes.h>
#include <linux/input.h>
#include <fcntl.h>
#include <errno.h>
#define MAX_STRING_LENGTH 128
#define MAX_EVENTS 16
#define EVID_LENGTH 4
#define DEVICE_INPUT_FOLDER "/dev/input/"
#define BTRMGR_DEBUG_DIRECTORY "/tmp/btrMgr_DebugArtifacts/"
#define CHECK_IF_GOBJECT_NULL(obj, msg) if (!obj) { \
                                     printf(msg); \
                                     if (pGerr) \
                                     { \
                                        g_clear_error (&pGerr); \
                                        pGerr = NULL; \
                                     }\
                                     goto error; \
                                    }

void monitor_device_thread(char * fpath)
{
    
    struct input_event    events[MAX_EVENTS];
    char                  *pcEvDeviceName, *pcDebugWriteFilePath;
    FILE *                fpDebugPrintFile = NULL;
    unsigned short        pui16DeviceID[EVID_LENGTH];
    uint8_t               i;
    int                   event_count, ret;
    struct tm *           stTime;
    time_t                currTime;
    int                   ev_sock;
    if (!fpath)
    {
        printf("Not a valid gfile object\n");
        return;
    }
    ev_sock = open(fpath, O_RDONLY);
    g_free(fpath);
    //get name and determine if it is bluetooth
    pcEvDeviceName = malloc(MAX_STRING_LENGTH);
    if (!pcEvDeviceName)
    {
        printf("Out of memory");
        close(ev_sock);
        return;
    }
    ret = ioctl(ev_sock, EVIOCGNAME(MAX_STRING_LENGTH - 1), pcEvDeviceName);
    if (ret < 0)
    {
        printf("Could not get device name, %d\n", ret);
        close(ev_sock);
        return;
    }
    printf("Name: %s\n", pcEvDeviceName);
    ret = ioctl(ev_sock, EVIOCGID,pui16DeviceID);
    if (ret < 0)
    {
        printf("Could not get device ID, %d\n", ret);
        close(ev_sock);
        return;
    }
    printf("BUS ID: 0x%x\n", pui16DeviceID[ID_BUS]);
    if (pui16DeviceID[ID_BUS] != BUS_BLUETOOTH)
    {
        printf("Not a bluetooth device\n");
        close(ev_sock);
        return;
    }
    time(&currTime);
    stTime = gmtime(&currTime);
    pcDebugWriteFilePath =malloc(MAX_STRING_LENGTH);
    if (!pcDebugWriteFilePath)
    {
        printf("Out of memory");
        close(ev_sock);
        return;
    }
    snprintf(pcDebugWriteFilePath, MAX_STRING_LENGTH, "%s/%s-%s-%02d:%02d:%02d.txt", BTRMGR_DEBUG_DIRECTORY, "hidevts", pcEvDeviceName, stTime->tm_hour, stTime->tm_min, stTime->tm_sec);
    //replace spaces in device name
    for (unsigned short j = 0; j < strlen(pcDebugWriteFilePath); j++)
    {
        if (pcDebugWriteFilePath[j] == ' ')
        {
            pcDebugWriteFilePath[j] = '_';
        }
    }
    printf("Opening %s\n", pcDebugWriteFilePath);
    fpDebugPrintFile = fopen(pcDebugWriteFilePath, "w");
    free(pcDebugWriteFilePath);
    free(pcEvDeviceName);
    if (!fpDebugPrintFile)
    {
        printf("Error %d \n", errno);
        printf("Could not open file\n");
        return;
    }
    
    while (1)
    {
        event_count = read(ev_sock, events, sizeof(events));
        if (event_count == -1)
            break;
        if (event_count < sizeof(struct input_event))
        {
            printf("Too small payload\n");
            continue;
        }
        for ( i = 0; i < event_count/sizeof(struct input_event); i++)
        {
            if (!events[i].type && !events[i].code)
                continue;
            fprintf(fpDebugPrintFile, "Time: %ld.%06ld - type: %d code: %d value: %d\n", events[i].time.tv_sec, events[i].time.tv_usec, events[i].type, events[i].code,events[i].value);
        }
    }
    printf("Device disappeared exiting thread\n");
    close(ev_sock);
    fclose(fpDebugPrintFile);
}
void devices_changed_cb(GFileMonitor *monitor, GFile *pstGFileDev, GFile *secondaryFile, GFileMonitorEvent evtype, gpointer data)
{
    char *pstGFileDevPath = g_file_get_path(pstGFileDev);
    if (strstr(pstGFileDevPath, "event")) {
        if (evtype == G_FILE_MONITOR_EVENT_CREATED) {
            printf("Starting a thread to read %s\n", pstGFileDevPath);
            g_thread_new("gamepad_event_mon", (GThreadFunc) monitor_device_thread, pstGFileDevPath);
            return;
        }
    }
    g_free(pstGFileDevPath);
}

int main()
{
    GError *pGerr = NULL;
    char * pcThreadFilePath = NULL;
    const char * pcFileName = NULL;
    unsigned short ui16MaxFpLength;
    GFileEnumerator* pstGFileEnum = NULL;
    GFile *pstGFile = NULL;
    GFileMonitor *pstGFileMonObj = NULL;
    GFileInfo * pstGTmpFileInfo = NULL;
    GMainLoop *pstGMainLoop = g_main_loop_new(NULL, FALSE);
    CHECK_IF_GOBJECT_NULL(pstGMainLoop, "Failed to create Main loop\n");
    pstGFile = g_file_new_for_path(DEVICE_INPUT_FOLDER);
    CHECK_IF_GOBJECT_NULL(pstGFile, "Failed to create GFile\n");
    
    //Check current code
    pstGFileEnum = g_file_enumerate_children (pstGFile,G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, NULL, &pGerr);
    CHECK_IF_GOBJECT_NULL(pstGFileEnum, "Failed to create GFileEnumerator\n");
    pstGTmpFileInfo = g_file_enumerator_next_file(pstGFileEnum, NULL, &pGerr);
    CHECK_IF_GOBJECT_NULL(pstGTmpFileInfo, "Failed to get file info\n");
    while (pstGTmpFileInfo)
    {
        pcFileName = g_file_info_get_name(pstGTmpFileInfo);
        printf("file: %s\n", pcFileName);
        if (strstr(pcFileName, "event"))
        {
            ui16MaxFpLength = strlen(DEVICE_INPUT_FOLDER) + strlen(pcFileName) + 1;
            pcThreadFilePath = malloc(ui16MaxFpLength); //freed in thread
            snprintf(pcThreadFilePath, ui16MaxFpLength, "%s%s", DEVICE_INPUT_FOLDER, pcFileName);
            g_thread_new("gamepad_event_mon", (GThreadFunc) monitor_device_thread, pcThreadFilePath);
        }
        g_object_unref(pstGTmpFileInfo);
        pstGTmpFileInfo = NULL;
        pstGTmpFileInfo = g_file_enumerator_next_file(pstGFileEnum, NULL, &pGerr);
    }
    g_object_unref(pstGFileEnum);
    pstGFileEnum = NULL;

    pstGFileMonObj = g_file_monitor(pstGFile, G_FILE_MONITOR_NONE, NULL, &pGerr);
    CHECK_IF_GOBJECT_NULL(pstGFileMonObj, "Failed to get file monitor object\n");

    g_signal_connect(G_OBJECT(pstGFileMonObj), "changed", G_CALLBACK(devices_changed_cb), NULL);

    char *devFilePath = g_file_get_path(pstGFile);
    g_free(devFilePath);

    g_main_loop_run(pstGMainLoop);
    g_main_loop_unref(pstGMainLoop);

    return 0;
error:
    if (pstGMainLoop)
        g_main_loop_unref(pstGMainLoop);
    if (pstGFile)
        g_object_unref(pstGFile);
    if (pstGFileEnum)
        g_object_unref(pstGFileEnum);
    if (pstGTmpFileInfo)
        g_object_unref(pstGTmpFileInfo);
    if (pstGFileMonObj)
        g_object_unref(pstGFileMonObj);
    return -1;

}