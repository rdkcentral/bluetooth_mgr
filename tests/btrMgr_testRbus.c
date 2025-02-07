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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "btmgr.h"
#include "btrMgr_platform_spec.h"

int                cliDisabled   = 0;
BTRMgrDeviceHandle gDeviceHandle = 0;

static void printOptions (void)
{
    printf ("\n\n");
    printf (" 6. Set Discoverable\n");
    printf (" 8. Start Discovering\n");
    printf (" 9. Stop Discovering\n");
    printf ("10. Get List of Discovered Devices\n");
    printf ("11. Pair a device\n");
    printf ("12. UnPair a device\n");
    printf ("13. Get List of Paired Devices\n");
    printf ("14. Connect to Device\n");
    printf ("15. DisConnect from Device\n");
    printf ("16. Get Device Properities \n");
    printf ("21. Accept External Pair Request\n");
    printf ("22. Deny External Pair Request\n");
    printf ("23. Accept External Connect Request\n");
    printf ("24. Deny External Connect Request\n");
    printf ("38. Set LTE service - Enabled/Disabled/Reset\n");
    printf ("39. Set Broadcast status - Enabled/Disabled\n");
    printf ("41. Set Battery Operations State - Enabled/Disabled\n");
    printf ("49. Start advertisement\n");
    printf ("52. Stop advertisement\n");
    printf ("56. Quit\n");
    printf ("\n\n");
    printf ("Please enter the option that you want to test\t");

    return;
}


static int getUserSelection (void)
{
    int mychoice = 0;
    if (cliDisabled)
    {
       printf("Enter a choice...\n");
       if (scanf("%d", &mychoice)) {
       }
       if (getchar() != EOF) {
       }   //to catch newline
    }
   /* else
    {
    cliArgCounter++;
    if (cliArgCounter < gArgc){
       mychoice = atoi(gArgv[cliArgCounter]);
    }*/
    else{
       printf("\n No Value entered , Sending 0\n");
    }
    return mychoice;
}

static BTRMgrDeviceHandle getDeviceSelection(void)
{
    BTRMgrDeviceHandle mychoice = 0;
    if (cliDisabled)
    {
       printf("Enter a choice...\n");
       if (scanf("%llu", &mychoice)) {
       }
       if (getchar() != EOF) {
       }   //to catch newline
    }
   /* else
    {
        cliArgCounter++;
        if (cliArgCounter < gArgc){
           mychoice = strtoll(gArgv[cliArgCounter],NULL, 0);
        }
        else{
           printf("\n No Value entered , Sending 0\n");
        }

    }*/
    return mychoice;
}

const char* getEventAsString (BTRMGR_Events_t etype)
{
  char *event = "\0";
  switch(etype)
  {
    case BTRMGR_EVENT_DEVICE_OUT_OF_RANGE               : event = "DEVICE_OUT_OF_RANGE_OR_LOST";         break;
    case BTRMGR_EVENT_DEVICE_DISCOVERY_UPDATE           : event = "DEVICE_DISCOVERY_UPDATE";             break;
    case BTRMGR_EVENT_DEVICE_PAIRING_COMPLETE           : event = "DEVICE_PAIRING_COMPLETE";             break;
    case BTRMGR_EVENT_DEVICE_UNPAIRING_COMPLETE         : event = "DEVICE_UNPAIRING_COMPLETE";           break;
    case BTRMGR_EVENT_DEVICE_CONNECTION_COMPLETE        : event = "DEVICE_CONNECTION_COMPLETE";          break;
    case BTRMGR_EVENT_DEVICE_DISCONNECT_COMPLETE        : event = "DEVICE_DISCONNECT_COMPLETE";          break;
    case BTRMGR_EVENT_DEVICE_PAIRING_FAILED             : event = "DEVICE_PAIRING_FAILED";               break;
    case BTRMGR_EVENT_DEVICE_UNPAIRING_FAILED           : event = "DEVICE_UNPAIRING_FAILED";             break;
    case BTRMGR_EVENT_DEVICE_CONNECTION_FAILED          : event = "DEVICE_CONNECTION_FAILED";            break;
    case BTRMGR_EVENT_DEVICE_DISCONNECT_FAILED          : event = "DEVICE_DISCONNECT_FAILED";            break;
    case BTRMGR_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST    : event = "RECEIVED_EXTERNAL_PAIR_REQUEST";      break;
    case BTRMGR_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST : event = "RECEIVED_EXTERNAL_CONNECT_REQUEST";   break;
    case BTRMGR_EVENT_DEVICE_FOUND                      : event = "DEVICE_FOUND";                        break;
    case BTRMGR_EVENT_DEVICE_OP_READY                   : event = "DEVICE_OP_READY";                     break;
    case BTRMGR_EVENT_DEVICE_OP_INFORMATION             : event = "DEVICE_OP_INFORMATION";               break;
    case BTRMGR_EVENT_BATTERY_INFO                      : event = "BATTERY_INFO";                        break;
    default                                             : event = "##INVALID##";
  }
  return event;
}

BTRMGR_Result_t eventCallback (BTRMGR_EventMessage_t event)
{

    /*printf ("\n\t@@@@@@@@ %d : %s eventCallback ::::  Event ID %d @@@@@@@@\n", event.m_pairedDevice.m_deviceType
                                                                             , event.m_pairedDevice.m_name
    									     , event.m_eventType);*/
    printf ("\n\t@@@@@@@@ Received Event ID %s @@@@@@@@\n", getEventAsString(event.m_eventType));
    switch(event.m_eventType) {
      case BTRMGR_EVENT_DEVICE_OUT_OF_RANGE:
          printf("\tYour device %s has either been Lost or Out of Range\n", event.m_pairedDevice.m_name);
          break;
      case BTRMGR_EVENT_DEVICE_FOUND:
          printf("\tYour device %s is Up and Ready\n", event.m_pairedDevice.m_name);
          if(event.m_pairedDevice.m_isLastConnectedDevice) {
	     gDeviceHandle = event.m_pairedDevice.m_deviceHandle;
             int rc = BTRMGR_ConnectToDevice(0, gDeviceHandle, BTRMGR_DEVICE_OP_TYPE_LE);
             if (BTRMGR_RESULT_SUCCESS != rc)
                 printf ("failed\n");
             else
                 printf ("\nSuccess....\n");
          }
          break;
      case BTRMGR_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST:
          printf ("\t DevHandle =  %lld\n", event.m_externalDevice.m_deviceHandle);
          printf ("\t DevName   = %s\n", event.m_externalDevice.m_name);
          printf ("\t DevAddr   = %s\n", event.m_externalDevice.m_deviceAddress);
          printf ("\t PassCode  = %06d\n", event.m_externalDevice.m_externalDevicePIN);
          if (event.m_externalDevice.m_requestConfirmation) {
              printf ("\t Enter Option 21 to Accept Pairing Request\n");
              printf ("\t Enter Option 22 to Deny Pairing Request\n");
              gDeviceHandle = event.m_externalDevice.m_deviceHandle;
          }
          else {
              printf("\n\n\t@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
              printf("\tEnter PIN: %06d in Your \"%s\" to make them paired\n", event.m_externalDevice.m_externalDevicePIN, event.m_externalDevice.m_name);
              printf("\t@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n\n");
         }
         break;
     case BTRMGR_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST:
         printf ("\t DevHandle =  %lld\n", event.m_externalDevice.m_deviceHandle);
         printf ("\t DevName   = %s\n", event.m_externalDevice.m_name);
         printf ("\t DevAddr   = %s\n", event.m_externalDevice.m_deviceAddress);
         printf ("\t Enter Option 23 to Accept Connect Request\n");
         printf ("\t Enter Option 24 to Deny Connect Request\n");
         gDeviceHandle = event.m_externalDevice.m_deviceHandle;
         break;
     case BTRMGR_EVENT_DEVICE_PAIRING_COMPLETE:
     case BTRMGR_EVENT_DEVICE_PAIRING_FAILED:
         printf("\t DevHandle = %lld\n", event.m_discoveredDevice.m_deviceHandle);
         printf("\t DevType   = %s\n", BTRMGR_GetDeviceTypeAsString(event.m_discoveredDevice.m_deviceType));
         printf("\t DevAddr   = %s\n", event.m_discoveredDevice.m_deviceAddress);
         break;
     case BTRMGR_EVENT_DEVICE_UNPAIRING_COMPLETE:
     case BTRMGR_EVENT_DEVICE_UNPAIRING_FAILED:
     case BTRMGR_EVENT_DEVICE_CONNECTION_FAILED:
     case BTRMGR_EVENT_DEVICE_DISCONNECT_FAILED:
     case BTRMGR_EVENT_DEVICE_CONNECTION_COMPLETE:
     case BTRMGR_EVENT_DEVICE_DISCONNECT_COMPLETE:
         printf("\t DevHandle = %lld\n", event.m_pairedDevice.m_deviceHandle);
         printf("\t DevType   = %s\n", BTRMGR_GetDeviceTypeAsString(event.m_pairedDevice.m_deviceType));
         printf("\t DevAddr   = %s\n", event.m_pairedDevice.m_deviceAddress);
         break;
     case BTRMGR_EVENT_DEVICE_DISCOVERY_UPDATE:
         printf ("\n\tDiscovered %s device of type %s\n", event.m_discoveredDevice.m_name, BTRMGR_GetDeviceTypeAsString(event.m_discoveredDevice.m_deviceType));
         break;
     case BTRMGR_EVENT_DEVICE_OP_READY:
         printf("\tDevice %s Op Ready\n", event.m_deviceOpInfo.m_name);
         break;
     case BTRMGR_EVENT_DEVICE_OP_INFORMATION:
         printf("\tRecieved %s Event from BTRMgr\n", getEventAsString(event.m_eventType));
         printf("\tDevice %s Op Information\n", event.m_deviceOpInfo.m_name);
         printf("\tUUID is %s\n", event.m_deviceOpInfo.m_uuid);
        /* if (BTRMGR_DEVICE_TYPE_LE == event.m_deviceOpInfo.m_deviceType)
         {
             printf("\t%s\n", event.m_deviceOpInfo.m_notifyData);
         }*/
         if(BTRMGR_LE_OP_WRITE_VALUE == event.m_deviceOpInfo.m_leOpType)
         {
             printf("\t%s\n", event.m_deviceOpInfo.m_writeData);
             {
                 BTRMGR_SysDiagInfo(0, event.m_deviceOpInfo.m_uuid, event.m_deviceOpInfo.m_writeData, event.m_deviceOpInfo.m_leOpType);
             }
         }
         else if (BTRMGR_LE_OP_READ_VALUE == event.m_deviceOpInfo.m_leOpType)
         {
             printf("received OP_READ event for UUID %s\n", event.m_deviceOpInfo.m_uuid);
#if 0
             BTRMGR_SysDiagInfo(0, event.m_deviceOpInfo.m_uuid, event.m_deviceOpInfo.m_writeData, event.m_deviceOpInfo.m_leOpType);

             /* Send event response */
             BTRMGR_EventResponse_t  lstBtrMgrEvtRsp;

             memset(&lstBtrMgrEvtRsp, 0, sizeof(lstBtrMgrEvtRsp));
             lstBtrMgrEvtRsp.m_eventResp = 1;
             lstBtrMgrEvtRsp.m_eventType = BTRMGR_EVENT_DEVICE_OP_INFORMATION;
             strncpy(lstBtrMgrEvtRsp.m_writeData, event.m_deviceOpInfo.m_writeData, BTRMGR_MAX_DEV_OP_DATA_LEN - 1);
             if (BTRMGR_RESULT_SUCCESS != BTRMGR_SetEventResponse(0, &lstBtrMgrEvtRsp)) {
                 printf("Failed to send event response");
             }
             gDeviceHandle = 0;
#endif
         }
         break;
     case BTRMGR_EVENT_BATTERY_INFO:
         printf("\tRecieved %s Event from BTRMgr\n", getEventAsString(event.m_eventType));
         printf ("\t DevName      = %s\n", event.m_batteryInfo.m_name);
         printf ("\t DevType      = %d\n", event.m_batteryInfo.m_deviceType);
         printf ("\t UUID         = %s\n", event.m_batteryInfo.m_uuid);
         printf ("\t DeviceHandle = %lld\n", event.m_batteryInfo.m_deviceHandle);
	 
	 if (!strcmp(event.m_batteryInfo.m_uuid,BTRMGR_UUID_BATTERY_LEVEL)) {
	     printf ("\t Notification Value : Battery Level - %s\n", event.m_batteryInfo.m_notifyData);
	 } else if (!strcmp(event.m_batteryInfo.m_uuid,BTRMGR_UUID_BATTERY_ERROR_VALUES)) {
             printf ("\t Notification Value : Battery Error Values - %s\n", event.m_batteryInfo.m_notifyData);
	 } else if (!strcmp(event.m_batteryInfo.m_uuid,BTRMGR_UUID_BATTERY_FLAGS)) {
             printf ("\t Notification Value : Battery Error Flags - %s\n", event.m_batteryInfo.m_notifyData);
	 }
	 break;
     default:
         printf("\tReceived %s Event from BTRMgr\n", getEventAsString(event.m_eventType));
         break;
    }

    return BTRMGR_RESULT_SUCCESS;
}



int main(int argc, char *argv[])
{
    BTRMGR_Result_t rc = BTRMGR_RESULT_SUCCESS;
    int loop = 1, i = 0;
    char array[32] = "";
    BTRMgrDeviceHandle handle = 0;

    rc = BTRMGR_Init();

    if (BTRMGR_RESULT_SUCCESS != rc)
    {
        printf ("Failed to init BTRMgr.. Quiting.. \n");
        return 0;
    }

    BTRMGR_RegisterEventCallback (eventCallback);
    printf("<<<<< in btrMgrTest main function >>>>\n");
    if(argc==1){
       printf("\nNo Extra Command Line Argument Passed Other Than Program Name");
       cliDisabled = 1;
    }

    do
    {
        printOptions();
        i = getUserSelection();
        switch (i)
        {
            case 6:
                {
                    unsigned char power_status = 0;
                    int timeout = -1;

                    printf ("Please enter 1 or 0 to Make it Discoverable ON or OFF \t");
                    power_status = (unsigned char) getUserSelection();

                    printf ("Please set the timeout for the discoverable \t");
                    timeout = (int) getUserSelection();
                    printf ("timeout = %d\t\n",timeout);

                    rc = BTRMGR_SetAdapterDiscoverable(0, power_status, timeout);
                    if (BTRMGR_RESULT_SUCCESS != rc)
                        printf ("failed\n");
                    else
                        printf ("Success;\n");
                }
                break;

           case 8:
               {
                  int ch = 0;
                  BTRMGR_DeviceOperationType_t discoveryType = BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT;
                  printf ("Enter Scan Type : [0 - Normal(BR/EDR) | 1 - LE (BLE) | 2 - HID ]\n");
                  ch = getDeviceSelection();
                  if (0 == ch)
                  discoveryType = BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT;
                  else if (1 == ch)
                       discoveryType = BTRMGR_DEVICE_OP_TYPE_LE;
                  else if (2 == ch)
                       discoveryType = BTRMGR_DEVICE_OP_TYPE_HID;
                  printf("BTRMGR_StartDeviceDiscovery....\n");
                  rc = BTRMGR_StartDeviceDiscovery(0, discoveryType);
                  if (BTRMGR_RESULT_SUCCESS != rc)
                      printf ("failed\n");
                  else
                       printf ("Success;\n");
               }
               break;
            case 9:
               {
                   int ch = 0;
                   BTRMGR_DeviceOperationType_t discoveryType = BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT;
                   printf ("Enter Scan Type : [0 - Normal(BR/EDR) | 1 - LE (BLE) | 2 - HID ]\n");
                   ch = getDeviceSelection();
                   if (0 == ch)
                       discoveryType = BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT;
                   else if (1 == ch)
                       discoveryType = BTRMGR_DEVICE_OP_TYPE_LE;
                   else if (2 == ch)
                       discoveryType = BTRMGR_DEVICE_OP_TYPE_HID;

                   rc = BTRMGR_StopDeviceDiscovery(0, discoveryType);
                   if (BTRMGR_RESULT_SUCCESS != rc)
                       printf ("failed\n");
                   else
                       printf ("Success;\n");
               }
               break;
            case 10:
                {
                    BTRMGR_DiscoveredDevicesList_t discoveredDevices;

                    memset (&discoveredDevices, 0, sizeof(discoveredDevices));
                    printf("calling BTRMGR_GetDiscoveredDevices....\n");
                    rc = BTRMGR_GetDiscoveredDevices(0, &discoveredDevices);
                    if (BTRMGR_RESULT_SUCCESS != rc)
                        printf ("failed\n");
                    else
                    {
                        int j = 0;
                        printf ("\nSuccess....   Discovered Devices (%d) are, \n", discoveredDevices.m_numOfDevices);
                        printf ("\n\tSN %-17s %-30s %-17s   %s\n\n", "Device Id", "Device Name", "Device Address", "Device Type");
                        for (; j< discoveredDevices.m_numOfDevices; j++)
                        {
                            printf ("\t%02d %-17llu %-30s %17s   %s\n", j,
                                     discoveredDevices.m_deviceProperty[j].m_deviceHandle,
                                     discoveredDevices.m_deviceProperty[j].m_name,
                                     discoveredDevices.m_deviceProperty[j].m_deviceAddress,
                                     BTRMGR_GetDeviceTypeAsString(discoveredDevices.m_deviceProperty[j].m_deviceType));
                        }
                        printf ("\n\n");
                    }
                }
                break;
            case 11:
                {
                    handle = 0;
                    printf ("Please Enter the device Handle number of the device that you want to pair \t: ");
                    handle = getDeviceSelection();

                    rc = BTRMGR_PairDevice(0, handle);
                    if (BTRMGR_RESULT_SUCCESS != rc)
                        printf ("Pair failed\n");
                    else
                        printf ("\nPair Success....\n");
                }
                break;
            case 12:
                {
                    handle = 0;
                    printf ("Please Enter the device Handle number of the device that you want to Unpair \t: ");
                    handle = getDeviceSelection();

                    rc = BTRMGR_UnpairDevice(0, handle);
                    if (BTRMGR_RESULT_SUCCESS != rc)
                        printf ("failed\n");
                    else
                        printf ("\nSuccess....\n");
                }
                break;
            case 13:
                {
                    BTRMGR_PairedDevicesList_t pairedDevices;

                    memset (&pairedDevices, 0, sizeof(pairedDevices));
                    rc = BTRMGR_GetPairedDevices(0, &pairedDevices);
                    if (BTRMGR_RESULT_SUCCESS != rc)
                        printf ("failed\n");
                    else
                    {
                        int j = 0;
                        printf ("\nSuccess....   Paired Devices (%d) are, \n", pairedDevices.m_numOfDevices);
                        printf ("\n\tSN %-17s %-30s %-17s   %s\n\n", "Device Id", "Device Name", "Device Address", "Device Type");
                        for (; j< pairedDevices.m_numOfDevices; j++)
                        {
                            printf ("\t%02d %-17llu %-30s %17s   %s\n", j,
                                                              pairedDevices.m_deviceProperty[j].m_deviceHandle,
                                                              pairedDevices.m_deviceProperty[j].m_name,
                                                              pairedDevices.m_deviceProperty[j].m_deviceAddress,
                                                              BTRMGR_GetDeviceTypeAsString(pairedDevices.m_deviceProperty[j].m_deviceType));
                        }
                        printf ("\n\n");
                    }
                }
                break;

             case 14:
                 {
                    handle = 0;
                    int ch = 0;
                    printf ("Please Enter the device Handle number of the device that you want to Connect \t: ");
                    handle = getDeviceSelection();
                    printf ("Enter Device ConnectAs  Type : [0 - AUDIO_OUTPUT | 1 - AUDIO_INPUT | 2 - LE | 3 - HID | 4 - UNKNOWN]\n");
                    ch = getDeviceSelection();

                    rc = BTRMGR_ConnectToDevice(0, handle, (1 << ch));
                    if (BTRMGR_RESULT_SUCCESS != rc)
                        printf ("failed\n");
                    else
                        printf ("\nSuccess....\n");
                }
                break;
            case 15:
                {
                    handle = 0;
                    printf ("Please Enter the device Handle number of the device that you want to DisConnect \t: ");
                    handle = getDeviceSelection();

                    rc = BTRMGR_DisconnectFromDevice(0, handle);
                    if (BTRMGR_RESULT_SUCCESS != rc)
                        printf ("failed\n");
                    else
                        printf ("\nSuccess....\n");
                }
                break;
            case 16:
                {
                    BTRMGR_DevicesProperty_t deviceProperty;
                    int i = 0;

                    handle = 0;
                    memset (array, '\0', sizeof(array));
                    memset (&deviceProperty, 0, sizeof(deviceProperty));

                    printf ("Please Enter the device Handle number of the device that you want to query \t: ");
                    handle = getDeviceSelection();

                    rc = BTRMGR_GetDeviceProperties(0, handle, &deviceProperty);
                    if (BTRMGR_RESULT_SUCCESS != rc)
                        printf ("failed\n");
                    else
                    {
                        printf ("\nSuccess.... Properties are, \n");
                        printf ("Handle       : %llu\n", deviceProperty.m_deviceHandle);
                        printf ("Name         : %s\n", deviceProperty.m_name);
                        printf ("Address      : %s\n", deviceProperty.m_deviceAddress);
                        printf ("RSSI         : %d\n", deviceProperty.m_rssi);
                        printf ("Paired       : %d\n", deviceProperty.m_isPaired);
                        printf ("Connected    : %d\n", deviceProperty.m_isConnected);
                        printf ("Vendor ID    : %u\n", deviceProperty.m_vendorID);
                        for (i = 0; i < deviceProperty.m_serviceInfo.m_numOfService; i++)
                        {
                            printf ("Profile ID   : 0x%.4x\n", deviceProperty.m_serviceInfo.m_profileInfo[i].m_uuid);
                            printf ("Profile Name : %s\n", deviceProperty.m_serviceInfo.m_profileInfo[i].m_profile);
                        }
                        printf ("######################\n\n\n");
                    }
                }
                break;
           case 21:
               {
                   BTRMGR_EventResponse_t  lstBtrMgrEvtRsp;
                   memset(&lstBtrMgrEvtRsp, 0, sizeof(lstBtrMgrEvtRsp));

                   lstBtrMgrEvtRsp.m_deviceHandle = gDeviceHandle;
                   lstBtrMgrEvtRsp.m_eventType = BTRMGR_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST;
                   lstBtrMgrEvtRsp.m_eventResp = 1;
                   printf("<< calling BTRMGR_SetEventResponse from testapp >>\n");
                   if (BTRMGR_RESULT_SUCCESS != BTRMGR_SetEventResponse(0, &lstBtrMgrEvtRsp)) {
                       printf ("Failed to send event response");
                   }
                   gDeviceHandle = 0;
               }
               break;
          case 22:
               {
                   BTRMGR_EventResponse_t  lstBtrMgrEvtRsp;
                   memset(&lstBtrMgrEvtRsp, 0, sizeof(lstBtrMgrEvtRsp));

                   lstBtrMgrEvtRsp.m_deviceHandle = gDeviceHandle;
                   lstBtrMgrEvtRsp.m_eventType = BTRMGR_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST;
                   lstBtrMgrEvtRsp.m_eventResp = 0;

                   if (BTRMGR_RESULT_SUCCESS != BTRMGR_SetEventResponse(0, &lstBtrMgrEvtRsp)) {
                       printf ("Failed to send event response");
                   }
                   gDeviceHandle = 0;
               }
               break;
           case 23:
               {
                   BTRMGR_EventResponse_t  lstBtrMgrEvtRsp;
                   memset(&lstBtrMgrEvtRsp, 0, sizeof(lstBtrMgrEvtRsp));

                   lstBtrMgrEvtRsp.m_deviceHandle = gDeviceHandle;
                   lstBtrMgrEvtRsp.m_eventType = BTRMGR_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST;
                   lstBtrMgrEvtRsp.m_eventResp = 1;

                   if (BTRMGR_RESULT_SUCCESS != BTRMGR_SetEventResponse(0, &lstBtrMgrEvtRsp)) {
                       printf ("Failed to send event response");
                   }
                   gDeviceHandle = 0;
               }
               break;
           case 24:
               {
                   BTRMGR_EventResponse_t  lstBtrMgrEvtRsp;
                   memset(&lstBtrMgrEvtRsp, 0, sizeof(lstBtrMgrEvtRsp));

                   lstBtrMgrEvtRsp.m_deviceHandle = gDeviceHandle;
                   lstBtrMgrEvtRsp.m_eventType = BTRMGR_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST;
                   lstBtrMgrEvtRsp.m_eventResp = 0;

                   if (BTRMGR_RESULT_SUCCESS != BTRMGR_SetEventResponse(0, &lstBtrMgrEvtRsp)) {
                       printf ("Failed to send event response");
                   }
                   gDeviceHandle = 0;
               }
               break;
           case 38:
               {
                    int choice = 0;

                    printf ("Press 1 to Enable, 0 to Disable LTE(Cellular modem) Service, or 255 to reset the test!\n");
                    if (scanf("%d", &choice)) {
                    }

                    rc = BTRMGR_SetLTEServiceState (0, choice);

                    if (BTRMGR_RESULT_SUCCESS == rc) {
                        if (choice == 255) {
                            printf("\nSuccessfully Reset .\n");
                        }
                        else if(choice == 0) {
                            printf("\nSuccessfully Disabled .\n");
                        } else {
                            printf("\nSuccessfully Enabled .\n");
                        }
                    }
                    else {
                        printf("\nCall Failed : %d\n", rc);
                    }
               }
               break;

            case 39:
               {
                   int choice = 0;

                   printf("Press 1 to Enable and 0 to Disable the Adv broadcast.\n");
                   if (scanf("%d", &choice)) {}

                   rc = BTRMGR_SetBroadcastState(0, choice);

                   if (BTRMGR_RESULT_SUCCESS == rc) {
                       if (choice) {
                            printf("\nSuccessfully Enabled .\n");
                       }
                       else {
                            printf("\nSuccessfully Disabled .\n");
                       }
                   }
                   else {
                       printf("\nCall Failed : %d\n", rc);
                   }
               }
               break;

            case 41:
               {
                    int choice = 0;

                    printf ("Press 1 to Enable and 0 to Disable Battery Operations ..\n");
                    if (scanf("%d", &choice)) {
                    }

                    rc = BTRMGR_SetBatteryOpsState(0, choice);

                    if (BTRMGR_RESULT_SUCCESS == rc) {
                        if (choice) {
                            printf("\nSuccessfully Enabled .\n");
                        }
                        else {
                            printf("\nSuccessfully Disabled .\n");
                        }
                    }
                    else {
                        printf("\nCall Failed : %d\n", rc);
                    }
               }
               break;


           case 49:
               {

                   char lPropertyValue[BTRMGR_MAX_STR_LEN] = "\0";
                   char modelNumber[BTRMGR_STR_LEN] = "\0";
                   char serialNumber[BTRMGR_STR_LEN] = "\0";
                   char deviceMac[BTRMGR_STR_LEN] = "\0";
                   char ImeiNumber[BTRMGR_STR_LEN] = "\0";

                   printf("<<<<invoking BTRMGR_LE_SetServiceInfo from testapp\n");
                   BTRMGR_LE_SetServiceInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, 1);
		   BTRMGR_LE_SetServiceInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, 1);

                   printf("Adding char for the default local services \n");
                   /* Get system ID - device MAC */
                   BTRMGR_SysDiagInfo(0, BTRMGR_SYSTEM_ID_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
                   strncpy(deviceMac, lPropertyValue, BTRMGR_STR_LEN - 1);
                   BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_SYSTEM_ID_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);

                   /* model number */
                   BTRMGR_SysDiagInfo(0, BTRMGR_MODEL_NUMBER_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
                   strncpy(modelNumber, lPropertyValue, BTRMGR_STR_LEN - 1);
                   BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_MODEL_NUMBER_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);

                   /*Get HW revision*/
                   BTRMGR_SysDiagInfo(0, BTRMGR_HARDWARE_REVISION_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
                   BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_HARDWARE_REVISION_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);

                   /* Get serial number */
                   BTRMGR_SysDiagInfo(0, BTRMGR_SERIAL_NUMBER_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
                   strncpy(serialNumber, lPropertyValue, BTRMGR_STR_LEN - 1);
                   BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_SERIAL_NUMBER_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);

                   /* Get firmware/software revision */
                   BTRMGR_SysDiagInfo(0, BTRMGR_FIRMWARE_REVISION_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
                   BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_FIRMWARE_REVISION_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
                   BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_SOFTWARE_REVISION_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);

                   /* Get manufacturer name */
                   BTRMGR_SysDiagInfo(0, BTRMGR_MANUFACTURER_NAME_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
                   BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_MANUFACTURER_NAME_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
                   
		   /* Modem IMEI */
                   BTRMGR_SysDiagInfo(0, BTRMGR_UUID_MODEM_IMEI, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
                   strncpy(ImeiNumber, lPropertyValue, BTRMGR_STR_LEN - 1);
                   BTRMGR_LE_SetGattInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, BTRMGR_UUID_MODEM_IMEI, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);

		   /* QR CODE */
                   // BTRMGR_SysDiagInfo(0, BTRMGR_UUID_QR_CODE, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
                   snprintf(lPropertyValue, BTRMGR_MAX_STR_LEN - 1, "MN:%s.SN:%s.DM:%s.IM:%s", modelNumber, serialNumber, deviceMac, ImeiNumber);
                   printf("<<< Qr_code = %s >>>\n", lPropertyValue);
                   BTRMGR_LE_SetGattInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, BTRMGR_UUID_QR_CODE, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);

                   /* Provison status */
                   BTRMGR_SysDiagInfo(0, BTRMGR_UUID_PROVISION_STATUS, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
                   BTRMGR_LE_SetGattInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, BTRMGR_UUID_PROVISION_STATUS, 0x101, lPropertyValue, BTRMGR_LE_PROP_CHAR);

                   /* Sim ICCID */
                   BTRMGR_SysDiagInfo(0, BTRMGR_UUID_SIM_ICCID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
                   BTRMGR_LE_SetGattInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, BTRMGR_UUID_SIM_ICCID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);

                   /* CELLULAR SIGNAL STRENGTH */
                   BTRMGR_SysDiagInfo(0, BTRMGR_UUID_CELLULAR_SIGNAL_STRENGTH, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
                   BTRMGR_LE_SetGattInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, BTRMGR_UUID_CELLULAR_SIGNAL_STRENGTH, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);

                   /* MESH BACKHAUL TYPE */
                   BTRMGR_SysDiagInfo(0, BTRMGR_UUID_MESH_BACKHAUL_TYPE, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
                   BTRMGR_LE_SetGattInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, BTRMGR_UUID_MESH_BACKHAUL_TYPE, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);

                   /* WIFI BACKHAUL STATS */
                   BTRMGR_SysDiagInfo(0, BTRMGR_UUID_WIFI_BACKHAUL_STATS, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
                   BTRMGR_LE_SetGattInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, BTRMGR_UUID_WIFI_BACKHAUL_STATS, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);

		   printf("Starting the ad\n");
                   BTRMGR_LE_StartAdvertisement(0, NULL);
                }
                break;
	  case 52:
		{
		   BTRMGR_LE_StopAdvertisement(0);
		}
		break;
          case 56:
                loop = 0;
                break;
          default:
                printf ("Invalid Selection.....\n");
                break;
        }
    }while(loop && cliDisabled);

     BTRMGR_DeInit();
     return 0;
}
