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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef UNIT_TEST
#define STATIC static
#else 
#define STATIC 
#define STREAM_IN_SUPPORTED
#endif 

/* System Headers */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <glib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "btrCore.h"
#include "btmgr.h"
#include "btrMgr_logger.h"

#ifndef BUILD_FOR_PI
#include "rfcapi.h"
#include "libIBus.h"
#endif

#include "safec_lib.h"

#include "btrMgr_Types.h"
#ifndef LE_MODE
#include "btrMgr_mediaTypes.h"
#include "btrMgr_streamOut.h"
#include "btrMgr_audioCap.h"
#ifdef STREAM_IN_SUPPORTED
#include "btrMgr_streamIn.h"
#endif
#endif
#include "btrMgr_persistIface.h"
#include "btrMgr_SysDiag.h"
#include "btrMgr_Columbo.h"
#include "btrMgr_LEOnboarding.h"


#ifdef LE_MODE
#include "btrMgr_batteryService.h"
#include "btrMgr_platform_spec.h"
#endif

/* Private Macro definitions */
#define BTRMGR_SIGNAL_POOR       (-90)
#define BTRMGR_SIGNAL_FAIR       (-70)
#define BTRMGR_SIGNAL_GOOD       (-60)

#define BTRMGR_MODALIAS_RETRY_ATTEMPTS      5
#define BTRMGR_CONNECT_RETRY_ATTEMPTS       2
#define BTRMGR_PAIR_RETRY_ATTEMPTS          10
#define BTRMGR_DEVCONN_CHECK_RETRY_ATTEMPTS 3
#define BTRMGR_DEVCONN_PWRST_CHANGE_TIME    3
#define BTRMGR_DEVPAIR_HANDLE_RESET_TIME    5
#define BTMGR_RECONNECTION_HOLD_OFF 3
#define BTMGR_RECONNECTION_ATTEMPTS 3
#define BTMGR_AVDTP_SUSPEND_MAX_RETRIES 3
#define BTRMGR_DISCOVERY_HOLD_OFF_TIME      120
#define BTRMGR_UNITACTIVATION_STATUS_CHECK_TIME_INTERVAL 20
#define BTRMGR_REMOTE_CONTROL_APPEARANCE 0x0180
#define BTRMGR_MODELINE_MAX_LEN 38

#define BTRMGR_BATTERY_DISCOVERY_TIMEOUT             360
#define BTRMGR_BATTERY_DISCOVERY_TIME_INTERVAL       30
#define BTRMGR_PAIRING_MAX_WAIT_TIME                 10
#define BTRMGR_BATTERY_CONNECT_TIME_INTERVAL         15
#define BTRMGR_BATTERY_START_NOTIFY_TIME_INTERVAL    20
#define BTRMGR_DEVICE_DISCONNECT_STATUS_TIME_INTERVAL 3
#define BTRMGR_AUTOCONNECT_ON_STARTUP_TIMEOUT        40
#define MAX_FIRMWARE_FILES                           5
#define BTRMGR_DISCONNECT_CLEAR_STATUS_TIME_INTERVAL 10
#define BTRMGR_POST_OUT_OF_RANGE_HOLD_OFF_TIME       6
#define BTRMGR_POST_DEVICE_FOUND_EVENT_WAIT_TIME     3
#define BTRMGR_CLEAR_LAST_PAIRED_STATUS_INTERVAL     (2*60)
#define BTMGR_FLUSH_TIMEOUT_INTERVAL_MS 100
#define BTMGR_FLUSH_TIMEOUT_LARGE_MTU_INTERVAL_MS 255
#define BTMGR_LARGE_MTU_THRESHOLD 800

#define BTMGR_PROCESS_NAME        "btMgrBus"
#define RDK_LOGGER_BTMGR_NAME "LOG.RDK.BTRMGR"
#define RDK_LOGGER_BTCORE_NAME "LOG.RDK.BTRCORE"
#define BTRMGR_REMOTE_DEVICE   "Remote Device"

#ifdef RDKTV_PERSIST_VOLUME
#define BTRMGR_DEFAULT_SET_VOLUME_INTERVAL             2
#define BTRMGR_SKIP_VOLUME_UPDATE_INTERVAL             6
#define BTRMGR_DEFAULT_CONNECTION_IN_PROGRESS_INTERVAL 2
#endif

typedef enum _BTRMGR_DiscoveryState_t {
    BTRMGR_DISCOVERY_ST_UNKNOWN,
    BTRMGR_DISCOVERY_ST_STARTED,
    BTRMGR_DISCOVERY_ST_PAUSED,
    BTRMGR_DISCOVERY_ST_RESUMED,
    BTRMGR_DISCOVERY_ST_STOPPED,
} BTRMGR_DiscoveryState_t;

typedef enum _enBTRMGR_Mode_t {
    BTRMGR_MODE_ON = 0,
    BTRMGR_MODE_OFF,
    BTRMGR_MODE_UNKNOWN,
} _enBTRMGR_Mode_t;

#ifndef LE_MODE

typedef enum _enBTRMGRStartupAudio {
    BTRMGR_STARTUP_AUD_INPROGRESS,
    BTRMGR_STARTUP_AUD_SKIPPED,
    BTRMGR_STARTUP_AUD_COMPLETED,
    BTRMGR_STARTUP_AUD_UNKNOWN,
    BTRMGR_STARTUP_AUD_RETRY,
} enBTRMGRStartupAudio;

// Move to private header ?
typedef struct _stBTRMgrStreamingInfo {
    tBTRMgrAcHdl            hBTRMgrAcHdl;
    tBTRMgrSoHdl            hBTRMgrSoHdl;
    tBTRMgrSoHdl            hBTRMgrSiHdl;
    BTRMGR_StreamOut_Type_t tBTRMgrSoType;
    unsigned long           bytesWritten;
    unsigned                samplerate;
    unsigned                channels;
    unsigned                bitsPerSample;
    int                     i32BytesToEncode;
} stBTRMgrStreamingInfo;

#else

typedef struct _stBTRMgrBatteryInfo {
    tBTRMgrBatteryHdl       hBTRMgrBatteryHdl;
} stBTRMgrBatteryInfo;

#endif

typedef struct _BTRMGR_DiscoveryHandle_t {
    BTRMGR_DeviceOperationType_t    m_devOpType;
    BTRMGR_DiscoveryState_t         m_disStatus;
    BTRMGR_DiscoveryFilterHandle_t  m_disFilter;
} BTRMGR_DiscoveryHandle_t;

typedef struct _BTRMGR_ConnectionInformation_t {
    BTRMgrDeviceHandle deviceHandle;
    guint timeToWait;
    BTRMGR_DeviceOperationType_t connectAs;
    unsigned char lui8AdapterIdx;
} BTRMGR_ConnectionInformation_t;

//TODO: Move to a local handle. Mutex protect all
STATIC GMainContext*                    gmainContext                = NULL;
STATIC tBTRCoreHandle                   ghBTRCoreHdl                = NULL;
STATIC tBTRMgrPIHdl                     ghBTRMgrPiHdl               = NULL;
STATIC tBTRMgrSDHdl                     ghBTRMgrSdHdl               = NULL;
STATIC BTRMgrDeviceHandle               ghBTRMgrDevHdlLastConnected = 0;
STATIC BTRMgrDeviceHandle               ghBTRMgrDevHdlDisConStatusCheck = 0;
STATIC BTRMgrDeviceHandle               ghBTRMgrDevHdlLastDisconnected = 0;
STATIC BTRMgrDeviceHandle               ghBTRMgrDevHdlLastPaired    = 0;
STATIC BTRMgrDeviceHandle               ghBTRMgrDevHdlCurStreaming  = 0;
STATIC BTRMgrDeviceHandle               ghBTRMGRDevHdlTestStreaming = 0;
STATIC BTRMgrDeviceHandle               ghBTRMgrDevHdlStreamStartUp = 0;
STATIC BTRMgrDeviceHandle               ghBTRMgrDevHdlPairingInProgress = 0;
STATIC BTRMgrDeviceHandle               ghBTRMgrDevHdlConnInProgress = 0;
#ifdef RDKTV_PERSIST_VOLUME
STATIC BTRMgrDeviceHandle               ghBTRMgrDevHdlVolSetupInProgress = 0;
#endif
gboolean                                isDeinitInProgress = FALSE;

STATIC BTRMGR_DiscoveryHandle_t         ghBTRMgrDiscoveryHdl;
STATIC BTRMGR_DiscoveryHandle_t         ghBTRMgrBgDiscoveryHdl;

STATIC stBTRCoreAdapter                 gDefaultAdapterContext;
STATIC stBTRCoreListAdapters            gListOfAdapters;
STATIC stBTRCoreDevMediaInfo            gstBtrCoreDevMediaInfo;
STATIC BTRMGR_DiscoveredDevicesList_t   gListOfDiscoveredDevices;
STATIC BTRMGR_PairedDevicesList_t       gListOfPairedDevices;
#ifndef LE_MODE
STATIC stBTRMgrStreamingInfo            gstBTRMgrStreamingInfo;
#endif
STATIC unsigned char                    gui8IsSoDevAvrcpSupported   = 0;
STATIC unsigned char                    gIsLeDeviceConnected        = 0;
STATIC unsigned char                    gIsAgentActivated           = 0;
STATIC unsigned char                    gEventRespReceived          = 0;
STATIC unsigned char                    gAcceptConnection           = 0;
STATIC unsigned char                    gIsUserInitiated            = 0;
STATIC unsigned char                    gDiscHoldOffTimeOutCbData   = 0;
STATIC unsigned char                    gConnPwrStChTimeOutCbData   = 0;
STATIC unsigned char                    gPairCompRstTimeOutCbData   = 0;
STATIC unsigned char                    gIsAudioInEnabled           = 0;
STATIC unsigned char                    gIsHidGamePadEnabled        = 0;
STATIC volatile guint                   gTimeOutRef                 = 0;
STATIC volatile guint                   gConnPwrStChangeTimeOutRef  = 0;
STATIC volatile guint                   gConnPairCompRstTimeOutRef  = 0;
STATIC volatile guint                   gAuthDisconnDevTimeOutRef   = 0;
STATIC volatile guint                   gGetDevDisStatusTimeoutRef  = 0;
#ifdef AUTO_CONNECT_ENABLED
STATIC volatile guint                   gAutoConnStartUpTimeoutRef  = 0;
#endif
STATIC volatile guint                   gDisconnTimeoutRef          = 0;
STATIC volatile unsigned int            gIsAdapterDiscovering       = 0;
STATIC volatile guint                   deviceActstChangeTimeOutRef = 0;
STATIC volatile guint                   gOORTimeOutRef              = 0;
STATIC volatile guint                   gDeviceFoundEvtTimeOutRef   = 0;
STATIC volatile guint                   gLastPairedTimeoutRef       = 0;
unsigned int                            CheckStatusRetryCount       = 0;

STATIC BTRMGR_DeviceOperationType_t     gBgDiscoveryType            = BTRMGR_DEVICE_OP_TYPE_UNKNOWN;
#ifndef LE_MODE
STATIC BTRMGR_Events_t                  gMediaPlaybackStPrev        = BTRMGR_EVENT_MAX;
STATIC enBTRMGRStartupAudio             gIsAudOutStartupInProgress  = BTRMGR_STARTUP_AUD_UNKNOWN;
STATIC char                             gLeReadOpResponse[BTRMGR_MAX_DEV_OP_DATA_LEN] = "\0";
STATIC pid_t                            gPidOfRunningPacketCapture  = 0;
STATIC pid_t                            gPidOfRunningHidMonitor     = 0;
unsigned char                           gDebugModeEnabled           = 0; //shared with audiocaptureMgr
#endif

STATIC void*                            gpvMainLoop                 = NULL;
STATIC void*                            gpvMainLoopThread           = NULL;
STATIC BTRMGR_EventCallback             gfpcBBTRMgrEventOut         = NULL;
STATIC BOOLEAN                          gIsAdvertisementSet         = FALSE;
STATIC BOOLEAN                          gIsDeviceAdvertising        = FALSE;
STATIC BOOLEAN                          gIsDiscoveryOpInternal      = FALSE;
STATIC BOOLEAN                          gEliteIncomCon              = FALSE;
STATIC BOOLEAN                          gbGamepadStandbyMode        = FALSE;
#ifdef RDKTV_PERSIST_VOLUME
STATIC BOOLEAN                          gSkipVolumeUpdate           = FALSE;
STATIC volatile guint                   gSkipVolumeUpdateTimeoutRef = 0;
STATIC volatile guint                   gDefaultSetVolumeTimeoutRef = 0;
STATIC volatile guint                   gDefaultConInProgressTimeoutRef = 0;
#endif

#ifdef LE_MODE
STATIC BOOLEAN                          gCellularModemTstOverride   = FALSE;
STATIC BOOLEAN                          gCellularModemIsOnline      = FALSE;
STATIC BOOLEAN                          gIsBatteryOperationsEnabled = FALSE;
STATIC BOOLEAN                          gIsBroadcastEnable          = TRUE;
STATIC BOOLEAN                          gXbbConnected               = FALSE;
STATIC unsigned int                     gTimeoutSeconds             = 0;
STATIC unsigned int                     gBatteryConnectRetry        = 0;
STATIC BTRMgrDeviceHandle               gBatteryDevHandle           = 0;
STATIC unsigned int                     gConnectionCheckAttempt     = 0;
STATIC stBTRMgrBatteryInfo              gstBTRMgrBatteryInfo;
STATIC volatile guint                   gGetDisBatDevTimeoutRef     = 0;
STATIC volatile guint                   gConBatDevTimeoutRef        = 0;
STATIC volatile guint                   gBatConnStatusTimeotRef     = 0;
STATIC volatile guint                   gBatStartNotifyTimeoutRef   = 0;
STATIC volatile guint                   gStartFirmUpTimeoutRef      = 0;
STATIC volatile guint                   gCheckFirmTimeoutRef        = 0;

// Notify variables
STATIC volatile guint                   gdeviceActstChangeTimeOutRef = 0;
STATIC volatile guint                   gProvisionNotifyTimerHdl     = 0;
STATIC char                             gPrePropertyValue[BTRMGR_MAX_STR_LEN]   = {'\0'};
#endif

BTRMGR_LeCustomAdvertisement_t stCoreCustomAdv =
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
    0x0B ,
    0xFF ,
    0xA3 ,
    0x07 ,
    0x0101,
    {
        0x00,
        0x11,
        0x22,
        0x33,
        0x44,
        0x55,
        0x66,
        0x77,
        0x88,
        0x99,
        0xAA,
        0xBB
    },
    {
        0xC8,
        0xB3,
        0x73,
        0x32,
        0xEA,
        0x3D
    }
};

#ifdef RDK_LOGGER_ENABLED
int b_rdk_logger_enabled = 0;
#endif



/* Static Function Prototypes */
static inline unsigned char btrMgr_GetAdapterCnt (void);
static const char* btrMgr_GetAdapterPath (unsigned char aui8AdapterIdx);

static inline void btrMgr_SetAgentActivated (unsigned char aui8AgentActivated);
static inline unsigned char btrMgr_GetAgentActivated (void);

void btrMgr_IncomingConnectionAuthentication(stBTRCoreDevStatusCBInfo* p_StatusCB, int *auth);
#ifdef LE_MODE
static void btrMgr_CheckBroadcastAvailability (void);
#else
STATIC void btrMgr_CheckAudioInServiceAvailability (void);
static void btrMgr_CheckHidGamePadServiceAvailability (void);
STATIC void btrMgr_CheckDebugModeAvailability(void);
#endif

STATIC const char* btrMgr_GetDiscoveryDeviceTypeAsString (BTRMGR_DeviceOperationType_t adevOpType);
//static const char* btrMgr_GetDiscoveryFilterAsString (BTRMGR_ScanFilter_t ascanFlt);
STATIC const char* btrMgr_GetDiscoveryStateAsString (BTRMGR_DiscoveryState_t  aScanStatus);

static inline void btrMgr_SetDiscoveryHandle (BTRMGR_DeviceOperationType_t adevOpType, BTRMGR_DiscoveryState_t aScanStatus);
static inline void btrMgr_SetBgDiscoveryType (BTRMGR_DeviceOperationType_t adevOpType);
static inline void btrMgr_SetDiscoveryState (BTRMGR_DiscoveryHandle_t* ahdiscoveryHdl, BTRMGR_DiscoveryState_t aScanStatus);
static inline void btrMgr_SetDiscoveryDeviceType (BTRMGR_DiscoveryHandle_t*  ahdiscoveryHdl, BTRMGR_DeviceOperationType_t aeDevOpType);

static inline BTRMGR_DeviceOperationType_t btrMgr_GetBgDiscoveryType (void);
static inline BTRMGR_DiscoveryState_t btrMgr_GetDiscoveryState (BTRMGR_DiscoveryHandle_t* ahdiscoveryHdl);
static inline BTRMGR_DeviceOperationType_t btrMgr_GetDiscoveryDeviceType (BTRMGR_DiscoveryHandle_t*  ahdiscoveryHdl);
//static inline BTRMGR_DiscoveryFilterHandle_t* btrMgr_GetDiscoveryFilter (BTRMGR_DiscoveryHandle_t*   ahdiscoveryHdl);

static inline gboolean btrMgr_isTimeOutSet (void);
static inline void btrMgr_SetDeviceDisStatusHoldOffTimer(void);
static inline void btrMgr_ClearDiscoveryHoldOffTimer(void);
static inline void btrMgr_SetDiscoveryHoldOffTimer(unsigned char aui8AdapterIdx);
static inline void btrMgr_ClearDisconnDevHoldOffTimer(void);
static inline void btrMgr_SetDisconnDevHoldOffTimer(tBTRCoreDevId aBTRCoreDevId);
static inline void btrMgr_PostDeviceOORHoldOffTimer(stBTRCoreDevStatusCBInfo*   p_StatusCB);
static inline void btrMgr_ClearDeviceOORHoldOffTimer(void);
#ifdef RDKTV_PERSIST_VOLUME
static inline void btrMgr_SetVolumeHoldOffTimer(stBTRCoreMediaStatusCBInfo*  mediaStatusCB);
static inline void btrMgr_ClearVolumeHoldOffTimer(void);
static inline void btrMgr_ResetSkipVolumeUpdateTimer(void);
static inline void btrMgr_ClearSkipVolumeUpdateTimer(void);
static inline void btrMgr_SetConInProgressStatusHoldOffTimer(void);
static inline void btrMgr_ClearConInProgressStatusHoldOffTimer(void);
#endif
static inline void btrMgr_SetDisconnectStatusHoldOffTimer (void);
static inline void btrMgr_ClearDisconnectStatusHoldOffTimer (void);

#ifdef LE_MODE
static inline void btrMgr_CheckFirmwareVersionHoldOffTimer(void);
static inline void btrMgr_StartBatteryFirmwareUpgradeHoldOffTimer(void);
static inline void btrMgr_SetDiscoverbatteryDevicesHoldOffTimer(void);
static inline void btrMgr_SetConnectBatteryHoldOffTimer(void);
static inline void btrMgr_SetBatteryConnectionStatusHoldOffTimer(void);
static inline void btrMgr_SetBatteryStartNotifyHoldOffTimer(void);
static inline void btrMgr_SetLastPairedDeviceStatusHoldOffTimer (void);
static inline void btrMgr_ClearBatteryConnectionStatusHoldOffTimer(void);
static inline void btrMgr_ClearBatteryStartNotifyHoldOffTimer(void);
static inline void btrMgr_ClearDiscoverbatteryDevicesHoldOffTimer(void);
static inline void btrMgr_ClearConnectBatteryHoldOffTimer(void);
static inline void btrMgr_ClearBatteryFirmwareUpgradeHoldOffTimer(void);
static inline void btrMgr_ClearCheckFirmwareVersionHoldOffTimer(void);
static inline void btrMgr_ClearLastPairedDeviceStatusHoldOffTimer (void);
#endif

//static eBTRMgrRet btrMgr_SetDiscoveryFilter (BTRMGR_DiscoveryHandle_t* ahdiscoveryHdl, BTRMGR_ScanFilter_t aeScanFilterType, void* aFilterValue);
//static eBTRMgrRet btrMgr_ClearDiscoveryFilter (BTRMGR_DiscoveryHandle_t*   ahdiscoveryHdl);

STATIC BTRMGR_DiscoveryHandle_t* btrMgr_GetDiscoveryInProgress (void);

STATIC BTRMGR_Result_t BTRMGR_StartDeviceDiscovery_Internal (unsigned char aui8AdapterIdx, BTRMGR_DeviceOperationType_t aenBTRMgrDevOpT);
STATIC BTRMGR_Result_t BTRMGR_StopDeviceDiscovery_Internal (unsigned char aui8AdapterIdx, BTRMGR_DeviceOperationType_t aenBTRMgrDevOpT);
STATIC BTRMGR_Result_t BTRMGR_GetDiscoveredDevices_Internal (unsigned char aui8AdapterIdx, BTRMGR_DiscoveredDevicesList_t* pDiscoveredDevices);

STATIC eBTRMgrRet btrMgr_PauseDeviceDiscovery (unsigned char aui8AdapterIdx, BTRMGR_DiscoveryHandle_t* ahdiscoveryHdl);
STATIC eBTRMgrRet btrMgr_ResumeDeviceDiscovery (unsigned char aui8AdapterIdx, BTRMGR_DiscoveryHandle_t* ahdiscoveryHdl);
STATIC eBTRMgrRet btrMgr_StopDeviceDiscovery (unsigned char aui8AdapterIdx, BTRMGR_DiscoveryHandle_t* ahdiscoveryHdl);

STATIC eBTRMgrRet btrMgr_PreCheckDiscoveryStatus (unsigned char aui8AdapterIdx, BTRMGR_DeviceOperationType_t aDevOpType);
STATIC eBTRMgrRet btrMgr_PostCheckDiscoveryStatus (unsigned char aui8AdapterIdx, BTRMGR_DeviceOperationType_t aDevOpType);

STATIC void btrMgr_GetPairedDevInfo (BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_PairedDevices_t* apBtMgrPairedDevInfo);
STATIC void btrMgr_GetDiscoveredDevInfo (BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DiscoveredDevices_t* apBtMgrDiscoveredDevInfo);

STATIC unsigned char btrMgr_GetDevPaired (BTRMgrDeviceHandle ahBTRMgrDevHdl);

STATIC void btrMgr_SetDevConnected (BTRMgrDeviceHandle ahBTRMgrDevHdl, unsigned char aui8isDeviceConnected);
STATIC unsigned char btrMgr_IsDevConnected (BTRMgrDeviceHandle ahBTRMgrDevHdl);

STATIC unsigned char btrMgr_IsDevNameSameAsAddress (char* apcDeviceName, char* apcDeviceAddress, unsigned int ui32StrLen);
STATIC unsigned char btrMgr_CheckIfDevicePrevDetected (BTRMgrDeviceHandle ahBTRMgrDevHdl);

STATIC BTRMGR_DeviceType_t btrMgr_MapDeviceTypeFromCore (enBTRCoreDeviceClass device_type);
STATIC BTRMGR_DeviceOperationType_t btrMgr_MapDeviceOpFromDeviceType (BTRMGR_DeviceType_t device_type);
STATIC BTRMGR_RSSIValue_t btrMgr_MapSignalStrengthToRSSI (int signalStrength);
STATIC eBTRMgrRet btrMgr_MapDevstatusInfoToEventInfo (void* p_StatusCB, BTRMGR_EventMessage_t* apstEventMessage, BTRMGR_Events_t type);
STATIC eBTRMgrRet btrMgr_GetDeviceProductDetails(BTRMgrDeviceHandle  ahBTRMgrDevHdl, unsigned int *pui32MproductId,unsigned int *pui32MVendorId);
STATIC gboolean btrMgr_IsPS4Gamepad(BTRMgrDeviceHandle  ahBTRMgrDevHdl);
STATIC eBTRMgrRet btrMgr_GetDeviceDetails (BTRMgrDeviceHandle ahBTRMgrDevHdl,stBTRCoreBTDevice *pstDeviceInfo);

#ifndef LE_MODE
STATIC eBTRMgrRet btrMgr_StartCastingAudio (int outFileFd, int outMTUSize, unsigned int outDevDelay, eBTRCoreDevMediaType aenBtrCoreDevOutMType, void* apstBtrCoreDevOutMCodecInfo,BTRMgrDeviceHandle  devHandle,char* profileStr);
STATIC eBTRMgrRet btrMgr_StopCastingAudio (void);
STATIC eBTRMgrRet btrMgr_SwitchCastingAudio_AC (BTRMGR_StreamOut_Type_t aenCurrentSoType);
STATIC eBTRMgrRet btrMgr_StartReceivingAudio (int inFileFd, int inMTUSize, unsigned int inDevDelay, eBTRCoreDevMediaType aenBtrCoreDevInMType, void* apstBtrCoreDevInMCodecInfo);
STATIC eBTRMgrRet btrMgr_StopReceivingAudio (void);
STATIC eBTRMgrRet btrMgr_UpdateDynamicDelay(unsigned short newDelay);
STATIC eBTRMgrRet btrMgr_StartPacketCapture(const char * pcFolderPath);
STATIC eBTRMgrRet btrMgr_StopPacketCapture();
STATIC eBTRMgrRet btrMgr_StartHidEventMonitor();
STATIC eBTRMgrRet btrMgr_StopHidEventMonitor();
#ifndef BUILD_FOR_PI
STATIC eBTRMgrRet btrMgr_SetProcessLogLevel(char * pcProcessName, char * pcComponent, char * pcLogLevel);
#endif
#endif
STATIC eBTRMgrRet btrMgr_ConnectToDevice (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DeviceOperationType_t connectAs, unsigned int aui32ConnectRetryIdx, unsigned int aui32ConfirmIdx);
#ifndef LE_MODE
STATIC eBTRMgrRet btrMgr_StartAudioStreamingOut (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DeviceOperationType_t streamOutPref, unsigned int aui32ConnectRetryIdx, unsigned int aui32ConfirmIdx, unsigned int aui32SleepIdx);
#endif
STATIC eBTRMgrRet btrMgr_AddPersistentEntry(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, const char* apui8ProfileStr, int ai32DevConnected);
STATIC eBTRMgrRet btrMgr_RemovePersistentEntry(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, const char* apui8ProfileStr);
#ifndef LE_MODE
STATIC eBTRMgrRet btrMgr_MediaControl(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_MediaDeviceStatus_t* apstMediaDeviceStatus, enBTRCoreDeviceType aenBtrCoreDevTy, enBTRCoreDeviceClass aenBtrCoreDevCl, stBTRCoreMediaCtData *apstBtrCoreMediaCData);
#endif
#if 0
static void btrMgr_AddStandardAdvGattInfo(void);

static void btrMgr_AddColumboGATTInfo(void);
#endif

STATIC BTRMGR_SysDiagChar_t btrMgr_MapUUIDtoDiagElement(char *aUUID);

/*  Local Op Threads Prototypes */
static gpointer btrMgr_g_main_loop_Task (gpointer appvMainLoop);


/* Incoming Callbacks Prototypes */
static gboolean btrMgr_GetDeviceDisconnectStatusCb (gpointer gptr);
#ifdef AUTO_CONNECT_ENABLED
static gboolean btrMgr_AutoconnectOnStartUpStatusCb (gpointer gptr);
#endif //AUTO_CONNECT_ENABLED
static gboolean btrMgr_DiscoveryHoldOffTimerCb (gpointer gptr);
static gboolean btrMgr_ConnPwrStChangeTimerCb (gpointer gptr);
STATIC gboolean btrMgr_PairCompleteRstTimerCb (gpointer gptr);
STATIC gboolean btrmgr_DisconnectDeviceTimerCb (gpointer gptr);
static gboolean btrMgr_CheckDisconnectionStatus (gpointer gptr);
static gboolean btrMgr_PostOutOfRangeHoldOffTimerCb(gpointer user_data);
static gboolean btrMgr_PostDeviceFoundEvtTimerCb (gpointer user_data);
static gboolean btrMgr_ClearLastPairedStatus(gpointer user_data);
#ifdef RDKTV_PERSIST_VOLUME
static gboolean btrMgr_SetVolumeHoldOffTimerCb (gpointer gptr);
static gboolean ResetVolumeUpdateFlagCb(gpointer data);
static gboolean btrMgr_ClearConnectionInProgressflagCb(gpointer data);
#endif

#ifndef LE_MODE
STATIC eBTRMgrRet btrMgr_ACDataReadyCb (void* apvAcDataBuf, unsigned int aui32AcDataLen, void* apvUserData);
STATIC eBTRMgrRet btrMgr_ACStatusCb (stBTRMgrMediaStatus* apstBtrMgrAcStatus, void* apvUserData);
STATIC eBTRMgrRet btrMgr_SOStatusCb (stBTRMgrMediaStatus* apstBtrMgrSoStatus, void* apvUserData);
#ifdef STREAM_IN_SUPPORTED
STATIC eBTRMgrRet btrMgr_SIStatusCb (stBTRMgrMediaStatus* apstBtrMgrSiStatus, void* apvUserData);
#endif
#endif
STATIC eBTRMgrRet btrMgr_SDStatusCb (stBTRMgrSysDiagStatus* apstBtrMgrSdStatus, void* apvUserData);

STATIC enBTRCoreRet btrMgr_DeviceStatusCb (stBTRCoreDevStatusCBInfo* p_StatusCB, void* apvUserData);
STATIC enBTRCoreRet btrMgr_DeviceDiscoveryCb (stBTRCoreDiscoveryCBInfo* astBTRCoreDiscoveryCbInfo, void* apvUserData);
STATIC enBTRCoreRet btrMgr_ConnectionInIntimationCb (stBTRCoreConnCBInfo* apstConnCbInfo, int* api32ConnInIntimResp, void* apvUserData);
STATIC enBTRCoreRet btrMgr_ConnectionInAuthenticationCb (stBTRCoreConnCBInfo* apstConnCbInfo, int* api32ConnInAuthResp, void* apvUserData);
#ifndef LE_MODE
STATIC enBTRCoreRet btrMgr_MediaStatusCb (stBTRCoreMediaStatusCBInfo* mediaStatusCB, void* apvUserData);
#endif


STATIC eBTRMgrRet btrMgr_SetLastConnectionStatus(unsigned char aui8AdapterIdx, int ConStatus,BTRMgrDeviceHandle deviceID,const char *ProfileStr);
#ifdef RDKTV_PERSIST_VOLUME
STATIC eBTRMgrRet btrMgr_SetLastVolume(unsigned char aui8AdapterIdx, unsigned char ui8Volume,BTRMgrDeviceHandle deviceID,const char *ProfileStr);
STATIC eBTRMgrRet btrMgr_GetLastVolume(unsigned char aui8AdapterIdx, unsigned char *pVolume,BTRMgrDeviceHandle deviceID,const char *ProfileStr);
STATIC eBTRMgrRet btrMgr_SetLastMuteState(unsigned char aui8AdapterIdx, gboolean Mute);
STATIC eBTRMgrRet btrMgr_GetLastMuteState(unsigned char aui8AdapterIdx, gboolean *pMute);
#endif

#ifdef LE_MODE
// timer function for start notify, the usrData could be UUID
static gboolean btrMgr_ProvisionNotifyUpdateCb(gpointer usrData);
static gboolean btrMgr_CheckDeviceActivationStatus(gpointer user_data);
static gboolean btrMgr_GetDiscoveredBatteryDevices(gpointer user_data);
static gboolean btrMgr_ConnectBatteryDevices(gpointer user_data);
STATIC eBTRMgrRet btrMgr_BatteryOperations(void);
static gboolean btrMgr_CheckBatteryConnectionStatus(gpointer user_data);
static gboolean btrMgr_BatteryTriggerStartNotify(gpointer user_data);
static gboolean btrMgr_CheckFirmwareVersion(gpointer user_data);
static gboolean btrMgr_StartBatteryFirmwareUpgrade(gpointer user_data);
static BTRMGR_Result_t BTRMGR_LE_RemoveServiceInfo(unsigned char aui8AdapterIdx,char *aUUID);
#endif


STATIC BTRMGR_Result_t BTRMGR_LE_ReleaseAdvertisement (unsigned char aui8AdapterIdx);

STATIC void btrMgr_RemoveChars(char *src, char ch)
{
    if (NULL != src)
    {
        int wIdx = 0, rIdx = 0;
        while (src[rIdx])
        {
            if (src[rIdx] != ch)
            {
                src[wIdx++] = toupper(src[rIdx]);
            }

            rIdx++;
        }

        src[wIdx] = 0;
    }
}

STATIC void btrMgr_SetCMMac(unsigned char *devMac, const char* mac)
{
    char macStr[BTRMGR_MAX_STR_LEN] = {"\0"};
    strncpy(macStr, mac, BTRMGR_MAX_STR_LEN - 1);
    btrMgr_RemoveChars(macStr, ':');
    const char *pos = macStr;
    const char *hexstring = macStr;
    unsigned char *devMacPos = devMac;
    while (*pos)
    {
        if (!((pos - hexstring) & 1))
            sscanf(pos, "%02x", (unsigned int *)devMacPos++);
        ++pos;
    }
    BTRMGRLOG_INFO("CM Mac :\n");
    for (int i = 0; i < 6; i++)
    {
        BTRMGRLOG_INFO("0x%02x\n", *(devMac + i));
    }
}

/* STATIC Function Definitions */
STATIC inline unsigned char
btrMgr_GetAdapterCnt (
    void
) {
    return gListOfAdapters.number_of_adapters;
}

STATIC const char* 
btrMgr_GetAdapterPath (
    unsigned char   aui8AdapterIdx
) {
    const char* pReturn = NULL;

    if (gListOfAdapters.number_of_adapters) {
        if ((aui8AdapterIdx < gListOfAdapters.number_of_adapters) && (aui8AdapterIdx < BTRCORE_MAX_NUM_BT_ADAPTERS)) {
            pReturn = gListOfAdapters.adapter_path[aui8AdapterIdx];
        }
    }

    return pReturn;
}

static inline void
btrMgr_SetAgentActivated (
    unsigned char aui8AgentActivated
) {
    gIsAgentActivated = aui8AgentActivated;
}

STATIC inline unsigned char
btrMgr_GetAgentActivated (
    void
) {
    return gIsAgentActivated;
}

#ifdef LE_MODE
STATIC void
btrMgr_CheckBroadcastAvailability(
    void
) {
#ifdef BUILD_FOR_PI
    char lpui8stateBuff[BTRMGR_STR_LEN_MAX] = {'\0'};
    BTRMGR_SysDiagChar_t   lenDiagElement = BTRMGR_SYS_DIAG_BLE_BROADCAST_STATUS;

    if (eBTRMgrSuccess != BTRMGR_SD_GetData(ghBTRMgrSdHdl, lenDiagElement, lpui8stateBuff)) {
        BTRMGRLOG_INFO("Error getting BLE.Broadcast RFC \n");
        return;
    }

    if ((strcmp(lpui8stateBuff, "true") == 0)) {
        gIsBroadcastEnable = TRUE;
    }
    else {
        gIsBroadcastEnable = FALSE;
        memset(lpui8stateBuff, '\0', sizeof(lpui8stateBuff));
    }

    BTRMGRLOG_INFO("Broadcast is : %s.\n", gIsBroadcastEnable ? "Enabled" : "Disabled");
#endif

#if 0
    RFC_ParamData_t param = {0};
    /* We shall make this api generic and macro defined tr181 params as we start to enable diff services based on RFC */
    WDMP_STATUS status = getRFCParameter("BTRMGR", "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.BLE.Broadcast.Enable", &param);

    if (status == WDMP_SUCCESS)
    {
        BTRMGRLOG_DEBUG("name = %s, type = %d, value = %s\n", param.name, param.type, param.value);

        if (!strncmp(param.value, "true", strlen("true")))
        {
            gIsBroadcastEnable = TRUE;
            BTRMGRLOG_INFO("BLE Broadcast is enabled.\n");
        }
        else
        {
            BTRMGRLOG_INFO("BLE Broadcast is NOT enabled.\n");
            gIsBroadcastEnable = FALSE;
        }
    }
    else
    {
        BTRMGRLOG_ERROR("getRFCParameter Failed : %s\n", getRFCErrorString(status));
    }
#endif
}

BTRMGR_Result_t
BTRMGR_SetBroadcastState(
    unsigned char aui8AdapterIdx,
    unsigned char aui8State)
{
    gIsBroadcastEnable = ((aui8State == 0) ? FALSE : TRUE);
    BTRMGRLOG_INFO("Broadcast is : %s.\n", gIsBroadcastEnable ? "Enabled" : "Disabled");
    BTRMGR_SysDiagChar_t   lenDiagElement = BTRMGR_SYS_DIAG_BLE_BROADCAST_STATUS;

    if (eBTRMgrSuccess != BTRMGR_SD_SetData(ghBTRMgrSdHdl, lenDiagElement, &aui8State)) {
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return BTRMGR_RESULT_SUCCESS;
}
#endif

#ifndef LE_MODE
STATIC void
btrMgr_CheckAudioInServiceAvailability (
    void
) {
#ifdef BUILD_FOR_PI
   //Since RFC is not enabled for PI devices, enabling by default
    gIsAudioInEnabled = 1;
    BTRMGRLOG_INFO ("Enabling BTR AudioIn Service for raspberry pi devices.\n");
#else
    RFC_ParamData_t param = {0};
    /* We shall make this api generic and macro defined tr181 params as we start to enable diff services based on RFC */
    WDMP_STATUS status = getRFCParameter("BTRMGR", "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.BTR.AudioIn.Enable", &param);

    if (status == WDMP_SUCCESS) {
        BTRMGRLOG_DEBUG ("name = %s, type = %d, value = %s\n", param.name, param.type, param.value);

        if (!strncmp(param.value, "true", strlen("true"))) {
            gIsAudioInEnabled = 1;
            BTRMGRLOG_INFO ("BTR AudioIn Serivce will be available.\n");
        }
        else {
            BTRMGRLOG_INFO ("BTR AudioIn Serivce will not be available.\n");
        }
    }
    else {
        BTRMGRLOG_ERROR ("getRFCParameter Failed : %s\n", getRFCErrorString(status));
    }
#endif
}
#endif

#ifndef LE_MODE
STATIC void
btrMgr_CheckHidGamePadServiceAvailability (
    void
) {
#ifdef BUILD_FOR_PI
   //Since RFC is not enabled for PI devices, enabling by default
    gIsHidGamePadEnabled = 1;
    BTRMGRLOG_INFO ("Enabling BTR HidGamePad Service for raspberry pi devices.\n");
#else
    RFC_ParamData_t param = {0};
    /* We shall make this api generic and macro defined tr181 params as we start to enable diff services based on RFC */
    WDMP_STATUS status = getRFCParameter("BTRMGR", "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.BTR.GamePad.Enable", &param);

    if (status == WDMP_SUCCESS || status == WDMP_ERR_DEFAULT_VALUE) {
        if(status == WDMP_ERR_DEFAULT_VALUE){
            BTRMGRLOG_INFO ("getRFCParameter : Default value\n");
        }
        BTRMGRLOG_DEBUG ("name = %s, type = %d, value = %s\n", param.name, param.type, param.value);

        if (!strncmp(param.value, "true", strlen("true"))) {
            gIsHidGamePadEnabled = 1;
            BTRMGRLOG_INFO ("BTR HidGamePad Serivce will be available.\n");
        }
        else {
            BTRMGRLOG_INFO ("BTR HidGamePad Serivce will not be available.\n");
        }
    }
    else {
        BTRMGRLOG_ERROR ("getRFCParameter Failed : %s\n", getRFCErrorString(status));
    }
#endif
}

STATIC void
btrMgr_CheckDebugModeAvailability (
    void
) {
#ifdef BUILD_FOR_PI
   //Since RFC is not enabled for PI devices, disabling by default - can be enabled through test tool
    gDebugModeEnabled = 0;
    BTRMGRLOG_INFO ("Disabling BTR Debug mode for raspberry pi devices.\n");
#else
    RFC_ParamData_t param = {0};
    /* We shall make this api generic and macro defined tr181 params as we start to enable diff services based on RFC */
    WDMP_STATUS status = getRFCParameter("BTRMGR", "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.BTR.DebugMode.Enable", &param);

    if (status == WDMP_SUCCESS || status == WDMP_ERR_DEFAULT_VALUE) {
        if(status == WDMP_ERR_DEFAULT_VALUE){
            BTRMGRLOG_INFO ("getRFCParameter : Default value\n");
        }
        BTRMGRLOG_DEBUG ("name = %s, type = %d, value = %s\n", param.name, param.type, param.value);

        if (!strncmp(param.value, "true", strlen("true"))) {
            gDebugModeEnabled = 1;
            BTRMGRLOG_INFO ("BTR Debug Mode will turned on.\n");
            BTRMGR_SetBtmgrDebugModeState(gDebugModeEnabled);
        }
        else {
            BTRMGRLOG_INFO ("BTR Debug Mode will not be available.\n");
        }
    }
    else {
        BTRMGRLOG_ERROR ("getRFCParameter Failed : %s\n", getRFCErrorString(status));
    }
#endif
}
#endif

STATIC const char*
btrMgr_GetDiscoveryDeviceTypeAsString (
    BTRMGR_DeviceOperationType_t    adevOpType
) {
    char* opType = NULL;

    switch (adevOpType) {
    case BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT:
        opType = "AUDIO_OUT";
        break;
    case BTRMGR_DEVICE_OP_TYPE_AUDIO_INPUT:
        opType = "AUDIO_IN";
        break;
    case BTRMGR_DEVICE_OP_TYPE_LE:
        opType = "LE";
        break;
    case BTRMGR_DEVICE_OP_TYPE_HID:
        opType = "HID";
        break;
    case BTRMGR_DEVICE_OP_TYPE_AUDIO_AND_HID:
        opType = "AUDIO_OUT+HID";
        break;
    case BTRMGR_DEVICE_OP_TYPE_UNKNOWN:
        opType = "UNKNOWN";
    }

    return opType;
}

#if 0
static const char*
btrMgr_GetDiscoveryFilterAsString (
    BTRMGR_ScanFilter_t ascanFlt
) {
    char* filter = NULL;

    switch (ascanFlt) {
    case BTRMGR_DISCOVERY_FILTER_UUID:
        filter = "UUID";
        break;
    case BTRMGR_DISCOVERY_FILTER_RSSI:
        filter = "RSSI";
        break;
    case BTRMGR_DISCOVERY_FILTER_PATH_LOSS:
        filter = "PATH_LOSS";
        break;
    case BTRMGR_DISCOVERY_FILTER_SCAN_TYPE:
        filter = "SCAN_TYPE";
    }

    return filter;
}
#endif

STATIC const char*
btrMgr_GetDiscoveryStateAsString (
    BTRMGR_DiscoveryState_t         aScanStatus
) {
    char* state = NULL;

    switch (aScanStatus) { 
    case BTRMGR_DISCOVERY_ST_STARTED:
        state = "ST_STARTED";
        break;
    case BTRMGR_DISCOVERY_ST_PAUSED:
        state = "ST_PAUSED";
        break;
    case BTRMGR_DISCOVERY_ST_RESUMED:
        state = "ST_RESUMED";
        break;
    case BTRMGR_DISCOVERY_ST_STOPPED:
        state = "ST_STOPPED";
        break;
    case BTRMGR_DISCOVERY_ST_UNKNOWN:
    default:
        state = "ST_UNKNOWN";
    }

    return state;
}

static inline void
btrMgr_SetBgDiscoveryType (
    BTRMGR_DeviceOperationType_t    adevOpType
) {
    gBgDiscoveryType = adevOpType;
}

static inline void
btrMgr_SetDiscoveryState (
    BTRMGR_DiscoveryHandle_t*   ahdiscoveryHdl,
    BTRMGR_DiscoveryState_t     aScanStatus
) {
    ahdiscoveryHdl->m_disStatus = aScanStatus;
}

static inline void
btrMgr_SetDiscoveryDeviceType (
    BTRMGR_DiscoveryHandle_t*       ahdiscoveryHdl,
    BTRMGR_DeviceOperationType_t    aeDevOpType
) {
    ahdiscoveryHdl->m_devOpType = aeDevOpType;
}

static inline BTRMGR_DeviceOperationType_t
btrMgr_GetBgDiscoveryType (
    void
) {
    return gBgDiscoveryType;
}

static inline BTRMGR_DiscoveryState_t
btrMgr_GetDiscoveryState (
    BTRMGR_DiscoveryHandle_t*   ahdiscoveryHdl
) {
    return ahdiscoveryHdl->m_disStatus;
}

static inline BTRMGR_DeviceOperationType_t
btrMgr_GetDiscoveryDeviceType (
    BTRMGR_DiscoveryHandle_t*   ahdiscoveryHdl
) {
    return ahdiscoveryHdl->m_devOpType;
}

static inline void
btrMgr_SetDiscoveryHandle (
    BTRMGR_DeviceOperationType_t    aDevOpType,
    BTRMGR_DiscoveryState_t         aScanStatus
) {
    BTRMGR_DiscoveryHandle_t*   ldiscoveryHdl = NULL;

    if (aDevOpType == btrMgr_GetBgDiscoveryType()) {
        ldiscoveryHdl = &ghBTRMgrBgDiscoveryHdl;
    }
    else {
        ldiscoveryHdl = &ghBTRMgrDiscoveryHdl;
    }


    if (btrMgr_GetDiscoveryState(ldiscoveryHdl) != BTRMGR_DISCOVERY_ST_PAUSED) {
        btrMgr_SetDiscoveryDeviceType (ldiscoveryHdl, aDevOpType);
        btrMgr_SetDiscoveryState (ldiscoveryHdl, aScanStatus);
        // set Filter in the handle from the global Filter
    }
}

#if 0
static inline BTRMGR_DiscoveryFilterHandle_t*
btrMgr_GetDiscoveryFilter (
    BTRMGR_DiscoveryHandle_t*   ahdiscoveryHdl
) {
    return &ahdiscoveryHdl->m_disFilter;
}
#endif

static inline gboolean
btrMgr_isTimeOutSet (
    void
) {
    return (gTimeOutRef > 0) ? TRUE : FALSE;
}

static inline void
btrMgr_ClearDiscoveryHoldOffTimer (
    void
) {
    if (gTimeOutRef) {
        BTRMGRLOG_DEBUG ("Cancelling previous Discovery hold off TimeOut Session : %u\n", gTimeOutRef);
        g_source_destroy(g_main_context_find_source_by_id(gmainContext, gTimeOutRef));
        gTimeOutRef = 0;
    }
}

static inline void
btrMgr_SetDiscoveryHoldOffTimer (
    unsigned char   aui8AdapterIdx
) {
    GSource* source = g_timeout_source_new(BTRMGR_DISCOVERY_HOLD_OFF_TIME * 1000);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(source, btrMgr_DiscoveryHoldOffTimerCb, (gpointer)&gDiscHoldOffTimeOutCbData, NULL);

    gTimeOutRef = g_source_attach(source, gmainContext);
    g_source_unref(source);

    BTRMGRLOG_WARN ("DiscoveryHoldOffTimeOut set to  +%u  seconds || TimeOutReference : %u\n", BTRMGR_DISCOVERY_HOLD_OFF_TIME, gTimeOutRef);
}

static inline void
btrMgr_ClearDisconnDevHoldOffTimer (
    void
) {
    if (gAuthDisconnDevTimeOutRef) {
        BTRMGRLOG_DEBUG ("Cancelling previous Disconnect hold off TimeOut Session : %u\n", gAuthDisconnDevTimeOutRef);
        g_source_destroy(g_main_context_find_source_by_id(gmainContext, gAuthDisconnDevTimeOutRef));
        gAuthDisconnDevTimeOutRef = 0;
    }
}

static inline void
btrMgr_SetDisconnectStatusHoldOffTimer (void)
{
    GSource* source = g_timeout_source_new(BTRMGR_DISCONNECT_CLEAR_STATUS_TIME_INTERVAL * 1000);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(source, (GSourceFunc)btrMgr_CheckDisconnectionStatus, NULL, NULL);
    gDisconnTimeoutRef = g_source_attach(source,(void *)gmainContext);
    BTRMGRLOG_INFO("Disconnection clear status timeout - %u TimeOutRef - %u\n",BTRMGR_DISCONNECT_CLEAR_STATUS_TIME_INTERVAL,gDisconnTimeoutRef);
}

static inline void
btrMgr_ClearDisconnectStatusHoldOffTimer (void)
{
    if (gDisconnTimeoutRef) {
        BTRMGRLOG_DEBUG ("Cancelling Disconnect Status HoldOff Timer Session : %u\n",gDisconnTimeoutRef);
        g_source_destroy(g_main_context_find_source_by_id(gmainContext,gDisconnTimeoutRef));
    }
}

static inline void
btrMgr_SetLastPairedDeviceStatusHoldOffTimer (void)
{
    GSource* source = g_timeout_source_new(BTRMGR_CLEAR_LAST_PAIRED_STATUS_INTERVAL * 1000);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(source, (GSourceFunc)btrMgr_ClearLastPairedStatus, NULL, NULL);
    gLastPairedTimeoutRef = g_source_attach(source,(void *)gmainContext);
    BTRMGRLOG_INFO("Clear Last Paired status timeout - %u TimeOutRef - %u\n",BTRMGR_CLEAR_LAST_PAIRED_STATUS_INTERVAL,gLastPairedTimeoutRef);
}

static inline void
btrMgr_ClearLastPairedDeviceStatusHoldOffTimer (void)
{
    if (gLastPairedTimeoutRef) {
        BTRMGRLOG_DEBUG ("Cancelling Last Paired Status HoldOff Timer Session : %u\n",gLastPairedTimeoutRef);
        g_source_destroy(g_main_context_find_source_by_id(gmainContext,gLastPairedTimeoutRef));
    }
}

static inline void
btrMgr_SetDisconnDevHoldOffTimer (
    tBTRCoreDevId    aBTRCoreDevId
) {
    tBTRCoreDevId lBTRCoreDevId = aBTRCoreDevId;

    GSource* source = g_timeout_source_new(5000);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(source, btrmgr_DisconnectDeviceTimerCb, (gpointer)&lBTRCoreDevId, NULL);

    gAuthDisconnDevTimeOutRef = g_source_attach(source, gmainContext);
    g_source_unref(source);

    BTRMGRLOG_WARN ("DisconnectHoldOffTimeOut set to  1 second || TimeOutReference : %u\n", gAuthDisconnDevTimeOutRef);
}

static inline void
btrMgr_SetDeviceDisStatusHoldOffTimer (
   void
) {
   GSource* source = g_timeout_source_new(BTRMGR_DEVICE_DISCONNECT_STATUS_TIME_INTERVAL * 1000);
   g_source_set_priority(source, G_PRIORITY_DEFAULT);
   g_source_set_callback(source, btrMgr_GetDeviceDisconnectStatusCb , NULL, NULL);

   gGetDevDisStatusTimeoutRef = g_source_attach(source, gmainContext);
   g_source_unref(source);

   BTRMGRLOG_INFO ("DeviceDisconnectStatusHoldOffTimeOut - %u TimeOutReference - %u\n",BTRMGR_DEVICE_DISCONNECT_STATUS_TIME_INTERVAL,gGetDevDisStatusTimeoutRef);
}

static inline void btrMgr_ClearDeviceOORHoldOffTimer(void)
{
   if (gOORTimeOutRef) {
       BTRMGRLOG_DEBUG ("Cancelling previous OOR hold off TimeOut Session : %u\n", gOORTimeOutRef);
       g_source_destroy(g_main_context_find_source_by_id(gmainContext, gOORTimeOutRef));
       gOORTimeOutRef = 0;
   }
}

#ifdef AUTO_CONNECT_ENABLED
static inline void
btrMgr_SetAutoconnectOnStartUpTimer (
   void
) {
   GSource* source = g_timeout_source_new(BTRMGR_AUTOCONNECT_ON_STARTUP_TIMEOUT * 1000);
   g_source_set_priority(source, G_PRIORITY_DEFAULT);
   g_source_set_callback(source, btrMgr_AutoconnectOnStartUpStatusCb , NULL, NULL);

   gAutoConnStartUpTimeoutRef = g_source_attach(source, gmainContext);
   g_source_unref(source);

   BTRMGRLOG_INFO ("Autoconnect on start up timeout - %u TimeOutReference - %u\n",BTRMGR_AUTOCONNECT_ON_STARTUP_TIMEOUT,gAutoConnStartUpTimeoutRef);
}

static inline void btrMgr_ClearAutoconnectOnStartUpTimer(void)
{
   if (gAutoConnStartUpTimeoutRef) {
       BTRMGRLOG_DEBUG ("Cancelling Autoconnect start up retry Session : %u\n", gAutoConnStartUpTimeoutRef);
       g_source_destroy(g_main_context_find_source_by_id(gmainContext, gAutoConnStartUpTimeoutRef));
       gAutoConnStartUpTimeoutRef = 0;
   }
}
#endif // AUTO_CONNECT_ENABLED

#ifdef RDKTV_PERSIST_VOLUME
static inline void btrMgr_ResetSkipVolumeUpdateTimer(void)
{
    GSource* source = g_timeout_source_new(BTRMGR_SKIP_VOLUME_UPDATE_INTERVAL * 1000);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(source, (GSourceFunc)ResetVolumeUpdateFlagCb, NULL, NULL);

    gSkipVolumeUpdateTimeoutRef = g_source_attach(source, gmainContext);
    g_source_unref(source);

    BTRMGRLOG_INFO (" gSkipVolumeUpdateTimeoutRef set to  +%u  seconds || TimeOutReference : %u\n", BTRMGR_SKIP_VOLUME_UPDATE_INTERVAL, gSkipVolumeUpdateTimeoutRef);
}

static inline void btrMgr_ClearSkipVolumeUpdateTimer(void)
{
    if (gSkipVolumeUpdateTimeoutRef) {
        BTRMGRLOG_DEBUG ("Cancelling previous volume update hold off TimeOut Session : %u\n", gSkipVolumeUpdateTimeoutRef);
        g_source_destroy(g_main_context_find_source_by_id(gmainContext, gSkipVolumeUpdateTimeoutRef));
        gSkipVolumeUpdateTimeoutRef = 0;
    }
}

static inline void btrMgr_SetVolumeHoldOffTimer(stBTRCoreMediaStatusCBInfo*  mediaStatusCB)
{
	GSource* source = g_timeout_source_new(BTRMGR_DEFAULT_SET_VOLUME_INTERVAL * 1000);
        g_source_set_priority(source, G_PRIORITY_DEFAULT);
        g_source_set_callback(source, (GSourceFunc)btrMgr_SetVolumeHoldOffTimerCb, (gpointer)mediaStatusCB, NULL);

        gDefaultSetVolumeTimeoutRef = g_source_attach(source, gmainContext);
        g_source_unref(source);

        BTRMGRLOG_INFO (" gDefaultSetVolumeTimeoutRef set to  +%u  seconds || TimeOutReference : %u\n", BTRMGR_DEFAULT_SET_VOLUME_INTERVAL, gDefaultSetVolumeTimeoutRef);
}
static inline void btrMgr_ClearVolumeHoldOffTimer(void)
{
   if (gDefaultSetVolumeTimeoutRef) {
       BTRMGRLOG_DEBUG ("Cancelling previous volume set hold off TimeOut Session : %u\n", gDefaultSetVolumeTimeoutRef);
       g_source_destroy(g_main_context_find_source_by_id(gmainContext, gDefaultSetVolumeTimeoutRef));
       gDefaultSetVolumeTimeoutRef = 0;
   }
}
static inline void btrMgr_SetConInProgressStatusHoldOffTimer(void)
{
    GSource* source = g_timeout_source_new(BTRMGR_DEFAULT_CONNECTION_IN_PROGRESS_INTERVAL * 1000);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(source, (GSourceFunc)btrMgr_ClearConnectionInProgressflagCb, NULL, NULL);

    gDefaultConInProgressTimeoutRef = g_source_attach(source, gmainContext);
    g_source_unref(source);

    BTRMGRLOG_INFO (" gDefaultConInProgressTimeoutRef set to  +%u  seconds || TimeOutReference : %u\n", BTRMGR_DEFAULT_CONNECTION_IN_PROGRESS_INTERVAL, gDefaultConInProgressTimeoutRef);

}
static inline void btrMgr_ClearConInProgressStatusHoldOffTimer(void)
{
    if (gDefaultConInProgressTimeoutRef) {
        BTRMGRLOG_DEBUG ("Cancelling connection In Progress TimeOut Session : %u\n", gDefaultConInProgressTimeoutRef);
        g_source_destroy(g_main_context_find_source_by_id(gmainContext, gDefaultConInProgressTimeoutRef));
        gDefaultConInProgressTimeoutRef = 0;
    }
}
#endif
static inline void btrMgr_PostDeviceOORHoldOffTimer(stBTRCoreDevStatusCBInfo*   p_StatusCB)
{
    GSource* source = g_timeout_source_new(BTRMGR_POST_OUT_OF_RANGE_HOLD_OFF_TIME * 1000);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(source, (GSourceFunc)btrMgr_PostOutOfRangeHoldOffTimerCb, (gpointer)p_StatusCB, NULL);

    gOORTimeOutRef = g_source_attach(source, gmainContext);
    g_source_unref(source);

    BTRMGRLOG_INFO (" gOORTimeOut set to  +%u  seconds || TimeOutReference : %u\n", BTRMGR_POST_OUT_OF_RANGE_HOLD_OFF_TIME, gOORTimeOutRef);
}

#ifdef LE_MODE
static inline void
btrMgr_SetDiscoverbatteryDevicesHoldOffTimer (
    void
) {
    GSource* source = g_timeout_source_new(BTRMGR_BATTERY_DISCOVERY_TIME_INTERVAL * 1000); //TODO: Shouldn't this be BTRMGR_BATTERY_DISCOVERY_TIME_INTERVAL ??
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(source, btrMgr_GetDiscoveredBatteryDevices , NULL, NULL);

    gGetDisBatDevTimeoutRef = g_source_attach(source, gmainContext);
    g_source_unref(source);

    BTRMGRLOG_INFO ("DiscoverBatteryDeviceHoldOffTimeOut - %u TimeOutReference - %u\n",BTRMGR_BATTERY_DISCOVERY_TIME_INTERVAL,gGetDisBatDevTimeoutRef);
}

static inline void
btrMgr_SetConnectBatteryHoldOffTimer (
    void
) {
    GSource* source = g_timeout_source_new(BTRMGR_BATTERY_CONNECT_TIME_INTERVAL * 1000);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(source, btrMgr_ConnectBatteryDevices , NULL, NULL);

    gConBatDevTimeoutRef = g_source_attach(source, gmainContext);
    g_source_unref(source);

    BTRMGRLOG_INFO ("ConnectBatteryDeviceHoldOffTimeOut - %u TimeOutReference - %u\n",BTRMGR_BATTERY_CONNECT_TIME_INTERVAL,gConBatDevTimeoutRef);
}

static inline void
btrMgr_SetBatteryConnectionStatusHoldOffTimer (void)
{
    gBatConnStatusTimeotRef = g_timeout_add_seconds(BTRMGR_BATTERY_CONNECT_TIME_INTERVAL,btrMgr_CheckBatteryConnectionStatus,NULL);
    BTRMGRLOG_INFO ("ConnectionStatusDeviceHoldOffTimeOut - %u TimeOutReference - %u\n",BTRMGR_BATTERY_CONNECT_TIME_INTERVAL,gBatConnStatusTimeotRef);
}

static inline void
btrMgr_SetBatteryStartNotifyHoldOffTimer (void)
{
    gBatStartNotifyTimeoutRef = g_timeout_add_seconds(BTRMGR_BATTERY_START_NOTIFY_TIME_INTERVAL,btrMgr_BatteryTriggerStartNotify,NULL);
    BTRMGRLOG_INFO ("TriggerStartNotifyHoldOffTimeOut - %u TimeOutReference - %u\n",BTRMGR_BATTERY_START_NOTIFY_TIME_INTERVAL,gBatStartNotifyTimeoutRef);
}

static inline void
btrMgr_ClearDiscoverbatteryDevicesHoldOffTimer (
    void
) {
    if (gGetDisBatDevTimeoutRef) {
        BTRMGRLOG_DEBUG ("Cancelling Battery Discovery hold off TimeOut Session : %u\n", gGetDisBatDevTimeoutRef);
        g_source_destroy(g_main_context_find_source_by_id(gmainContext, gGetDisBatDevTimeoutRef));
        gGetDisBatDevTimeoutRef = 0;
    }
}

static inline void
btrMgr_ClearConnectBatteryHoldOffTimer (
    void
) {
    if (gConBatDevTimeoutRef) {
        BTRMGRLOG_DEBUG ("Cancelling Battery Connect hold off TimeOut Session : %u\n", gConBatDevTimeoutRef);
        g_source_destroy(g_main_context_find_source_by_id(gmainContext, gConBatDevTimeoutRef));
        gConBatDevTimeoutRef = 0;
    }
}
static inline void
btrMgr_ClearBatteryConnectionStatusHoldOffTimer (void)
{
    if (gBatConnStatusTimeotRef) {
        BTRMGRLOG_DEBUG ("Cancelling Battery Discovery hold off TimeOut Session : %u\n", gBatConnStatusTimeotRef);
        g_source_remove (gBatConnStatusTimeotRef);
        gBatConnStatusTimeotRef = 0;
    }
}

static inline void
btrMgr_ClearBatteryStartNotifyHoldOffTimer (void)
{
    if (gBatStartNotifyTimeoutRef) {
        BTRMGRLOG_DEBUG ("Cancelling Battery Start Notify hold off TimeOut Session : %u\n", gBatStartNotifyTimeoutRef);
        g_source_remove (gBatStartNotifyTimeoutRef);
        gBatStartNotifyTimeoutRef = 0;
    }
}

static inline void
btrMgr_StartBatteryFirmwareUpgradeHoldOffTimer (void)
{
    GSource* source = g_timeout_source_new(35 * 1000);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(source, (GSourceFunc)btrMgr_StartBatteryFirmwareUpgrade , NULL, NULL);
    gStartFirmUpTimeoutRef= g_source_attach(source,(void *)gmainContext);
    BTRMGRLOG_INFO ("StartBatteryFirmwareUpgradeHoldOffTimeOut - 20 TimeOutReference - %u\n",gStartFirmUpTimeoutRef);
}

static inline void
btrMgr_ClearBatteryFirmwareUpgradeHoldOffTimer (void)
{
     if (gStartFirmUpTimeoutRef) {
        BTRMGRLOG_DEBUG ("Cancelling Battery Firmware upgrade hold off TimeOut Session : %u\n", gStartFirmUpTimeoutRef);
        g_source_destroy(g_main_context_find_source_by_id((void *)gmainContext,gStartFirmUpTimeoutRef));
        gStartFirmUpTimeoutRef = 0;
    }
}

static inline void
btrMgr_CheckFirmwareVersionHoldOffTimer (void)
{
    GSource* source = g_timeout_source_new(300 * 1000);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(source, (GSourceFunc)btrMgr_CheckFirmwareVersion , NULL, NULL);
    gCheckFirmTimeoutRef= g_source_attach(source,(void *)gmainContext);
    BTRMGRLOG_INFO ("CheckFirmwareVersionHoldOffTimeOut - 300 TimeOutReference - %u\n",gCheckFirmTimeoutRef);
}

static inline void
btrMgr_ClearCheckFirmwareVersionHoldOffTimer (void)
{
     if(gCheckFirmTimeoutRef) {
        BTRMGRLOG_DEBUG ("Cancelling Check Battery Firmware Version hold off TimeOut Session : %u\n", gCheckFirmTimeoutRef);
        g_source_destroy(g_main_context_find_source_by_id((void *)gmainContext,gCheckFirmTimeoutRef));
        gCheckFirmTimeoutRef = 0;
    }
}
#endif

#if 0
static eBTRMgrRet
btrMgr_SetDiscoveryFilter (
    BTRMGR_DiscoveryHandle_t*   ahdiscoveryHdl,
    BTRMGR_ScanFilter_t         aeScanFilterType,
    void*                       aFilterValue
) {
    BTRMGR_DiscoveryFilterHandle_t* ldisFilter = btrMgr_GetDiscoveryFilter(ahdiscoveryHdl);

    if (btrMgr_GetDiscoveryState(ahdiscoveryHdl) != BTRMGR_DISCOVERY_ST_INITIALIZING){
        BTRMGRLOG_ERROR ("Not in Initializing state !!!. Current state is %s\n"
                        , btrMgr_GetDiscoveryStateAsString (btrMgr_GetDiscoveryState(ahdiscoveryHdl)));
        return eBTRMgrFailure;
    }

    switch (aeScanFilterType) {
    case BTRMGR_DISCOVERY_FILTER_UUID:
        ldisFilter->m_btuuid.m_uuid     = (char**) realloc (ldisFilter->m_btuuid.m_uuid, (sizeof(char*) * (++ldisFilter->m_btuuid.m_uuidCount)));
        ldisFilter->m_btuuid.m_uuid[ldisFilter->m_btuuid.m_uuidCount]   = (char*)  malloc  (BTRMGR_NAME_LEN_MAX);
        strncpy (ldisFilter->m_btuuid.m_uuid[ldisFilter->m_btuuid.m_uuidCount-1], (char*)aFilterValue, BTRMGR_NAME_LEN_MAX-1);
        break;
    case BTRMGR_DISCOVERY_FILTER_RSSI:
        ldisFilter->m_rssi      = *(short*)aFilterValue;
        break;
    case BTRMGR_DISCOVERY_FILTER_PATH_LOSS:
        ldisFilter->m_pathloss  = *(short*)aFilterValue;
        break;
    case BTRMGR_DISCOVERY_FILTER_SCAN_TYPE:
        ldisFilter->m_scanType  = *(BTRMGR_DeviceScanType_t*)aFilterValue;
    }

    BTRMGRLOG_DEBUG ("Discovery Filter is set successfully with the given %s...\n"
                    , btrMgr_GetDiscoveryFilterAsString(aeScanFilterType));

    return eBTRMgrSuccess;
}

static eBTRMgrRet
btrMgr_ClearDiscoveryFilter (
    BTRMGR_DiscoveryHandle_t*   ahdiscoveryHdl
) {
    BTRMGR_DiscoveryFilterHandle_t* ldisFilter = btrMgr_GetDiscoveryFilter(ahdiscoveryHdl);

    if (btrMgr_GetDiscoveryState(ahdiscoveryHdl) == BTRMGR_DISCOVERY_ST_INITIALIZED ||
        btrMgr_GetDiscoveryState(ahdiscoveryHdl) == BTRMGR_DISCOVERY_ST_STARTED     ||
        btrMgr_GetDiscoveryState(ahdiscoveryHdl) == BTRMGR_DISCOVERY_ST_RESUMED     ||
        btrMgr_GetDiscoveryState(ahdiscoveryHdl) == BTRMGR_DISCOVERY_ST_PAUSED      ){
        BTRMGRLOG_WARN ("Cannot clear Discovery Filter when Discovery is in %s\n"
                        , btrMgr_GetDiscoveryStateAsString(btrMgr_GetDiscoveryState(ahdiscoveryHdl)));
        return eBTRMgrFailure;
     }

    if (ldisFilter->m_btuuid.m_uuidCount) {
        while (ldisFilter->m_btuuid.m_uuidCount) {
            free (ldisFilter->m_btuuid.m_uuid[ldisFilter->m_btuuid.m_uuidCount-1]);
            ldisFilter->m_btuuid.m_uuid[ldisFilter->m_btuuid.m_uuidCount-1] = NULL;
            ldisFilter->m_btuuid.m_uuidCount--;
        }
        free (ldisFilter->m_btuuid.m_uuid);
    }

    ldisFilter->m_btuuid.m_uuid = NULL;
    ldisFilter->m_rssi          = 0;
    ldisFilter->m_pathloss      = 0;
    ldisFilter->m_scanType      = BTRMGR_DEVICE_SCAN_TYPE_AUTO;

    return eBTRMgrSuccess;
}
#endif

STATIC BTRMGR_DiscoveryHandle_t*
btrMgr_GetDiscoveryInProgress (
    void
) {
    BTRMGR_DiscoveryHandle_t*   ldiscoveryHdl = NULL;

    if (btrMgr_GetDiscoveryState(&ghBTRMgrDiscoveryHdl) == BTRMGR_DISCOVERY_ST_STARTED ||
        btrMgr_GetDiscoveryState(&ghBTRMgrDiscoveryHdl) == BTRMGR_DISCOVERY_ST_RESUMED ){
        ldiscoveryHdl = &ghBTRMgrDiscoveryHdl;
    }
    else if (btrMgr_GetDiscoveryState(&ghBTRMgrBgDiscoveryHdl) == BTRMGR_DISCOVERY_ST_STARTED ||
             btrMgr_GetDiscoveryState(&ghBTRMgrBgDiscoveryHdl) == BTRMGR_DISCOVERY_ST_RESUMED ){
        ldiscoveryHdl = &ghBTRMgrBgDiscoveryHdl;
    }

    if (ldiscoveryHdl) {
        BTRMGRLOG_DEBUG ("[%s] Scan in Progress...\n"
                        , btrMgr_GetDiscoveryDeviceTypeAsString (btrMgr_GetDiscoveryDeviceType(ldiscoveryHdl)));
    }

    return ldiscoveryHdl;
}

STATIC eBTRMgrRet
btrMgr_PauseDeviceDiscovery (
    unsigned char               aui8AdapterIdx,
    BTRMGR_DiscoveryHandle_t*   ahdiscoveryHdl
) {
    eBTRMgrRet  lenBtrMgrRet   = eBTRMgrSuccess;

    if (btrMgr_GetDiscoveryState(ahdiscoveryHdl) == BTRMGR_DISCOVERY_ST_STARTED ||
        btrMgr_GetDiscoveryState(ahdiscoveryHdl) == BTRMGR_DISCOVERY_ST_RESUMED ){

        if (BTRMGR_RESULT_SUCCESS == BTRMGR_StopDeviceDiscovery_Internal (aui8AdapterIdx, btrMgr_GetDiscoveryDeviceType(ahdiscoveryHdl))) {

            btrMgr_SetDiscoveryState (ahdiscoveryHdl, BTRMGR_DISCOVERY_ST_PAUSED);
            BTRMGRLOG_INFO ("[%s] Successfully Paused Scan\n"
                            , btrMgr_GetDiscoveryDeviceTypeAsString (btrMgr_GetDiscoveryDeviceType(ahdiscoveryHdl)));
        }
        else {
            BTRMGRLOG_ERROR ("[%s] Failed to Pause Scan\n"
                            , btrMgr_GetDiscoveryDeviceTypeAsString (btrMgr_GetDiscoveryDeviceType(ahdiscoveryHdl)));
            lenBtrMgrRet =  eBTRMgrFailure;
        }
    }
    return lenBtrMgrRet;
}

STATIC eBTRMgrRet
btrMgr_ResumeDeviceDiscovery (
    unsigned char               aui8AdapterIdx,
    BTRMGR_DiscoveryHandle_t*   ahdiscoveryHdl
) {
    eBTRMgrRet  lenBtrMgrRet   = eBTRMgrSuccess;

    if (btrMgr_GetDiscoveryState(ahdiscoveryHdl) != BTRMGR_DISCOVERY_ST_PAUSED) {
        BTRMGRLOG_WARN ("\n[%s] Device Discovery Resume is requested, but current state is %s !!!\n"
                        , btrMgr_GetDiscoveryDeviceTypeAsString (btrMgr_GetDiscoveryDeviceType(ahdiscoveryHdl))
                        , btrMgr_GetDiscoveryStateAsString (btrMgr_GetDiscoveryState(ahdiscoveryHdl)));
        BTRMGRLOG_WARN ("\n Still continuing to Resume Discovery\n");
    }
#if 0
    if (enBTRCoreSuccess != BTRCore_ApplyDiscoveryFilter (btrMgr_GetDiscoveryFilter(ahdiscoveryHdl))) {
        BTRMGRLOG_ERROR ("[%s] Failed to set Discovery Filter!!!"
                        , btrMgr_GetDiscoveryDeviceTypeAsString (btrMgr_GetDiscoveryDeviceType(ahdiscoveryHdl)));
        lenBtrMgrRet = eBTRMgrFailure;
    }
    else {
#endif
        if (BTRMGR_RESULT_SUCCESS == BTRMGR_StartDeviceDiscovery_Internal (aui8AdapterIdx, btrMgr_GetDiscoveryDeviceType(ahdiscoveryHdl))) {

            //TODO: Move before you make the call to StartDeviceDiscovery, store the previous state and restore the previous state in case of Failure
            btrMgr_SetDiscoveryState (ahdiscoveryHdl, BTRMGR_DISCOVERY_ST_RESUMED);
            BTRMGRLOG_INFO ("[%s] Successfully Resumed Scan\n"
                            , btrMgr_GetDiscoveryDeviceTypeAsString (btrMgr_GetDiscoveryDeviceType(ahdiscoveryHdl)));
        } else {
            BTRMGRLOG_ERROR ("[%s] Failed Resume Scan!!!\n"
                            , btrMgr_GetDiscoveryDeviceTypeAsString (btrMgr_GetDiscoveryDeviceType(ahdiscoveryHdl)));
            lenBtrMgrRet =  eBTRMgrFailure;
        }
#if 0
    }
#endif

    return lenBtrMgrRet;
}

STATIC eBTRMgrRet
btrMgr_StopDeviceDiscovery (
    unsigned char               aui8AdapterIdx,
    BTRMGR_DiscoveryHandle_t*   ahdiscoveryHdl
) {
    eBTRMgrRet  lenBtrMgrRet   = eBTRMgrSuccess;

    if (btrMgr_GetDiscoveryState(ahdiscoveryHdl) == BTRMGR_DISCOVERY_ST_STARTED ||
        btrMgr_GetDiscoveryState(ahdiscoveryHdl) == BTRMGR_DISCOVERY_ST_RESUMED ){

        if (BTRMGR_RESULT_SUCCESS == BTRMGR_StopDeviceDiscovery_Internal (aui8AdapterIdx, btrMgr_GetDiscoveryDeviceType(ahdiscoveryHdl))) {

            BTRMGRLOG_INFO ("[%s] Successfully Stopped scan\n"
                            , btrMgr_GetDiscoveryDeviceTypeAsString (btrMgr_GetDiscoveryDeviceType(ahdiscoveryHdl)));
        }
        else {
            BTRMGRLOG_ERROR ("[%s] Failed to Stop scan\n"
                            , btrMgr_GetDiscoveryDeviceTypeAsString (btrMgr_GetDiscoveryDeviceType(ahdiscoveryHdl)));
            lenBtrMgrRet =  eBTRMgrFailure;
        }
    }
    return lenBtrMgrRet;
}

STATIC eBTRMgrRet
btrMgr_PreCheckDiscoveryStatus (
    unsigned char                   aui8AdapterIdx,
    BTRMGR_DeviceOperationType_t    aDevOpType
) {
    eBTRMgrRet                lenBtrMgrRet  = eBTRMgrSuccess;
    BTRMGR_DiscoveryHandle_t* ldiscoveryHdl = NULL;

    if ((ldiscoveryHdl = btrMgr_GetDiscoveryInProgress())) {

        if ( btrMgr_GetDiscoveryDeviceType(ldiscoveryHdl) == btrMgr_GetBgDiscoveryType()) {
            BTRMGRLOG_WARN ("Calling btrMgr_PauseDeviceDiscovery\n");
            lenBtrMgrRet = btrMgr_PauseDeviceDiscovery (aui8AdapterIdx, ldiscoveryHdl);
        }
        else if (aDevOpType != btrMgr_GetBgDiscoveryType()) {
            BTRMGRLOG_WARN ("Calling btrMgr_StopDeviceDiscovery\n");
            lenBtrMgrRet = btrMgr_StopDeviceDiscovery (aui8AdapterIdx, ldiscoveryHdl);
        }
        else {
            BTRMGRLOG_WARN ("[%s] Scan in Progress.. Request for %s operation is rejected...\n"
                           , btrMgr_GetDiscoveryDeviceTypeAsString (btrMgr_GetDiscoveryDeviceType(ldiscoveryHdl))
                           , btrMgr_GetDiscoveryDeviceTypeAsString (aDevOpType));
            lenBtrMgrRet = eBTRMgrFailure;
        }
    }
    else if (btrMgr_isTimeOutSet()) {
        if (aDevOpType == btrMgr_GetBgDiscoveryType()) {
            BTRMGRLOG_WARN ("[NON-%s] Operation in Progress.. Request for %s operation is rejected...\n"
                            , btrMgr_GetDiscoveryDeviceTypeAsString (aDevOpType)
                            , btrMgr_GetDiscoveryDeviceTypeAsString (aDevOpType));
            lenBtrMgrRet = eBTRMgrFailure;
        }
    }

    if (aDevOpType != btrMgr_GetBgDiscoveryType()) {
        btrMgr_ClearDiscoveryHoldOffTimer();
    }

    return lenBtrMgrRet;
}

STATIC eBTRMgrRet
btrMgr_PostCheckDiscoveryStatus (
    unsigned char                   aui8AdapterIdx,
    BTRMGR_DeviceOperationType_t    aDevOpType
) {
    eBTRMgrRet  lenBtrMgrRet  = eBTRMgrSuccess;

    if (!btrMgr_isTimeOutSet()) {
        if (aDevOpType == BTRMGR_DEVICE_OP_TYPE_UNKNOWN) {
            if (btrMgr_GetDiscoveryState(&ghBTRMgrBgDiscoveryHdl) == BTRMGR_DISCOVERY_ST_PAUSED) {
                BTRMGRLOG_WARN ("Calling btrMgr_ResumeDeviceDiscovery\n");
                lenBtrMgrRet = btrMgr_ResumeDeviceDiscovery (aui8AdapterIdx, &ghBTRMgrBgDiscoveryHdl);
            }
        }
        else
        if (aDevOpType != btrMgr_GetBgDiscoveryType()) {
            if (btrMgr_GetDiscoveryState(&ghBTRMgrBgDiscoveryHdl) == BTRMGR_DISCOVERY_ST_PAUSED) {
                btrMgr_SetDiscoveryHoldOffTimer(aui8AdapterIdx);
            }
        }
    }

    return lenBtrMgrRet;
}


STATIC void
btrMgr_GetDiscoveredDevInfo (
    BTRMgrDeviceHandle          ahBTRMgrDevHdl,
    BTRMGR_DiscoveredDevices_t* apBtMgrDiscDevInfo
) {
    int j = 0;

    for (j = 0; j < gListOfDiscoveredDevices.m_numOfDevices; j++) {
        if (ahBTRMgrDevHdl == gListOfDiscoveredDevices.m_deviceProperty[j].m_deviceHandle) {
            MEMCPY_S(apBtMgrDiscDevInfo,sizeof(BTRMGR_DiscoveredDevices_t), &gListOfDiscoveredDevices.m_deviceProperty[j], sizeof(BTRMGR_DiscoveredDevices_t));
        }
    }
}


STATIC void
btrMgr_GetPairedDevInfo (
    BTRMgrDeviceHandle      ahBTRMgrDevHdl,
    BTRMGR_PairedDevices_t* apBtMgrPairedDevInfo
) {
    int j = 0;

    for (j = 0; j < gListOfPairedDevices.m_numOfDevices; j++) {
        if (ahBTRMgrDevHdl == gListOfPairedDevices.m_deviceProperty[j].m_deviceHandle) {
            MEMCPY_S(apBtMgrPairedDevInfo,sizeof(BTRMGR_PairedDevices_t), &gListOfPairedDevices.m_deviceProperty[j], sizeof(BTRMGR_PairedDevices_t));
            break;
        }
    }
}


STATIC unsigned char
btrMgr_GetDevPaired (
    BTRMgrDeviceHandle  ahBTRMgrDevHdl
) {
    int j = 0;

    for (j = 0; j < gListOfPairedDevices.m_numOfDevices; j++) {
        if (ahBTRMgrDevHdl == gListOfPairedDevices.m_deviceProperty[j].m_deviceHandle) {
            return 1;
        }
    }

    return 0;
}


STATIC void
btrMgr_SetDevConnected (
    BTRMgrDeviceHandle  ahBTRMgrDevHdl,
    unsigned char       aui8isDeviceConnected
) {
    int i = 0;

    for (i = 0; i < gListOfPairedDevices.m_numOfDevices; i++) {
        if (ahBTRMgrDevHdl == gListOfPairedDevices.m_deviceProperty[i].m_deviceHandle) {
            gListOfPairedDevices.m_deviceProperty[i].m_isConnected = aui8isDeviceConnected; 
            BTRMGRLOG_WARN ("Setting = %lld - \tConnected = %d\n", ahBTRMgrDevHdl, aui8isDeviceConnected);
            break;
        }
    }
}


STATIC unsigned char
btrMgr_IsDevConnected (
    BTRMgrDeviceHandle ahBTRMgrDevHdl
) {
    unsigned char lui8isDeviceConnected = 0;
    int           i = 0;

    for (i = 0; i < gListOfPairedDevices.m_numOfDevices; i++) {
        if (ahBTRMgrDevHdl == gListOfPairedDevices.m_deviceProperty[i].m_deviceHandle) {
            lui8isDeviceConnected = gListOfPairedDevices.m_deviceProperty[i].m_isConnected;
            BTRMGRLOG_WARN ("Getting = %lld - \tConnected = %d\n", ahBTRMgrDevHdl, lui8isDeviceConnected);
        }
    }
    
    return lui8isDeviceConnected;
}


STATIC unsigned char
btrMgr_IsDevNameSameAsAddress (
    char*           apcDeviceName,
    char*           apcDeviceAddress,
    unsigned int    ui32StrLen
) {
    if (ui32StrLen > 17)
        return 0;


    if ((apcDeviceName[0] == apcDeviceAddress[0]) &&
        (apcDeviceName[1] == apcDeviceAddress[1]) &&
        (apcDeviceName[3] == apcDeviceAddress[3]) &&
        (apcDeviceName[4] == apcDeviceAddress[4]) &&
        (apcDeviceName[6] == apcDeviceAddress[6]) &&
        (apcDeviceName[7] == apcDeviceAddress[7]) &&
        (apcDeviceName[9] == apcDeviceAddress[9]) &&
        (apcDeviceName[10] == apcDeviceAddress[10]) &&
        (apcDeviceName[12] == apcDeviceAddress[12]) &&
        (apcDeviceName[13] == apcDeviceAddress[13]) &&
        (apcDeviceName[15] == apcDeviceAddress[15]) &&
        (apcDeviceName[16] == apcDeviceAddress[16]) ) {
        return 1;
    }
    else {
        return 0;
    }
}


STATIC unsigned char
btrMgr_CheckIfDevicePrevDetected (
    BTRMgrDeviceHandle          ahBTRMgrDevHdl
) {
    int j = 0;

    for (j = 0; j < gListOfDiscoveredDevices.m_numOfDevices; j++) {
        if (ahBTRMgrDevHdl == gListOfDiscoveredDevices.m_deviceProperty[j].m_deviceHandle) {
            BTRMGRLOG_TRACE ("DevicePrevDetected = %lld - %s\n", gListOfDiscoveredDevices.m_deviceProperty[j].m_deviceHandle, gListOfDiscoveredDevices.m_deviceProperty[j].m_name);
            return 1;
        }
    }

    return 0;
}
#ifndef LE_MODE
STATIC gpointer
btrMgr_ConnectCb(
    gpointer gpDeviceConnectionHdl
)
{

    BTRMGR_ConnectionInformation_t * deviceConnectionHdl = (BTRMGR_ConnectionInformation_t *) gpDeviceConnectionHdl;
    guint8 reconnectAttepts = 0;
    if (gpDeviceConnectionHdl == NULL)
    {
        BTRMGRLOG_ERROR("Invalid parameter\n");
        return NULL;
    }
    BTRMGRLOG_INFO("waiting to reconnect for %u seconds\n", deviceConnectionHdl->timeToWait);
    sleep(deviceConnectionHdl->timeToWait);
    BTRMGRLOG_INFO("Connecting back to device %lld, and starting audio\n", deviceConnectionHdl->deviceHandle);
    while (eBTRMgrSuccess != btrMgr_StartAudioStreamingOut(deviceConnectionHdl->lui8AdapterIdx, deviceConnectionHdl->deviceHandle, deviceConnectionHdl->connectAs, 0,0,0) && reconnectAttepts < BTMGR_RECONNECTION_ATTEMPTS)
    {
        BTRMGRLOG_INFO("waiting to reconnect for %u seconds\n", deviceConnectionHdl->timeToWait);
        sleep(deviceConnectionHdl->timeToWait);
        BTRMGRLOG_INFO("Connecting back to device %lld, and starting audio\n", deviceConnectionHdl->deviceHandle);
        reconnectAttepts++;
    }
    free(deviceConnectionHdl);
    deviceConnectionHdl = NULL;
    return NULL;
}

STATIC unsigned char
btrMgr_ConnectBackToDevice(
    unsigned char                   aui8AdapterIdx,
    BTRMgrDeviceHandle              ahBTRMgrDevHdl,
    BTRMGR_DeviceOperationType_t    connectAs
)
{
    BTRMGR_ConnectionInformation_t *deviceConnectionHdl = malloc(sizeof(BTRMGR_ConnectionInformation_t));
    GThread * pReconnectThread = NULL;
    if (deviceConnectionHdl == NULL)
    {
        BTRMGRLOG_ERROR("Could not allocate memory\n");
        return 0;
    }
    deviceConnectionHdl->lui8AdapterIdx = aui8AdapterIdx;
    deviceConnectionHdl->connectAs = connectAs;
    deviceConnectionHdl->deviceHandle = ahBTRMgrDevHdl;
    deviceConnectionHdl->timeToWait = BTMGR_RECONNECTION_HOLD_OFF;
    pReconnectThread = g_thread_new("btrMgr_reconnect_thread", btrMgr_ConnectCb, (gpointer) deviceConnectionHdl);
    if (!pReconnectThread)
    {
        BTRMGRLOG_ERROR("Could not create thread to reconnect to device\n");
        free(deviceConnectionHdl);
        return 0;
    }
    g_thread_unref(pReconnectThread);
    /* coverity[leaked_storage] - memory is freed in pointer callback (thread was successfully created)*/
    return 1;
}
#endif
STATIC BTRMGR_DeviceType_t
btrMgr_MapDeviceTypeFromCore (
    enBTRCoreDeviceClass    device_type
) {
    BTRMGR_DeviceType_t type = BTRMGR_DEVICE_TYPE_UNKNOWN;
    switch (device_type) {
    case enBTRCore_DC_Tablet:
        type = BTRMGR_DEVICE_TYPE_TABLET;
        break;
    case enBTRCore_DC_SmartPhone:
        type = BTRMGR_DEVICE_TYPE_SMARTPHONE;
        break;
    case enBTRCore_DC_WearableHeadset:
        type = BTRMGR_DEVICE_TYPE_WEARABLE_HEADSET;
        break;
    case enBTRCore_DC_Handsfree:
        type = BTRMGR_DEVICE_TYPE_HANDSFREE;
        break;
    case enBTRCore_DC_Microphone:
        type = BTRMGR_DEVICE_TYPE_MICROPHONE;
        break;
    case enBTRCore_DC_Loudspeaker:
        type = BTRMGR_DEVICE_TYPE_LOUDSPEAKER;
        break;
    case enBTRCore_DC_Headphones:
        type = BTRMGR_DEVICE_TYPE_HEADPHONES;
        break;
    case enBTRCore_DC_PortableAudio:
        // type = BTRMGR_DEVICE_TYPE_PORTABLE_AUDIO;
        type = BTRMGR_DEVICE_TYPE_LOUDSPEAKER;
        break;
    case enBTRCore_DC_CarAudio:
        // type = BTRMGR_DEVICE_TYPE_CAR_AUDIO;
        type = BTRMGR_DEVICE_TYPE_LOUDSPEAKER;
        break;
    case enBTRCore_DC_STB:
        type = BTRMGR_DEVICE_TYPE_STB;
        break;
    case enBTRCore_DC_HIFIAudioDevice:
        // type = BTRMGR_DEVICE_TYPE_HIFI_AUDIO_DEVICE;
        type = BTRMGR_DEVICE_TYPE_LOUDSPEAKER;
        break;
    case enBTRCore_DC_VCR:
        type = BTRMGR_DEVICE_TYPE_VCR;
        break;
    case enBTRCore_DC_VideoCamera:
        type = BTRMGR_DEVICE_TYPE_VIDEO_CAMERA;
        break;
    case enBTRCore_DC_Camcoder:
        type = BTRMGR_DEVICE_TYPE_CAMCODER;
        break;
    case enBTRCore_DC_VideoMonitor:
        type = BTRMGR_DEVICE_TYPE_VIDEO_MONITOR;
        break;
    case enBTRCore_DC_TV:
        type = BTRMGR_DEVICE_TYPE_TV;
        break;
    case enBTRCore_DC_VideoConference:
        type = BTRMGR_DEVICE_TYPE_VIDEO_CONFERENCE;
        break;
    case enBTRCore_DC_Tile:
        type = BTRMGR_DEVICE_TYPE_TILE;
        break;
    case enBTRCore_DC_XBB:
        type = BTRMGR_DEVICE_TYPE_XBB;
        break;
    case enBTRCore_DC_HID_Keyboard:
        type = BTRMGR_DEVICE_TYPE_HID;
        break;
    case enBTRCore_DC_HID_Mouse:
        type = BTRMGR_DEVICE_TYPE_HID;
        break;
    case enBTRCore_DC_HID_MouseKeyBoard:
        type = BTRMGR_DEVICE_TYPE_HID;
        break;
    case enBTRCore_DC_HID_Joystick:
        type = BTRMGR_DEVICE_TYPE_HID;
        break;
    case enBTRCore_DC_HID_GamePad:
        type = BTRMGR_DEVICE_TYPE_HID_GAMEPAD;
        break;
    case enBTRCore_DC_HID_AudioRemote:
        type = BTRMGR_DEVICE_TYPE_HID;
        break;
    case enBTRCore_DC_Reserved:
    case enBTRCore_DC_Unknown:
        type = BTRMGR_DEVICE_TYPE_UNKNOWN;
        break;
    }

    return type;
}

STATIC BTRMGR_DeviceOperationType_t
btrMgr_MapDeviceOpFromDeviceType (
    BTRMGR_DeviceType_t device_type
) {
    BTRMGR_DeviceOperationType_t devOpType = BTRMGR_DEVICE_OP_TYPE_UNKNOWN;

    switch (device_type) {
    case BTRMGR_DEVICE_TYPE_WEARABLE_HEADSET:
    case BTRMGR_DEVICE_TYPE_HANDSFREE:
    case BTRMGR_DEVICE_TYPE_LOUDSPEAKER:
    case BTRMGR_DEVICE_TYPE_HEADPHONES:
    case BTRMGR_DEVICE_TYPE_PORTABLE_AUDIO:
    case BTRMGR_DEVICE_TYPE_CAR_AUDIO:
    case BTRMGR_DEVICE_TYPE_HIFI_AUDIO_DEVICE:
        devOpType = BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT;
        break;
    case BTRMGR_DEVICE_TYPE_SMARTPHONE:
    case BTRMGR_DEVICE_TYPE_TABLET:
        devOpType = BTRMGR_DEVICE_OP_TYPE_AUDIO_INPUT;
        break;
    case BTRMGR_DEVICE_TYPE_XBB:
    case BTRMGR_DEVICE_TYPE_TILE:
        devOpType = BTRMGR_DEVICE_OP_TYPE_LE;
        break;
    case BTRMGR_DEVICE_TYPE_HID:
    case BTRMGR_DEVICE_TYPE_HID_GAMEPAD:
        devOpType = BTRMGR_DEVICE_OP_TYPE_HID;
        break;
    case BTRMGR_DEVICE_TYPE_STB:
    case BTRMGR_DEVICE_TYPE_MICROPHONE:
    case BTRMGR_DEVICE_TYPE_VCR:
    case BTRMGR_DEVICE_TYPE_VIDEO_CAMERA:
    case BTRMGR_DEVICE_TYPE_CAMCODER:
    case BTRMGR_DEVICE_TYPE_VIDEO_MONITOR:
    case BTRMGR_DEVICE_TYPE_TV:
    case BTRMGR_DEVICE_TYPE_VIDEO_CONFERENCE:
    case BTRMGR_DEVICE_TYPE_RESERVED:
    case BTRMGR_DEVICE_TYPE_UNKNOWN:
    default:
        devOpType = BTRMGR_DEVICE_OP_TYPE_UNKNOWN;
    }

    return devOpType;
}

STATIC BTRMGR_RSSIValue_t
btrMgr_MapSignalStrengthToRSSI (
    int signalStrength
) {
    BTRMGR_RSSIValue_t rssi = BTRMGR_RSSI_NONE;

    if (signalStrength >= BTRMGR_SIGNAL_GOOD)
        rssi = BTRMGR_RSSI_EXCELLENT;
    else if (signalStrength >= BTRMGR_SIGNAL_FAIR)
        rssi = BTRMGR_RSSI_GOOD;
    else if (signalStrength >= BTRMGR_SIGNAL_POOR)
        rssi = BTRMGR_RSSI_FAIR;
    else
        rssi = BTRMGR_RSSI_POOR;

    return rssi;
}

STATIC eBTRMgrRet
btrMgr_MapDevstatusInfoToEventInfo (
    void*                   p_StatusCB,         /* device status info */
    BTRMGR_EventMessage_t*  apstEventMessage,   /* event message      */
    BTRMGR_Events_t         type                /* event type         */
) {
    eBTRMgrRet  lenBtrMgrRet = eBTRMgrSuccess;

    apstEventMessage->m_adapterIndex = 0;
    apstEventMessage->m_eventType    = type;

    if (!p_StatusCB)
        return eBTRMgrFailure;


    if (type == BTRMGR_EVENT_DEVICE_DISCOVERY_UPDATE) {
        apstEventMessage->m_discoveredDevice.m_deviceHandle      = ((stBTRCoreBTDevice*)p_StatusCB)->tDeviceId;
        apstEventMessage->m_discoveredDevice.m_signalLevel       = btrMgr_MapSignalStrengthToRSSI(((stBTRCoreBTDevice*)p_StatusCB)->i32RSSI);
        apstEventMessage->m_discoveredDevice.m_deviceType        = btrMgr_MapDeviceTypeFromCore(((stBTRCoreBTDevice*)p_StatusCB)->enDeviceType);
        apstEventMessage->m_discoveredDevice.m_rssi              = ((stBTRCoreBTDevice*)p_StatusCB)->i32RSSI;
        apstEventMessage->m_discoveredDevice.m_isPairedDevice    = btrMgr_GetDevPaired(apstEventMessage->m_discoveredDevice.m_deviceHandle);
        if ((apstEventMessage->m_discoveredDevice.m_deviceType == BTRMGR_DEVICE_TYPE_TILE) ||
            (apstEventMessage->m_discoveredDevice.m_deviceType == BTRMGR_DEVICE_TYPE_XBB)) {
            apstEventMessage->m_discoveredDevice.m_isLowEnergyDevice = 1;
        } else {
            apstEventMessage->m_discoveredDevice.m_isLowEnergyDevice = 0;
        }

        apstEventMessage->m_discoveredDevice.m_isDiscovered         = ((stBTRCoreBTDevice*)p_StatusCB)->bFound;
        apstEventMessage->m_discoveredDevice.m_isLastConnectedDevice= (ghBTRMgrDevHdlLastConnected == apstEventMessage->m_discoveredDevice.m_deviceHandle) ? 1 : 0;
        apstEventMessage->m_discoveredDevice.m_ui32DevClassBtSpec   = ((stBTRCoreBTDevice*)p_StatusCB)->ui32DevClassBtSpec;
        apstEventMessage->m_discoveredDevice.m_ui16DevAppearanceBleSpec   = ((stBTRCoreBTDevice*)p_StatusCB)->ui16DevAppearanceBleSpec;

        strncpy(apstEventMessage->m_discoveredDevice.m_name, ((stBTRCoreBTDevice*)p_StatusCB)->pcDeviceName, BTRMGR_NAME_LEN_MAX - 1);
        strncpy(apstEventMessage->m_discoveredDevice.m_deviceAddress, ((stBTRCoreBTDevice*)p_StatusCB)->pcDeviceAddress, BTRMGR_NAME_LEN_MAX - 1);
    }
    else if (type == BTRMGR_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST) {
        apstEventMessage->m_externalDevice.m_deviceHandle        = ((stBTRCoreConnCBInfo*)p_StatusCB)->stFoundDevice.tDeviceId;
        apstEventMessage->m_externalDevice.m_deviceType          = btrMgr_MapDeviceTypeFromCore(((stBTRCoreConnCBInfo*)p_StatusCB)->stFoundDevice.enDeviceType);
        apstEventMessage->m_externalDevice.m_vendorID            = ((stBTRCoreConnCBInfo*)p_StatusCB)->stFoundDevice.ui32VendorId;
        apstEventMessage->m_externalDevice.m_isLowEnergyDevice   = 0;
        apstEventMessage->m_externalDevice.m_externalDevicePIN   = ((stBTRCoreConnCBInfo*)p_StatusCB)->ui32devPassKey;
        apstEventMessage->m_externalDevice.m_requestConfirmation = ((stBTRCoreConnCBInfo*)p_StatusCB)->ucIsReqConfirmation;
        strncpy(apstEventMessage->m_externalDevice.m_name, ((stBTRCoreConnCBInfo*)p_StatusCB)->stFoundDevice.pcDeviceName, BTRMGR_NAME_LEN_MAX - 1);
        strncpy(apstEventMessage->m_externalDevice.m_deviceAddress, ((stBTRCoreConnCBInfo*)p_StatusCB)->stFoundDevice.pcDeviceAddress, BTRMGR_NAME_LEN_MAX - 1);
    }
    else if (type == BTRMGR_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST) {
        apstEventMessage->m_externalDevice.m_deviceHandle        = ((stBTRCoreConnCBInfo*)p_StatusCB)->stKnownDevice.tDeviceId;
        apstEventMessage->m_externalDevice.m_deviceType          = btrMgr_MapDeviceTypeFromCore(((stBTRCoreConnCBInfo*)p_StatusCB)->stKnownDevice.enDeviceType);
        apstEventMessage->m_externalDevice.m_vendorID            = ((stBTRCoreConnCBInfo*)p_StatusCB)->stKnownDevice.ui32VendorId;
        apstEventMessage->m_externalDevice.m_isLowEnergyDevice   = 0;
        strncpy(apstEventMessage->m_externalDevice.m_name, ((stBTRCoreConnCBInfo*)p_StatusCB)->stKnownDevice.pcDeviceName, BTRMGR_NAME_LEN_MAX - 1);
        strncpy(apstEventMessage->m_externalDevice.m_deviceAddress, ((stBTRCoreConnCBInfo*)p_StatusCB)->stKnownDevice.pcDeviceAddress, BTRMGR_NAME_LEN_MAX - 1);
    }
    else if (type == BTRMGR_EVENT_RECEIVED_EXTERNAL_PLAYBACK_REQUEST) {
        apstEventMessage->m_externalDevice.m_deviceHandle        = ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceId;
        apstEventMessage->m_externalDevice.m_deviceType          = btrMgr_MapDeviceTypeFromCore(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->eDeviceClass);
        apstEventMessage->m_externalDevice.m_vendorID            = 0;
        apstEventMessage->m_externalDevice.m_isLowEnergyDevice   = 0;
        strncpy(apstEventMessage->m_externalDevice.m_name, ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceName, BTRMGR_NAME_LEN_MAX - 1);
        strncpy(apstEventMessage->m_externalDevice.m_deviceAddress, "TO BE FILLED", BTRMGR_NAME_LEN_MAX - 1);
        // Need to check for devAddress, if possible ?
    }
    else if (type == BTRMGR_EVENT_DEVICE_OP_INFORMATION) {
        if (enBTRCoreLE == ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->eDeviceType) {
            apstEventMessage->m_deviceOpInfo.m_deviceHandle      = ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceId;
            apstEventMessage->m_deviceOpInfo.m_deviceType        = btrMgr_MapDeviceTypeFromCore(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->eDeviceClass);
            apstEventMessage->m_deviceOpInfo.m_leOpType          = ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->eCoreLeOper;

            strncpy(apstEventMessage->m_deviceOpInfo.m_name, ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceName,
                    strlen(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceName) < BTRMGR_NAME_LEN_MAX ? strlen (((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceName) : BTRMGR_NAME_LEN_MAX - 1);
            strncpy(apstEventMessage->m_deviceOpInfo.m_deviceAddress, ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceAddress,
                    strlen(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceAddress) < BTRMGR_NAME_LEN_MAX ? strlen (((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceAddress) : BTRMGR_NAME_LEN_MAX - 1);
            strncpy(apstEventMessage->m_deviceOpInfo.m_uuid, ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->uuid,
                    strlen(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->uuid) < BTRMGR_MAX_STR_LEN ? strlen(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->uuid) : BTRMGR_MAX_STR_LEN - 1);
        }
    }
    else if (type == BTRMGR_EVENT_BATTERY_INFO) {
        strncpy(apstEventMessage->m_batteryInfo.m_uuid, ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->uuid,
                    strlen(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->uuid) < BTRMGR_UUID_STR_LEN_MAX ? strlen(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->uuid) : BTRMGR_UUID_STR_LEN_MAX - 1);
        apstEventMessage->m_batteryInfo.m_deviceHandle      = ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceId;
        apstEventMessage->m_batteryInfo.m_deviceType        = btrMgr_MapDeviceTypeFromCore(((stBTRCoreBTDevice*)p_StatusCB)->enDeviceType);
        strncpy(apstEventMessage->m_batteryInfo.m_name, ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceName,
                    strlen(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceName) < BTRMGR_NAME_LEN_MAX ? strlen (((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceName) : BTRMGR_NAME_LEN_MAX - 1);
    }
    else {
        apstEventMessage->m_pairedDevice.m_deviceHandle          = ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceId;
        apstEventMessage->m_pairedDevice.m_deviceType            = btrMgr_MapDeviceTypeFromCore(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->eDeviceClass);
        apstEventMessage->m_pairedDevice.m_isLastConnectedDevice = (ghBTRMgrDevHdlLastConnected == apstEventMessage->m_pairedDevice.m_deviceHandle) ? 1 : 0;

        if ((apstEventMessage->m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_TILE) ||
            (apstEventMessage->m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_XBB)) {
            apstEventMessage->m_pairedDevice.m_isLowEnergyDevice = 1; // We shall make it generic later
        }
        else {
            apstEventMessage->m_pairedDevice.m_isLowEnergyDevice     = (((stBTRCoreDevStatusCBInfo*)p_StatusCB)->eDeviceType == enBTRCoreLE)?1:0;//We shall make it generic later
        }

        apstEventMessage->m_pairedDevice.m_ui32DevClassBtSpec       = ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->ui32DevClassBtSpec;
        apstEventMessage->m_pairedDevice.m_ui16DevAppearanceBleSpec = ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->ui16DevAppearanceBleSpec;

        strncpy(apstEventMessage->m_pairedDevice.m_name, ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceName,
                    strlen(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceName) < BTRMGR_NAME_LEN_MAX ? strlen (((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceName) : BTRMGR_NAME_LEN_MAX - 1);
        strncpy(apstEventMessage->m_pairedDevice.m_deviceAddress, ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceAddress,
                    strlen(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceAddress) < BTRMGR_NAME_LEN_MAX ? strlen (((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceAddress) : BTRMGR_NAME_LEN_MAX - 1);
    }

    return lenBtrMgrRet;
}

STATIC gboolean
btrMgr_IsPS4Gamepad(
    BTRMgrDeviceHandle  ahBTRMgrDevHdl
) {
    unsigned int ui32MProductId = 0, MVendorId = 0;
    gboolean bIsPs4 = FALSE;

    if (eBTRMgrSuccess == btrMgr_GetDeviceProductDetails(ahBTRMgrDevHdl, &ui32MProductId, &MVendorId)) {
        if ((BTRMGR_SONY_PS_VENDOR_ID == MVendorId) && ((BTRMGR_SONY_PS_PRODUCT_ID_1 == ui32MProductId) || (BTRMGR_SONY_PS_PRODUCT_ID_2 == ui32MProductId))) {
            char lpcBtVersion[BTRCORE_STR_LEN] = {'\0'};

            if (enBTRCoreSuccess == BTRCore_GetVersionInfo(ghBTRCoreHdl, lpcBtVersion)) {
                if (!strncmp(lpcBtVersion, "Bluez-5.54", strlen("Bluez-5.54"))) {
                    bIsPs4 = TRUE;
                    BTRMGRLOG_INFO ("It is a PS4 gamepad. Skipped the pair call\n");
                }
            }
            else {
                BTRMGRLOG_ERROR ("Failed to get version info\n");
            }
        }
    }
    else {
        BTRMGRLOG_ERROR ("Failed to get product details\n");
    }
    return bIsPs4;
}

STATIC eBTRMgrRet
btrMgr_GetDeviceDetails (
    BTRMgrDeviceHandle  ahBTRMgrDevHdl,
    stBTRCoreBTDevice   *pstDeviceInfo
){
    eBTRMgrRet                      lenBtrMgrRet = eBTRMgrFailure;
    stBTRCoreScannedDevicesCount    lstBtrCoreListOfSDevices;
    stBTRCorePairedDevicesCount     listOfPDevices;
    enBTRCoreRet                    lenBtrCoreRet   = enBTRCoreSuccess;
    int                             i=0;

    if (NULL == pstDeviceInfo) {
        BTRMGRLOG_ERROR ("Invalid device info\n");
        return eBTRMgrFailure;
    }

    MEMSET_S(&listOfPDevices, sizeof(listOfPDevices), 0, sizeof(listOfPDevices));
    MEMSET_S(&lstBtrCoreListOfSDevices, sizeof(lstBtrCoreListOfSDevices), 0, sizeof(lstBtrCoreListOfSDevices));
    lenBtrCoreRet = BTRCore_GetListOfPairedDevices(ghBTRCoreHdl, &listOfPDevices);
    if (lenBtrCoreRet == enBTRCoreSuccess) {
        if (listOfPDevices.numberOfDevices) {
            for (i = 0; i < listOfPDevices.numberOfDevices; i++) {
                if (ahBTRMgrDevHdl == listOfPDevices.devices[i].tDeviceId) {
                    MEMCPY_S(pstDeviceInfo,sizeof(stBTRCoreBTDevice),&listOfPDevices.devices[i],sizeof(listOfPDevices.devices[0]));
                    lenBtrMgrRet = eBTRMgrSuccess;
                    return lenBtrMgrRet;
                }
            }
        }
        else {
            BTRMGRLOG_WARN("No Device in paired list\n");
        }
    } else {
        BTRMGRLOG_ERROR ("Failed to get paired devices information\n");
    }
    lenBtrCoreRet  = BTRCore_GetListOfScannedDevices (ghBTRCoreHdl, &lstBtrCoreListOfSDevices);
    if (lenBtrCoreRet == enBTRCoreSuccess) {
        if (lstBtrCoreListOfSDevices.numberOfDevices) {
            for (i = 0; i < lstBtrCoreListOfSDevices.numberOfDevices; i++) {
                if (ahBTRMgrDevHdl == lstBtrCoreListOfSDevices.devices[i].tDeviceId) {
                    MEMCPY_S(pstDeviceInfo,sizeof(stBTRCoreBTDevice),&lstBtrCoreListOfSDevices.devices[i],sizeof(lstBtrCoreListOfSDevices.devices[0]));
                    lenBtrMgrRet = eBTRMgrSuccess;
                    return lenBtrMgrRet;
                }
            }
        }
        else {
            BTRMGRLOG_WARN("No Device in scan list\n");
        }
    }
    else {
        BTRMGRLOG_ERROR ("Failed to get scanned devices information\n");
    }
    return lenBtrMgrRet;
}

STATIC eBTRMgrRet
btrMgr_GetDeviceProductDetails (
    BTRMgrDeviceHandle  ahBTRMgrDevHdl,
    unsigned int *pui32MproductId,
    unsigned int *pui32MVendorId
) {
    eBTRMgrRet          lenBtrMgrRet = eBTRMgrFailure;
    stBTRCoreBTDevice   stDeviceInfo;

    MEMSET_S(&stDeviceInfo, sizeof(stBTRCoreBTDevice), 0, sizeof(stBTRCoreBTDevice));
    if (eBTRMgrSuccess == btrMgr_GetDeviceDetails(ahBTRMgrDevHdl,&stDeviceInfo)) {
        *pui32MproductId = stDeviceInfo.ui32ModaliasProductId;
        *pui32MVendorId  = stDeviceInfo.ui32ModaliasVendorId;
        BTRMGRLOG_DEBUG("MProduct: %04X Mvendor: %04X\n",*pui32MproductId,*pui32MVendorId);
        lenBtrMgrRet = eBTRMgrSuccess;
    }
    else {
        BTRMGRLOG_ERROR ("Failed to get product details\n");
    }

    return lenBtrMgrRet;
}

#ifndef LE_MODE
STATIC eBTRMgrRet
btrMgr_StartCastingAudio (
    int                     outFileFd, 
    int                     outMTUSize,
    unsigned int            outDevDelay,
    eBTRCoreDevMediaType    aenBtrCoreDevOutMType,
    void*                   apstBtrCoreDevOutMCodecInfo,
    BTRMgrDeviceHandle      devHandle,
    char*                   profileStr
) {
    stBTRMgrOutASettings    lstBtrMgrAcOutASettings;
    stBTRMgrInASettings     lstBtrMgrSoInASettings;
    stBTRMgrOutASettings    lstBtrMgrSoOutASettings;
    eBTRMgrRet              lenBtrMgrRet = eBTRMgrSuccess;

    int                     inBytesToEncode = 3072; // Corresponds to MTU size of 895

    BTRMGR_StreamOut_Type_t lenCurrentSoType  = gstBTRMgrStreamingInfo.tBTRMgrSoType;
    tBTRMgrAcType           lpi8BTRMgrAcType= BTRMGR_AC_TYPE_PRIMARY;
#ifdef RDKTV_PERSIST_VOLUME
    unsigned char           ui8Volume = BTRMGR_SO_MAX_VOLUME;
    gboolean                lbMute = FALSE;
#endif

    if ((ghBTRMgrDevHdlCurStreaming != 0) || (outMTUSize == 0)) {
        return eBTRMgrFailInArg;
    }


    /* Reset the buffer */
    MEMSET_S(&gstBTRMgrStreamingInfo, sizeof(gstBTRMgrStreamingInfo), 0, sizeof(gstBTRMgrStreamingInfo));

    MEMSET_S(&lstBtrMgrAcOutASettings,sizeof(lstBtrMgrAcOutASettings), 0, sizeof(lstBtrMgrAcOutASettings));
    MEMSET_S(&lstBtrMgrSoInASettings, sizeof(lstBtrMgrSoInASettings),  0, sizeof(lstBtrMgrSoInASettings));
    MEMSET_S(&lstBtrMgrSoOutASettings,sizeof(lstBtrMgrSoOutASettings), 0, sizeof(lstBtrMgrSoOutASettings));

    /* Init StreamOut module - Create Pipeline */
    if ((lenBtrMgrRet = BTRMgr_SO_Init(&gstBTRMgrStreamingInfo.hBTRMgrSoHdl, btrMgr_SOStatusCb, &gstBTRMgrStreamingInfo)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SO_Init FAILED\n");
        return eBTRMgrInitFailure;
    }

    if (lenCurrentSoType == BTRMGR_STREAM_PRIMARY) {
        lpi8BTRMgrAcType = BTRMGR_AC_TYPE_PRIMARY;
        gstBTRMgrStreamingInfo.tBTRMgrSoType = lenCurrentSoType;
    }
    else if (lenCurrentSoType == BTRMGR_STREAM_AUXILIARY) {
        lpi8BTRMgrAcType = BTRMGR_AC_TYPE_AUXILIARY;
        gstBTRMgrStreamingInfo.tBTRMgrSoType = lenCurrentSoType;
    }
    else {
        lpi8BTRMgrAcType = BTRMGR_AC_TYPE_PRIMARY;
        gstBTRMgrStreamingInfo.tBTRMgrSoType = BTRMGR_STREAM_PRIMARY;
    }

    if ((lenBtrMgrRet = BTRMgr_AC_Init(&gstBTRMgrStreamingInfo.hBTRMgrAcHdl, lpi8BTRMgrAcType, gDebugModeEnabled)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_AC_Init FAILED\n");
        return eBTRMgrInitFailure;
    }

    /* could get defaults from audio capture, but for the sample app we want to write a the wav header first*/
    gstBTRMgrStreamingInfo.bitsPerSample = 16;
    gstBTRMgrStreamingInfo.samplerate = 48000;
    gstBTRMgrStreamingInfo.channels = 2;


    lstBtrMgrAcOutASettings.pstBtrMgrOutCodecInfo = (void*)malloc((sizeof(stBTRMgrPCMInfo) > sizeof(stBTRMgrSBCInfo) ? sizeof(stBTRMgrPCMInfo) : sizeof(stBTRMgrSBCInfo)) > sizeof(stBTRMgrMPEGInfo) ?
                                                                    (sizeof(stBTRMgrPCMInfo) > sizeof(stBTRMgrSBCInfo) ? sizeof(stBTRMgrPCMInfo) : sizeof(stBTRMgrSBCInfo)) : sizeof(stBTRMgrMPEGInfo));
    lstBtrMgrSoInASettings.pstBtrMgrInCodecInfo   = (void*)malloc((sizeof(stBTRMgrPCMInfo) > sizeof(stBTRMgrSBCInfo) ? sizeof(stBTRMgrPCMInfo) : sizeof(stBTRMgrSBCInfo)) > sizeof(stBTRMgrMPEGInfo) ?
                                                                    (sizeof(stBTRMgrPCMInfo) > sizeof(stBTRMgrSBCInfo) ? sizeof(stBTRMgrPCMInfo) : sizeof(stBTRMgrSBCInfo)) : sizeof(stBTRMgrMPEGInfo));
    lstBtrMgrSoOutASettings.pstBtrMgrOutCodecInfo = (void*)malloc((sizeof(stBTRMgrPCMInfo) > sizeof(stBTRMgrSBCInfo) ? sizeof(stBTRMgrPCMInfo) : sizeof(stBTRMgrSBCInfo)) > sizeof(stBTRMgrMPEGInfo) ?
                                                                    (sizeof(stBTRMgrPCMInfo) > sizeof(stBTRMgrSBCInfo) ? sizeof(stBTRMgrPCMInfo) : sizeof(stBTRMgrSBCInfo)) : sizeof(stBTRMgrMPEGInfo));


    if (!(lstBtrMgrAcOutASettings.pstBtrMgrOutCodecInfo) || !(lstBtrMgrSoInASettings.pstBtrMgrInCodecInfo) || !(lstBtrMgrSoOutASettings.pstBtrMgrOutCodecInfo)) {
        BTRMGRLOG_ERROR ("MEMORY ALLOC FAILED\n");
        return eBTRMgrFailure;
    }


    if ((lenBtrMgrRet = BTRMgr_AC_GetDefaultSettings(gstBTRMgrStreamingInfo.hBTRMgrAcHdl, &lstBtrMgrAcOutASettings)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR("BTRMgr_AC_GetDefaultSettings FAILED\n");
    }


    lstBtrMgrSoInASettings.eBtrMgrInAType     = lstBtrMgrAcOutASettings.eBtrMgrOutAType;

    if (lstBtrMgrSoInASettings.eBtrMgrInAType == eBTRMgrATypePCM) {
        stBTRMgrPCMInfo* pstBtrMgrSoInPcmInfo   = (stBTRMgrPCMInfo*)(lstBtrMgrSoInASettings.pstBtrMgrInCodecInfo);
        stBTRMgrPCMInfo* pstBtrMgrAcOutPcmInfo  = (stBTRMgrPCMInfo*)(lstBtrMgrAcOutASettings.pstBtrMgrOutCodecInfo);

        MEMCPY_S(pstBtrMgrSoInPcmInfo,sizeof(stBTRMgrPCMInfo), pstBtrMgrAcOutPcmInfo, sizeof(stBTRMgrPCMInfo));
    }


    if (aenBtrCoreDevOutMType == eBTRCoreDevMediaTypeSBC) {
        stBTRMgrSBCInfo*            pstBtrMgrSoOutSbcInfo       = ((stBTRMgrSBCInfo*)(lstBtrMgrSoOutASettings.pstBtrMgrOutCodecInfo));
        stBTRCoreDevMediaSbcInfo*   pstBtrCoreDevMediaSbcInfo   = (stBTRCoreDevMediaSbcInfo*)apstBtrCoreDevOutMCodecInfo;

        lstBtrMgrSoOutASettings.eBtrMgrOutAType   = eBTRMgrATypeSBC;
        if (pstBtrMgrSoOutSbcInfo && pstBtrCoreDevMediaSbcInfo) {

            if (pstBtrCoreDevMediaSbcInfo->ui32DevMSFreq == 8000) {
                pstBtrMgrSoOutSbcInfo->eBtrMgrSbcSFreq  = eBTRMgrSFreq8K;
            }
            else if (pstBtrCoreDevMediaSbcInfo->ui32DevMSFreq == 16000) {
                pstBtrMgrSoOutSbcInfo->eBtrMgrSbcSFreq  = eBTRMgrSFreq16K;
            }
            else if (pstBtrCoreDevMediaSbcInfo->ui32DevMSFreq == 32000) {
                pstBtrMgrSoOutSbcInfo->eBtrMgrSbcSFreq  = eBTRMgrSFreq32K;
            }
            else if (pstBtrCoreDevMediaSbcInfo->ui32DevMSFreq == 44100) {
                pstBtrMgrSoOutSbcInfo->eBtrMgrSbcSFreq  = eBTRMgrSFreq44_1K;
            }
            else if (pstBtrCoreDevMediaSbcInfo->ui32DevMSFreq == 48000) {
                pstBtrMgrSoOutSbcInfo->eBtrMgrSbcSFreq  = eBTRMgrSFreq48K;
            }
            else {
                pstBtrMgrSoOutSbcInfo->eBtrMgrSbcSFreq  = eBTRMgrSFreqUnknown;
            }


            switch (pstBtrCoreDevMediaSbcInfo->eDevMAChan) {
            case eBTRCoreDevMediaAChanMono:
                pstBtrMgrSoOutSbcInfo->eBtrMgrSbcAChan  = eBTRMgrAChanMono;
                break;
            case eBTRCoreDevMediaAChanDualChannel:
                pstBtrMgrSoOutSbcInfo->eBtrMgrSbcAChan  = eBTRMgrAChanDualChannel;
                break;
            case eBTRCoreDevMediaAChanStereo:
                pstBtrMgrSoOutSbcInfo->eBtrMgrSbcAChan  = eBTRMgrAChanStereo;
                break;
            case eBTRCoreDevMediaAChanJointStereo:
                pstBtrMgrSoOutSbcInfo->eBtrMgrSbcAChan  = eBTRMgrAChanJStereo;
                break;
            case eBTRCoreDevMediaAChan5_1:
                pstBtrMgrSoOutSbcInfo->eBtrMgrSbcAChan  = eBTRMgrAChan5_1;
                break;
            case eBTRCoreDevMediaAChan7_1:
                pstBtrMgrSoOutSbcInfo->eBtrMgrSbcAChan  = eBTRMgrAChan7_1;
                break;
            case eBTRCoreDevMediaAChanUnknown:
            default:
                pstBtrMgrSoOutSbcInfo->eBtrMgrSbcAChan  = eBTRMgrAChanUnknown;
                break;
            }

            pstBtrMgrSoOutSbcInfo->ui8SbcAllocMethod  = pstBtrCoreDevMediaSbcInfo->ui8DevMSbcAllocMethod;
            pstBtrMgrSoOutSbcInfo->ui8SbcSubbands     = pstBtrCoreDevMediaSbcInfo->ui8DevMSbcSubbands;
            pstBtrMgrSoOutSbcInfo->ui8SbcBlockLength  = pstBtrCoreDevMediaSbcInfo->ui8DevMSbcBlockLength;
            pstBtrMgrSoOutSbcInfo->ui8SbcMinBitpool   = pstBtrCoreDevMediaSbcInfo->ui8DevMSbcMinBitpool;
            pstBtrMgrSoOutSbcInfo->ui8SbcMaxBitpool   = pstBtrCoreDevMediaSbcInfo->ui8DevMSbcMaxBitpool;
            pstBtrMgrSoOutSbcInfo->ui16SbcFrameLen    = pstBtrCoreDevMediaSbcInfo->ui16DevMSbcFrameLen;
            pstBtrMgrSoOutSbcInfo->ui16SbcBitrate     = pstBtrCoreDevMediaSbcInfo->ui16DevMSbcBitrate;
        }
    }

    lstBtrMgrSoOutASettings.i32BtrMgrDevFd      = outFileFd;
    lstBtrMgrSoOutASettings.i32BtrMgrDevMtu     = outMTUSize;
    lstBtrMgrSoOutASettings.ui32BtrMgrDevDelay  = outDevDelay;

    if ((lenBtrMgrRet = BTRMgr_SO_GetEstimatedInABufSize(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, &lstBtrMgrSoInASettings, &lstBtrMgrSoOutASettings)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SO_GetEstimatedInABufSize FAILED\n");
        lstBtrMgrSoInASettings.i32BtrMgrInBufMaxSize = inBytesToEncode;
    }
    else {
        gstBTRMgrStreamingInfo.i32BytesToEncode = lstBtrMgrSoInASettings.i32BtrMgrInBufMaxSize;
    }

#ifdef RDKTV_PERSIST_VOLUME
    if (btrMgr_GetLastVolume(0, &ui8Volume,devHandle,profileStr) == eBTRMgrSuccess) {
        if (!gui8IsSoDevAvrcpSupported && (BTRMgr_SO_SetVolume(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, ui8Volume) != eBTRMgrSuccess)) {
            BTRMGRLOG_ERROR (" BTRMgr_SO_SetVolume FAILED \n");
        }
    }

    if (btrMgr_GetLastMuteState(0, &lbMute) == eBTRMgrSuccess) {
        if (BTRMgr_SO_SetMute(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, lbMute) != eBTRMgrSuccess) {
            BTRMGRLOG_ERROR (" BTRMgr_SO_SetMute FAILED \n");
        }
    }
#endif

    if ((lenBtrMgrRet = BTRMgr_SO_Start(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, &lstBtrMgrSoInASettings, &lstBtrMgrSoOutASettings)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SO_Start FAILED\n");
    }

    if (lenBtrMgrRet == eBTRMgrSuccess) {
        lstBtrMgrAcOutASettings.i32BtrMgrOutBufMaxSize = lstBtrMgrSoInASettings.i32BtrMgrInBufMaxSize;
        lstBtrMgrAcOutASettings.ui32BtrMgrDevDelay = outDevDelay;

        if ((lenBtrMgrRet = BTRMgr_AC_Start(gstBTRMgrStreamingInfo.hBTRMgrAcHdl,
                                            &lstBtrMgrAcOutASettings,
                                            btrMgr_ACDataReadyCb,
                                            btrMgr_ACStatusCb,
                                            &gstBTRMgrStreamingInfo)) != eBTRMgrSuccess) {
            BTRMGRLOG_ERROR ("BTRMgr_AC_Start FAILED\n");
        }
    }

    if (lstBtrMgrSoOutASettings.pstBtrMgrOutCodecInfo)
        free(lstBtrMgrSoOutASettings.pstBtrMgrOutCodecInfo);

    if (lstBtrMgrSoInASettings.pstBtrMgrInCodecInfo)
        free(lstBtrMgrSoInASettings.pstBtrMgrInCodecInfo);

    if (lstBtrMgrAcOutASettings.pstBtrMgrOutCodecInfo)
        free(lstBtrMgrAcOutASettings.pstBtrMgrOutCodecInfo);

    return lenBtrMgrRet;
}

STATIC eBTRMgrRet
btrMgr_StopCastingAudio (
    void
) {
    eBTRMgrRet  lenBtrMgrRet = eBTRMgrSuccess;

    if (ghBTRMgrDevHdlCurStreaming == 0) {
        return eBTRMgrFailInArg;
    }


    if ((lenBtrMgrRet = BTRMgr_AC_Stop(gstBTRMgrStreamingInfo.hBTRMgrAcHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_AC_Stop FAILED\n");
    }

    if ((lenBtrMgrRet = BTRMgr_SO_SendEOS(gstBTRMgrStreamingInfo.hBTRMgrSoHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SO_SendEOS FAILED\n");
    }

    if ((lenBtrMgrRet = BTRMgr_SO_Stop(gstBTRMgrStreamingInfo.hBTRMgrSoHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SO_Stop FAILED\n");
    }

    if ((lenBtrMgrRet = BTRMgr_AC_DeInit(gstBTRMgrStreamingInfo.hBTRMgrAcHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_AC_DeInit FAILED\n");
    }

    if ((lenBtrMgrRet = BTRMgr_SO_DeInit(gstBTRMgrStreamingInfo.hBTRMgrSoHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SO_DeInit FAILED\n");
    }

    gstBTRMgrStreamingInfo.hBTRMgrAcHdl = NULL;
    gstBTRMgrStreamingInfo.hBTRMgrSoHdl = NULL;

    return lenBtrMgrRet;
}

STATIC eBTRMgrRet
btrMgr_SwitchCastingAudio_AC (
    BTRMGR_StreamOut_Type_t aenCurrentSoType
) {
    eBTRMgrRet              lenBtrMgrRet            = eBTRMgrSuccess;
    tBTRMgrAcType           lpi8BTRMgrAcType        = BTRMGR_AC_TYPE_PRIMARY;
    stBTRMgrOutASettings    lstBtrMgrAcOutASettings;


    if (ghBTRMgrDevHdlCurStreaming == 0) {
        return eBTRMgrFailInArg;
    }


    if ((lenBtrMgrRet = BTRMgr_AC_Stop(gstBTRMgrStreamingInfo.hBTRMgrAcHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_AC_Stop FAILED\n");
    }

    if ((lenBtrMgrRet = BTRMgr_SO_Pause(gstBTRMgrStreamingInfo.hBTRMgrSoHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SO_Pause FAILED\n");
    }

    if ((lenBtrMgrRet = BTRMgr_AC_DeInit(gstBTRMgrStreamingInfo.hBTRMgrAcHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_AC_DeInit FAILED\n");
    }

    gstBTRMgrStreamingInfo.hBTRMgrAcHdl = NULL;


    if (aenCurrentSoType == BTRMGR_STREAM_PRIMARY) {
        lpi8BTRMgrAcType = BTRMGR_AC_TYPE_PRIMARY;
    }
    else if (aenCurrentSoType == BTRMGR_STREAM_AUXILIARY) {
        lpi8BTRMgrAcType = BTRMGR_AC_TYPE_AUXILIARY;
    }
    else {
        lpi8BTRMgrAcType = BTRMGR_AC_TYPE_PRIMARY;
    }


    if ((lenBtrMgrRet = BTRMgr_AC_Init(&gstBTRMgrStreamingInfo.hBTRMgrAcHdl, lpi8BTRMgrAcType, gDebugModeEnabled)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_AC_Init FAILED\n");
        return eBTRMgrInitFailure;
    }

    /* Reset the buffer */
    MEMSET_S(&lstBtrMgrAcOutASettings, sizeof(lstBtrMgrAcOutASettings), 0, sizeof(lstBtrMgrAcOutASettings));
    lstBtrMgrAcOutASettings.pstBtrMgrOutCodecInfo = (void*)malloc((sizeof(stBTRMgrPCMInfo) > sizeof(stBTRMgrSBCInfo) ? sizeof(stBTRMgrPCMInfo) : sizeof(stBTRMgrSBCInfo)) > sizeof(stBTRMgrMPEGInfo) ?
                                                                    (sizeof(stBTRMgrPCMInfo) > sizeof(stBTRMgrSBCInfo) ? sizeof(stBTRMgrPCMInfo) : sizeof(stBTRMgrSBCInfo)) : sizeof(stBTRMgrMPEGInfo));
    if (!lstBtrMgrAcOutASettings.pstBtrMgrOutCodecInfo) {
        BTRMGRLOG_ERROR ("MEMORY ALLOC FAILED\n");
        return eBTRMgrFailure;
    }


    if ((lenBtrMgrRet = BTRMgr_AC_GetDefaultSettings(gstBTRMgrStreamingInfo.hBTRMgrAcHdl, &lstBtrMgrAcOutASettings)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR("BTRMgr_AC_GetDefaultSettings FAILED\n");
    }

    if ((lenBtrMgrRet = BTRMgr_SO_Resume(gstBTRMgrStreamingInfo.hBTRMgrSoHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SO_Resume FAILED\n");
    }

    lstBtrMgrAcOutASettings.i32BtrMgrOutBufMaxSize = gstBTRMgrStreamingInfo.i32BytesToEncode;
    if ((lenBtrMgrRet = BTRMgr_AC_Start(gstBTRMgrStreamingInfo.hBTRMgrAcHdl,
                                        &lstBtrMgrAcOutASettings,
                                        btrMgr_ACDataReadyCb,
                                        btrMgr_ACStatusCb,
                                        &gstBTRMgrStreamingInfo)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_AC_Start FAILED\n");
    }

    if (lstBtrMgrAcOutASettings.pstBtrMgrOutCodecInfo)
        free(lstBtrMgrAcOutASettings.pstBtrMgrOutCodecInfo);

    return lenBtrMgrRet;
}

STATIC eBTRMgrRet
btrMgr_StartReceivingAudio (
    int                  inFileFd,
    int                  inMTUSize,
    unsigned int         inDevDelay,
    eBTRCoreDevMediaType aenBtrCoreDevInMType,
    void*                apstBtrCoreDevInMCodecInfo
) {
#ifdef STREAM_IN_SUPPORTED
    eBTRMgrRet          lenBtrMgrRet = eBTRMgrSuccess;
    int                 inBytesToEncode = 3072;
    stBTRMgrInASettings lstBtrMgrSiInASettings;
#endif
    if ((ghBTRMgrDevHdlCurStreaming != 0) || (inMTUSize == 0)) {
        return eBTRMgrFailInArg;
    }
#ifndef STREAM_IN_SUPPORTED
    BTRMGRLOG_INFO("Streaming in not supported\n");
    return eBTRMgrFailure;

#else //STREAM_IN_SUPPORTED
    /* Reset the buffer */
    MEMSET_S(&lstBtrMgrSiInASettings, sizeof(lstBtrMgrSiInASettings), 0, sizeof(lstBtrMgrSiInASettings));


    /* Init StreamIn module - Create Pipeline */
    if ((lenBtrMgrRet = BTRMgr_SI_Init(&gstBTRMgrStreamingInfo.hBTRMgrSiHdl, btrMgr_SIStatusCb, &gstBTRMgrStreamingInfo)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SI_Init FAILED\n");
        return eBTRMgrInitFailure;
    }


    lstBtrMgrSiInASettings.pstBtrMgrInCodecInfo = (void*)malloc((sizeof(stBTRMgrPCMInfo) > sizeof(stBTRMgrSBCInfo) ? sizeof(stBTRMgrPCMInfo) : sizeof(stBTRMgrSBCInfo)) > sizeof(stBTRMgrMPEGInfo) ?
                                                                (sizeof(stBTRMgrPCMInfo) > sizeof(stBTRMgrSBCInfo) ? sizeof(stBTRMgrPCMInfo) : sizeof(stBTRMgrSBCInfo)) : sizeof(stBTRMgrMPEGInfo));

    if (aenBtrCoreDevInMType == eBTRCoreDevMediaTypeSBC) {
        stBTRMgrSBCInfo*            pstBtrMgrSiInSbcInfo       = ((stBTRMgrSBCInfo*)(lstBtrMgrSiInASettings.pstBtrMgrInCodecInfo));
        stBTRCoreDevMediaSbcInfo*   pstBtrCoreDevMediaSbcInfo  = (stBTRCoreDevMediaSbcInfo*)apstBtrCoreDevInMCodecInfo;

        lstBtrMgrSiInASettings.eBtrMgrInAType   = eBTRMgrATypeSBC;
        if (pstBtrMgrSiInSbcInfo && pstBtrCoreDevMediaSbcInfo) {

            if (pstBtrCoreDevMediaSbcInfo->ui32DevMSFreq == 8000) {
                pstBtrMgrSiInSbcInfo->eBtrMgrSbcSFreq  = eBTRMgrSFreq8K;
            }
            else if (pstBtrCoreDevMediaSbcInfo->ui32DevMSFreq == 16000) {
                pstBtrMgrSiInSbcInfo->eBtrMgrSbcSFreq  = eBTRMgrSFreq16K;
            }
            else if (pstBtrCoreDevMediaSbcInfo->ui32DevMSFreq == 32000) {
                pstBtrMgrSiInSbcInfo->eBtrMgrSbcSFreq  = eBTRMgrSFreq32K;
            }
            else if (pstBtrCoreDevMediaSbcInfo->ui32DevMSFreq == 44100) {
                pstBtrMgrSiInSbcInfo->eBtrMgrSbcSFreq  = eBTRMgrSFreq44_1K;
            }
            else if (pstBtrCoreDevMediaSbcInfo->ui32DevMSFreq == 48000) {
                pstBtrMgrSiInSbcInfo->eBtrMgrSbcSFreq  = eBTRMgrSFreq48K;
            }
            else {
                pstBtrMgrSiInSbcInfo->eBtrMgrSbcSFreq  = eBTRMgrSFreqUnknown;
            }


            switch (pstBtrCoreDevMediaSbcInfo->eDevMAChan) {
            case eBTRCoreDevMediaAChanMono:
                pstBtrMgrSiInSbcInfo->eBtrMgrSbcAChan  = eBTRMgrAChanMono;
                break;
            case eBTRCoreDevMediaAChanDualChannel:
                pstBtrMgrSiInSbcInfo->eBtrMgrSbcAChan  = eBTRMgrAChanDualChannel;
                break;
            case eBTRCoreDevMediaAChanStereo:
                pstBtrMgrSiInSbcInfo->eBtrMgrSbcAChan  = eBTRMgrAChanStereo;
                break;
            case eBTRCoreDevMediaAChanJointStereo:
                pstBtrMgrSiInSbcInfo->eBtrMgrSbcAChan  = eBTRMgrAChanJStereo;
                break;
            case eBTRCoreDevMediaAChan5_1:
                pstBtrMgrSiInSbcInfo->eBtrMgrSbcAChan  = eBTRMgrAChan5_1;
                break;
            case eBTRCoreDevMediaAChan7_1:
                pstBtrMgrSiInSbcInfo->eBtrMgrSbcAChan  = eBTRMgrAChan7_1;
                break;
            case eBTRCoreDevMediaAChanUnknown:
            default:
                pstBtrMgrSiInSbcInfo->eBtrMgrSbcAChan  = eBTRMgrAChanUnknown;
                break;
            }

            pstBtrMgrSiInSbcInfo->ui8SbcAllocMethod  = pstBtrCoreDevMediaSbcInfo->ui8DevMSbcAllocMethod;
            pstBtrMgrSiInSbcInfo->ui8SbcSubbands     = pstBtrCoreDevMediaSbcInfo->ui8DevMSbcSubbands;
            pstBtrMgrSiInSbcInfo->ui8SbcBlockLength  = pstBtrCoreDevMediaSbcInfo->ui8DevMSbcBlockLength;
            pstBtrMgrSiInSbcInfo->ui8SbcMinBitpool   = pstBtrCoreDevMediaSbcInfo->ui8DevMSbcMinBitpool;
            pstBtrMgrSiInSbcInfo->ui8SbcMaxBitpool   = pstBtrCoreDevMediaSbcInfo->ui8DevMSbcMaxBitpool;
            pstBtrMgrSiInSbcInfo->ui16SbcFrameLen    = pstBtrCoreDevMediaSbcInfo->ui16DevMSbcFrameLen;
            pstBtrMgrSiInSbcInfo->ui16SbcBitrate     = pstBtrCoreDevMediaSbcInfo->ui16DevMSbcBitrate;
        }
    }
    else if (aenBtrCoreDevInMType == eBTRCoreDevMediaTypeAAC) {
        stBTRMgrMPEGInfo*           pstBtrMgrSiInAacInfo       = ((stBTRMgrMPEGInfo*)(lstBtrMgrSiInASettings.pstBtrMgrInCodecInfo));
        stBTRCoreDevMediaMpegInfo*  pstBtrCoreDevMediaAacInfo  = (stBTRCoreDevMediaMpegInfo*)apstBtrCoreDevInMCodecInfo;

        lstBtrMgrSiInASettings.eBtrMgrInAType   = eBTRMgrATypeAAC;
        if (pstBtrMgrSiInAacInfo && pstBtrCoreDevMediaAacInfo) {

            if (pstBtrCoreDevMediaAacInfo->ui32DevMSFreq == 8000) {
                pstBtrMgrSiInAacInfo->eBtrMgrMpegSFreq  = eBTRMgrSFreq8K;
            }
            else if (pstBtrCoreDevMediaAacInfo->ui32DevMSFreq == 16000) {
                pstBtrMgrSiInAacInfo->eBtrMgrMpegSFreq  = eBTRMgrSFreq16K;
            }
            else if (pstBtrCoreDevMediaAacInfo->ui32DevMSFreq == 32000) {
                pstBtrMgrSiInAacInfo->eBtrMgrMpegSFreq  = eBTRMgrSFreq32K;
            }
            else if (pstBtrCoreDevMediaAacInfo->ui32DevMSFreq == 44100) {
                pstBtrMgrSiInAacInfo->eBtrMgrMpegSFreq  = eBTRMgrSFreq44_1K;
            }
            else if (pstBtrCoreDevMediaAacInfo->ui32DevMSFreq == 48000) {
                pstBtrMgrSiInAacInfo->eBtrMgrMpegSFreq  = eBTRMgrSFreq48K;
            }
            else {
                pstBtrMgrSiInAacInfo->eBtrMgrMpegSFreq  = eBTRMgrSFreqUnknown;
            }


            switch (pstBtrCoreDevMediaAacInfo->eDevMAChan) {
            case eBTRCoreDevMediaAChanMono:
                pstBtrMgrSiInAacInfo->eBtrMgrMpegAChan  = eBTRMgrAChanMono;
                break;
            case eBTRCoreDevMediaAChanDualChannel:
                pstBtrMgrSiInAacInfo->eBtrMgrMpegAChan  = eBTRMgrAChanDualChannel;
                break;
            case eBTRCoreDevMediaAChanStereo:
                pstBtrMgrSiInAacInfo->eBtrMgrMpegAChan  = eBTRMgrAChanStereo;
                break;
            case eBTRCoreDevMediaAChanJointStereo:
                pstBtrMgrSiInAacInfo->eBtrMgrMpegAChan  = eBTRMgrAChanJStereo;
                break;
            case eBTRCoreDevMediaAChan5_1:
                pstBtrMgrSiInAacInfo->eBtrMgrMpegAChan  = eBTRMgrAChan5_1;
                break;
            case eBTRCoreDevMediaAChan7_1:
                pstBtrMgrSiInAacInfo->eBtrMgrMpegAChan  = eBTRMgrAChan7_1;
                break;
            case eBTRCoreDevMediaAChanUnknown:
            default:
                pstBtrMgrSiInAacInfo->eBtrMgrMpegAChan  = eBTRMgrAChanUnknown;
                break;
            }
            
            pstBtrMgrSiInAacInfo->ui8MpegCrc       = pstBtrCoreDevMediaAacInfo->ui8DevMMpegCrc;
            pstBtrMgrSiInAacInfo->ui8MpegLayer     = pstBtrCoreDevMediaAacInfo->ui8DevMMpegLayer;
            pstBtrMgrSiInAacInfo->ui8MpegMpf       = pstBtrCoreDevMediaAacInfo->ui8DevMMpegMpf;
            pstBtrMgrSiInAacInfo->ui8MpegRfa       = pstBtrCoreDevMediaAacInfo->ui8DevMMpegRfa;
            pstBtrMgrSiInAacInfo->ui16MpegBitrate  = pstBtrCoreDevMediaAacInfo->ui16DevMMpegBitrate;
        }
    }
    


    lstBtrMgrSiInASettings.i32BtrMgrDevFd   = inFileFd;
    lstBtrMgrSiInASettings.i32BtrMgrDevMtu  = inMTUSize;


    if ((lenBtrMgrRet = BTRMgr_SI_Start(gstBTRMgrStreamingInfo.hBTRMgrSiHdl, inBytesToEncode, &lstBtrMgrSiInASettings)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SI_Start FAILED\n");
    }


    if (lstBtrMgrSiInASettings.pstBtrMgrInCodecInfo)
        free(lstBtrMgrSiInASettings.pstBtrMgrInCodecInfo);

    return lenBtrMgrRet;
#endif //STREAM_IN_SUPPORTED
}

STATIC eBTRMgrRet
btrMgr_StopReceivingAudio (
    void
) {
#ifdef STREAM_IN_SUPPORTED
    eBTRMgrRet  lenBtrMgrRet = eBTRMgrSuccess;
#endif
    if (ghBTRMgrDevHdlCurStreaming == 0) {
        return eBTRMgrFailInArg;
    }
#ifndef STREAM_IN_SUPPORTED
    BTRMGRLOG_INFO("Streaming in not supported\n");
    return eBTRMgrFailure;

#else
    if ((lenBtrMgrRet = BTRMgr_SI_SendEOS(gstBTRMgrStreamingInfo.hBTRMgrSiHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SI_SendEOS FAILED\n");
    }

    if ((lenBtrMgrRet = BTRMgr_SI_Stop(gstBTRMgrStreamingInfo.hBTRMgrSiHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SI_Stop FAILED\n");
    }

    if ((lenBtrMgrRet = BTRMgr_SI_DeInit(gstBTRMgrStreamingInfo.hBTRMgrSiHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SI_DeInit FAILED\n");
    }

    gstBTRMgrStreamingInfo.hBTRMgrSiHdl = NULL;

    return lenBtrMgrRet;
#endif
}

STATIC eBTRMgrRet btrMgr_UpdateDynamicDelay(unsigned short newDelay)
{
    if (gstBTRMgrStreamingInfo.hBTRMgrSoHdl == NULL)
    {
        BTRMGRLOG_ERROR("Stream not started, not updating delay\n");
        return eBTRMgrFailInArg;
    }
    BTRMGRLOG_INFO("Attempting to update delay value to %d\n", newDelay);
    if(BTRMgr_SO_UpdateDelayDynamically (gstBTRMgrStreamingInfo.hBTRMgrSoHdl, newDelay)!= eBTRMgrSuccess)
    {
        BTRMGRLOG_ERROR("Could not change delay\n");
        return eBTRMgrFailure;
    }
    BTRMGRLOG_INFO("Delay changed successfully\n");
    return eBTRMgrSuccess;
}

STATIC eBTRMgrRet btrMgr_StartPacketCapture(const char * pcFolderPath)
{
    char pcFilepath[BTRMGR_MAX_STR_LEN/4] = {'\0'};
    time_t currTime;
    struct tm * stTime;
    time(&currTime);
    stTime = gmtime(&currTime);
    pid_t pidOfChildProcess;
    char lpcBtVersion[BTRCORE_STR_LEN] = {'\0'};
    if (gPidOfRunningPacketCapture != 0)
    {
        BTRMGRLOG_ERROR("Already running packet capture, no need to start another\n");
        return eBTRMgrSuccess;
    }
    
    snprintf(pcFilepath, BTRMGR_MAX_STR_LEN/4 - 1, "%s/packet_capture-%02d:%02d:%02d.pcap", pcFolderPath, stTime->tm_hour, stTime->tm_min, stTime->tm_sec);
    

    //fork process to maintain the PID of the capture process
    pidOfChildProcess = fork();
    //exec hcidump or btmon script depending on version
    if (pidOfChildProcess == 0)
    {
        BTRMGRLOG_INFO("starting capture at %s\n", pcFilepath);
        if (enBTRCoreSuccess == BTRCore_GetVersionInfo(ghBTRCoreHdl, (char *) lpcBtVersion)) 
        {
            if (!strncmp(lpcBtVersion, "Bluez-5.48", strlen("Bluez-5.48"))) 
            {
                execl("/usr/bin/hcidump", "/usr/bin/hcidump","-w", (const char *) pcFilepath, NULL);
            }
            else 
            {
                execl("/usr/bin/btmon", "/usr/bin/btmon", "-w", (const char *) pcFilepath, NULL);
            }
        }
    }
    else
    {
        BTRMGRLOG_INFO("Created process %d\n", pidOfChildProcess);
        gPidOfRunningPacketCapture = pidOfChildProcess;
    }
    return eBTRMgrSuccess;
}
STATIC eBTRMgrRet btrMgr_StopPacketCapture()
{
    if (gPidOfRunningPacketCapture != 0)
    {
        kill(gPidOfRunningPacketCapture, SIGTERM);
        gPidOfRunningPacketCapture = 0;
        BTRMGRLOG_INFO("Stopped capturing pcaps\n");
        return eBTRMgrSuccess;
    }
    BTRMGRLOG_INFO("Pcaps are not being captured\n");
    return eBTRMgrSuccess;
}

STATIC eBTRMgrRet btrMgr_StartHidEventMonitor()
{
    pid_t pidOfChildProcess;
    if (gPidOfRunningHidMonitor != 0)
    {
        BTRMGRLOG_ERROR("Already running packet capture, no need to start another\n");
        return eBTRMgrSuccess;
    }
    
    //fork process to maintain the PID of the capture process
    pidOfChildProcess = fork();
    //exec hcidump or btmon script depending on version
    if (pidOfChildProcess == 0)
    {
        BTRMGRLOG_INFO("starting hid monitor\n");

        execl("/usr/bin/btrMgrHidEvtMonitor", "/usr/bin/btrMgrHidEvtMonitor", NULL);
            
    }
    else
    {
        BTRMGRLOG_INFO("Created process %d\n", pidOfChildProcess);
        gPidOfRunningHidMonitor = pidOfChildProcess;
    }
    return eBTRMgrSuccess;
}
STATIC eBTRMgrRet btrMgr_StopHidEventMonitor()
{
    if (gPidOfRunningHidMonitor != 0)
    {
        kill(gPidOfRunningHidMonitor, SIGTERM);
        gPidOfRunningHidMonitor = 0;
        BTRMGRLOG_INFO("Stopped capturing hid monitor\n");
        return eBTRMgrSuccess;
    }
    BTRMGRLOG_INFO("hid events are not being captured\n");
    return eBTRMgrSuccess;
}
#ifndef BUILD_FOR_PI

STATIC eBTRMgrRet btrMgr_SetProcessLogLevel(char * pcProcessName, char * pcComponent, char * pcLogLevel)
{
    char pcCmd[BTRMGR_MAX_STR_LEN/2] = {'\0'};
    snprintf(pcCmd, BTRMGR_MAX_STR_LEN/2, "rdklogctrl %s %s %s", pcProcessName, pcComponent, pcLogLevel);
    BTRMGRLOG_INFO("Running: %s\n", pcCmd);
    system(pcCmd);
    return eBTRMgrSuccess;
}
#endif //BUILD_FOR_PI
#endif


STATIC eBTRMgrRet
btrMgr_ConnectToDevice (
    unsigned char                   aui8AdapterIdx,
    BTRMgrDeviceHandle              ahBTRMgrDevHdl,
    BTRMGR_DeviceOperationType_t    connectAs,
    unsigned int                    aui32ConnectRetryIdx,
    unsigned int                    aui32ConfirmIdx
) {
    stBTRCoreBTDevice   stDeviceInfo;
    eBTRMgrRet          lenBtrMgrRet        = eBTRMgrSuccess;
    enBTRCoreRet        lenBtrCoreRet       = enBTRCoreSuccess;
    int                 i32PairDevIdx       = 0;
    unsigned int        ui32retryIdx        = aui32ConnectRetryIdx + 1;
    enBTRCoreDeviceType lenBTRCoreDeviceType= enBTRCoreUnknown;
    BOOLEAN lbtriggerConnectCompleteEvt     = FALSE;

    lenBtrMgrRet = btrMgr_PreCheckDiscoveryStatus(aui8AdapterIdx, connectAs);

    if (eBTRMgrSuccess != lenBtrMgrRet) {
        BTRMGRLOG_ERROR ("Pre Check Discovery State Rejected !!!\n");
        return lenBtrMgrRet;
    }

    switch (connectAs) {
    case BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT:
        gui8IsSoDevAvrcpSupported = 0;      // TODO: Find a better way to do this
        lenBTRCoreDeviceType = enBTRCoreSpeakers;
        break;
    case BTRMGR_DEVICE_OP_TYPE_AUDIO_INPUT:
        lenBTRCoreDeviceType = enBTRCoreMobileAudioIn;
        if (!gIsAudioInEnabled) {
            BTRMGRLOG_WARN ("Connection Rejected - BTR AudioIn is currently Disabled!\n");
            return BTRMGR_RESULT_GENERIC_FAILURE;
        }
        break;
    case BTRMGR_DEVICE_OP_TYPE_LE:
        lenBTRCoreDeviceType = enBTRCoreLE;
        break;
    case BTRMGR_DEVICE_OP_TYPE_HID:        
        lenBTRCoreDeviceType = enBTRCoreHID;
        ghBTRMgrDevHdlConnInProgress = ahBTRMgrDevHdl;
        break;
    case BTRMGR_DEVICE_OP_TYPE_UNKNOWN:
    default:
        lenBTRCoreDeviceType = enBTRCoreUnknown;
        break;
    } 


    do {
        /* connectAs param is unused for now.. */
        lenBtrCoreRet = BTRCore_ConnectDevice (ghBTRCoreHdl, ahBTRMgrDevHdl, lenBTRCoreDeviceType);
        if (lenBtrCoreRet != enBTRCoreSuccess) {
            BTRMGRLOG_ERROR ("Failed to Connect to this device - %llu\n", ahBTRMgrDevHdl);
            lenBtrMgrRet = eBTRMgrFailure;
        }
        else {
            for (i32PairDevIdx = 0; i32PairDevIdx <= gListOfPairedDevices.m_numOfDevices; i32PairDevIdx++) {
                 if (ahBTRMgrDevHdl == gListOfPairedDevices.m_deviceProperty[i32PairDevIdx].m_deviceHandle) {
                     BTRMGRLOG_INFO ("Connected Successfully handle,Name:  %llu,%s\n",ahBTRMgrDevHdl,gListOfPairedDevices.m_deviceProperty[i32PairDevIdx].m_name);
                     break;
                 }
            }

            lenBtrMgrRet = eBTRMgrSuccess;
#ifdef RDKTV_PERSIST_VOLUME
            unsigned char lui8Volume = BTRMGR_SO_MAX_VOLUME - 1;
            if (btrMgr_GetLastVolume(aui8AdapterIdx, &lui8Volume, ahBTRMgrDevHdl, BTRMGR_A2DP_SINK_PROFILE_ID) == eBTRMgrSuccess) {
                BTRMGRLOG_INFO("Device connection is in progress ..\n");
                ghBTRMgrDevHdlVolSetupInProgress = ahBTRMgrDevHdl;
            }
#endif
        }

        if (lenBtrMgrRet != eBTRMgrFailure) {
            /* Max 15 sec timeout - Polled at 1 second interval: Confirmed 5 times */
            unsigned int ui32sleepTimeOut = 1;
            unsigned int ui32confirmIdx = aui32ConfirmIdx + 1;
            unsigned int ui32AdjSleepIdx = (aui32ConfirmIdx > BTRMGR_DEVCONN_CHECK_RETRY_ATTEMPTS) ? 2 : 5; //Interval in attempts

            do {
                unsigned int ui32sleepIdx = ui32AdjSleepIdx;

                do {
                    sleep(ui32sleepTimeOut);
                    lenBtrCoreRet = BTRCore_GetDeviceConnected(ghBTRCoreHdl, ahBTRMgrDevHdl, lenBTRCoreDeviceType);
                } while ((lenBtrCoreRet != enBTRCoreSuccess) && (--ui32sleepIdx));
            } while (--ui32confirmIdx);

            //Inside this function there is a failure logs so, did not any failures logs here.
            btrMgr_GetDeviceDetails(ahBTRMgrDevHdl,&stDeviceInfo);

            if (lenBtrCoreRet != enBTRCoreSuccess) {
                //This is telemetry log. If we change this print,need to change and configure the telemetry string in xconf server.
                BTRMGRLOG_ERROR ("Failed to Connect to this device - Confirmed name,class,apperance,modalias: %s,%u,%u,v%04Xp%04Xd%04X\n",
                stDeviceInfo.pcDeviceName, stDeviceInfo.ui32DevClassBtSpec, stDeviceInfo.ui16DevAppearanceBleSpec,
                stDeviceInfo.ui32ModaliasVendorId, stDeviceInfo.ui32ModaliasProductId, stDeviceInfo.ui32ModaliasDeviceId);
                BTRMGRLOG_ERROR ("Connection Failed device MAC %s\n", stDeviceInfo.pcDeviceAddress);

                lenBtrMgrRet = eBTRMgrFailure;

                if (lenBTRCoreDeviceType == enBTRCoreHID) {
                    ghBTRMgrDevHdlConnInProgress = 0;
                    BTRMGR_EventMessage_t lstEventMessage;
                    MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));

                    lstEventMessage.m_adapterIndex  = aui8AdapterIdx;
                    lstEventMessage.m_eventType     = BTRMGR_EVENT_DEVICE_CONNECTION_FAILED;
                    MEMCPY_S(&lstEventMessage.m_pairedDevice, sizeof(BTRMGR_PairedDevices_t), &gListOfPairedDevices.m_deviceProperty[i32PairDevIdx], sizeof(BTRMGR_PairedDevices_t));

                    if (gfpcBBTRMgrEventOut) {
                        gfpcBBTRMgrEventOut(lstEventMessage); /*  Post a callback */
                    }
                }

                /* Skipped disconnecting the HID device. During connection failure, LE gamepads are removed from the kernel auto-connect list,
                * preventing auto-connection.
                */
                if (lenBTRCoreDeviceType != enBTRCoreHID) {
                    if (BTRCore_DisconnectDevice (ghBTRCoreHdl, ahBTRMgrDevHdl, lenBTRCoreDeviceType) != enBTRCoreSuccess) {
                        BTRMGRLOG_ERROR ("Failed to Disconnect - %llu\n", ahBTRMgrDevHdl);
                    }
                }
            }
            else {
                //This is telemetry log. If we change this print,need to change and configure the telemetry string in xconf server.
                BTRMGRLOG_INFO ("Succes Connect to this device - Confirmed name,class,apperance,modalias: %s,%u,%u,v%04Xp%04Xd%04X\n",
                stDeviceInfo.pcDeviceName, stDeviceInfo.ui32DevClassBtSpec, stDeviceInfo.ui16DevAppearanceBleSpec,
                stDeviceInfo.ui32ModaliasVendorId, stDeviceInfo.ui32ModaliasProductId, stDeviceInfo.ui32ModaliasDeviceId);

                BTRMGRLOG_INFO ("Connect device - Confirmed - %llu DeviceType - %d MAC %s\n", ahBTRMgrDevHdl,lenBTRCoreDeviceType,stDeviceInfo.pcDeviceAddress);

                if ((lenBTRCoreDeviceType == enBTRCoreSpeakers) || (lenBTRCoreDeviceType == enBTRCoreHeadSet)) {
                    btrMgr_AddPersistentEntry(aui8AdapterIdx, ahBTRMgrDevHdl, BTRMGR_A2DP_SINK_PROFILE_ID, 1);
                }
                else if ((lenBTRCoreDeviceType == enBTRCoreMobileAudioIn) || (lenBTRCoreDeviceType == enBTRCorePCAudioIn)) {
                    btrMgr_AddPersistentEntry(aui8AdapterIdx, ahBTRMgrDevHdl, BTRMGR_A2DP_SRC_PROFILE_ID, 1);
                }

                if (lenBTRCoreDeviceType == enBTRCoreLE) {
                    gIsLeDeviceConnected = 1;
                }
                else if ((lenBTRCoreDeviceType == enBTRCoreSpeakers) || (lenBTRCoreDeviceType == enBTRCoreHeadSet) ||
                         (lenBTRCoreDeviceType == enBTRCoreMobileAudioIn) || (lenBTRCoreDeviceType == enBTRCorePCAudioIn)) {
                    btrMgr_SetDevConnected(ahBTRMgrDevHdl, 1);
                    gIsUserInitiated = 0;
                    ghBTRMgrDevHdlLastConnected = ahBTRMgrDevHdl;
                }
                else if (lenBTRCoreDeviceType == enBTRCoreHID) {
                    btrMgr_SetDevConnected(ahBTRMgrDevHdl, 1);
                    BTRCore_newBatteryLevelDevice(ghBTRCoreHdl);
                    gIsUserInitiated = 0;
                    ghBTRMgrDevHdlConnInProgress = 0;
                    
                    BTRMGR_EventMessage_t lstEventMessage;
                    MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));

                    lstEventMessage.m_adapterIndex  = aui8AdapterIdx;
                    lstEventMessage.m_eventType     = BTRMGR_EVENT_DEVICE_CONNECTION_COMPLETE;
                    MEMCPY_S(&lstEventMessage.m_pairedDevice, sizeof(BTRMGR_PairedDevices_t), &gListOfPairedDevices.m_deviceProperty[i32PairDevIdx], sizeof(BTRMGR_PairedDevices_t));

                    if (gfpcBBTRMgrEventOut) {
                        gfpcBBTRMgrEventOut(lstEventMessage); /*  Post a callback */
                    }
                }
                else {
                    btrMgr_SetDevConnected(ahBTRMgrDevHdl, 1);
                }
                if (ghBTRMgrDevHdlLastDisconnected == ahBTRMgrDevHdl) {
                    BTRMGRLOG_INFO("Clearing the disconnected handle since the same device connected again\n");
                    ghBTRMgrDevHdlLastDisconnected = 0;
                }
            }
        }
    } while ((lenBtrMgrRet == eBTRMgrFailure) && (--ui32retryIdx));

    if (lenBtrMgrRet == eBTRMgrFailure && lenBTRCoreDeviceType == enBTRCoreLE) {
        connectAs = BTRMGR_DEVICE_OP_TYPE_UNKNOWN;
    }

    btrMgr_PostCheckDiscoveryStatus(aui8AdapterIdx, connectAs);

    return lenBtrMgrRet;
}

#ifdef RDKTV_PERSIST_VOLUME
STATIC gboolean btrMgr_ClearConnectionInProgressflagCb(gpointer user_data)
{
    BTRMGRLOG_DEBUG("Cleared the connection InProgress flag \n");
    ghBTRMgrDevHdlVolSetupInProgress = 0;
    btrMgr_ClearConInProgressStatusHoldOffTimer();
    return G_SOURCE_REMOVE;
}
#endif
#ifndef LE_MODE
STATIC eBTRMgrRet
btrMgr_StartAudioStreamingOut (
    unsigned char                   aui8AdapterIdx,
    BTRMgrDeviceHandle              ahBTRMgrDevHdl,
    BTRMGR_DeviceOperationType_t    streamOutPref,
    unsigned int                    aui32ConnectRetryIdx,
    unsigned int                    aui32ConfirmIdx,
    unsigned int                    aui32SleepIdx
) {
    BTRMGR_Result_t             lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    eBTRMgrRet                  lenBtrMgrRet    = eBTRMgrSuccess;
    enBTRCoreRet                lenBtrCoreRet   = enBTRCoreSuccess;
    unsigned char               isFound = 0;
    int                         i = 0, j = 0;
    int                         deviceFD = 0;
    int                         deviceReadMTU = 0;
    int                         deviceWriteMTU = 0;
    unsigned int                deviceDelay = 0xFFFFu;
    unsigned int                ui32retryIdx = aui32ConnectRetryIdx + 1;
    stBTRCorePairedDevicesCount listOfPDevices;
    eBTRCoreDevMediaType        lenBtrCoreDevOutMType = eBTRCoreDevMediaTypeUnknown;
    void*                       lpstBtrCoreDevOutMCodecInfo = NULL;
    enBTRCoreDeviceType         lenBTRCoreDevTy = enBTRCoreUnknown;
    enBTRCoreDeviceClass        lenBTRCoreDevCl = enBTRCore_DC_Unknown;
    char                        ProfileStr[BTRMGR_MAX_STR_LEN] = {'\0'};
    unsigned char               ui8FlushTimeoutMs = 0;


    if (ghBTRMgrDevHdlCurStreaming == ahBTRMgrDevHdl) {
        BTRMGRLOG_WARN ("Its already streaming out in this device.. Check the volume :)\n");
        return eBTRMgrSuccess;
    }

    ghBTRMgrDevHdlStreamStartUp = ahBTRMgrDevHdl;
    if ((ghBTRMgrDevHdlCurStreaming != 0) && (ghBTRMgrDevHdlCurStreaming != ahBTRMgrDevHdl)) {

        BTRMGRLOG_WARN ("Its already streaming out. lets stop this and start on other device \n");

        lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ghBTRMgrDevHdlCurStreaming, &lenBTRCoreDevTy, &lenBTRCoreDevCl);
        BTRMGRLOG_INFO ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBTRCoreDevTy, lenBTRCoreDevCl);

        if ((lenBTRCoreDevTy == enBTRCoreSpeakers) || (lenBTRCoreDevTy == enBTRCoreHeadSet)) {
            /* Streaming-Out is happening; stop it */
            if ((lenBtrMgrResult = BTRMGR_StopAudioStreamingOut(aui8AdapterIdx, ghBTRMgrDevHdlCurStreaming)) != BTRMGR_RESULT_SUCCESS) {
                BTRMGRLOG_ERROR ("This device is being Connected n Playing. Failed to stop Playback.-Out\n");
                BTRMGRLOG_ERROR ("Failed to stop streaming at the current device..\n");
                ghBTRMgrDevHdlStreamStartUp = 0;
                return lenBtrMgrResult;
            }
        }
        else if ((lenBTRCoreDevTy == enBTRCoreMobileAudioIn) || (lenBTRCoreDevTy == enBTRCorePCAudioIn)) {
            /* Streaming-In is happening; stop it */
            if ((lenBtrMgrResult = BTRMGR_StopAudioStreamingIn(aui8AdapterIdx, ghBTRMgrDevHdlCurStreaming)) != BTRMGR_RESULT_SUCCESS) {
                BTRMGRLOG_ERROR ("This device is being Connected n Playing. Failed to stop Playback.-In\n");
                BTRMGRLOG_ERROR ("Failed to stop streaming at the current device..\n");
                ghBTRMgrDevHdlStreamStartUp = 0;
                return lenBtrMgrResult;
            }
        }
    }

    /* Check whether the device is in the paired list */
    MEMSET_S(&listOfPDevices, sizeof(listOfPDevices), 0, sizeof(listOfPDevices));
    if ((lenBtrCoreRet = BTRCore_GetListOfPairedDevices(ghBTRCoreHdl, &listOfPDevices)) != enBTRCoreSuccess) {
        BTRMGRLOG_ERROR ("Failed to get the paired devices list\n");
        ghBTRMgrDevHdlStreamStartUp = 0;
        return eBTRMgrFailure;
    }


    if (!listOfPDevices.numberOfDevices) {
        BTRMGRLOG_ERROR ("No device is paired yet; Will not be able to play at this moment\n");
        ghBTRMgrDevHdlStreamStartUp = 0;
        return eBTRMgrFailure;
    }


    for (j = 0; j < listOfPDevices.numberOfDevices; j++) {
        //device is in paired list
        if (ahBTRMgrDevHdl == listOfPDevices.devices[j].tDeviceId) {
            isFound = 1;
            i = j; //i represents the target device position in the paired device array in the remainder of the function
        }
        BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, listOfPDevices.devices[j].tDeviceId, &lenBTRCoreDevTy, &lenBTRCoreDevCl);
        //there is another audio device connected but not streaming out, disconnect it
        if (listOfPDevices.devices[j].tDeviceId != ahBTRMgrDevHdl &&
           (lenBTRCoreDevTy == enBTRCoreSpeakers || lenBTRCoreDevTy == enBTRCoreHeadSet) &&
           listOfPDevices.devices[j].bDeviceConnected == true)
        {
            BTRMGRLOG_DEBUG("Device: %lld is connected but not streaming out\n", listOfPDevices.devices[j].tDeviceId);
            if (BTRCore_DisconnectDevice(ghBTRCoreHdl, listOfPDevices.devices[j].tDeviceId, listOfPDevices.devices[j].enDeviceType) != enBTRCoreSuccess)
            {
                BTRMGRLOG_ERROR("Audio device connected but we could not disconnect\n");
                ghBTRMgrDevHdlStreamStartUp = 0;
                return eBTRMgrFailure;
            }
        }
    }


    if (!isFound) {
        BTRMGRLOG_ERROR ("Failed to find this device in the paired devices list\n");
        ghBTRMgrDevHdlStreamStartUp = 0;
        return eBTRMgrFailure;
    }

    /* Populate the currently Paired Devices.*/
    BTRMGR_GetPairedDevices (aui8AdapterIdx, &gListOfPairedDevices);

    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBTRCoreDevTy, &lenBTRCoreDevCl);
    BTRMGRLOG_INFO ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBTRCoreDevTy, lenBTRCoreDevCl);

    if ((lenBTRCoreDevTy == enBTRCoreSpeakers) || (lenBTRCoreDevTy == enBTRCoreHeadSet)) {
        MEMCPY_S(ProfileStr,sizeof(ProfileStr),BTRMGR_A2DP_SINK_PROFILE_ID,sizeof(BTRMGR_A2DP_SINK_PROFILE_ID));
    } else if ((lenBTRCoreDevTy == enBTRCoreMobileAudioIn) || (lenBTRCoreDevTy == enBTRCorePCAudioIn)) {
        MEMCPY_S(ProfileStr,sizeof(ProfileStr),BTRMGR_A2DP_SRC_PROFILE_ID,sizeof(BTRMGR_A2DP_SRC_PROFILE_ID));
    } else if (lenBTRCoreDevTy == enBTRCoreHID) {
       BTRMGRLOG_ERROR("It's a gaming device, skipped the streaming ...\n");
       ghBTRMgrDevHdlStreamStartUp = 0;
       return eBTRMgrFailure;
    }

    if (aui32ConnectRetryIdx) {
        unsigned int ui32sleepTimeOut = 2;
        unsigned int ui32confirmIdx = aui32ConfirmIdx + 1;

        do {
            unsigned int ui32sleepIdx = aui32SleepIdx + 1;
            do {
                sleep(ui32sleepTimeOut);
                lenBtrCoreRet = BTRCore_IsDeviceConnectable(ghBTRCoreHdl, listOfPDevices.devices[i].tDeviceId);
            } while ((lenBtrCoreRet != enBTRCoreSuccess) && (--ui32sleepIdx));
        } while ((lenBtrCoreRet != enBTRCoreSuccess) && (--ui32confirmIdx));

        if (lenBtrCoreRet != enBTRCoreSuccess) {
            BTRMGRLOG_ERROR ("Device Not Connectable\n");
            ghBTRMgrDevHdlStreamStartUp = 0;
            return eBTRMgrFailure;
        }
        else if ( (ghBTRMgrDevHdlCurStreaming == listOfPDevices.devices[i].tDeviceId) &&
                  (ghBTRMgrDevHdlLastConnected == listOfPDevices.devices[i].tDeviceId)) {
            BTRMGRLOG_INFO ("Device Already Connected and Streaming = %lld\n", listOfPDevices.devices[i].tDeviceId);
            ghBTRMgrDevHdlStreamStartUp = 0;
            return eBTRMgrSuccess;
        }
    }


    do {
        unsigned short  ui16DevMediaBitrate = 0;
        /* Connect the device  - If the device is not connected, Connect to it */
        if (aui32ConnectRetryIdx) {
            lenBtrMgrRet = btrMgr_ConnectToDevice(aui8AdapterIdx, listOfPDevices.devices[i].tDeviceId, streamOutPref, BTRMGR_CONNECT_RETRY_ATTEMPTS, BTRMGR_DEVCONN_CHECK_RETRY_ATTEMPTS);
        }
        else if (strstr(listOfPDevices.devices[i].pcDeviceName, "Denon") || strstr(listOfPDevices.devices[i].pcDeviceName, "AVR") ||
                 strstr(listOfPDevices.devices[i].pcDeviceName, "DENON") || strstr(listOfPDevices.devices[i].pcDeviceName, "Avr")) {
            lenBtrMgrRet = btrMgr_ConnectToDevice(aui8AdapterIdx, listOfPDevices.devices[i].tDeviceId, streamOutPref, 0, BTRMGR_DEVCONN_CHECK_RETRY_ATTEMPTS + 2);
        }
        else {
            lenBtrMgrRet = btrMgr_ConnectToDevice(aui8AdapterIdx, listOfPDevices.devices[i].tDeviceId, streamOutPref, 0, 1);
        }

        if (lenBtrMgrRet == eBTRMgrSuccess) {
            if (gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo) {
                free (gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo);
                gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo = NULL;
            }


            gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo = (void*)malloc((sizeof(stBTRCoreDevMediaPcmInfo) > sizeof(stBTRCoreDevMediaSbcInfo) ? sizeof(stBTRCoreDevMediaPcmInfo) : sizeof(stBTRCoreDevMediaSbcInfo)) > sizeof(stBTRCoreDevMediaMpegInfo) ?
                                                                           (sizeof(stBTRCoreDevMediaPcmInfo) > sizeof(stBTRCoreDevMediaSbcInfo) ? sizeof(stBTRCoreDevMediaPcmInfo) : sizeof(stBTRCoreDevMediaSbcInfo)) : sizeof(stBTRCoreDevMediaMpegInfo));

            lenBtrCoreRet = BTRCore_GetDeviceMediaInfo(ghBTRCoreHdl, listOfPDevices.devices[i].tDeviceId, enBTRCoreSpeakers, &gstBtrCoreDevMediaInfo);
            if (lenBtrCoreRet == enBTRCoreSuccess) {
                lenBtrCoreDevOutMType      = gstBtrCoreDevMediaInfo.eBtrCoreDevMType;
                lpstBtrCoreDevOutMCodecInfo= gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo;

                if (lenBtrCoreDevOutMType == eBTRCoreDevMediaTypeSBC) {
                    stBTRCoreDevMediaSbcInfo*   lpstBtrCoreDevMSbcInfo = (stBTRCoreDevMediaSbcInfo*)lpstBtrCoreDevOutMCodecInfo;

                    ui16DevMediaBitrate = lpstBtrCoreDevMSbcInfo->ui16DevMSbcBitrate;

                    BTRMGRLOG_INFO ("DevMedInfo SFreq         = %d\n", lpstBtrCoreDevMSbcInfo->ui32DevMSFreq);
                    BTRMGRLOG_INFO ("DevMedInfo AChan         = %d\n", lpstBtrCoreDevMSbcInfo->eDevMAChan);
                    BTRMGRLOG_INFO ("DevMedInfo SbcAllocMethod= %d\n", lpstBtrCoreDevMSbcInfo->ui8DevMSbcAllocMethod);
                    BTRMGRLOG_INFO ("DevMedInfo SbcSubbands   = %d\n", lpstBtrCoreDevMSbcInfo->ui8DevMSbcSubbands);
                    BTRMGRLOG_INFO ("DevMedInfo SbcBlockLength= %d\n", lpstBtrCoreDevMSbcInfo->ui8DevMSbcBlockLength);
                    BTRMGRLOG_INFO ("DevMedInfo SbcMinBitpool = %d\n", lpstBtrCoreDevMSbcInfo->ui8DevMSbcMinBitpool);
                    BTRMGRLOG_INFO ("DevMedInfo SbcMaxBitpool = %d\n", lpstBtrCoreDevMSbcInfo->ui8DevMSbcMaxBitpool);
                    BTRMGRLOG_INFO ("DevMedInfo SbcFrameLen   = %d\n", lpstBtrCoreDevMSbcInfo->ui16DevMSbcFrameLen);
                    BTRMGRLOG_INFO ("DevMedInfo SbcBitrate    = %d\n", lpstBtrCoreDevMSbcInfo->ui16DevMSbcBitrate);
                }
            }

            if (ui16DevMediaBitrate) {
                /* Aquire Device Data Path to start the audio casting */
                lenBtrCoreRet = BTRCore_AcquireDeviceDataPath(ghBTRCoreHdl, listOfPDevices.devices[i].tDeviceId, enBTRCoreSpeakers, &deviceFD, &deviceReadMTU, &deviceWriteMTU, &deviceDelay);
                if ((lenBtrCoreRet == enBTRCoreSuccess) && deviceWriteMTU) {
                    /* Now that you got the FD & Read/Write MTU, start casting the audio */
                    if ((lenBtrMgrRet = btrMgr_StartCastingAudio(deviceFD, deviceWriteMTU, deviceDelay, lenBtrCoreDevOutMType, lpstBtrCoreDevOutMCodecInfo,listOfPDevices.devices[i].tDeviceId,ProfileStr)) == eBTRMgrSuccess) {
                        ghBTRMgrDevHdlCurStreaming = listOfPDevices.devices[i].tDeviceId;
                        BTRMGRLOG_INFO("Streaming Started.. Enjoy the show..! :)\n");
                        ghBTRMgrDevHdlStreamStartUp = 0;

                        ui8FlushTimeoutMs = (deviceWriteMTU >= BTMGR_LARGE_MTU_THRESHOLD) ? BTMGR_FLUSH_TIMEOUT_LARGE_MTU_INTERVAL_MS : BTMGR_FLUSH_TIMEOUT_INTERVAL_MS;
                        if (BTRCore_SetDeviceDataAckTimeout(ghBTRCoreHdl, ui8FlushTimeoutMs) != enBTRCoreSuccess) {
                            BTRMGRLOG_WARN ("Failed to set timeout for Audio drop. EXPECT AV SYNC ISSUES!\n");
                        }
                    }
                    else {
                        BTRMGRLOG_ERROR ("Failed to stream now\n");
                    }
                }
            }

        }


        if (!ui16DevMediaBitrate || (lenBtrCoreRet != enBTRCoreSuccess) || (lenBtrMgrRet != eBTRMgrSuccess)) {
            BTRMGRLOG_ERROR ("Failed to get Device Data Path. So Will not be able to stream now\n");
            BTRMGRLOG_ERROR ("Failed to get Valid MediaBitrate. So Will not be able to stream now\n");
            BTRMGRLOG_ERROR ("Failed to StartCastingAudio. So Will not be able to stream now\n");
            BTRMGRLOG_ERROR ("Failed to connect to device and not playing\n");
            lenBtrCoreRet = BTRCore_DisconnectDevice (ghBTRCoreHdl, listOfPDevices.devices[i].tDeviceId, enBTRCoreSpeakers);
            if (lenBtrCoreRet == enBTRCoreSuccess) {
                /* Max 4 sec timeout - Polled at 1 second interval: Confirmed 2 times */
                unsigned int ui32sleepTimeOut = 1;
                unsigned int ui32confirmIdx = aui32ConfirmIdx + 1;
                
                do {
                    unsigned int ui32sleepIdx = aui32SleepIdx + 1;

                    do {
                        sleep(ui32sleepTimeOut);
                        lenBtrCoreRet = BTRCore_GetDeviceDisconnected(ghBTRCoreHdl, listOfPDevices.devices[i].tDeviceId, enBTRCoreSpeakers);
                    } while ((lenBtrCoreRet != enBTRCoreSuccess) && (--ui32sleepIdx));
                } while (--ui32confirmIdx);

                if (lenBtrCoreRet != enBTRCoreSuccess) {
                    BTRMGRLOG_ERROR ("Failed to Disconnect from this device - Confirmed - %llu\n", listOfPDevices.devices[i].tDeviceId);
                    lenBtrMgrRet = eBTRMgrFailure; 
                }
                else {
                    BTRMGRLOG_DEBUG ("Success Disconnect from this device - Confirmed - %llu\n", listOfPDevices.devices[i].tDeviceId);
                    if ((lenBTRCoreDevTy == enBTRCoreSpeakers) || (lenBTRCoreDevTy == enBTRCoreHeadSet)) {
#ifdef RDKTV_PERSIST_VOLUME
                        ghBTRMgrDevHdlVolSetupInProgress = 0;
#endif
                        btrMgr_SetLastConnectionStatus(aui8AdapterIdx, 0, listOfPDevices.devices[i].tDeviceId, BTRMGR_A2DP_SINK_PROFILE_ID);
                    }
                }
            }

            lenBtrMgrRet = eBTRMgrFailure; 
        }


    } while ((lenBtrMgrRet == eBTRMgrFailure) && (--ui32retryIdx));


    {
        BTRMGR_EventMessage_t lstEventMessage;
        MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));

        lstEventMessage.m_adapterIndex                 = aui8AdapterIdx;
        lstEventMessage.m_pairedDevice.m_deviceHandle  = listOfPDevices.devices[i].tDeviceId;
        lstEventMessage.m_pairedDevice.m_deviceType    = btrMgr_MapDeviceTypeFromCore(listOfPDevices.devices[i].enDeviceType);
        lstEventMessage.m_pairedDevice.m_isConnected   = (btrMgr_IsDevConnected(listOfPDevices.devices[i].tDeviceId)) ? 1 : 0;
        if ((lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_TILE) ||
            (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_XBB)) {
            lstEventMessage.m_pairedDevice.m_isLowEnergyDevice = 1;
        } else {
            lstEventMessage.m_pairedDevice.m_isLowEnergyDevice = 0;
        }
        strncpy(lstEventMessage.m_pairedDevice.m_name, listOfPDevices.devices[i].pcDeviceName, BTRMGR_NAME_LEN_MAX - 1);
        strncpy(lstEventMessage.m_pairedDevice.m_deviceAddress, listOfPDevices.devices[i].pcDeviceAddress, BTRMGR_NAME_LEN_MAX - 1);

        if (lenBtrMgrRet == eBTRMgrSuccess) {
            lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_CONNECTION_COMPLETE;

            if (gfpcBBTRMgrEventOut) {
                gfpcBBTRMgrEventOut(lstEventMessage); /*  Post a callback */
            }
        }
        else if (lenBtrMgrRet == eBTRMgrFailure) {
            lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_CONNECTION_FAILED;

            if (gfpcBBTRMgrEventOut) {
                gfpcBBTRMgrEventOut(lstEventMessage); /*  Post a callback */
            }
        }
        else {
            //TODO: Some error specific event to XRE
        }
    }
#ifdef RDKTV_PERSIST_VOLUME
    if (ghBTRMgrDevHdlVolSetupInProgress != 0) {
        BTRMGRLOG_INFO("Initiating a timer on different context to clear the connection in progress flag \n");
        btrMgr_SetConInProgressStatusHoldOffTimer();
    }
#endif
    ghBTRMgrDevHdlStreamStartUp = 0;
    return lenBtrMgrRet;
}
#endif

STATIC eBTRMgrRet
btrMgr_AddPersistentEntry (
    unsigned char       aui8AdapterIdx,
    BTRMgrDeviceHandle  ahBTRMgrDevHdl,
    const char*         apui8ProfileStr,
    int                 ai32DevConnected
) {
    char        lui8adapterAddr[BD_NAME_LEN] = {'\0'};
    eBTRMgrRet  lenBtrMgrPiRet = eBTRMgrFailure;

    BTRCore_GetAdapterAddr(ghBTRCoreHdl, aui8AdapterIdx, lui8adapterAddr);

    // Device connected add data from json file
    BTRMGR_Profile_t btProfile;
    strncpy(btProfile.adapterId, lui8adapterAddr, BTRMGR_NAME_LEN_MAX -1);
    btProfile.adapterId[BTRMGR_NAME_LEN_MAX -1] = '\0';
    strncpy(btProfile.profileId, apui8ProfileStr, BTRMGR_NAME_LEN_MAX -1);
    btProfile.profileId[BTRMGR_NAME_LEN_MAX -1] = '\0'; //CID:136398 - Bufefr size warning
    btProfile.deviceId  = ahBTRMgrDevHdl;
    btProfile.isConnect = ai32DevConnected;
    /* Setting it as volume 255 during Pairing/Connection on first time.
     * once after the connection was successful,through the MediaStatus callback
     * volume will be updated. For BT devices with no AVRCP support volume status
     * will be updated when we set the volume from UI (or) BT device.
     */

#ifdef RDKTV_PERSIST_VOLUME
    btProfile.Volume = BTRMGR_SO_MAX_VOLUME;
#endif

    lenBtrMgrPiRet = BTRMgr_PI_AddProfile(ghBTRMgrPiHdl, &btProfile);
    if(lenBtrMgrPiRet == eBTRMgrSuccess) {
        BTRMGRLOG_INFO ("Persistent File updated successfully\n");
    }
    else {
        BTRMGRLOG_ERROR ("Persistent File update failed \n");
    }

    return lenBtrMgrPiRet;
}

STATIC eBTRMgrRet
btrMgr_RemovePersistentEntry (
    unsigned char       aui8AdapterIdx,
    BTRMgrDeviceHandle  ahBTRMgrDevHdl,
    const char*         apui8ProfileStr
) {
    char         lui8adapterAddr[BD_NAME_LEN] = {'\0'};
    eBTRMgrRet   lenBtrMgrPiRet = eBTRMgrFailure;

    BTRCore_GetAdapterAddr(ghBTRCoreHdl, aui8AdapterIdx, lui8adapterAddr);

    // Device disconnected remove data from json file
    BTRMGR_Profile_t btPtofile;
    strncpy(btPtofile.adapterId, lui8adapterAddr, BTRMGR_NAME_LEN_MAX -1);
    btPtofile.adapterId[BTRMGR_NAME_LEN_MAX -1] = '\0';
    strncpy(btPtofile.profileId, apui8ProfileStr, BTRMGR_NAME_LEN_MAX -1);
    btPtofile.profileId[BTRMGR_NAME_LEN_MAX -1] = '\0';  //CID:136475 - Buffer size warning
    btPtofile.deviceId = ahBTRMgrDevHdl;
    btPtofile.isConnect = 1;

    lenBtrMgrPiRet = BTRMgr_PI_RemoveProfile(ghBTRMgrPiHdl, &btPtofile);
    if(lenBtrMgrPiRet == eBTRMgrSuccess) {
       BTRMGRLOG_INFO ("Persistent File updated successfully\n");
    }
    else {
       BTRMGRLOG_ERROR ("Persistent File update failed \n");
    }

    return lenBtrMgrPiRet;
}

STATIC BTRMGR_SysDiagChar_t
btrMgr_MapUUIDtoDiagElement(
    char *aUUID
) {
    BTRMGR_SysDiagChar_t lDiagChar = BTRMGR_SYS_DIAG_UNKNOWN;
#ifndef BTRTEST_LE_ONBRDG_ENABLE
    if (!strcmp(aUUID, BTRMGR_SYSTEM_ID_UUID)) { lDiagChar = BTRMGR_SYS_DIAG_SYSTEMID; }
    else if (!strcmp(aUUID, BTRMGR_MODEL_NUMBER_UUID)) { lDiagChar = BTRMGR_SYS_DIAG_MODELNUMBER; }
    else if (!strcmp(aUUID, BTRMGR_SERIAL_NUMBER_UUID)) { lDiagChar = BTRMGR_SYS_DIAG_SERIALNUMBER; }
    else if (!strcmp(aUUID, BTRMGR_FIRMWARE_REVISION_UUID)) { lDiagChar = BTRMGR_SYS_DIAG_FWREVISION; }
    else if (!strcmp(aUUID, BTRMGR_HARDWARE_REVISION_UUID)) { lDiagChar = BTRMGR_SYS_DIAG_HWREVISION; }
    else if (!strcmp(aUUID, BTRMGR_SOFTWARE_REVISION_UUID)) { lDiagChar = BTRMGR_SYS_DIAG_SWREVISION; }
    else if (!strcmp(aUUID, BTRMGR_MANUFACTURER_NAME_UUID)) { lDiagChar = BTRMGR_SYS_DIAG_MFGRNAME; }
#else
    if (!strcmp(aUUID, BTRMGR_SYSTEM_ID_UUID)) { lDiagChar = BTRMGR_LE_ONBRDG_SYSTEMID; }
    else if (!strcmp(aUUID, BTRMGR_MODEL_NUMBER_UUID)) { lDiagChar = BTRMGR_LE_ONBRDG_MODELNUMBER; }
    else if (!strcmp(aUUID, BTRMGR_SERIAL_NUMBER_UUID)) { lDiagChar = BTRMGR_LE_ONBRDG_SERIALNUMBER; }
    else if (!strcmp(aUUID, BTRMGR_FIRMWARE_REVISION_UUID)) { lDiagChar = BTRMGR_LE_ONBRDG_FWREVISION; }
    else if (!strcmp(aUUID, BTRMGR_HARDWARE_REVISION_UUID)) { lDiagChar = BTRMGR_LE_ONBRDG_HWREVISION; }
    else if (!strcmp(aUUID, BTRMGR_SOFTWARE_REVISION_UUID)) { lDiagChar = BTRMGR_LE_ONBRDG_SWREVISION; }
    else if (!strcmp(aUUID, BTRMGR_MANUFACTURER_NAME_UUID)) { lDiagChar = BTRMGR_LE_ONBRDG_MFGRNAME; }
#endif
    else if (!strcmp(aUUID, BTRMGR_LEONBRDG_UUID_QR_CODE)) { lDiagChar = BTRMGR_LE_ONBRDG_UUID_QR_CODE; }
    else if (!strcmp(aUUID, BTRMGR_LEONBRDG_UUID_PROVISION_STATUS)) { lDiagChar = BTRMGR_LE_ONBRDG_UUID_PROVISION_STATUS; }
    else if (!strcmp(aUUID, BTRMGR_LEONBRDG_UUID_PUBLIC_KEY)) { lDiagChar = BTRMGR_LE_ONBRDG_UUID_PUBLIC_KEY; }
    else if (!strcmp(aUUID, BTRMGR_LEONBRDG_UUID_WIFI_CONFIG)) { lDiagChar = BTRMGR_LE_ONBRDG_UUID_WIFI_CONFIG; }
    else if (!strcmp(aUUID, BTRMGR_DEVICE_STATUS_UUID)) { lDiagChar = BTRMGR_SYS_DIAG_DEVICESTATUS; }
    else if (!strcmp(aUUID, BTRMGR_FWDOWNLOAD_STATUS_UUID)) { lDiagChar = BTRMGR_SYS_DIAG_FWDOWNLOADSTATUS; }
    else if (!strcmp(aUUID, BTRMGR_WEBPA_STATUS_UUID)) { lDiagChar = BTRMGR_SYS_DIAG_WEBPASTATUS; }
    else if (!strcmp(aUUID, BTRMGR_WIFIRADIO1_STATUS_UUID)) { lDiagChar = BTRMGR_SYS_DIAG_WIFIRADIO1STATUS; }
    else if (!strcmp(aUUID, BTRMGR_WIFIRADIO2_STATUS_UUID)) { lDiagChar = BTRMGR_SYS_DIAG_WIFIRADIO2STATUS; }
    else if (!strcmp(aUUID, BTRMGR_RF_STATUS_UUID)) { lDiagChar = BTRMGR_SYS_DIAG_RFSTATUS; }
    else if (!strcmp(aUUID, BTRMGR_DEVICE_MAC)) { lDiagChar = BTRMGR_SYS_DIAG_DEVICEMAC; }
    else if (!strcmp(aUUID, BTRMGR_COLUMBO_START)) { lDiagChar = BTRMGR_SYSDIAG_COLUMBO_START; }
    else if (!strcmp(aUUID, BTRMGR_COLUMBO_STOP)) { lDiagChar = BTRMGR_SYSDIAG_COLUMBO_STOP; }
#ifdef LE_MODE   
    else if (!strcmp(aUUID, BTRMGR_UUID_QR_CODE)) { lDiagChar = BTRMGR_SYS_DIAG_QR_CODE; }
    else if (!strcmp(aUUID, BTRMGR_UUID_PROVISION_STATUS)) { lDiagChar = BTRMGR_SYS_DIAG_PROVISION_STATUS; }
    else if (!strcmp(aUUID, BTRMGR_UUID_SIM_ICCID)) { lDiagChar = BTRMGR_SYS_DIAG_SIM_ICCID; }
    else if (!strcmp(aUUID, BTRMGR_UUID_MODEM_IMEI)) { lDiagChar = BTRMGR_SYS_DIAG_MODEM_IMEI; }
    else if (!strcmp(aUUID, BTRMGR_UUID_CELLULAR_SIGNAL_STRENGTH)) { lDiagChar = BTRMGR_SYS_DIAG_CELLULAR_SIGNAL_STRENGTH; }
    else if (!strcmp(aUUID, BTRMGR_UUID_MESH_BACKHAUL_TYPE)) { lDiagChar = BTRMGR_SYS_DIAG_MESH_BACKHAUL_TYPE; }
    else if (!strcmp(aUUID, BTRMGR_UUID_WIFI_BACKHAUL_STATS)) { lDiagChar = BTRMGR_SYS_DIAG_WIFI_BACKHAUL_STATS; }
#endif    
    else { lDiagChar = BTRMGR_SYS_DIAG_UNKNOWN; }

    return lDiagChar;
}

STATIC gboolean btrMgr_GetDeviceDisconnectStatusCb(gpointer user_data)
{
    enBTRCoreDeviceType     lenBTRCoreDevTy = enBTRCoreSpeakers;
    enBTRCoreDeviceClass    lenBTRCoreDevCl = enBTRCore_DC_Unknown;
    enBTRCoreRet                lenBtrCoreRet   = enBTRCoreSuccess;
    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ghBTRMgrDevHdlDisConStatusCheck, &lenBTRCoreDevTy, &lenBTRCoreDevCl);
    BTRMGRLOG_DEBUG ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBTRCoreDevTy, lenBTRCoreDevCl);

    if (CheckStatusRetryCount < 3) {
        if (enBTRCoreSuccess != BTRCore_GetDeviceDisconnected(ghBTRCoreHdl, ghBTRMgrDevHdlDisConStatusCheck, lenBTRCoreDevTy)) {
            BTRMGRLOG_ERROR ("Failed to Disconnect from previous AudioIn connection(%llu)!\n", ghBTRMgrDevHdlDisConStatusCheck);
	    CheckStatusRetryCount++;
        } else {
	    BTRMGRLOG_INFO ("Disconnected successfully\n");
	    btrMgr_ClearDisconnDevHoldOffTimer();
	    return G_SOURCE_REMOVE;
        }
    } else {
        btrMgr_ClearDisconnDevHoldOffTimer();
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}
#ifdef AUTO_CONNECT_ENABLED
static gboolean btrMgr_AutoconnectOnStartUpStatusCb(gpointer user_data)
{
    gIsAudOutStartupInProgress = BTRMGR_STARTUP_AUD_RETRY;
    BTRMGR_StartAudioStreamingOut_StartUp(0, BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT);
    btrMgr_ClearAutoconnectOnStartUpTimer();
       return G_SOURCE_REMOVE;
}
#endif //AUTO_CONNECT_ENABLED

static gboolean btrMgr_ClearLastPairedStatus(gpointer user_data)
{
    ghBTRMgrDevHdlLastPaired = 0;
    return G_SOURCE_REMOVE;
}

#ifdef LE_MODE
STATIC gboolean btrMgr_BatteryTriggerStartNotify(gpointer user_data)
{
    int rc;
    rc = BTRMgr_TriggerBatteryStartNotify(gBatteryDevHandle,gstBTRMgrBatteryInfo.hBTRMgrBatteryHdl,ghBTRCoreHdl);

    if (eBTRMgrSuccess == rc) {
        BTRMGRLOG_INFO ("Performed Start Notify Operation\n");
    } else {
        BTRMGRLOG_INFO ("Failed to perform start notify operation\n");
    }

    btrMgr_ClearBatteryStartNotifyHoldOffTimer();
    return G_SOURCE_REMOVE;
}

static gboolean btrMgr_CheckBatteryConnectionStatus(gpointer user_data)
{
    int rc;
    BTRMGRLOG_INFO("In btrMgr_CheckBatteryConnectionstatus \n");
    if (gXbbConnected != TRUE && gConnectionCheckAttempt < BTRMGR_DEVCONN_CHECK_RETRY_ATTEMPTS) {
       rc = btrMgr_ConnectToDevice(gDefaultAdapterContext.adapter_number, gBatteryDevHandle, BTRMGR_DEVICE_OP_TYPE_LE, 0, 1);
       if (BTRMGR_RESULT_SUCCESS == rc) {
           gXbbConnected = TRUE;
           /* Started the timer here with 20sec interval to trigger start notify on the characteristics of XBB ,once after
            * all the characteristics was added successfully on lower layer.
            */
           btrMgr_StartBatteryFirmwareUpgradeHoldOffTimer();
           BTRMGRLOG_INFO ("Connected with Battery successfully after connection lost\n");
           return G_SOURCE_REMOVE;
       } else {
           BTRMGRLOG_INFO ("Connected with Battery failed after connection lost\n");
       }
    } else if (gConnectionCheckAttempt >= BTRMGR_DEVCONN_CHECK_RETRY_ATTEMPTS) {
       BTRMGRLOG_INFO ("Maximum wait time exceeded to connect the XBB after connection lost, Try manually\n");
       btrMgr_ClearBatteryConnectionStatusHoldOffTimer();
       return G_SOURCE_REMOVE;
    } else if (gXbbConnected == TRUE) {
       btrMgr_StartBatteryFirmwareUpgradeHoldOffTimer();
       btrMgr_ClearBatteryConnectionStatusHoldOffTimer();
       return G_SOURCE_REMOVE;
    }
    gConnectionCheckAttempt++;
    return G_SOURCE_CONTINUE;
}

static gboolean btrMgr_CheckFirmwareVersion(gpointer user_data)
{
   int rc;
   char val[BTRMGR_MAX_STR_LEN] = "\0";

   /* Removed the sources added for firmware upgrade after Battery connect */

   btrMgr_ClearBatteryFirmwareUpgradeHoldOffTimer();
   rc = BTRMGR_DisconnectFromDevice (gDefaultAdapterContext.adapter_number,gBatteryDevHandle);
 
   if (BTRMGR_RESULT_SUCCESS ==  rc) {
       BTRMGRLOG_INFO("Disconnected with battery successfully\n");
   } else {
       BTRMGRLOG_INFO("Failed to disconnect with the device \n");
   }

   sleep(5);

   rc = BTRMGR_ConnectToDevice(gDefaultAdapterContext.adapter_number,gBatteryDevHandle,BTRMGR_DEVICE_OP_TYPE_LE);

   if (BTRMGR_RESULT_SUCCESS ==  rc) {
       BTRMGRLOG_INFO("Connect to battery successfully\n");
   } else {
       BTRMGRLOG_INFO("Failed to connect with battery \n");
   }

   sleep(2);

   if (enBTRCoreSuccess != BTRCore_PerformLEOp(ghBTRCoreHdl,gBatteryDevHandle,BTRMGR_UUID_BATTERY_FIRMWARE_REVISION,1,NULL,val)) {
       btrMgr_ClearCheckFirmwareVersionHoldOffTimer();
       return G_SOURCE_REMOVE;
   }

   if (val !=NULL) {
        char a1,a2,a3,a4,CurrentVersion[BTRMGR_STR_LEN];
        sscanf( val, "%02hhX%02hhX%02hhX%02hhX", &a1, &a2, &a3, &a4);
        sprintf( CurrentVersion ,"%c%c%c%c",a1,a2,a3,a4);
        BTRMGRLOG_INFO("Loaded Firmware Version in battery - %s \n",CurrentVersion);
    }
    btrMgr_SetBatteryStartNotifyHoldOffTimer();
    btrMgr_ClearCheckFirmwareVersionHoldOffTimer();
    return G_SOURCE_REMOVE;
}

static gboolean btrMgr_StartBatteryFirmwareUpgrade(gpointer user_data)
{
    int i,nof = 0,j,count,FileLen,k,OTAControlValue = 0;
    char CurrentVersion[BTRMGR_MAX_STR_LEN],FoundVersion[BTRMGR_MAX_STR_LEN],arr[MAX_FIRMWARE_FILES][BTRMGR_MAX_STR_LEN],val[BTRMGR_MAX_STR_LEN],FileName[BTRMGR_MAX_STR_LEN],uuid[BTRMGR_MAX_STR_LEN],writeArg[BTRMGR_MAX_STR_LEN] = "\0";
    struct dirent *de;
    double FoundVer,CurrentVer;
    gboolean FileFound = FALSE;

    memset(CurrentVersion,'\0',sizeof(CurrentVersion));
    memset(FoundVersion,'\0',sizeof(FoundVersion));
    memset(val,'\0',sizeof(val));
    memset(FileName,'\0',sizeof(FileName));
    memset(uuid,'\0',sizeof(uuid));

    BTRMGRLOG_INFO("In BTRMGR_CheckFirmwareUpgrade \n");
    /* Added the connect for testing purpose, will be removed */
    btrMgr_ConnectToDevice(gDefaultAdapterContext.adapter_number, gBatteryDevHandle, BTRMGR_DEVICE_OP_TYPE_LE, 0, 1);
    sleep(2);

    strncpy(uuid,BTRMGR_UUID_BATTERY_FIRMWARE_REVISION,BTRMGR_MAX_STR_LEN - 1);

    if (enBTRCoreSuccess != BTRCore_PerformLEOp(ghBTRCoreHdl,gBatteryDevHandle,uuid,BTRMGR_LE_OP_READ_VALUE,writeArg,val)) {
        btrMgr_ClearBatteryFirmwareUpgradeHoldOffTimer();
        return G_SOURCE_REMOVE;
    }

    if (val !=NULL) {
        char a1,a2,a3,a4;
        sscanf( val, "%02hhX%02hhX%02hhX%02hhX", &a1, &a2, &a3, &a4);
        sprintf( CurrentVersion ,"%c%c%c%c",a1,a2,a3,a4);
        BTRMGRLOG_INFO("Current Firmware Version in battery - %s \n",CurrentVersion);
        sscanf(CurrentVersion,"%lf",&CurrentVer);
    }
   
    BTRMGRLOG_INFO("Searching for .gbl files in /etc....\n");

    DIR *dr = opendir("/etc");

    if (dr == NULL) {
        BTRMGRLOG_INFO("Could not open current directory\n");
        btrMgr_ClearBatteryFirmwareUpgradeHoldOffTimer();
    }

    while ((de = readdir(dr)) != NULL) {
        if (strstr(de->d_name,".gbl")) {
            strcpy(arr[nof],de->d_name);
            nof++;
        }
    }
    closedir(dr);

    for (i=0 ; i<nof ;i++) {
         count = 0;
         BTRMGRLOG_INFO("Files Found - %d - %s\n",i,arr[i]);
         FileLen = strlen(arr[i]);
         /* Parsing the version from file name */
         for (k=FileLen - 6; k > 0 ; k--) {
              if (arr[i][k] >= 48 && arr[i][k] <= 57) {
                  count++;
              }
         }
         BTRMGRLOG_INFO("version length  %d \n",count);
         for (j=FileLen - 6 ; count >=0 ;j--) {
              FoundVersion[count] = arr[i][j];
              count--;
         }
         sscanf(FoundVersion,"%lf",&FoundVer);
         BTRMGRLOG_INFO("Found Version %s\n",FoundVersion);

         if (FoundVer > CurrentVer) {
             BTRMGRLOG_INFO("Found the higher version %s \n",arr[i]);
             FileFound = TRUE;
             strcpy(FileName,arr[i]);
             break;
         } else {
             BTRMGRLOG_INFO("higher version not found \n");
         }
    }

    if (!FileFound) {
        BTRMGRLOG_INFO("No higher version of firmware image found, Triggering notification\n");
        btrMgr_SetBatteryStartNotifyHoldOffTimer();
        btrMgr_ClearBatteryFirmwareUpgradeHoldOffTimer();
        return G_SOURCE_REMOVE;
    }

    /* Setting battery LED to green */
    BTRMGRLOG_INFO("Setting Battery LED for Firmware Flash using file %s \n",FileName);
    BTRCore_BatterySetLED (ghBTRCoreHdl,gBatteryDevHandle,BTRMGR_UUID_BATTERY_COMMAND);
    sleep(2);

    /* Writing OTA control value */
    BTRMGRLOG_INFO("Writing OTA control\n");
    BTRCore_BatteryWriteOTAControl (ghBTRCoreHdl,gBatteryDevHandle,BTRMGR_UUID_BATTERY_OTA_CONTROL,OTAControlValue);
    sleep(2);

    /* Writing OTA control data */
    BTRMGRLOG_INFO("Writing OTA data\n");
    BTRCore_BatteryOTADataTransfer (ghBTRCoreHdl,gBatteryDevHandle,BTRMGR_UUID_BATTERY_OTA_DATA,FileName);

    /* Initiating Battery disconnect/connect after the OTA data was written successfully.
     * currently doing after 5 mins from OTA Transfer start
     * TODO : Implement a seperate callback on btrCore to intimate that OTA firmware download is completed
     */
    btrMgr_CheckFirmwareVersionHoldOffTimer();
    return G_SOURCE_REMOVE;
}

static gboolean btrMgr_ConnectBatteryDevices(gpointer user_data)
{
    int rc;
    BTRMGRLOG_INFO("In btrMgr_ConnectBatteryDevices \n");

    if (gBatteryConnectRetry <= BTRMGR_CONNECT_RETRY_ATTEMPTS) {
        rc = btrMgr_ConnectToDevice(gDefaultAdapterContext.adapter_number, gBatteryDevHandle, BTRMGR_DEVICE_OP_TYPE_LE, 0, 1);
        if (rc == eBTRMgrSuccess) {
            gXbbConnected = TRUE;
            BTRMGRLOG_INFO ("Connected with Battery successfully\n");
            btrMgr_SetBatteryConnectionStatusHoldOffTimer();
            btrMgr_ClearConnectBatteryHoldOffTimer();
            return G_SOURCE_REMOVE;
        } else {
            BTRMGRLOG_INFO ("Connection to Battery failed Retrying\n");
            gBatteryConnectRetry++;
        }
    } else {
        rc = BTRMGR_UnpairDevice(gDefaultAdapterContext.adapter_number, gBatteryDevHandle);
        if (BTRMGR_RESULT_SUCCESS == rc) {
            BTRMGRLOG_INFO ("Unpaired with Battery successfully\n");
        } else {
            BTRMGRLOG_INFO ("Unpairing failed\n");
        }
        rc = BTRMGR_StartDeviceDiscovery(gDefaultAdapterContext.adapter_number, BTRMGR_DEVICE_OP_TYPE_LE);

        if (BTRMGR_RESULT_SUCCESS == rc) {
            BTRMGRLOG_INFO (" Started Device Discovery for battery\n");
        } else {
            BTRMGRLOG_INFO ("Failed to start device discovery for battery\n");
        }
        btrMgr_SetDiscoverbatteryDevicesHoldOffTimer();
        btrMgr_ClearConnectBatteryHoldOffTimer();
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static gboolean btrMgr_GetDiscoveredBatteryDevices(gpointer user_data)
{
    BTRMGR_DiscoveredDevicesList_t discoveredDevices;
    int MinSignalLevel,rc,i,WaitIdx = 0;
    gboolean InitCheck = FALSE,BatteryDeviceFound = FALSE;

    BTRMGRLOG_INFO(">>>>>>> in handler %d <<<<< \n",gTimeoutSeconds);

    gTimeoutSeconds = gTimeoutSeconds + BTRMGR_BATTERY_DISCOVERY_TIME_INTERVAL;

    memset(&discoveredDevices, 0, sizeof(discoveredDevices));
    rc = BTRMGR_GetDiscoveredDevices(gDefaultAdapterContext.adapter_number, &discoveredDevices);

    for (i=0 ; i< discoveredDevices.m_numOfDevices; i++) {
        if (discoveredDevices.m_deviceProperty[i].m_deviceType == BTRMGR_DEVICE_TYPE_XBB) {
            BatteryDeviceFound = 1;
            if (InitCheck == FALSE) {
                MinSignalLevel = discoveredDevices.m_deviceProperty[i].m_signalLevel;
                gBatteryDevHandle = discoveredDevices.m_deviceProperty[i].m_deviceHandle;
                InitCheck = 1;
                continue;
             }

             if (MinSignalLevel < discoveredDevices.m_deviceProperty[i].m_signalLevel) {
                 MinSignalLevel = discoveredDevices.m_deviceProperty[i].m_signalLevel;
                 gBatteryDevHandle = discoveredDevices.m_deviceProperty[i].m_deviceHandle;
             }
         }
    }

    if (BatteryDeviceFound == 1) {
        /* when the battery device was found in the discovered devices list stopping the scan
         * on trying to pair.
         */
        rc = BTRMGR_StopDeviceDiscovery(gDefaultAdapterContext.adapter_number, BTRMGR_DEVICE_OP_TYPE_LE);
        if (BTRMGR_RESULT_SUCCESS != rc) {
            BTRMGRLOG_INFO (" Stopped Device Discovery for XBB\n");
        } else {
            BTRMGRLOG_INFO ("Failed to stop device discovery for XBB\n");
        }

        sleep(1);
        rc = BTRMGR_PairDevice(gDefaultAdapterContext.adapter_number,gBatteryDevHandle);
        if (BTRMGR_RESULT_SUCCESS == rc) {
            BTRMGRLOG_INFO ("Paired with XBB successfully\n");
        } else {
            BTRMGRLOG_INFO ("Pairing with XBB failed\n");
        }

        BTRMGR_GetPairedDevices (gDefaultAdapterContext.adapter_number, &gListOfPairedDevices);

        while(WaitIdx < BTRMGR_PAIRING_MAX_WAIT_TIME) {
            if (1 == btrMgr_GetDevPaired(gBatteryDevHandle)) {
                BTRMGRLOG_INFO ("Battery Device Paired/Connected successfully\n");
                break;
            } else {
                sleep(1);
                WaitIdx++;
            }
        }

        if (WaitIdx == BTRMGR_PAIRING_MAX_WAIT_TIME) {
            BTRMGRLOG_INFO ("Waited for 10 seconds pairing not success\n");
        }

        /* Considering the XBB is connected since the XBB will be connected
         * on successful pairing.
         * TODO : Make the code generic for all battery devices even if the devices
         * was not connected after successful pairing based the callback received from
         * lower layer.
         */
        if (btrMgr_GetDevPaired(gBatteryDevHandle) == 1) {
            gXbbConnected = TRUE;
        }
    }

    if (gXbbConnected != TRUE && gTimeoutSeconds < BTRMGR_BATTERY_DISCOVERY_TIMEOUT) {
        /* Restarted the scan on pairing/Connection failure */
        rc = BTRMGR_StartDeviceDiscovery(gDefaultAdapterContext.adapter_number, BTRMGR_DEVICE_OP_TYPE_LE);

        if (BTRMGR_RESULT_SUCCESS == rc) {
            BTRMGRLOG_INFO ("Restarted Device Discovery for battery\n");
        } else {
            BTRMGRLOG_INFO ("Failed to restart device discovery for battery\n");
        }
    }

    if (gXbbConnected == TRUE || gTimeoutSeconds >= BTRMGR_BATTERY_DISCOVERY_TIMEOUT) {
	btrMgr_StartBatteryFirmwareUpgradeHoldOffTimer();
        btrMgr_ClearDiscoverbatteryDevicesHoldOffTimer();
    }

    return G_SOURCE_CONTINUE;
}
#endif

#if 0
static void btrMgr_AddColumboGATTInfo(
){
    char l16BitUUID[4] = "";
    char l128BitUUID[BTRMGR_NAME_LEN_MAX] = "";

    strncpy(l128BitUUID, BTRMGR_COLUMBO_UUID, BTRMGR_NAME_LEN_MAX - 1);
    snprintf(l16BitUUID, 5, "%s", l128BitUUID);

    /* add to GAP advertisement */
    BTRCore_SetServiceUUIDs(ghBTRCoreHdl, l16BitUUID);
    /* Add to GATT Info */
    BTRCore_SetServiceInfo(ghBTRCoreHdl, BTRMGR_COLUMBO_UUID, TRUE);
    BTRCore_SetGattInfo(ghBTRCoreHdl, BTRMGR_COLUMBO_UUID, BTRMGR_COLUMBO_START, 0x2, " ", enBTRCoreLePropGChar);           /* uuid_columbo_service_char_start */
    BTRCore_SetGattInfo(ghBTRCoreHdl, BTRMGR_COLUMBO_UUID, BTRMGR_COLUMBO_STOP, 0x2, " ", enBTRCoreLePropGChar);           /* uuid_columbo_service_char_stop */
    BTRCore_SetGattInfo(ghBTRCoreHdl, BTRMGR_COLUMBO_UUID, BTRMGR_COLUMBO_STATUS, 0x1, " ", enBTRCoreLePropGChar);           /* uuid_columbo_service_char_status */
    BTRCore_SetGattInfo(ghBTRCoreHdl, BTRMGR_COLUMBO_UUID, BTRMGR_COLUMBO_REPORT, 0x1, " ", enBTRCoreLePropGChar);           /* uuid_columbo_service_char_report */
}

static void btrMgr_AddStandardAdvGattInfo(
){
    char lPropertyValue[BTRMGR_MAX_STR_LEN] = "";

    BTRMGRLOG_INFO("Adding default local services : DEVICE_INFORMATION_UUID - 0x180a, RDKDIAGNOSTICS_UUID - 0xFDB9\n");
    BTRMGR_LE_SetServiceInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, 1);
    BTRMGR_LE_SetServiceInfo(0, BTRMGR_RDKDIAGNOSTICS_UUID, 1);

    /* Get model number */
    BTRMGR_SysDiagInfo(0, BTRMGR_SYSTEM_ID_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
    BTRMGRLOG_INFO("Adding char for the default local services : 0x180a, 0xFDB9\n");
    BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_SYSTEM_ID_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);                                /* system ID            */
    BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_MODEL_NUMBER_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);                             /* model number         */
    /*Get HW revision*/
    BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_HARDWARE_REVISION_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);                        /* Hardware revision    */
    /* Get serial number */
    BTRMGR_SysDiagInfo(0, BTRMGR_SERIAL_NUMBER_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
    BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_SERIAL_NUMBER_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);                            /* serial number        */
    /* Get firmware revision */
    BTRMGR_SysDiagInfo(0, BTRMGR_FIRMWARE_REVISION_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
    BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_FIRMWARE_REVISION_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);                        /* Firmware revision    */
    BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_SOFTWARE_REVISION_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);                        /* Software revision    */
    /* Get manufacturer name */
    BTRMGR_SysDiagInfo(0, BTRMGR_MANUFACTURER_NAME_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
    BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_MANUFACTURER_NAME_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);                        /* Manufacturer name    */

    /* 0xFDB9 */
    BTRMGR_LE_SetGattInfo(0, BTRMGR_RDKDIAGNOSTICS_UUID, BTRMGR_DEVICE_STATUS_UUID, 0x1, "READY", BTRMGR_LE_PROP_CHAR);                                       /* DeviceStatus         */
    BTRMGR_SysDiagInfo(0, BTRMGR_FWDOWNLOAD_STATUS_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
    BTRMGR_LE_SetGattInfo(0, BTRMGR_RDKDIAGNOSTICS_UUID, BTRMGR_FWDOWNLOAD_STATUS_UUID, 0x103, lPropertyValue, BTRMGR_LE_PROP_CHAR);                          /* FWDownloadStatus     */
    BTRMGR_SysDiagInfo(0, BTRMGR_WEBPA_STATUS_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
    BTRMGR_LE_SetGattInfo(0, BTRMGR_RDKDIAGNOSTICS_UUID, BTRMGR_WEBPA_STATUS_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);                                 /* WebPAStatus          */
    BTRMGR_SysDiagInfo(0, BTRMGR_WIFIRADIO1_STATUS_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
    BTRMGR_LE_SetGattInfo(0, BTRMGR_RDKDIAGNOSTICS_UUID, BTRMGR_WIFIRADIO1_STATUS_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);                            /* WiFiRadio1Status     */
    BTRMGR_LE_SetGattInfo(0, BTRMGR_RDKDIAGNOSTICS_UUID, BTRMGR_WIFIRADIO2_STATUS_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);                            /* WiFiRadio2Status     */
    BTRMGR_SysDiagInfo(0, BTRMGR_RF_STATUS_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
    BTRMGR_LE_SetGattInfo(0, BTRMGR_RDKDIAGNOSTICS_UUID, BTRMGR_RF_STATUS_UUID, 0x1, "NOT CONNECTED", BTRMGR_LE_PROP_CHAR);                                   /* RFStatus             */
}
#endif
/*  Local Op Threads */


/* Interfaces - Public Functions */
BTRMGR_Result_t
BTRMGR_Init (
    void
) {
    BTRMGR_Result_t lenBtrMgrResult= BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet    lenBtrCoreRet  = enBTRCoreSuccess;
    eBTRMgrRet      lenBtrMgrPiRet = eBTRMgrFailure;
    eBTRMgrRet      lenBtrMgrSdRet = eBTRMgrFailure;
    GMainLoop*      pMainLoop      = NULL;
    GThread*        pMainLoopThread= NULL;

    char            lpcBtVersion[BTRCORE_STR_LEN] = {'\0'};
    isDeinitInProgress = FALSE;

    if (ghBTRCoreHdl) {
        BTRMGRLOG_WARN("Already Inited; Return Success\n");
        return lenBtrMgrResult;
    }

#ifdef RDK_LOGGER_ENABLED
    const char* pDebugConfig = NULL;
    const char* BTRMGR_DEBUG_ACTUAL_PATH    = "/etc/debug.ini";
    const char* BTRMGR_DEBUG_OVERRIDE_PATH  = "/opt/debug.ini";

    /* Init the logger */
    if (access(BTRMGR_DEBUG_OVERRIDE_PATH, F_OK) != -1) {
        pDebugConfig = BTRMGR_DEBUG_OVERRIDE_PATH;
    }
    else {
        pDebugConfig = BTRMGR_DEBUG_ACTUAL_PATH;
    }

    if (0 == rdk_logger_init(pDebugConfig)) {
        b_rdk_logger_enabled = 1;
    }
#endif

    /* Initialze all the database */
    MEMSET_S(&gDefaultAdapterContext, sizeof(gDefaultAdapterContext), 0, sizeof(gDefaultAdapterContext));
    MEMSET_S(&gListOfAdapters, sizeof(gListOfAdapters), 0, sizeof(gListOfAdapters));
#ifndef LE_MODE
    MEMSET_S(&gstBTRMgrStreamingInfo, sizeof(gstBTRMgrStreamingInfo), 0, sizeof(gstBTRMgrStreamingInfo));
#endif
    MEMSET_S(&gListOfPairedDevices, sizeof(gListOfPairedDevices), 0, sizeof(gListOfPairedDevices));
    MEMSET_S(&gstBtrCoreDevMediaInfo, sizeof(gstBtrCoreDevMediaInfo), 0, sizeof(gstBtrCoreDevMediaInfo));
    //gIsDiscoveryInProgress = 0;


    /* Init the mutex */

    /* Call the Core/HAL init */
    lenBtrCoreRet = BTRCore_Init(&ghBTRCoreHdl);
    if ((!ghBTRCoreHdl) || (lenBtrCoreRet != enBTRCoreSuccess)) {
        BTRMGRLOG_ERROR ("Could not initialize BTRCore/HAL module\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }


    if (enBTRCoreSuccess != BTRCore_GetVersionInfo(ghBTRCoreHdl, lpcBtVersion)) {
        BTRMGRLOG_ERROR ("BTR Bluetooth Version: FAILED\n");
    }
    BTRMGRLOG_INFO("BTR Bluetooth Version: %s\n", lpcBtVersion);

    if (enBTRCoreSuccess != BTRCore_GetListOfAdapters (ghBTRCoreHdl, &gListOfAdapters)) {
        BTRMGRLOG_ERROR ("Failed to get the total number of Adapters present\n"); /* Not a Error case anyway */
    }
    BTRMGRLOG_INFO ("Number of Adapters found are = %u\n", gListOfAdapters.number_of_adapters);

    if (0 == gListOfAdapters.number_of_adapters) {
        BTRMGRLOG_WARN("Bluetooth adapter NOT Found..!!!!\n");
        return  BTRMGR_RESULT_GENERIC_FAILURE;
    }


    /* you have atlesat one Bluetooth adapter. Now get the Default Adapter path for future usages; */
    gDefaultAdapterContext.bFirstAvailable = 1; /* This is unused by core now but lets fill it */
    if (enBTRCoreSuccess != BTRCore_GetAdapter(ghBTRCoreHdl, &gDefaultAdapterContext)) {
        BTRMGRLOG_WARN("Bluetooth adapter NOT received..!!!!\n");
        return  BTRMGR_RESULT_GENERIC_FAILURE;
    }

    BTRMGRLOG_DEBUG ("Aquired default Adapter; Path is %s\n", gDefaultAdapterContext.pcAdapterPath);
    /* TODO: Handling multiple Adapters */
    if (gListOfAdapters.number_of_adapters > 1) {
        BTRMGRLOG_WARN("Number of Bluetooth Adapters Found : %u !! Lets handle it properly\n", gListOfAdapters.number_of_adapters);
    }
#ifndef LE_MODE
    btrMgr_CheckDebugModeAvailability();
#endif //LE_MODE
    /* Register for callback to get the status of connected Devices */
    BTRCore_RegisterStatusCb(ghBTRCoreHdl, btrMgr_DeviceStatusCb, NULL);

    /* Register for callback to get the Discovered Devices */
    BTRCore_RegisterDiscoveryCb(ghBTRCoreHdl, btrMgr_DeviceDiscoveryCb, NULL);

    /* Register for callback to process incoming pairing requests */
    BTRCore_RegisterConnectionIntimationCb(ghBTRCoreHdl, btrMgr_ConnectionInIntimationCb, NULL);

    /* Register for callback to process incoming connection requests */
    BTRCore_RegisterConnectionAuthenticationCb(ghBTRCoreHdl, btrMgr_ConnectionInAuthenticationCb, NULL);

#ifndef LE_MODE
    /* Register for callback to process incoming media events */
    BTRCore_RegisterMediaStatusCb(ghBTRCoreHdl, btrMgr_MediaStatusCb, NULL);
#endif

    /* Activate Agent on Init */
    if (!btrMgr_GetAgentActivated()) {
        BTRMGRLOG_INFO ("Activate agent\n");
        if ((lenBtrCoreRet = BTRCore_RegisterAgent(ghBTRCoreHdl, 1)) != enBTRCoreSuccess) {
            BTRMGRLOG_ERROR ("Failed to Activate Agent\n");
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }
        else {
            btrMgr_SetAgentActivated(1);
        }
    }

    btrMgr_SetBgDiscoveryType (BTRMGR_DEVICE_OP_TYPE_LE);

    /* Initialize the Paired Device List for Default adapter */
    BTRMGR_GetPairedDevices (gDefaultAdapterContext.adapter_number, &gListOfPairedDevices);


    // Init Persistent handles
    if ((lenBtrMgrPiRet = BTRMgr_PI_Init(&ghBTRMgrPiHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("Could not initialize PI module\n");
    }

    // Init SysDiag handles
    if ((lenBtrMgrSdRet = BTRMgr_SD_Init(&ghBTRMgrSdHdl, btrMgr_SDStatusCb, NULL)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("Could not initialize SD - SysDiag module\n");
    }

#ifdef LE_MODE
    btrMgr_CheckBroadcastAvailability();
#else
    btrMgr_CheckAudioInServiceAvailability();
    btrMgr_CheckHidGamePadServiceAvailability();
    //this should be the gamepad state anyway, but make sure in case of an unexpected shutdown in standby/lightsleep mode
    BTRCore_refreshLEActionListForGamepads(ghBTRCoreHdl);
#endif

#if 0
    // Set the beacon and start the advertisement

    char lDeviceMAC[BTRMGR_MAX_STR_LEN] = "";

    BTRMGR_SysDiagInfo(0, BTRMGR_DEVICE_MAC, lDeviceMAC, BTRMGR_LE_OP_READ_VALUE);
    strncpy((char*)stCoreCustomAdv.device_mac, lDeviceMAC, strlen(lDeviceMAC));
    /* Add Columbo Gatt info */
    btrMgr_AddColumboGATTInfo();

    BTRMGR_LE_StartAdvertisement(0, &stCoreCustomAdv);
#endif

    if ((gmainContext = g_main_context_new()) == NULL) {
        BTRMGRLOG_ERROR ("Could not initialize g_main context\n");
        BTRMGR_DeInit();
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    pMainLoop    = g_main_loop_new (gmainContext, FALSE);
    gpvMainLoop = (void*)pMainLoop;


    pMainLoopThread   = g_thread_new("btrMgr_g_main_loop_Task", btrMgr_g_main_loop_Task, gpvMainLoop);
    gpvMainLoopThread = (void*)pMainLoopThread;
    if ((pMainLoop == NULL) || (pMainLoopThread == NULL)) {
        BTRMGRLOG_ERROR ("Could not initialize g_main module\n");
        BTRMGR_DeInit();
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }


    return lenBtrMgrResult;
}

static BOOLEAN btrMgr_IsDeviceRdkRcu(
    BTRMGR_DeviceServiceList_t *DeviceServiceInfo,
    unsigned short ui16Appearance
) {
    unsigned short Idx1;
    if (ui16Appearance == BTRMGR_REMOTE_CONTROL_APPEARANCE) {
        BTRMGRLOG_ERROR("Device appearance is remote control , skipping disconnect ...\n");
        return TRUE;
    }
    if (DeviceServiceInfo == NULL) {
        BTRMGRLOG_ERROR("No Service UUID's Present\n");
        return FALSE;
    }
    for (Idx1 = 0; Idx1 < DeviceServiceInfo->m_numOfService; Idx1++) {
        BTRMGRLOG_INFO("Profile - %s Appearance - %hu\n",DeviceServiceInfo->m_profileInfo[Idx1].m_profile,ui16Appearance);
        if (!strncmp(DeviceServiceInfo->m_profileInfo[Idx1].m_profile, BTRMGR_REMOTE_DEVICE, BTRMGR_NAME_LEN_MAX - 1)) {
            BTRMGRLOG_ERROR("Remote device UUID detected , skipping diconnect ...\n");
            return TRUE;
        }
    }
    return FALSE;
}

BTRMGR_Result_t
BTRMGR_DeInit (
    void
) {
    eBTRMgrRet                      lenBtrMgrRet      = eBTRMgrSuccess;
    enBTRCoreRet                    lenBtrCoreRet     = enBTRCoreSuccess;
    BTRMGR_Result_t                 lenBtrMgrResult   = BTRMGR_RESULT_SUCCESS;
    BTRMGR_DiscoveryHandle_t*       ldiscoveryHdl     = NULL;
    unsigned short                  ui16LoopIdx       = 0;
    BTRMGR_ConnectedDevicesList_t   lstConnectedDevices;
    unsigned int                    ui32sleepTimeOut = 1;
    gboolean isRemoteDev = FALSE;

    isDeinitInProgress = TRUE;

    if (btrMgr_isTimeOutSet()) {
        btrMgr_ClearDiscoveryHoldOffTimer();
        gDiscHoldOffTimeOutCbData = 0;
    }
#ifndef LE_MODE
    btrMgr_StopPacketCapture();
    btrMgr_StopHidEventMonitor();
#endif //LE_MODE
    if ((ldiscoveryHdl = btrMgr_GetDiscoveryInProgress())) {
        lenBtrMgrRet = btrMgr_StopDeviceDiscovery (0, ldiscoveryHdl);
        BTRMGRLOG_DEBUG ("Exit Discovery Status = %d\n", lenBtrMgrRet);
    }

    if ((lenBtrMgrResult = BTRMGR_GetConnectedDevices(0, &lstConnectedDevices)) == BTRMGR_RESULT_SUCCESS) {
        BTRMGRLOG_DEBUG ("Connected Devices = %d\n", lstConnectedDevices.m_numOfDevices);

        for (ui16LoopIdx = 0; ui16LoopIdx < lstConnectedDevices.m_numOfDevices; ui16LoopIdx++) {
            unsigned int            ui32confirmIdx  = 2;
            enBTRCoreDeviceType     lenBtrCoreDevTy = enBTRCoreUnknown;
            enBTRCoreDeviceClass    lenBtrCoreDevCl = enBTRCore_DC_Unknown;

            BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, lstConnectedDevices.m_deviceProperty[ui16LoopIdx].m_deviceHandle, &lenBtrCoreDevTy, &lenBtrCoreDevCl);
            isRemoteDev = btrMgr_IsDeviceRdkRcu(&lstConnectedDevices.m_deviceProperty[ui16LoopIdx].m_serviceInfo,lstConnectedDevices.m_deviceProperty[ui16LoopIdx].m_ui16DevAppearanceBleSpec);
            if (BTRCore_DisconnectDevice(ghBTRCoreHdl, lstConnectedDevices.m_deviceProperty[ui16LoopIdx].m_deviceHandle, lenBtrCoreDevTy) != enBTRCoreSuccess) {
                BTRMGRLOG_ERROR ("Failed to Disconnect - %llu\n", lstConnectedDevices.m_deviceProperty[ui16LoopIdx].m_deviceHandle);
            }

            /* Removed the wait time for disconnection confirmation from BlueZ for remote devices. */
            if (!isRemoteDev) {
                do {
                    unsigned int ui32sleepIdx = 2;

                    do {
                        sleep(ui32sleepTimeOut);
                        lenBtrCoreRet = BTRCore_GetDeviceDisconnected(ghBTRCoreHdl, lstConnectedDevices.m_deviceProperty[ui16LoopIdx].m_deviceHandle, lenBtrCoreDevTy);
                    } while ((lenBtrCoreRet != enBTRCoreSuccess) && (--ui32sleepIdx));
                } while (--ui32confirmIdx);
            }
        }
    }

    if (gConnPwrStChangeTimeOutRef) {
        BTRMGRLOG_DEBUG ("Cancelling previous Power state Cb : %u\n", gConnPwrStChangeTimeOutRef);
        g_source_destroy(g_main_context_find_source_by_id(gmainContext, gConnPwrStChangeTimeOutRef));
        gConnPwrStChangeTimeOutRef = 0;
    }

    if (gConnPairCompRstTimeOutRef) {
        BTRMGRLOG_DEBUG ("Cancelling previous Pair Reset handle TimeOut Session : %u\n", gConnPairCompRstTimeOutRef);
        g_source_destroy(g_main_context_find_source_by_id(gmainContext, gConnPairCompRstTimeOutRef));
        gConnPairCompRstTimeOutRef = 0;
    }

    btrMgr_ClearDisconnDevHoldOffTimer();

#ifdef LE_MODE
    if (gProvisionNotifyTimerHdl != 0)  {
        BTRMGRLOG_TRACE("Stop Notify for provision\n");
        g_source_destroy(g_main_context_find_source_by_id(gmainContext, gProvisionNotifyTimerHdl));
        gProvisionNotifyTimerHdl = 0;
    }

    if (gdeviceActstChangeTimeOutRef) {
        BTRMGRLOG_DEBUG ("Cancelling previous LEDeviceActivation : %u\n", gdeviceActstChangeTimeOutRef);
        g_source_destroy(g_main_context_find_source_by_id(gmainContext, gdeviceActstChangeTimeOutRef));
        gdeviceActstChangeTimeOutRef = 0;
    }

    btrMgr_ClearConnectBatteryHoldOffTimer();

    btrMgr_ClearDiscoverbatteryDevicesHoldOffTimer();
#endif

    if (gpvMainLoop) {
        g_main_loop_quit(gpvMainLoop);
    }

    if (gpvMainLoopThread) {
        g_thread_join(gpvMainLoopThread);
        gpvMainLoopThread = NULL;
    }

    if (gpvMainLoop) {
        g_main_loop_unref(gpvMainLoop);
        gpvMainLoop = NULL;
    }

    if (gmainContext) {
        g_main_context_unref(gmainContext);
        gmainContext = NULL;
    }

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (ghBTRMgrSdHdl) {
        lenBtrMgrRet = BTRMgr_SD_DeInit(ghBTRMgrSdHdl);
        ghBTRMgrSdHdl = NULL;
        BTRMGRLOG_ERROR ("SD Module DeInited = %d\n", lenBtrMgrRet);
    }

    if (ghBTRMgrPiHdl) {
        lenBtrMgrRet = BTRMgr_PI_DeInit(ghBTRMgrPiHdl);
        ghBTRMgrPiHdl = NULL;
        BTRMGRLOG_ERROR ("PI Module DeInited = %d\n", lenBtrMgrRet);
    }


    if (ghBTRCoreHdl) {
        lenBtrCoreRet = BTRCore_DeInit(ghBTRCoreHdl);
        ghBTRCoreHdl = NULL;
        BTRMGRLOG_ERROR ("BTRCore DeInited; Now will we exit the app = %d\n", lenBtrCoreRet);
    }

    lenBtrMgrResult =  ((lenBtrMgrRet == eBTRMgrSuccess) &&
                        (lenBtrCoreRet == enBTRCoreSuccess)) ? BTRMGR_RESULT_SUCCESS : BTRMGR_RESULT_GENERIC_FAILURE;
    BTRMGRLOG_DEBUG ("Exit Status = %d\n", lenBtrMgrResult);


    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_GetNumberOfAdapters (
    unsigned char*  pNumOfAdapters
) {
    BTRMGR_Result_t         lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet            lenBtrCoreRet   = enBTRCoreSuccess;
    stBTRCoreListAdapters   listOfAdapters;


    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }
    else if (!pNumOfAdapters) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    MEMSET_S(&listOfAdapters, sizeof(listOfAdapters), 0, sizeof(listOfAdapters));

    lenBtrCoreRet = BTRCore_GetListOfAdapters(ghBTRCoreHdl, &listOfAdapters);
    if (lenBtrCoreRet == enBTRCoreSuccess) {
        *pNumOfAdapters = listOfAdapters.number_of_adapters;
        /* Copy to our backup */
        if (listOfAdapters.number_of_adapters != gListOfAdapters.number_of_adapters) {
            MEMCPY_S(&gListOfAdapters,sizeof(gListOfAdapters), &listOfAdapters, sizeof (stBTRCoreListAdapters));
        }

        BTRMGRLOG_DEBUG ("Available Adapters = %d\n", listOfAdapters.number_of_adapters);
    }
    else {
        BTRMGRLOG_ERROR ("Could not find Adapters\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }


    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_ResetAdapter (
    unsigned char aui8AdapterIdx
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet    lenBtrCoreRet   = enBTRCoreSuccess;
    const char*     pAdapterPath    = NULL;
    char            name[BTRMGR_NAME_LEN_MAX] = {'\0'};

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }
    else if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    if (!(pAdapterPath = btrMgr_GetAdapterPath(aui8AdapterIdx))) {
        BTRMGRLOG_ERROR ("Failed to get adapter path\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }


    lenBtrCoreRet = BTRCore_DisableAdapter(ghBTRCoreHdl,&gDefaultAdapterContext);

    if (lenBtrCoreRet != enBTRCoreSuccess) {
        BTRMGRLOG_ERROR ("Failed to disable Adapter - %s\n", pAdapterPath);
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        BTRMGRLOG_INFO ("Bluetooth Adapter Disabled Successfully - %s\n", pAdapterPath);
        sleep(1);

        lenBtrCoreRet = BTRCore_SetAdapterName(ghBTRCoreHdl, pAdapterPath, name);
        if (lenBtrCoreRet != enBTRCoreSuccess) {
            BTRMGRLOG_ERROR ("Failed to set Adapter Name\n");
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }

        
        lenBtrCoreRet = BTRCore_EnableAdapter(ghBTRCoreHdl,&gDefaultAdapterContext);
        if (lenBtrCoreRet != enBTRCoreSuccess) {
            BTRMGRLOG_ERROR ("Failed to enable Adapter\n");
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
         }
        else {
            BTRMGRLOG_INFO ("Bluetooth Adapter Enabled Successfully - %s\n", pAdapterPath);
        }
    }
    
    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_SetAdapterName (
    unsigned char   aui8AdapterIdx,
    const char*     pNameOfAdapter
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet    lenBtrCoreRet   = enBTRCoreSuccess;
    const char*     pAdapterPath    = NULL;
    char            name[BTRMGR_NAME_LEN_MAX] = {'\0'};

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }
    else if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!pNameOfAdapter)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    if (!(pAdapterPath = btrMgr_GetAdapterPath(aui8AdapterIdx))) {
        BTRMGRLOG_ERROR ("Failed to get adapter path\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }


    strncpy (name, pNameOfAdapter, (BTRMGR_NAME_LEN_MAX - 1));
    lenBtrCoreRet = BTRCore_SetAdapterName(ghBTRCoreHdl, pAdapterPath, name);
    if (lenBtrCoreRet != enBTRCoreSuccess) {
        BTRMGRLOG_ERROR ("Failed to set Adapter Name\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        BTRMGRLOG_INFO ("Set Successfully\n");
    }


    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_GetAdapterName (
    unsigned char   aui8AdapterIdx,
    char*           pNameOfAdapter
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet    lenBtrCoreRet   = enBTRCoreSuccess;
    const char*     pAdapterPath    = NULL;
    char            name[BTRMGR_NAME_LEN_MAX] = {'\0'};

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }
    else if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!pNameOfAdapter)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    if (!(pAdapterPath = btrMgr_GetAdapterPath(aui8AdapterIdx))) {
        BTRMGRLOG_ERROR ("Failed to get adapter path\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }


    lenBtrCoreRet = BTRCore_GetAdapterName(ghBTRCoreHdl, pAdapterPath, name);
    if (lenBtrCoreRet != enBTRCoreSuccess) {
        BTRMGRLOG_ERROR ("Failed to get Adapter Name\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        BTRMGRLOG_INFO ("Fetched Successfully\n");
    }

    /*  Copy regardless of success or failure. */
    strncpy (pNameOfAdapter, name, (strlen(pNameOfAdapter) > BTRMGR_NAME_LEN_MAX -1) ? BTRMGR_NAME_LEN_MAX -1 : strlen(name));
    pNameOfAdapter[(strlen(pNameOfAdapter) > BTRMGR_NAME_LEN_MAX -1) ? BTRMGR_NAME_LEN_MAX -1 : strlen(name)] = '\0';


    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_SetAdapterPowerStatus (
    unsigned char   aui8AdapterIdx,
    unsigned char   power_status
) {
    BTRMGR_Result_t         lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet            lenBtrCoreRet   = enBTRCoreSuccess;
    enBTRCoreDeviceType     lenBTRCoreDevTy = enBTRCoreSpeakers;
    enBTRCoreDeviceClass    lenBTRCoreDevCl = enBTRCore_DC_Unknown;
    const char*             pAdapterPath    = NULL;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }
    else if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (power_status > 1)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    /* Check whether the requested device is connected n playing. */
    if ((ghBTRMgrDevHdlCurStreaming) && (power_status == 0)) {
        /* This will internall stops the playback as well as disconnects. */
        lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ghBTRMgrDevHdlCurStreaming, &lenBTRCoreDevTy, &lenBTRCoreDevCl);
        BTRMGRLOG_INFO ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBTRCoreDevTy, lenBTRCoreDevCl);

        if ((lenBTRCoreDevTy == enBTRCoreSpeakers) || (lenBTRCoreDevTy == enBTRCoreHeadSet)) {
            /* Streaming-Out is happening; stop it */
            if ((lenBtrMgrResult = BTRMGR_StopAudioStreamingOut(aui8AdapterIdx, ghBTRMgrDevHdlCurStreaming)) != BTRMGR_RESULT_SUCCESS) {
                BTRMGRLOG_ERROR ("This device is being Connected n Playing. Failed to stop Playback. Going Ahead to power off Adapter.-Out\n");
            }
        }
        else if ((lenBTRCoreDevTy == enBTRCoreMobileAudioIn) || (lenBTRCoreDevTy == enBTRCorePCAudioIn)) {
            /* Streaming-In is happening; stop it */
            if ((lenBtrMgrResult = BTRMGR_StopAudioStreamingIn(aui8AdapterIdx, ghBTRMgrDevHdlCurStreaming)) != BTRMGR_RESULT_SUCCESS) {
                BTRMGRLOG_ERROR ("This device is being Connected n Playing. Failed to stop Playback. Going Ahead to  power off Adapter.-In\n");
            }
        }
    }


    if (!(pAdapterPath = btrMgr_GetAdapterPath(aui8AdapterIdx))) {
        BTRMGRLOG_ERROR ("Failed to get adapter path\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }



    lenBtrCoreRet = BTRCore_SetAdapterPower(ghBTRCoreHdl, pAdapterPath, power_status);
    if (lenBtrCoreRet != enBTRCoreSuccess) {
        BTRMGRLOG_ERROR ("Failed to set Adapter Power Status\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        BTRMGRLOG_INFO ("Set Successfully\n");
    }


    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_GetAdapterPowerStatus (
    unsigned char   aui8AdapterIdx,
    unsigned char*  pPowerStatus
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet    lenBtrCoreRet   = enBTRCoreSuccess;
    const char*     pAdapterPath    = NULL;
    unsigned char   power_status    = 0;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }
    else if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!pPowerStatus)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    if (!(pAdapterPath = btrMgr_GetAdapterPath(aui8AdapterIdx))) {
        BTRMGRLOG_ERROR ("Failed to get adapter path\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }


    lenBtrCoreRet = BTRCore_GetAdapterPower(ghBTRCoreHdl, pAdapterPath, &power_status);
    if (lenBtrCoreRet != enBTRCoreSuccess) {
        BTRMGRLOG_ERROR ("Failed to get Adapter Power\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        BTRMGRLOG_INFO ("Fetched Successfully\n");
        *pPowerStatus = power_status;
    }


    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_SetAdapterDiscoverable (
    unsigned char   aui8AdapterIdx,
    unsigned char   discoverable,
    int  timeout
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet    lenBtrCoreRet   = enBTRCoreSuccess;
    const char*     pAdapterPath    = NULL;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;

    }

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (discoverable > 1)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    if (!(pAdapterPath = btrMgr_GetAdapterPath(aui8AdapterIdx))) {
        BTRMGRLOG_ERROR ("Failed to get adapter path\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }


    //timeout=0 -> no timeout, -1 -> default timeout (180 secs), x -> x seconds
    if (timeout >= 0) {
        lenBtrCoreRet = BTRCore_SetAdapterDiscoverableTimeout(ghBTRCoreHdl, pAdapterPath, timeout);
        if (lenBtrCoreRet != enBTRCoreSuccess) {
            BTRMGRLOG_ERROR ("Failed to set Adapter discovery timeout\n");
        }
        else {
            BTRMGRLOG_INFO ("Set timeout Successfully\n");
        }
    }

    /* Set the  discoverable state */
    if ((lenBtrCoreRet = BTRCore_SetAdapterDiscoverable(ghBTRCoreHdl, pAdapterPath, discoverable)) != enBTRCoreSuccess) {
        BTRMGRLOG_ERROR ("Failed to set Adapter discoverable status\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        BTRMGRLOG_INFO ("Set discoverable status Successfully\n");
        if (discoverable) {
            if (!btrMgr_GetAgentActivated()) {
                BTRMGRLOG_INFO ("Activate agent\n");
                if ((lenBtrCoreRet = BTRCore_RegisterAgent(ghBTRCoreHdl, 1)) != enBTRCoreSuccess) {
                    BTRMGRLOG_ERROR ("Failed to Activate Agent\n");
                    lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
                }
                else {
                    btrMgr_SetAgentActivated(1);
                }
            }
        }
    }

    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_IsAdapterDiscoverable (
    unsigned char   aui8AdapterIdx,
    unsigned char*  pDiscoverable
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet    lenBtrCoreRet   = enBTRCoreSuccess;
    const char*     pAdapterPath    = NULL;
    unsigned char   discoverable    = 0;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!pDiscoverable)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    
    if (!(pAdapterPath = btrMgr_GetAdapterPath(aui8AdapterIdx))) {
        BTRMGRLOG_ERROR ("Failed to get adapter path\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }
    

    if ((lenBtrCoreRet = BTRCore_GetAdapterDiscoverableStatus(ghBTRCoreHdl, pAdapterPath, &discoverable)) != enBTRCoreSuccess) {
        BTRMGRLOG_ERROR ("Failed to get Adapter Status\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        BTRMGRLOG_INFO ("Fetched Successfully\n");
        *pDiscoverable = discoverable;
        if (discoverable) {
            if (!btrMgr_GetAgentActivated()) {
                BTRMGRLOG_INFO ("Activate agent\n");
                if ((lenBtrCoreRet = BTRCore_RegisterAgent(ghBTRCoreHdl, 1)) != enBTRCoreSuccess) {
                    BTRMGRLOG_ERROR ("Failed to Activate Agent\n");
                    lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
                }
                else {
                    btrMgr_SetAgentActivated(1);
                }
            }
        }
    }

    return lenBtrMgrResult;
}

STATIC BTRMGR_Result_t
BTRMGR_StartDeviceDiscovery_Internal (
    unsigned char                aui8AdapterIdx, 
    BTRMGR_DeviceOperationType_t aenBTRMgrDevOpT
) {
    BTRMGR_Result_t     lenBtrMgrResult      = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet        lenBtrCoreRet        = enBTRCoreSuccess;
    enBTRCoreDeviceType lenBTRCoreDeviceType = enBTRCoreUnknown;
    const char*         pAdapterPath         = NULL;
    BTRMGR_DiscoveryHandle_t* ldiscoveryHdl  = NULL;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }
    else if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (!(pAdapterPath = btrMgr_GetAdapterPath(aui8AdapterIdx))) {
        BTRMGRLOG_ERROR ("Failed to get adapter path\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }
    // TODO Try to move this check logically into btrMgr_PreCheckDiscoveryStatus
    if ((ldiscoveryHdl = btrMgr_GetDiscoveryInProgress())) {
        if (aenBTRMgrDevOpT == btrMgr_GetDiscoveryDeviceType(ldiscoveryHdl)) {
            BTRMGRLOG_WARN ("[%s] Scan already in Progress...\n"
                           , btrMgr_GetDiscoveryDeviceTypeAsString (btrMgr_GetDiscoveryDeviceType(ldiscoveryHdl)));
            return BTRMGR_RESULT_SUCCESS;
        }
    }

    if (eBTRMgrSuccess != btrMgr_PreCheckDiscoveryStatus(aui8AdapterIdx, aenBTRMgrDevOpT)) {
        BTRMGRLOG_ERROR ("Pre Check Discovery State Rejected !!!\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    /* Populate the currently Paired Devices. This will be used only for the callback DS update */
    BTRMGR_GetPairedDevices (aui8AdapterIdx, &gListOfPairedDevices);

    switch (aenBTRMgrDevOpT) {
    case BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT:
        lenBTRCoreDeviceType = enBTRCoreSpeakers;
        break;
    case BTRMGR_DEVICE_OP_TYPE_AUDIO_INPUT:
        lenBTRCoreDeviceType = enBTRCoreMobileAudioIn;
        break;
    case BTRMGR_DEVICE_OP_TYPE_LE:
        lenBTRCoreDeviceType = enBTRCoreLE;
        break;
    case BTRMGR_DEVICE_OP_TYPE_HID:
        lenBTRCoreDeviceType = enBTRCoreHID;
        break;
    case BTRMGR_DEVICE_OP_TYPE_AUDIO_AND_HID:
        lenBTRCoreDeviceType = enBTRCoreAudioAndHID;
        break;
    case BTRMGR_DEVICE_OP_TYPE_UNKNOWN:
    default:
        lenBTRCoreDeviceType = enBTRCoreUnknown;
        break;
    } 


    lenBtrCoreRet = BTRCore_StartDiscovery(ghBTRCoreHdl, pAdapterPath, lenBTRCoreDeviceType, 0);
    if (lenBtrCoreRet != enBTRCoreSuccess) {
        BTRMGRLOG_ERROR ("Failed to start discovery\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        {   /* Max 5 sec timeout - Polled at 5ms second interval */
            unsigned int ui32sleepIdx = 1000;

            do {
                usleep(5000);
            } while ((!gIsAdapterDiscovering) && (--ui32sleepIdx));
        }

        if (!gIsAdapterDiscovering) {
            BTRMGRLOG_WARN ("Discovery is not yet Started !!!\n");
        }
        else {
            BTRMGRLOG_INFO ("Discovery started Successfully\n");
        }

        btrMgr_SetDiscoveryHandle (aenBTRMgrDevOpT, BTRMGR_DISCOVERY_ST_STARTED);
    }

    return lenBtrMgrResult;
}


STATIC BTRMGR_Result_t
BTRMGR_StopDeviceDiscovery_Internal (
    unsigned char                aui8AdapterIdx,
    BTRMGR_DeviceOperationType_t aenBTRMgrDevOpT
) {
    BTRMGR_Result_t     lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet        lenBtrCoreRet   = enBTRCoreSuccess;
    enBTRCoreDeviceType lenBTRCoreDeviceType = enBTRCoreUnknown;
    const char*         pAdapterPath    = NULL;
    BTRMGR_DiscoveryHandle_t* ahdiscoveryHdl = NULL;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (!(pAdapterPath = btrMgr_GetAdapterPath(aui8AdapterIdx))) {
        BTRMGRLOG_ERROR ("Failed to get adapter path\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    if (!(ahdiscoveryHdl = btrMgr_GetDiscoveryInProgress())) {
        BTRMGRLOG_WARN("Scanning is not running now\n");
    }

    switch (aenBTRMgrDevOpT) {
    case BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT:
        lenBTRCoreDeviceType = enBTRCoreSpeakers;
        break;
    case BTRMGR_DEVICE_OP_TYPE_AUDIO_INPUT:
        lenBTRCoreDeviceType = enBTRCoreMobileAudioIn;
        break;
    case BTRMGR_DEVICE_OP_TYPE_LE:
        lenBTRCoreDeviceType = enBTRCoreLE;
        break;
    case BTRMGR_DEVICE_OP_TYPE_HID:
        lenBTRCoreDeviceType = enBTRCoreHID;
        break;
    case BTRMGR_DEVICE_OP_TYPE_AUDIO_AND_HID:
        lenBTRCoreDeviceType = enBTRCoreAudioAndHID;
        break;
    case BTRMGR_DEVICE_OP_TYPE_UNKNOWN:
    default:
        lenBTRCoreDeviceType = enBTRCoreUnknown;
        break;
    }


    lenBtrCoreRet = BTRCore_StopDiscovery(ghBTRCoreHdl, pAdapterPath, lenBTRCoreDeviceType);
    if (lenBtrCoreRet != enBTRCoreSuccess) {
        BTRMGRLOG_ERROR ("Failed to stop discovery\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        BTRMGRLOG_INFO ("Discovery Stopped Successfully\n");

        {   /* Max 3 sec timeout - Polled at 50ms second interval */
            unsigned int ui32sleepIdx = 60;

            while ((gIsAdapterDiscovering) && (ui32sleepIdx--)) {
                usleep(50000);
            }
        }

        if (gIsAdapterDiscovering) {
            BTRMGRLOG_WARN ("Discovery is not yet Stopped !!!\n");
        }
        else {
            BTRMGRLOG_INFO ("Discovery Stopped Successfully\n");
        }

        if (ahdiscoveryHdl) {
            if ((btrMgr_GetDiscoveryDeviceType(ahdiscoveryHdl) == BTRMGR_DEVICE_OP_TYPE_LE && aenBTRMgrDevOpT == BTRMGR_DEVICE_OP_TYPE_LE)
                || btrMgr_GetDiscoveryDeviceType(ahdiscoveryHdl) != BTRMGR_DEVICE_OP_TYPE_LE) {

                btrMgr_SetDiscoveryState (ahdiscoveryHdl, BTRMGR_DISCOVERY_ST_STOPPED);
                if (btrMgr_GetDiscoveryDeviceType(ahdiscoveryHdl) != BTRMGR_DEVICE_OP_TYPE_LE &&
                    btrMgr_GetDiscoveryState(&ghBTRMgrBgDiscoveryHdl) == BTRMGR_DISCOVERY_ST_PAUSED) {
                    btrMgr_ClearDiscoveryHoldOffTimer();
                    btrMgr_SetDiscoveryHoldOffTimer(aui8AdapterIdx);
                }
            }
            else {
                btrMgr_SetDiscoveryState (ahdiscoveryHdl, BTRMGR_DISCOVERY_ST_PAUSED);
                btrMgr_SetDiscoveryHoldOffTimer(aui8AdapterIdx);
            }
        }
        else if (aenBTRMgrDevOpT == BTRMGR_DEVICE_OP_TYPE_LE) {
            if (btrMgr_GetDiscoveryState(&ghBTRMgrBgDiscoveryHdl) == BTRMGR_DISCOVERY_ST_PAUSED) {
                btrMgr_ClearDiscoveryHoldOffTimer();
                btrMgr_SetDiscoveryState (&ghBTRMgrBgDiscoveryHdl, BTRMGR_DISCOVERY_ST_STOPPED);
            }
        }
    }

    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_StartDeviceDiscovery (
    unsigned char                aui8AdapterIdx, 
    BTRMGR_DeviceOperationType_t aenBTRMgrDevOpT
) {
    BTRMGR_Result_t result = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsDiscoveryOpInternal = FALSE;

    MEMSET_S(&gListOfDiscoveredDevices, sizeof(gListOfDiscoveredDevices), 0, sizeof(gListOfDiscoveredDevices));
    result = BTRMGR_StartDeviceDiscovery_Internal(aui8AdapterIdx, aenBTRMgrDevOpT);

    if ((result == BTRMGR_RESULT_SUCCESS) && gfpcBBTRMgrEventOut) {
        BTRMGR_EventMessage_t lstEventMessage;
        MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));

        lstEventMessage.m_adapterIndex = aui8AdapterIdx;
        if (gIsAdapterDiscovering) {
            lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_DISCOVERY_STARTED;
        }

        gfpcBBTRMgrEventOut(lstEventMessage); /*  Post a callback */
    }

    return result;
}


BTRMGR_Result_t
BTRMGR_StopDeviceDiscovery (
    unsigned char                aui8AdapterIdx,
    BTRMGR_DeviceOperationType_t aenBTRMgrDevOpT
) {
    BTRMGR_Result_t result = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsDiscoveryOpInternal = FALSE;
    result = BTRMGR_StopDeviceDiscovery_Internal(aui8AdapterIdx, aenBTRMgrDevOpT);

    if ((result == BTRMGR_RESULT_SUCCESS) && gfpcBBTRMgrEventOut) {
        BTRMGR_EventMessage_t lstEventMessage;
        MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));

        lstEventMessage.m_adapterIndex = aui8AdapterIdx;
        if (!gIsAdapterDiscovering) {
            lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_DISCOVERY_COMPLETE;
        }

        gfpcBBTRMgrEventOut(lstEventMessage); /*  Post a callback */
    }

    return result;
}


BTRMGR_Result_t
BTRMGR_GetDiscoveryStatus (
    unsigned char                   aui8AdapterIdx, 
    BTRMGR_DiscoveryStatus_t*       isDiscoveryInProgress,
    BTRMGR_DeviceOperationType_t*   aenBTRMgrDevOpT
) {
    BTRMGR_Result_t result = BTRMGR_RESULT_SUCCESS;
    BTRMGR_DiscoveryHandle_t* ldiscoveryHdl  = NULL;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }
    else if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!isDiscoveryInProgress) || (!aenBTRMgrDevOpT)) {
            BTRMGRLOG_ERROR ("Input is invalid\n");
            return BTRMGR_RESULT_INVALID_INPUT;
    }

    if ((ldiscoveryHdl = btrMgr_GetDiscoveryInProgress())) {
        *isDiscoveryInProgress = BTRMGR_DISCOVERY_STATUS_IN_PROGRESS;
        *aenBTRMgrDevOpT = btrMgr_GetDiscoveryDeviceType(ldiscoveryHdl);
        BTRMGRLOG_WARN ("[%s] Scan already in Progress\n"
                               , btrMgr_GetDiscoveryDeviceTypeAsString (btrMgr_GetDiscoveryDeviceType(ldiscoveryHdl)));
    }
    else {
         *isDiscoveryInProgress = BTRMGR_DISCOVERY_STATUS_OFF;
         *aenBTRMgrDevOpT = BTRMGR_DEVICE_OP_TYPE_UNKNOWN;
    }
    return result;
}

BTRMGR_Result_t
BTRMGR_GetDiscoveredDevices (
    unsigned char                   aui8AdapterIdx,
    BTRMGR_DiscoveredDevicesList_t* pDiscoveredDevices
) {
    return BTRMGR_GetDiscoveredDevices_Internal(aui8AdapterIdx, pDiscoveredDevices);
}


STATIC BTRMGR_Result_t
BTRMGR_GetDiscoveredDevices_Internal (
    unsigned char                   aui8AdapterIdx,
    BTRMGR_DiscoveredDevicesList_t* pDiscoveredDevices
) {
    BTRMGR_Result_t                 lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet                    lenBtrCoreRet   = enBTRCoreSuccess;
    stBTRCoreScannedDevicesCount    lstBtrCoreListOfSDevices;
    BTRMGR_DeviceType_t         lenBtrMgrDevType  = BTRMGR_DEVICE_TYPE_UNKNOWN;
    int i = 0;
    int j = 0;

    if (isDeinitInProgress) {
        BTRMGRLOG_ERROR ("Process shutdown in progress, So not able read the list of discovered devices ...\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!pDiscoveredDevices)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    lenBtrCoreRet = BTRCore_GetListOfScannedDevices(ghBTRCoreHdl, &lstBtrCoreListOfSDevices);
    if (lenBtrCoreRet != enBTRCoreSuccess) {
        BTRMGRLOG_ERROR ("Failed to get list of discovered devices\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        MEMSET_S(pDiscoveredDevices, sizeof(BTRMGR_DiscoveredDevicesList_t), 0, sizeof(BTRMGR_DiscoveredDevicesList_t));

        if (lstBtrCoreListOfSDevices.numberOfDevices) {
            BTRMGR_DiscoveredDevices_t *lpstBtrMgrSDevice = NULL;

            pDiscoveredDevices->m_numOfDevices = (lstBtrCoreListOfSDevices.numberOfDevices < BTRMGR_DISCOVERED_DEVICE_COUNT_MAX) ? 
                                                    lstBtrCoreListOfSDevices.numberOfDevices : BTRMGR_DISCOVERED_DEVICE_COUNT_MAX;

            for (i = 0,j = 0; j < pDiscoveredDevices->m_numOfDevices; i++,j++) {
                lenBtrMgrDevType =  btrMgr_MapDeviceTypeFromCore(lstBtrCoreListOfSDevices.devices[i].enDeviceType);
                if (!gIsHidGamePadEnabled && (lenBtrMgrDevType == BTRMGR_DEVICE_TYPE_HID_GAMEPAD)) {
                    j--;
                    pDiscoveredDevices->m_numOfDevices--;
                    BTRMGRLOG_WARN ("HID GamePad RFC not enable, HID GamePad devices are not listing \n");
                    continue;
                }

                lpstBtrMgrSDevice = &pDiscoveredDevices->m_deviceProperty[j];

                lpstBtrMgrSDevice->m_deviceHandle   = lstBtrCoreListOfSDevices.devices[i].tDeviceId;
                lpstBtrMgrSDevice->m_rssi           = lstBtrCoreListOfSDevices.devices[i].i32RSSI;
                lpstBtrMgrSDevice->m_signalLevel    = btrMgr_MapSignalStrengthToRSSI (lstBtrCoreListOfSDevices.devices[i].i32RSSI);
                lpstBtrMgrSDevice->m_deviceType     = btrMgr_MapDeviceTypeFromCore(lstBtrCoreListOfSDevices.devices[i].enDeviceType);
                lpstBtrMgrSDevice->m_isPairedDevice = btrMgr_GetDevPaired(lstBtrCoreListOfSDevices.devices[i].tDeviceId);
                lpstBtrMgrSDevice->m_ui32DevClassBtSpec = lstBtrCoreListOfSDevices.devices[i].ui32DevClassBtSpec;
                lpstBtrMgrSDevice->m_ui16DevAppearanceBleSpec = lstBtrCoreListOfSDevices.devices[i].ui16DevAppearanceBleSpec;
                strncpy(lpstBtrMgrSDevice->m_name,          lstBtrCoreListOfSDevices.devices[i].pcDeviceName,   (BTRMGR_NAME_LEN_MAX - 1));
                strncpy(lpstBtrMgrSDevice->m_deviceAddress, lstBtrCoreListOfSDevices.devices[i].pcDeviceAddress,(BTRMGR_NAME_LEN_MAX - 1));
                if (lstBtrCoreListOfSDevices.devices[i].bDeviceConnected) {
                    lpstBtrMgrSDevice->m_isConnected = 1;
                }

                if (!btrMgr_CheckIfDevicePrevDetected(lpstBtrMgrSDevice->m_deviceHandle) && (gListOfDiscoveredDevices.m_numOfDevices < BTRMGR_DISCOVERED_DEVICE_COUNT_MAX)) {
                    /* Update BTRMgr DiscoveredDev List cache */
                   MEMCPY_S(&gListOfDiscoveredDevices.m_deviceProperty[gListOfDiscoveredDevices.m_numOfDevices],sizeof(gListOfDiscoveredDevices.m_deviceProperty[0]), lpstBtrMgrSDevice, sizeof(BTRMGR_DiscoveredDevices_t));
                    gListOfDiscoveredDevices.m_numOfDevices++;
                }
            }
            /*  Success */
            BTRMGRLOG_DEBUG ("Successful\n");
        }
        else {
            BTRMGRLOG_WARN("No Device is found yet\n");
        }
    }

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_PairDevice (
    unsigned char       aui8AdapterIdx,
    BTRMgrDeviceHandle  ahBTRMgrDevHdl
) {
    stBTRCoreBTDevice       stDeviceInfo;
    BTRMGR_EventMessage_t   lstEventMessage;
    BTRMGR_Result_t         lenBtrMgrResult     = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet            lenBtrCoreRet       = enBTRCoreSuccess;
    BTRMGR_Events_t         lBtMgrOutEvent      = -1;
    unsigned char           ui8reActivateAgent  = 0;
    unsigned char           ui8isDevicePaired   = 0;
    enBTRCoreDeviceType     lenBTRCoreDevTy     = enBTRCoreSpeakers;
    enBTRCoreDeviceClass    lenBTRCoreDevCl     = enBTRCore_DC_Unknown;
    eBTRMgrRet              lenBtrMgrRet        = eBTRMgrFailure;
    int                     j;
    gboolean                bIsPS4 = FALSE;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!ahBTRMgrDevHdl)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    btrMgr_PreCheckDiscoveryStatus(aui8AdapterIdx, BTRMGR_DEVICE_OP_TYPE_UNKNOWN);

    /* Update the Paired Device List */
    BTRMGR_GetPairedDevices (aui8AdapterIdx, &gListOfPairedDevices);
    if (1 == btrMgr_GetDevPaired(ahBTRMgrDevHdl)) {
        BTRMGRLOG_INFO ("Already a Paired Device; Nothing Done...\n");
        return BTRMGR_RESULT_SUCCESS;
    }

    BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBTRCoreDevTy, &lenBTRCoreDevCl);
    BTRMGRLOG_INFO ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBTRCoreDevTy, lenBTRCoreDevCl);

    if (!gIsHidGamePadEnabled && (lenBTRCoreDevCl == enBTRCore_DC_HID_GamePad)) {
        BTRMGRLOG_WARN ("BTR HID GamePad is currently Disabled\n");
       return BTRMGR_RESULT_GENERIC_FAILURE;
    }
    if (!gIsAudioInEnabled && ((lenBTRCoreDevTy == enBTRCoreMobileAudioIn) || (lenBTRCoreDevTy == enBTRCorePCAudioIn))) {
        BTRMGRLOG_WARN ("Pairing Rejected - BTR AudioIn is currently Disabled!\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }
    /* We always need a agent to get the pairing done.. if no agent registered, default agent will be used.
     * But we will not able able to get the PIN and other stuff received at our level. We have to have agent
     * for pairing process..
     *
     * We are using the default-agent for the audio devices and we have it deployed to field already.
     *
     * Since HID is new device and to not to break the Audio devices, BTMGR specific Agent is registered only for HID devices.
     */
#ifdef LE_MODE
    if (lenBTRCoreDevTy != enBTRCoreHID && lenBTRCoreDevTy != enBTRCoreLE) {
#else
    if (lenBTRCoreDevTy != enBTRCoreHID) {
#endif
        if (btrMgr_GetAgentActivated()) {
            BTRMGRLOG_INFO ("De-Activate agent\n");
            if ((lenBtrCoreRet = BTRCore_UnregisterAgent(ghBTRCoreHdl)) != enBTRCoreSuccess) {
                BTRMGRLOG_ERROR ("Failed to Deactivate Agent\n");
                lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
            }
            else {
                btrMgr_SetAgentActivated(0);
                ui8reActivateAgent = 1;
            }
        }
    }
    else
        BTRMGRLOG_DEBUG ("For HID Devices, let us use the agent\n");

    if(lenBTRCoreDevTy == enBTRCoreHID){
        ghBTRMgrDevHdlPairingInProgress = ahBTRMgrDevHdl;
        if ((ghBTRMgrDevHdlLastPaired == ahBTRMgrDevHdl) && (gLastPairedTimeoutRef != 0)) {
            ghBTRMgrDevHdlLastPaired = 0;
            btrMgr_ClearLastPairedDeviceStatusHoldOffTimer();
        }

        bIsPS4 = btrMgr_IsPS4Gamepad(ahBTRMgrDevHdl);
    }

    if (!bIsPS4) {
        if (enBTRCoreSuccess != BTRCore_PairDevice(ghBTRCoreHdl, ahBTRMgrDevHdl)) {
            BTRMGRLOG_ERROR ("Failed to pair a device\n");
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
            lBtMgrOutEvent  = BTRMGR_EVENT_DEVICE_PAIRING_FAILED;
        }
        else {
            lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
            lBtMgrOutEvent  = BTRMGR_EVENT_DEVICE_PAIRING_COMPLETE;
        }
    }
    /* The Paring is not a blocking call for HID. So we can sent the pairing complete msg when paring actually completed and gets a notification.
     * But the pairing failed, we must send it right here. */
    MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));
    btrMgr_GetDiscoveredDevInfo (ahBTRMgrDevHdl, &lstEventMessage.m_discoveredDevice);

    if (lstEventMessage.m_discoveredDevice.m_deviceHandle != ahBTRMgrDevHdl) {
        BTRMGRLOG_WARN ("Attempted to pair Undiscovered device - %lld\n", ahBTRMgrDevHdl);
        lstEventMessage.m_discoveredDevice.m_deviceHandle = ahBTRMgrDevHdl;
    }

    lstEventMessage.m_adapterIndex = aui8AdapterIdx;
    lstEventMessage.m_eventType    = lBtMgrOutEvent;

    if ((lenBtrMgrResult != BTRMGR_RESULT_SUCCESS) ||
        ((lenBtrMgrResult == BTRMGR_RESULT_SUCCESS) &&
        ((lenBTRCoreDevTy != enBTRCoreHID) && (lenBTRCoreDevTy != enBTRCoreMobileAudioIn) && (lenBTRCoreDevTy != enBTRCorePCAudioIn)))) {
        if (gfpcBBTRMgrEventOut) {
            gfpcBBTRMgrEventOut(lstEventMessage);
        }
    }

    /* Update the Paired Device List */
    if (lenBTRCoreDevTy == enBTRCoreHID && !bIsPS4) {
        if (BTRMGR_EVENT_DEVICE_PAIRING_COMPLETE == lBtMgrOutEvent) {
            unsigned int ui32retrycount = BTRMGR_PAIR_RETRY_ATTEMPTS;

            do {
                sleep(1);
                 /* Avoid invoking BTRMGR_GetPairedDevices here just after initiating the pair
                 * without delay.It causes current state of device updated as paired on btrCore
                 * by parsing the properties on bluez before the pairing was completely done.
                 * Paired device handle updated on the local structure once pairig is
                 * completed and received the event in btrMgr_DeviceStatusCb.
                 */
                for (j = 0; j <= gListOfPairedDevices.m_numOfDevices; j++) {
                    if (ahBTRMgrDevHdl == gListOfPairedDevices.m_deviceProperty[j].m_deviceHandle) {
                        ui8isDevicePaired = 1;
                        break;
                    }
                }
                if (1 == ui8isDevicePaired) {
                    break;
                }
            }while (--ui32retrycount);

            ui8isDevicePaired = 0;
        }
    }
    //Inside this function there is a failure logs so, did not any failures logs here.
    lenBtrMgrRet = btrMgr_GetDeviceDetails(ahBTRMgrDevHdl,&stDeviceInfo);

    BTRMGR_GetPairedDevices (aui8AdapterIdx, &gListOfPairedDevices);
    for (j = 0; j <= gListOfPairedDevices.m_numOfDevices; j++) {
         if ((ahBTRMgrDevHdl == gListOfPairedDevices.m_deviceProperty[j].m_deviceHandle) && (BTRMGR_EVENT_DEVICE_PAIRING_COMPLETE == lBtMgrOutEvent )) {
            BTRMGRLOG_DEBUG ("Paired device info: handle, Name -  %llu,%s\n",ahBTRMgrDevHdl,gListOfPairedDevices.m_deviceProperty[j].m_name);
            //This is telemetry log. If we change this print,need to change and configure the telemetry string in xconf server.
            BTRMGRLOG_INFO ("Paired Successfully name,class,apperance,modalias: %s,%u,%u,v%04Xp%04Xd%04X\n",
            stDeviceInfo.pcDeviceName, stDeviceInfo.ui32DevClassBtSpec, stDeviceInfo.ui16DevAppearanceBleSpec,
            stDeviceInfo.ui32ModaliasVendorId, stDeviceInfo.ui32ModaliasProductId, stDeviceInfo.ui32ModaliasDeviceId);
            BTRMGRLOG_INFO ("Pairing success device MAC %s\n", stDeviceInfo.pcDeviceAddress);

            ui8isDevicePaired = 1;

            if (gConnPairCompRstTimeOutRef) {
                BTRMGRLOG_DEBUG ("Cancelling previous Pair Reset handle TimeOut Session : %u\n", gConnPairCompRstTimeOutRef);
                g_source_destroy(g_main_context_find_source_by_id(gmainContext, gConnPairCompRstTimeOutRef));
                gConnPairCompRstTimeOutRef = 0;
            }

            gPairCompRstTimeOutCbData = 0; // TODO: Change when this entire file is made re-entrant

            GSource* source = g_timeout_source_new(BTRMGR_DEVPAIR_HANDLE_RESET_TIME * 1000);
            g_source_set_priority(source, G_PRIORITY_DEFAULT);
            g_source_set_callback(source, btrMgr_PairCompleteRstTimerCb, (gpointer)&gPairCompRstTimeOutCbData, NULL);

            gConnPairCompRstTimeOutRef = g_source_attach(source, gmainContext);
            g_source_unref(source);

            if (gConnPairCompRstTimeOutRef != 0) {
                BTRMGRLOG_INFO ("Triggered Pair timeout handle reset : %u\n", gConnPairCompRstTimeOutRef);
            }

            break;
        }
    }
   
    if (ui8reActivateAgent) {
        BTRMGRLOG_INFO ("Activate agent\n");
        if ((lenBtrCoreRet = BTRCore_RegisterAgent(ghBTRCoreHdl, 1)) != enBTRCoreSuccess) {
            BTRMGRLOG_ERROR ("Failed to Activate Agent\n");
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }
        else {
            btrMgr_SetAgentActivated(1);
        }
    }

    if(lenBTRCoreDevTy == enBTRCoreHID) {
        if(!ui8isDevicePaired) {

            if(bIsPS4) {
                lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_PAIRING_COMPLETE;
                lstEventMessage.m_discoveredDevice.m_isPairedDevice = 1;
                lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
            } else {
                lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_PAIRING_FAILED;
                lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
            }

            if (gfpcBBTRMgrEventOut) {
                gfpcBBTRMgrEventOut(lstEventMessage);
            }

            BTRMGRLOG_INFO("Get device details status %u\n", lenBtrMgrRet);
            if(lenBtrMgrRet == eBTRMgrSuccess) {
                //This is telemetry log. If we change this print,need to change and configure the telemetry string in xconf server.
                BTRMGRLOG_ERROR ("Failed to pair a device name,class,apperance,modalias: %s,%u,%u,v%04Xp%04Xd%04X\n",
                stDeviceInfo.pcDeviceName, stDeviceInfo.ui32DevClassBtSpec, stDeviceInfo.ui16DevAppearanceBleSpec,
                stDeviceInfo.ui32ModaliasVendorId, stDeviceInfo.ui32ModaliasProductId, stDeviceInfo.ui32ModaliasDeviceId);
            }

            BTRMGRLOG_ERROR ("pairing failed device MAC %s\n",stDeviceInfo.pcDeviceAddress);
            ghBTRMgrDevHdlLastPaired = ahBTRMgrDevHdl;
            btrMgr_SetLastPairedDeviceStatusHoldOffTimer();
        }
        if (stDeviceInfo.ui32ModaliasProductId == BTRMGR_XBOX_ELITE_PRODUCT_ID && stDeviceInfo.ui32ModaliasVendorId == BTRMGR_XBOX_ELITE_VENDOR_ID && stDeviceInfo.ui32ModaliasDeviceId == BTRMGR_XBOX_ELITE_DEFAULT_FIRMWARE && stDeviceInfo.bDeviceConnected == 1) {
            BTRMGRLOG_INFO("Disconnecting the connection for xbox elite ....\n");
            lenBtrCoreRet = BTRCore_DisconnectDevice (ghBTRCoreHdl, ahBTRMgrDevHdl, lenBTRCoreDevTy);
            if (lenBtrCoreRet != enBTRCoreSuccess) {
                BTRMGRLOG_ERROR ("Failed to Disconnect - %llu\n", ahBTRMgrDevHdl);
            }
        } 
    }

    btrMgr_PostCheckDiscoveryStatus(aui8AdapterIdx, btrMgr_MapDeviceOpFromDeviceType( btrMgr_MapDeviceTypeFromCore(lenBTRCoreDevCl)));
    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_UnpairDevice (
    unsigned char       aui8AdapterIdx,
    BTRMgrDeviceHandle  ahBTRMgrDevHdl
) {
    stBTRCoreBTDevice       stDeviceInfo;
    BTRMGR_Result_t         lenBtrMgrResult     = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet            lenBtrCoreRet       = enBTRCoreSuccess;
    enBTRCoreDeviceType     lenBTRCoreDevTy     = enBTRCoreSpeakers;
    enBTRCoreDeviceClass    lenBTRCoreDevCl     = enBTRCore_DC_Unknown;
    BTRMGR_Events_t         lBtMgrOutEvent      = -1;
    unsigned char           ui8reActivateAgent  = 0;
    BTRMGR_EventMessage_t   lstEventMessage;


    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!ahBTRMgrDevHdl)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    btrMgr_PreCheckDiscoveryStatus(aui8AdapterIdx, BTRMGR_DEVICE_OP_TYPE_UNKNOWN);

    /* Get the latest Paired Device List; This is added as the developer could add a device thro test application and try unpair thro' UI */
    BTRMGR_GetPairedDevices (aui8AdapterIdx, &gListOfPairedDevices);
    MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));
    btrMgr_GetPairedDevInfo (ahBTRMgrDevHdl, &lstEventMessage.m_pairedDevice);

    if (!lstEventMessage.m_pairedDevice.m_deviceHandle) {
        BTRMGRLOG_ERROR ("Not a Paired device...\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBTRCoreDevTy, &lenBTRCoreDevCl);
    BTRMGRLOG_INFO ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBTRCoreDevTy, lenBTRCoreDevCl);


#ifdef LE_MODE
    if (lenBTRCoreDevTy != enBTRCoreHID && lenBTRCoreDevTy != enBTRCoreLE)
#endif
    {
        if (btrMgr_GetAgentActivated()) {
            BTRMGRLOG_INFO ("De-Activate agent\n");
            if ((lenBtrCoreRet = BTRCore_UnregisterAgent(ghBTRCoreHdl)) != enBTRCoreSuccess) {
                BTRMGRLOG_ERROR ("Failed to Deactivate Agent\n");
                lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
            }
            else {
                btrMgr_SetAgentActivated(0);
                ui8reActivateAgent = 1;
            }
        }
    }


    /* Check whether the requested device is connected n playing. */
    if (ghBTRMgrDevHdlCurStreaming == ahBTRMgrDevHdl) {
        /* This will internall stops the playback as well as disconnects. */
        if ((lenBTRCoreDevTy == enBTRCoreSpeakers) || (lenBTRCoreDevTy == enBTRCoreHeadSet)) {
            /* Streaming-Out is happening; stop it */
            if ((lenBtrMgrResult = BTRMGR_StopAudioStreamingOut(aui8AdapterIdx, ghBTRMgrDevHdlCurStreaming)) != BTRMGR_RESULT_SUCCESS) {
                BTRMGRLOG_ERROR ("BTRMGR_UnpairDevice :This device is being Connected n Playing. Failed to stop Playback. Going Ahead to unpair.-Out\n");
            }
        }
        else if ((lenBTRCoreDevTy == enBTRCoreMobileAudioIn) || (lenBTRCoreDevTy == enBTRCorePCAudioIn)) {
            /* Streaming-In is happening; stop it */
            if ((lenBtrMgrResult = BTRMGR_StopAudioStreamingIn(aui8AdapterIdx, ghBTRMgrDevHdlCurStreaming)) != BTRMGR_RESULT_SUCCESS) {
                BTRMGRLOG_ERROR ("BTRMGR_UnpairDevice :This device is being Connected n Playing. Failed to stop Playback. Going Ahead to unpair.-In\n");
            }
        }
    }

    //Inside this function there is a failure logs so, did not any failures logs here.
    btrMgr_GetDeviceDetails(ahBTRMgrDevHdl,&stDeviceInfo);

    if (enBTRCoreSuccess != BTRCore_UnPairDevice(ghBTRCoreHdl, ahBTRMgrDevHdl)) {
        //This is telemetry log. If we change this print,need to change and configure the telemetry string in xconf server.
        BTRMGRLOG_ERROR ("Failed to unpair name,class,apperance,modalias: %s,%u,%u,v%04Xp%04Xd%04X\n",
        stDeviceInfo.pcDeviceName, stDeviceInfo.ui32DevClassBtSpec, stDeviceInfo.ui16DevAppearanceBleSpec,
        stDeviceInfo.ui32ModaliasVendorId, stDeviceInfo.ui32ModaliasProductId, stDeviceInfo.ui32ModaliasDeviceId);
        BTRMGRLOG_ERROR ("Unpairing failed device MAC %s\n",stDeviceInfo.pcDeviceAddress);

        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        lBtMgrOutEvent  = BTRMGR_EVENT_DEVICE_UNPAIRING_FAILED;
    }
    else {
        int j;
        for (j = 0; j <= gListOfPairedDevices.m_numOfDevices; j++) {
            if (ahBTRMgrDevHdl == gListOfPairedDevices.m_deviceProperty[j].m_deviceHandle) {
                //This is telemetry log. If we change this print,need to change and configure the telemetry string in xconf server.
                BTRMGRLOG_INFO ("Unpaired Successfully name,class,apperance,modalias: %s,%u,%u,v%04Xp%04Xd%04X\n",
                stDeviceInfo.pcDeviceName, stDeviceInfo.ui32DevClassBtSpec, stDeviceInfo.ui16DevAppearanceBleSpec,
                stDeviceInfo.ui32ModaliasVendorId, stDeviceInfo.ui32ModaliasProductId, stDeviceInfo.ui32ModaliasDeviceId);

                BTRMGRLOG_INFO ("Unpaired device handle -  %llu - Name -  %s MAC %s\n",ahBTRMgrDevHdl,gListOfPairedDevices.m_deviceProperty[j].m_name, stDeviceInfo.pcDeviceAddress);
                lstEventMessage.m_pairedDevice.m_isConnected = 0;
                break;
            }
        }   
        lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
        lBtMgrOutEvent  = BTRMGR_EVENT_DEVICE_UNPAIRING_COMPLETE;
    }


    {
        lstEventMessage.m_adapterIndex = aui8AdapterIdx;
        lstEventMessage.m_eventType    = lBtMgrOutEvent;

        if (gfpcBBTRMgrEventOut) {
            gfpcBBTRMgrEventOut(lstEventMessage); /*  Post a callback */
        }
    }


    if ((lenBTRCoreDevTy == enBTRCoreSpeakers) || (lenBTRCoreDevTy == enBTRCoreHeadSet)) {
            btrMgr_RemovePersistentEntry(aui8AdapterIdx, ahBTRMgrDevHdl, BTRMGR_A2DP_SINK_PROFILE_ID);
    }
    else if ((lenBTRCoreDevTy == enBTRCoreMobileAudioIn) || (lenBTRCoreDevTy == enBTRCorePCAudioIn)) {
            btrMgr_RemovePersistentEntry(aui8AdapterIdx, ahBTRMgrDevHdl, BTRMGR_A2DP_SRC_PROFILE_ID);
    }

    /* Update the Paired Device List */
    BTRMGR_GetPairedDevices (aui8AdapterIdx, &gListOfPairedDevices);

    if (ui8reActivateAgent) {
        BTRMGRLOG_INFO ("Activate agent\n");
        if ((lenBtrCoreRet = BTRCore_RegisterAgent(ghBTRCoreHdl, 1)) != enBTRCoreSuccess) {
            BTRMGRLOG_ERROR ("Failed to Activate Agent\n");
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }
        else {
            btrMgr_SetAgentActivated(1);
        }
    }

    btrMgr_PostCheckDiscoveryStatus(aui8AdapterIdx, btrMgr_MapDeviceOpFromDeviceType( btrMgr_MapDeviceTypeFromCore(lenBTRCoreDevCl)));

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_GetPairedDevices (
    unsigned char                aui8AdapterIdx,
    BTRMGR_PairedDevicesList_t*  pPairedDevices
) {
    BTRMGR_Result_t             lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet                lenBtrCoreRet   = enBTRCoreSuccess;
    stBTRCorePairedDevicesCount lstBtrCoreListOfPDevices;
    int i = 0;
    int j = 0;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!pPairedDevices)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    lstBtrCoreListOfPDevices.numberOfDevices = 0;
    for (i = 0; i < BTRCORE_MAX_NUM_BT_DEVICES; i++) {
        MEMSET_S(&lstBtrCoreListOfPDevices.devices[i], sizeof(stBTRCoreBTDevice), 0, sizeof(stBTRCoreBTDevice));
    }


    lenBtrCoreRet = BTRCore_GetListOfPairedDevices(ghBTRCoreHdl, &lstBtrCoreListOfPDevices);
    if (lenBtrCoreRet != enBTRCoreSuccess) {
        BTRMGRLOG_ERROR ("Failed to get list of paired devices\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        pPairedDevices->m_numOfDevices = 0;
        for (i = 0; i < BTRMGR_DEVICE_COUNT_MAX; i++) {
            MEMSET_S(&pPairedDevices->m_deviceProperty[i], sizeof(BTRMGR_PairedDevices_t), 0, sizeof(BTRMGR_PairedDevices_t));     /* Reset the values to 0 */
        }

        if (lstBtrCoreListOfPDevices.numberOfDevices) {
            BTRMGR_PairedDevices_t* lpstBtrMgrPDevice = NULL;

            pPairedDevices->m_numOfDevices = (lstBtrCoreListOfPDevices.numberOfDevices < BTRMGR_DEVICE_COUNT_MAX) ? 
                                                lstBtrCoreListOfPDevices.numberOfDevices : BTRMGR_DEVICE_COUNT_MAX;

            for (i = 0; i < pPairedDevices->m_numOfDevices; i++) {
                lpstBtrMgrPDevice = &pPairedDevices->m_deviceProperty[i];

                lpstBtrMgrPDevice->m_deviceHandle = lstBtrCoreListOfPDevices.devices[i].tDeviceId;
                lpstBtrMgrPDevice->m_deviceType   = btrMgr_MapDeviceTypeFromCore (lstBtrCoreListOfPDevices.devices[i].enDeviceType);

                strncpy(lpstBtrMgrPDevice->m_name,          lstBtrCoreListOfPDevices.devices[i].pcDeviceName,   (BTRMGR_NAME_LEN_MAX - 1));
                strncpy(lpstBtrMgrPDevice->m_deviceAddress, lstBtrCoreListOfPDevices.devices[i].pcDeviceAddress,(BTRMGR_NAME_LEN_MAX - 1));

                strncpy(lpstBtrMgrPDevice->m_deviceAddress, lstBtrCoreListOfPDevices.devices[i].pcDeviceAddress,strlen(lstBtrCoreListOfPDevices.devices[i].pcDeviceAddress)-1);

                lpstBtrMgrPDevice->m_ui32DevClassBtSpec = lstBtrCoreListOfPDevices.devices[i].ui32DevClassBtSpec;
                lpstBtrMgrPDevice->m_ui16DevAppearanceBleSpec = lstBtrCoreListOfPDevices.devices[i].ui16DevAppearanceBleSpec;

                BTRMGRLOG_INFO ("Paired Device ID = %lld Connected = %d Dev Address = %s Dev Type = %d\n", lstBtrCoreListOfPDevices.devices[i].tDeviceId, lstBtrCoreListOfPDevices.devices[i].bDeviceConnected, lpstBtrMgrPDevice->m_deviceAddress,lpstBtrMgrPDevice->m_deviceType);
                //BTRMGRLOG_INFO("Dev Address = %s Dev Type = %d \n",lpstBtrMgrPDevice->m_deviceAddress,lpstBtrMgrPDevice->m_deviceType);

                lpstBtrMgrPDevice->m_serviceInfo.m_numOfService = lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.numberOfService;
                for (j = 0; j < lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.numberOfService; j++) {
                    BTRMGRLOG_DEBUG ("Profile ID = %u; Profile Name = %s\n", lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.profile[j].uuid_value,
                                                                            lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.profile[j].profile_name);
                    lpstBtrMgrPDevice->m_serviceInfo.m_profileInfo[j].m_uuid = lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.profile[j].uuid_value;
                    STRCPY_S(lpstBtrMgrPDevice->m_serviceInfo.m_profileInfo[j].m_profile,sizeof(lpstBtrMgrPDevice->m_serviceInfo.m_profileInfo[j].m_profile), lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.profile[j].profile_name);
                }

                if (lstBtrCoreListOfPDevices.devices[i].bDeviceConnected) {
                    lpstBtrMgrPDevice->m_isConnected = 1;
                }
            }
            /*  Success */
            BTRMGRLOG_TRACE ("Successful\n");
        }
        else {
            BTRMGRLOG_WARN("No Device is paired yet\n");
        }
    }

    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_ConnectToDevice (
    unsigned char                   aui8AdapterIdx,
    BTRMgrDeviceHandle              ahBTRMgrDevHdl,
    BTRMGR_DeviceOperationType_t    connectAs
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreDeviceType     lenBTRCoreDevTy     = enBTRCoreUnknown;
    enBTRCoreDeviceClass    lenBTRCoreDevCl     = enBTRCore_DC_Unknown;


    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;

    }

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!ahBTRMgrDevHdl)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBTRCoreDevTy, &lenBTRCoreDevCl);
    BTRMGRLOG_INFO (" Device Type = %d\t Device Class = %x\n", lenBTRCoreDevTy, lenBTRCoreDevCl);

    if (!gIsHidGamePadEnabled && (lenBTRCoreDevCl == enBTRCore_DC_HID_GamePad)) {
        BTRMGRLOG_WARN ("BTR HID GamePad is currently Disabled\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    if (btrMgr_ConnectToDevice(aui8AdapterIdx, ahBTRMgrDevHdl, connectAs, 0, 1) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("Failure\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return  lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_DisconnectFromDevice (
    unsigned char       aui8AdapterIdx,
    BTRMgrDeviceHandle  ahBTRMgrDevHdl
) {
    stBTRCoreBTDevice       stDeviceInfo;
    BTRMGR_Result_t         lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet            lenBtrCoreRet   = enBTRCoreSuccess;
    enBTRCoreDeviceType     lenBTRCoreDevTy = enBTRCoreSpeakers;
    enBTRCoreDeviceClass    lenBTRCoreDevCl = enBTRCore_DC_Unknown;
    BTRMGR_DeviceOperationType_t lenBtrMgrDevOpType = BTRMGR_DEVICE_OP_TYPE_UNKNOWN;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!ahBTRMgrDevHdl)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBTRCoreDevTy, &lenBTRCoreDevCl);
    BTRMGRLOG_INFO ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBTRCoreDevTy, lenBTRCoreDevCl);

    if (((lenBtrMgrDevOpType = btrMgr_MapDeviceOpFromDeviceType( btrMgr_MapDeviceTypeFromCore(lenBTRCoreDevCl))) == BTRMGR_DEVICE_OP_TYPE_UNKNOWN) &&
        (lenBtrCoreRet   == enBTRCoreFailure) &&
        (lenBTRCoreDevTy == enBTRCoreUnknown)) {
        //TODO: Bad Bad Bad way to treat a Unknown device as LE device, when we are unable to determine from BTRCore
        //      BTRCore_GetDeviceTypeClass. Means device was removed from the lower layer Scanned/Paired devices
        //      After Discovered/Paired lists by operation of another client of BTRMgr
        lenBTRCoreDevTy     = enBTRCoreLE;
        lenBtrMgrDevOpType  = BTRMGR_DEVICE_OP_TYPE_LE;
    }

    if (eBTRMgrSuccess != btrMgr_PreCheckDiscoveryStatus(aui8AdapterIdx, lenBtrMgrDevOpType)) {
        BTRMGRLOG_ERROR ("Pre Check Discovery State Failed !!!\n");
        if (lenBTRCoreDevTy == enBTRCoreLE) {
            btrMgr_PostCheckDiscoveryStatus(aui8AdapterIdx, BTRMGR_DEVICE_OP_TYPE_UNKNOWN);
        }
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    if ((lenBTRCoreDevTy != enBTRCoreLE) && !btrMgr_IsDevConnected(ahBTRMgrDevHdl)) {
        btrMgr_PostCheckDiscoveryStatus(aui8AdapterIdx, lenBtrMgrDevOpType);
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    if (lenBTRCoreDevTy == enBTRCoreLE && !gIsLeDeviceConnected) {
        BTRMGRLOG_ERROR ("No LE Device is connected at this time\n");
        btrMgr_PostCheckDiscoveryStatus(aui8AdapterIdx, BTRMGR_DEVICE_OP_TYPE_UNKNOWN);
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }


    if (lenBTRCoreDevTy != enBTRCoreLE) {

        if (ghBTRMgrDevHdlCurStreaming) {
            if ((lenBTRCoreDevTy == enBTRCoreSpeakers) || (lenBTRCoreDevTy == enBTRCoreHeadSet)) {
                /* Streaming-Out is happening; stop it */
                if ((lenBtrMgrResult = BTRMGR_StopAudioStreamingOut(aui8AdapterIdx, ghBTRMgrDevHdlCurStreaming)) != BTRMGR_RESULT_SUCCESS) {
                    BTRMGRLOG_ERROR ("Streamout failed to stop\n");
                }
            }
            else if ((lenBTRCoreDevTy == enBTRCoreMobileAudioIn) || (lenBTRCoreDevTy == enBTRCorePCAudioIn)) {
                /* Streaming-In is happening; stop it */
                if ((lenBtrMgrResult = BTRMGR_StopAudioStreamingIn(aui8AdapterIdx, ghBTRMgrDevHdlCurStreaming)) != BTRMGR_RESULT_SUCCESS) {
                    BTRMGRLOG_ERROR ("Streamin failed to stop\n");
                }
            }
        }
        //TODO: If we have more than one Bluetooth device connected, we need to enhance the code.
        gIsUserInitiated = 1;
    }


    /* connectAs param is unused for now.. */
    lenBtrCoreRet = BTRCore_DisconnectDevice (ghBTRCoreHdl, ahBTRMgrDevHdl, lenBTRCoreDevTy);
    if (lenBtrCoreRet != enBTRCoreSuccess) {
        BTRMGRLOG_ERROR ("Failed to Disconnect - %llu\n", ahBTRMgrDevHdl);
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;

        {
            BTRMGR_EventMessage_t lstEventMessage;
            MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));

            btrMgr_GetPairedDevInfo(ahBTRMgrDevHdl, &lstEventMessage.m_pairedDevice);

            lstEventMessage.m_adapterIndex = aui8AdapterIdx;
            lstEventMessage.m_eventType    = BTRMGR_EVENT_DEVICE_DISCONNECT_FAILED;
            lstEventMessage.m_pairedDevice.m_isLowEnergyDevice = (lenBTRCoreDevCl==enBTRCore_DC_Tile)?1:0;//Will make it generic later

            if (gfpcBBTRMgrEventOut) {
                gfpcBBTRMgrEventOut(lstEventMessage); /*  Post a callback */
            }
        }
    }
    else {
        int j;
        for (j = 0; j <= gListOfPairedDevices.m_numOfDevices; j++) {
             if (ahBTRMgrDevHdl == gListOfPairedDevices.m_deviceProperty[j].m_deviceHandle) {
                 BTRMGRLOG_INFO ("Disconnected Successfully -  %llu - Name -  %s\n",ahBTRMgrDevHdl,gListOfPairedDevices.m_deviceProperty[j].m_name);
                 /* Took the disconnected device handle before the confirmation of disconnection
                  * because the incoming connection of gamepad is received very early on the
                  * disconnection process.
                  */
                 ghBTRMgrDevHdlLastDisconnected = ahBTRMgrDevHdl;
                 if (lenBTRCoreDevTy == enBTRCoreHID) {
                     btrMgr_SetDisconnectStatusHoldOffTimer();
                 }
                 break;
             }
        }
    }


    if (lenBtrMgrResult != BTRMGR_RESULT_GENERIC_FAILURE) {
        /* Max 4 sec timeout - Polled at 1 second interval: Confirmed 2 times */
        unsigned int ui32sleepTimeOut = 1;
        unsigned int ui32confirmIdx = 2;
        
        do {
            unsigned int ui32sleepIdx = 2;

            do {
                sleep(ui32sleepTimeOut);
                lenBtrCoreRet = BTRCore_GetDeviceDisconnected(ghBTRCoreHdl, ahBTRMgrDevHdl, lenBTRCoreDevTy);
            } while ((lenBtrCoreRet != enBTRCoreSuccess) && (--ui32sleepIdx));
        } while (--ui32confirmIdx);

        //Inside this function there is a failure logs so, did not any failures logs here.
        btrMgr_GetDeviceDetails(ahBTRMgrDevHdl,&stDeviceInfo);

        if (lenBtrCoreRet != enBTRCoreSuccess) {
            //This is telemetry log. If we change this print,need to change and configure the telemetry string in xconf server.
            BTRMGRLOG_ERROR ("Failed to Disconnect from this device - Confirmed name,class,apperance,modalias: %s,%u,%u,v%04Xp%04Xd%04X\n",
            stDeviceInfo.pcDeviceName, stDeviceInfo.ui32DevClassBtSpec, stDeviceInfo.ui16DevAppearanceBleSpec,
            stDeviceInfo.ui32ModaliasVendorId, stDeviceInfo.ui32ModaliasProductId, stDeviceInfo.ui32ModaliasDeviceId);
            BTRMGRLOG_ERROR ("Disconnect failure device MAC %s\n", stDeviceInfo.pcDeviceAddress);

            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }
        else {
            //This is telemetry log. If we change this print,need to change and configure the telemetry string in xconf server.
            BTRMGRLOG_INFO ("Success Disconnect from this device - Confirmed name,class,apperance,modalias: %s,%u,%u,v%04Xp%04Xd%04X\n",
            stDeviceInfo.pcDeviceName, stDeviceInfo.ui32DevClassBtSpec, stDeviceInfo.ui16DevAppearanceBleSpec,
            stDeviceInfo.ui32ModaliasVendorId, stDeviceInfo.ui32ModaliasProductId, stDeviceInfo.ui32ModaliasDeviceId);
            BTRMGRLOG_INFO ("Disconnect success device MAC %s\n", stDeviceInfo.pcDeviceAddress);

            if (lenBTRCoreDevTy == enBTRCoreLE) {
                gIsLeDeviceConnected = 0;
            }
            else if ((lenBTRCoreDevTy == enBTRCoreSpeakers) || (lenBTRCoreDevTy == enBTRCoreHeadSet)) {
                btrMgr_SetDevConnected(ahBTRMgrDevHdl, 0);
                ghBTRMgrDevHdlLastConnected = 0;
            }
            else if ((lenBTRCoreDevTy == enBTRCoreMobileAudioIn) || (lenBTRCoreDevTy == enBTRCorePCAudioIn)) {
                btrMgr_SetDevConnected(ahBTRMgrDevHdl, 0);
                ghBTRMgrDevHdlLastConnected = 0;
            }
            else if (lenBTRCoreDevTy == enBTRCoreHID) {
                btrMgr_SetDevConnected(ahBTRMgrDevHdl, 0);
            }
            else {
                btrMgr_SetDevConnected(ahBTRMgrDevHdl, 0);
            }

            if ((lenBTRCoreDevTy == enBTRCoreSpeakers) || (lenBTRCoreDevTy == enBTRCoreHeadSet)) {
                btrMgr_SetLastConnectionStatus(aui8AdapterIdx, 0,ahBTRMgrDevHdl, BTRMGR_A2DP_SINK_PROFILE_ID);
            } else if ((lenBTRCoreDevTy == enBTRCoreMobileAudioIn) || (lenBTRCoreDevTy == enBTRCorePCAudioIn)) {
                btrMgr_SetLastConnectionStatus(aui8AdapterIdx, 0,ahBTRMgrDevHdl, BTRMGR_A2DP_SRC_PROFILE_ID);
            }
        }
    }

    if (lenBtrMgrDevOpType == BTRMGR_DEVICE_OP_TYPE_LE) {
        lenBtrMgrDevOpType = BTRMGR_DEVICE_OP_TYPE_UNKNOWN;
    }

    btrMgr_PostCheckDiscoveryStatus(aui8AdapterIdx, lenBtrMgrDevOpType);

    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_GetConnectedDevices (
    unsigned char                   aui8AdapterIdx,
    BTRMGR_ConnectedDevicesList_t*  pConnectedDevices
) {
    BTRMGR_Result_t              lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet                 lenBtrCoreRet   = enBTRCoreSuccess;
    stBTRCorePairedDevicesCount  lstBtrCoreListOfPDevices;
    stBTRCoreScannedDevicesCount lstBtrCoreListOfSDevices;
    unsigned char i = 0;
    unsigned char j = 0;
    unsigned char k = 0;
    gboolean lbDeviceFound = FALSE;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!pConnectedDevices)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    MEMSET_S(pConnectedDevices, sizeof(BTRMGR_ConnectedDevicesList_t), 0, sizeof(BTRMGR_ConnectedDevicesList_t));
    MEMSET_S(&lstBtrCoreListOfPDevices, sizeof(lstBtrCoreListOfPDevices), 0, sizeof(lstBtrCoreListOfPDevices));
    MEMSET_S(&lstBtrCoreListOfSDevices, sizeof(lstBtrCoreListOfSDevices), 0, sizeof(lstBtrCoreListOfSDevices));

    lenBtrCoreRet = BTRCore_GetListOfPairedDevices(ghBTRCoreHdl, &lstBtrCoreListOfPDevices);
    if (lenBtrCoreRet == enBTRCoreSuccess) {
        if (lstBtrCoreListOfPDevices.numberOfDevices) {
            BTRMGR_ConnectedDevice_t* lpstBtrMgrPDevice = NULL;

            for (i = 0; i < lstBtrCoreListOfPDevices.numberOfDevices; i++) {
                if ((lstBtrCoreListOfPDevices.devices[i].bDeviceConnected) && (pConnectedDevices->m_numOfDevices < BTRMGR_DEVICE_COUNT_MAX)) {
                   lpstBtrMgrPDevice = &pConnectedDevices->m_deviceProperty[pConnectedDevices->m_numOfDevices];
                   lpstBtrMgrPDevice->m_deviceType   = btrMgr_MapDeviceTypeFromCore(lstBtrCoreListOfPDevices.devices[i].enDeviceType);
                   lpstBtrMgrPDevice->m_deviceHandle = lstBtrCoreListOfPDevices.devices[i].tDeviceId;

                    if ((lpstBtrMgrPDevice->m_deviceType == BTRMGR_DEVICE_TYPE_WEARABLE_HEADSET)  ||
                        (lpstBtrMgrPDevice->m_deviceType == BTRMGR_DEVICE_TYPE_HANDSFREE)         ||
                        (lpstBtrMgrPDevice->m_deviceType == BTRMGR_DEVICE_TYPE_LOUDSPEAKER)       ||
                        (lpstBtrMgrPDevice->m_deviceType == BTRMGR_DEVICE_TYPE_HEADPHONES)        ||
                        (lpstBtrMgrPDevice->m_deviceType == BTRMGR_DEVICE_TYPE_PORTABLE_AUDIO)    ||
                        (lpstBtrMgrPDevice->m_deviceType == BTRMGR_DEVICE_TYPE_CAR_AUDIO)         ||
                        (lpstBtrMgrPDevice->m_deviceType == BTRMGR_DEVICE_TYPE_HIFI_AUDIO_DEVICE) ){

                        if (lpstBtrMgrPDevice->m_deviceHandle != ghBTRMgrDevHdlCurStreaming)
                            continue;
                    }

                   lpstBtrMgrPDevice->m_vendorID     = lstBtrCoreListOfPDevices.devices[i].ui32VendorId;
                   lpstBtrMgrPDevice->m_isConnected  = 1;
                   lpstBtrMgrPDevice->m_powerStatus  = BTRMGR_DEVICE_POWER_ACTIVE;

                   strncpy (lpstBtrMgrPDevice->m_name,          lstBtrCoreListOfPDevices.devices[i].pcDeviceName,   (BTRMGR_NAME_LEN_MAX - 1));
                   strncpy (lpstBtrMgrPDevice->m_deviceAddress, lstBtrCoreListOfPDevices.devices[i].pcDeviceAddress,(BTRMGR_NAME_LEN_MAX - 1));

                   lpstBtrMgrPDevice->m_ui32DevClassBtSpec = lstBtrCoreListOfPDevices.devices[i].ui32DevClassBtSpec;
                   lpstBtrMgrPDevice->m_ui16DevAppearanceBleSpec = lstBtrCoreListOfPDevices.devices[i].ui16DevAppearanceBleSpec;

                   lpstBtrMgrPDevice->m_serviceInfo.m_numOfService = lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.numberOfService;
                   for (j = 0; j < lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.numberOfService; j++) {
                       lpstBtrMgrPDevice->m_serviceInfo.m_profileInfo[j].m_uuid = lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.profile[j].uuid_value;
                       strncpy (lpstBtrMgrPDevice->m_serviceInfo.m_profileInfo[j].m_profile, lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.profile[j].profile_name, BTRMGR_NAME_LEN_MAX -1);
                       lpstBtrMgrPDevice->m_serviceInfo.m_profileInfo[j].m_profile[BTRMGR_NAME_LEN_MAX -1] = '\0';   ///CID:136654 - Buffer size warning
                   }

                   pConnectedDevices->m_numOfDevices++;
                   BTRMGRLOG_TRACE ("Successfully obtained the connected device information from paried list\n");
                }
            }
        }
        else {
            BTRMGRLOG_WARN("No Device in paired list\n");
        }
    }

    lenBtrCoreRet  = BTRCore_GetListOfScannedDevices (ghBTRCoreHdl, &lstBtrCoreListOfSDevices);
    if (lenBtrCoreRet == enBTRCoreSuccess) {
        if (lstBtrCoreListOfSDevices.numberOfDevices) {
            BTRMGR_ConnectedDevice_t* lpstBtrMgrSDevice =  NULL;

            for (i = 0; i < lstBtrCoreListOfSDevices.numberOfDevices; i++) {
                if ((lstBtrCoreListOfSDevices.devices[i].bDeviceConnected) && (pConnectedDevices->m_numOfDevices < BTRMGR_DEVICE_COUNT_MAX)) {
                    for (k = 0; k < pConnectedDevices->m_numOfDevices; k++ ) {
                        if (pConnectedDevices->m_deviceProperty[k].m_deviceHandle == lstBtrCoreListOfSDevices.devices[i].tDeviceId) {
                            //Device already added to connectedDevice list from previous pairedDevices list, So skipping to add again from scanned list
                            lbDeviceFound = TRUE;
                            break; 
                        }
                    }
                    if ( !lbDeviceFound ) {
                        lpstBtrMgrSDevice = &pConnectedDevices->m_deviceProperty[pConnectedDevices->m_numOfDevices];
                        lpstBtrMgrSDevice->m_deviceType   = btrMgr_MapDeviceTypeFromCore(lstBtrCoreListOfSDevices.devices[i].enDeviceType);
                        lpstBtrMgrSDevice->m_deviceHandle = lstBtrCoreListOfSDevices.devices[i].tDeviceId;
                        lpstBtrMgrSDevice->m_isConnected  = 1;
                        lpstBtrMgrSDevice->m_vendorID     = lstBtrCoreListOfSDevices.devices[i].ui32VendorId;
                        lpstBtrMgrSDevice->m_powerStatus  = BTRMGR_DEVICE_POWER_ACTIVE;

                        if ((lpstBtrMgrSDevice->m_deviceType == BTRMGR_DEVICE_TYPE_WEARABLE_HEADSET)  ||
                        (lpstBtrMgrSDevice->m_deviceType == BTRMGR_DEVICE_TYPE_HANDSFREE)         ||
                        (lpstBtrMgrSDevice->m_deviceType == BTRMGR_DEVICE_TYPE_LOUDSPEAKER)       ||
                        (lpstBtrMgrSDevice->m_deviceType == BTRMGR_DEVICE_TYPE_HEADPHONES)        ||
                        (lpstBtrMgrSDevice->m_deviceType == BTRMGR_DEVICE_TYPE_PORTABLE_AUDIO)    ||
                        (lpstBtrMgrSDevice->m_deviceType == BTRMGR_DEVICE_TYPE_CAR_AUDIO)         ||
                        (lpstBtrMgrSDevice->m_deviceType == BTRMGR_DEVICE_TYPE_HIFI_AUDIO_DEVICE) ){

                            if (lpstBtrMgrSDevice->m_deviceHandle != ghBTRMgrDevHdlCurStreaming)
                            {
                                lbDeviceFound = FALSE;
                                continue;
                            }
                        }

                        strncpy (lpstBtrMgrSDevice->m_name,          lstBtrCoreListOfSDevices.devices[i].pcDeviceName,   (BTRMGR_NAME_LEN_MAX - 1));
                        strncpy (lpstBtrMgrSDevice->m_deviceAddress, lstBtrCoreListOfSDevices.devices[i].pcDeviceAddress,(BTRMGR_NAME_LEN_MAX - 1));

                        lpstBtrMgrSDevice->m_ui32DevClassBtSpec = lstBtrCoreListOfSDevices.devices[i].ui32DevClassBtSpec;
                        lpstBtrMgrSDevice->m_ui16DevAppearanceBleSpec = lstBtrCoreListOfSDevices.devices[i].ui16DevAppearanceBleSpec;

                        lpstBtrMgrSDevice->m_serviceInfo.m_numOfService = lstBtrCoreListOfSDevices.devices[i].stDeviceProfile.numberOfService;
                        for (j = 0; j < lstBtrCoreListOfSDevices.devices[i].stDeviceProfile.numberOfService; j++) {
                            lpstBtrMgrSDevice->m_serviceInfo.m_profileInfo[j].m_uuid = lstBtrCoreListOfSDevices.devices[i].stDeviceProfile.profile[j].uuid_value;
                            strncpy (lpstBtrMgrSDevice->m_serviceInfo.m_profileInfo[j].m_profile, lstBtrCoreListOfSDevices.devices[i].stDeviceProfile.profile[j].profile_name, BTRMGR_NAME_LEN_MAX -1);
                            lpstBtrMgrSDevice->m_serviceInfo.m_profileInfo[j].m_profile[BTRMGR_NAME_LEN_MAX -1] = '\0';
                        }

                        pConnectedDevices->m_numOfDevices++;
                        BTRMGRLOG_INFO ("Successfully obtained the connected device information from scanned list\n");
                    }
                    lbDeviceFound = FALSE;
                }
            }
        }
        else {
            BTRMGRLOG_WARN("No Device in scan list\n");
        }
    }

    if (enBTRCoreSuccess != lenBtrCoreRet) {
        BTRMGRLOG_ERROR ("Failed to get connected device information\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_GetDeviceProperties (
    unsigned char               aui8AdapterIdx,
    BTRMgrDeviceHandle          ahBTRMgrDevHdl,
    BTRMGR_DevicesProperty_t*   pDeviceProperty
) {
    BTRMGR_Result_t                 lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet                    lenBtrCoreRet   = enBTRCoreSuccess;
    stBTRCorePairedDevicesCount     lstBtrCoreListOfPDevices;
    stBTRCoreScannedDevicesCount    lstBtrCoreListOfSDevices;
    unsigned char                   isFound = 0;
    int                             i = 0;
    int                             j = 0;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!pDeviceProperty) || (!ahBTRMgrDevHdl)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    /* Reset the values to 0 */
    MEMSET_S(&lstBtrCoreListOfPDevices, sizeof(lstBtrCoreListOfPDevices), 0, sizeof(lstBtrCoreListOfPDevices));
    MEMSET_S(&lstBtrCoreListOfSDevices, sizeof(lstBtrCoreListOfSDevices), 0, sizeof(lstBtrCoreListOfSDevices));
    MEMSET_S(pDeviceProperty, sizeof(BTRMGR_DevicesProperty_t), 0, sizeof(BTRMGR_DevicesProperty_t));

    lenBtrCoreRet = BTRCore_GetListOfPairedDevices(ghBTRCoreHdl, &lstBtrCoreListOfPDevices);
    if (lenBtrCoreRet == enBTRCoreSuccess) {
        if (lstBtrCoreListOfPDevices.numberOfDevices) {

            for (i = 0; i < lstBtrCoreListOfPDevices.numberOfDevices; i++) {
                if (ahBTRMgrDevHdl == lstBtrCoreListOfPDevices.devices[i].tDeviceId) {
                    pDeviceProperty->m_deviceHandle      = lstBtrCoreListOfPDevices.devices[i].tDeviceId;
                    pDeviceProperty->m_deviceType        = btrMgr_MapDeviceTypeFromCore(lstBtrCoreListOfPDevices.devices[i].enDeviceType);
                    pDeviceProperty->m_vendorID          = lstBtrCoreListOfPDevices.devices[i].ui32VendorId;

                    if ((pDeviceProperty->m_deviceType == BTRMGR_DEVICE_TYPE_TILE) ||
                        (pDeviceProperty->m_deviceType == BTRMGR_DEVICE_TYPE_XBB)) {
                        pDeviceProperty->m_isLowEnergyDevice = 1;
                    } else {
                        pDeviceProperty->m_isLowEnergyDevice = 0;
                    }

                    pDeviceProperty->m_isPaired          = 1;
                    //CID 342778: String not null terminated (STRING_NULL)
                    strncpy(pDeviceProperty->m_name,          lstBtrCoreListOfPDevices.devices[i].pcDeviceName,    (BTRMGR_NAME_LEN_MAX - 1));
                    pDeviceProperty->m_name[BTRMGR_NAME_LEN_MAX - 1] = '\0';
                    strncpy(pDeviceProperty->m_deviceAddress, lstBtrCoreListOfPDevices.devices[i].pcDeviceAddress, (BTRMGR_NAME_LEN_MAX - 1));
                    pDeviceProperty->m_deviceAddress[BTRMGR_NAME_LEN_MAX - 1] = '\0';
                    pDeviceProperty->m_serviceInfo.m_numOfService = lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.numberOfService;
                    for (j = 0; j < lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.numberOfService; j++) {
                        BTRMGRLOG_TRACE ("Profile ID = %d; Profile Name = %s \n", lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.profile[j].uuid_value,
                                                                                                   lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.profile[j].profile_name);
                        pDeviceProperty->m_serviceInfo.m_profileInfo[j].m_uuid = lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.profile[j].uuid_value;
                        strncpy (pDeviceProperty->m_serviceInfo.m_profileInfo[j].m_profile, lstBtrCoreListOfPDevices.devices[i].stDeviceProfile.profile[j].profile_name, BTRMGR_NAME_LEN_MAX -1);
                        pDeviceProperty->m_serviceInfo.m_profileInfo[j].m_profile[BTRMGR_NAME_LEN_MAX -1] = '\0';  //CID:136651 - Buffer size warning
                    }

                    if (lstBtrCoreListOfPDevices.devices[i].bDeviceConnected) {
                        pDeviceProperty->m_isConnected = 1;
                    }
                    pDeviceProperty->m_batteryLevel = lstBtrCoreListOfPDevices.devices[i].ui8batteryLevel;
                  
                    if (lstBtrCoreListOfPDevices.devices[i].ui32ModaliasProductId || lstBtrCoreListOfPDevices.devices[i].ui32ModaliasVendorId || lstBtrCoreListOfPDevices.devices[i].ui32ModaliasDeviceId)
                    {
                        snprintf(pDeviceProperty->m_modalias, BTRMGR_NAME_LEN_MAX/2 - 1, "v:%04X, p:%04X, d:%04X", lstBtrCoreListOfPDevices.devices[i].ui32ModaliasVendorId, lstBtrCoreListOfPDevices.devices[i].ui32ModaliasProductId, lstBtrCoreListOfPDevices.devices[i].ui32ModaliasDeviceId);
                    }
                    if (lstBtrCoreListOfPDevices.devices[i].ui32ModaliasDeviceId)
                    {
                        //create displayable firmware string out of modalias device ID
                        if (lstBtrCoreListOfPDevices.devices[i].ui32ModaliasDeviceId & 0xF000)
                        {
                            snprintf(pDeviceProperty->m_firmwareRevision, BTRMGR_NAME_LEN_MAX/2 - 1, "%d.%d.%d.%d", (lstBtrCoreListOfPDevices.devices[i].ui32ModaliasDeviceId & 0xF000) >> 12,
                                                    (lstBtrCoreListOfPDevices.devices[i].ui32ModaliasDeviceId & 0x0F00) >> 8,
                                                    (lstBtrCoreListOfPDevices.devices[i].ui32ModaliasDeviceId & 0x00F0) >> 4,
                                                    (lstBtrCoreListOfPDevices.devices[i].ui32ModaliasDeviceId & 0x000F));
                        }
                        else
                        {
                            snprintf(pDeviceProperty->m_firmwareRevision, BTRMGR_NAME_LEN_MAX/2 - 1, "%d.%d.%d", (lstBtrCoreListOfPDevices.devices[i].ui32ModaliasDeviceId & 0x0F00) >> 8,
                                                    (lstBtrCoreListOfPDevices.devices[i].ui32ModaliasDeviceId & 0x00F0) >> 4,
                                                    (lstBtrCoreListOfPDevices.devices[i].ui32ModaliasDeviceId & 0x000F));
                        }
                        BTRMGRLOG_INFO("Created version string: %s\n", pDeviceProperty->m_firmwareRevision);
                    }

                    isFound = 1;
                    break;
                }
            }
        }
        else {
            BTRMGRLOG_WARN("No Device is paired yet\n");
        }
    }


    if (isFound) {
        BTRMGRLOG_DEBUG("GetDeviceProperties - Paired Device\n");
        return lenBtrMgrResult;
    }


    lenBtrCoreRet  = BTRCore_GetListOfScannedDevices (ghBTRCoreHdl, &lstBtrCoreListOfSDevices);
    if (lenBtrCoreRet == enBTRCoreSuccess) {
        if (lstBtrCoreListOfSDevices.numberOfDevices) {

            for (i = 0; i < lstBtrCoreListOfSDevices.numberOfDevices; i++) {
                if (ahBTRMgrDevHdl == lstBtrCoreListOfSDevices.devices[i].tDeviceId) {
                    if (!isFound) {
                        pDeviceProperty->m_deviceHandle      = lstBtrCoreListOfSDevices.devices[i].tDeviceId;
                        pDeviceProperty->m_deviceType        = btrMgr_MapDeviceTypeFromCore(lstBtrCoreListOfSDevices.devices[i].enDeviceType);
                        pDeviceProperty->m_vendorID          = lstBtrCoreListOfSDevices.devices[i].ui32VendorId;

                         if ((pDeviceProperty->m_deviceType==BTRMGR_DEVICE_TYPE_TILE) ||
                            (pDeviceProperty->m_deviceType==BTRMGR_DEVICE_TYPE_XBB)) {
                            pDeviceProperty->m_isLowEnergyDevice = 1;
                        } else {
                            pDeviceProperty->m_isLowEnergyDevice = 0;
                        }

                        //CID 342778: String not null terminated (STRING_NULL)
                        strncpy(pDeviceProperty->m_name,          lstBtrCoreListOfSDevices.devices[i].pcDeviceName,    (BTRMGR_NAME_LEN_MAX - 1));
                        pDeviceProperty->m_name[BTRMGR_NAME_LEN_MAX - 1] = '\0';
                        strncpy(pDeviceProperty->m_deviceAddress, lstBtrCoreListOfSDevices.devices[i].pcDeviceAddress, (BTRMGR_NAME_LEN_MAX - 1));
                        pDeviceProperty->m_deviceAddress[BTRMGR_NAME_LEN_MAX - 1] = '\0';
                        pDeviceProperty->m_serviceInfo.m_numOfService = lstBtrCoreListOfSDevices.devices[i].stDeviceProfile.numberOfService;
                        for (j = 0; j < lstBtrCoreListOfSDevices.devices[i].stDeviceProfile.numberOfService; j++) {
                            BTRMGRLOG_TRACE ("Profile ID = %d; Profile Name = %s \n", lstBtrCoreListOfSDevices.devices[i].stDeviceProfile.profile[j].uuid_value,
                                                                                                       lstBtrCoreListOfSDevices.devices[i].stDeviceProfile.profile[j].profile_name);
                            pDeviceProperty->m_serviceInfo.m_profileInfo[j].m_uuid = lstBtrCoreListOfSDevices.devices[i].stDeviceProfile.profile[j].uuid_value;
                            strncpy (pDeviceProperty->m_serviceInfo.m_profileInfo[j].m_profile, lstBtrCoreListOfSDevices.devices[i].stDeviceProfile.profile[j].profile_name, BTRMGR_NAME_LEN_MAX -1);
                            pDeviceProperty->m_serviceInfo.m_profileInfo[j].m_profile[BTRMGR_NAME_LEN_MAX -1] = '\0';

                            if(0 != lstBtrCoreListOfSDevices.devices[i].stAdServiceData[j].len)
                            {
                                fprintf(stderr, "%d\t: %s - ServiceData for UUID : %s \n", __LINE__, __FUNCTION__, lstBtrCoreListOfSDevices.devices[i].stAdServiceData[j].pcUUIDs);
                                strncpy (pDeviceProperty->m_adServiceData[j].m_UUIDs, lstBtrCoreListOfSDevices.devices[i].stAdServiceData[j].pcUUIDs, (BTRMGR_UUID_STR_LEN_MAX - 1));
                                MEMCPY_S(pDeviceProperty->m_adServiceData[j].m_ServiceData,sizeof(pDeviceProperty->m_adServiceData[j].m_ServiceData), lstBtrCoreListOfSDevices.devices[i].stAdServiceData[j].pcData, lstBtrCoreListOfSDevices.devices[i].stAdServiceData[j].len);
                                pDeviceProperty->m_adServiceData[j].m_len = lstBtrCoreListOfSDevices.devices[i].stAdServiceData[j].len;

                                for (int k=0; k < pDeviceProperty->m_adServiceData[j].m_len; k++){
                                    fprintf(stderr, "%d\t: %s - ServiceData[%d] = [%x]\n ", __LINE__, __FUNCTION__, k, pDeviceProperty->m_adServiceData[j].m_ServiceData[k]);
                                }
                            }
                        }
                    }

                    pDeviceProperty->m_rssi = lstBtrCoreListOfSDevices.devices[i].i32RSSI;
                    pDeviceProperty->m_signalLevel = btrMgr_MapSignalStrengthToRSSI(lstBtrCoreListOfSDevices.devices[i].i32RSSI);

                    if (lstBtrCoreListOfSDevices.devices[i].bDeviceConnected) {
                       pDeviceProperty->m_isConnected = 1;
                    }
                    //this shouldn't be anything but 0, but included for completeness
                    pDeviceProperty->m_batteryLevel = lstBtrCoreListOfPDevices.devices[i].ui8batteryLevel;
                    isFound = 1;
                    break;
                }
            }
        }
        else {
            BTRMGRLOG_WARN("No Device in scan list\n");
        }
    }
    pDeviceProperty->m_signalLevel = btrMgr_MapSignalStrengthToRSSI (pDeviceProperty->m_rssi);
    
    if (!isFound) {
        BTRMGRLOG_ERROR ("Could not retrive info for this device\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    BTRMGRLOG_DEBUG("GetDeviceProperties - Scanned Device\n");
    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_GetDeviceBatteryLevel (
    unsigned char               aui8AdapterIdx,
    BTRMgrDeviceHandle          ahBTRMgrDevHdl,
    unsigned char *             pDeviceBatteryLevel
)
{
    enBTRCoreRet lenBtrCoreRet = enBTRCoreSuccess;
    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!pDeviceBatteryLevel) || (!ahBTRMgrDevHdl)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenBtrCoreRet = BTRCore_GetDeviceBatteryLevel(ghBTRCoreHdl, ahBTRMgrDevHdl, enBTRCoreUnknown, pDeviceBatteryLevel);
    if (enBTRCoreSuccess == lenBtrCoreRet)
    {
        return BTRMGR_RESULT_SUCCESS;
    }
    else
    {
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }
}
#ifndef LE_MODE
BTRMGR_Result_t
BTRMGR_ConnectGamepads_StartUp (
    unsigned char                   aui8AdapterIdx,
    BTRMGR_DeviceOperationType_t    aenBTRMgrDevConT
) {
    BTRMGR_Result_t           lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    int                       auth;
    stBTRCoreDevStatusCBInfo  stRecreatedEvent = { 0 };
    BTRMGR_EventMessage_t     lstEventMessage;

    char lcPowerState[BTRMGR_LE_STR_LEN_MAX] = {'\0'};
    BTRMGR_SysDiagChar_t lpcPowerString = BTRMGR_SYS_DIAG_POWERSTATE;

    if (eBTRMgrSuccess == BTRMGR_SD_GetData(ghBTRMgrSdHdl, lpcPowerString, lcPowerState)) {
        if(strncmp(lcPowerState, BTRMGR_SYS_DIAG_PWRST_ON, strlen(BTRMGR_SYS_DIAG_PWRST_ON) != 0))
        {
            BTRMGRLOG_ERROR("Power state is %s no need to try connecting device as it will connect once ON\n", lcPowerState);
            return BTRMGR_RESULT_GENERIC_FAILURE;
        }
        else
        {
            BTRMGRLOG_INFO("Power state is ON\n");
        }
    }
    else
    {
        BTRMGRLOG_ERROR("Could not get power state\n");
    }

    BTRMGR_GetPairedDevices (aui8AdapterIdx, &gListOfPairedDevices);
    for (int i = 0; i < gListOfPairedDevices.m_numOfDevices; i++)
    {
        stBTRCoreBTDevice stDeviceInfo = { 0 };
        btrMgr_GetDeviceDetails(gListOfPairedDevices.m_deviceProperty[i].m_deviceHandle, &stDeviceInfo);
        if (stDeviceInfo.bDeviceConnected
        && (gListOfPairedDevices.m_deviceProperty[i].m_deviceType == BTRMGR_DEVICE_TYPE_HID
        || gListOfPairedDevices.m_deviceProperty[i].m_deviceType ==BTRMGR_DEVICE_TYPE_HID_GAMEPAD)
        && (!btrMgr_IsDeviceRdkRcu(&gListOfPairedDevices.m_deviceProperty[i].m_serviceInfo, stDeviceInfo.ui16DevAppearanceBleSpec)))
        {
            strncpy(stRecreatedEvent.deviceName, stDeviceInfo.pcDeviceName, BTRMGR_NAME_LEN_MAX - 1);
            strncpy(stRecreatedEvent.deviceAddress, stDeviceInfo.pcDeviceAddress, BTRMGR_NAME_LEN_MAX - 1);
            stRecreatedEvent.deviceId = stDeviceInfo.tDeviceId;
            stRecreatedEvent.eDeviceClass = stDeviceInfo.enDeviceType;
            stRecreatedEvent.eDeviceType = stDeviceInfo.enDeviceType;
            stRecreatedEvent.isPaired = 1;
            stRecreatedEvent.ui32VendorId = stDeviceInfo.ui32ModaliasVendorId;
            stRecreatedEvent.ui32ProductId = stDeviceInfo.ui32ModaliasProductId;
            stRecreatedEvent.ui32DeviceId = stDeviceInfo.ui32ModaliasDeviceId;
            stRecreatedEvent.ui16DevAppearanceBleSpec = stDeviceInfo.ui16DevAppearanceBleSpec;
            stRecreatedEvent.eDevicePrevState = enBTRCoreDevStPaired;
            stRecreatedEvent.eDeviceCurrState = enBTRCoreDevStConnected;


            //recreate event that would have been received from the connectCb
            btrMgr_IncomingConnectionAuthentication(&stRecreatedEvent, &auth);

            if (!auth)
                continue;

            btrMgr_MapDevstatusInfoToEventInfo ((void *)&stRecreatedEvent, &lstEventMessage, BTRMGR_EVENT_DEVICE_FOUND);
            lstEventMessage.m_pairedDevice.m_deviceType = BTRMGR_DEVICE_TYPE_HID;
            btrMgr_SetDevConnected(lstEventMessage.m_pairedDevice.m_deviceHandle, 1);
            BTRCore_newBatteryLevelDevice(ghBTRCoreHdl);

            BTRMGRLOG_DEBUG("Posting Device Found Event ..\n");

            if (gbGamepadStandbyMode && ((lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HID) ||
                (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HID_GAMEPAD)))
            {
                BTRMGRLOG_WARN("Device is in standby mode, we won't post to the upper layers if a device is found\n");
                continue;
            }
            if (gfpcBBTRMgrEventOut) {
                gfpcBBTRMgrEventOut(lstEventMessage);
            }
        }
    }

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_StartAudioStreamingOut_StartUp (
    unsigned char                   aui8AdapterIdx,
    BTRMGR_DeviceOperationType_t    aenBTRMgrDevConT
) {
    char                    lui8adapterAddr[BD_NAME_LEN] = {'\0'};
    int                     i32ProfileIdx = 0;
    int                     i32DeviceIdx = 0;
    int                     numOfProfiles = 0;
    int                     deviceCount = 0;
    int                     isConnected = 0;
    int                     lastConnected = 0;

    BTRMGR_PersistentData_t lstPersistentData;
    BTRMgrDeviceHandle      lDeviceHandle;
    BTRMGR_Result_t         lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
#ifdef AUTO_CONNECT_ENABLED    
    enBTRCoreRet            lenBtrCoreRet = enBTRCoreSuccess;
    int                     api32ConnInAuthResp = 1;
    gboolean                lbSecondAttempt = false;

    if (gIsAudOutStartupInProgress == BTRMGR_STARTUP_AUD_RETRY)
    {
        lbSecondAttempt = true;
    }
#endif //AUTO_CONNECT_ENABLED

    //get power state as we do not need to connect if the power state is not ON and power callback will always be registered
    char lcPowerState[BTRMGR_LE_STR_LEN_MAX] = {'\0'};
    BTRMGR_SysDiagChar_t lpcPowerString = BTRMGR_SYS_DIAG_POWERSTATE;

    if (eBTRMgrSuccess == BTRMGR_SD_GetData(ghBTRMgrSdHdl, lpcPowerString, lcPowerState)) {
        if(strncmp(lcPowerState, BTRMGR_SYS_DIAG_PWRST_ON, strlen(BTRMGR_SYS_DIAG_PWRST_ON) != 0))
        {
            BTRMGRLOG_ERROR("Power state is %s no need to try connecting device as it will connect once ON\n", lcPowerState);
            return BTRMGR_RESULT_GENERIC_FAILURE;
        }
        else
        {
            BTRMGRLOG_INFO("Power state is ON\n");
        }
    }
    else
    {
        BTRMGRLOG_ERROR("Could not get power state\n");
    }

    gIsAudOutStartupInProgress = BTRMGR_STARTUP_AUD_UNKNOWN;
    if (BTRMgr_PI_GetAllProfiles(ghBTRMgrPiHdl, &lstPersistentData) == eBTRMgrFailure) {
        btrMgr_AddPersistentEntry (aui8AdapterIdx, 0, BTRMGR_A2DP_SINK_PROFILE_ID, isConnected);
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }


    BTRMGRLOG_INFO ("Successfully get all profiles\n");
    BTRCore_GetAdapterAddr(ghBTRCoreHdl, aui8AdapterIdx, lui8adapterAddr);

    if (strcmp(lstPersistentData.adapterId, lui8adapterAddr) == 0) {
        gIsAudOutStartupInProgress = BTRMGR_STARTUP_AUD_INPROGRESS;
        numOfProfiles = lstPersistentData.numOfProfiles;

        BTRMGRLOG_DEBUG ("Adapter matches = %s\n", lui8adapterAddr);
        BTRMGRLOG_DEBUG ("Number of Profiles = %d\n", numOfProfiles);

        for (i32ProfileIdx = 0; i32ProfileIdx < numOfProfiles; i32ProfileIdx++) {
            deviceCount = lstPersistentData.profileList[i32ProfileIdx].numOfDevices;

            for (i32DeviceIdx = 0; i32DeviceIdx < deviceCount ; i32DeviceIdx++) {
                lDeviceHandle   = lstPersistentData.profileList[i32ProfileIdx].deviceList[i32DeviceIdx].deviceId;
                isConnected     = lstPersistentData.profileList[i32ProfileIdx].deviceList[i32DeviceIdx].isConnected;
                lastConnected     = lstPersistentData.profileList[i32ProfileIdx].deviceList[i32DeviceIdx].lastConnected;

                if ((lastConnected || isConnected) && lDeviceHandle) {
                    if(strcmp(lstPersistentData.profileList[i32ProfileIdx].profileId, BTRMGR_A2DP_SINK_PROFILE_ID) == 0) {
                        char                   lPropValue[BTRMGR_LE_STR_LEN_MAX] = {'\0'};
                        BTRMGR_SysDiagChar_t   lenDiagElement = BTRMGR_SYS_DIAG_POWERSTATE;

                        if (eBTRMgrSuccess != BTRMGR_SD_GetData(ghBTRMgrSdHdl, lenDiagElement, lPropValue)) {
                            gIsAudOutStartupInProgress = BTRMGR_STARTUP_AUD_UNKNOWN;
                            BTRMGRLOG_ERROR("Could not get diagnostic data\n");
                            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
                        }
#ifdef AUTO_CONNECT_ENABLED
                        //Before automatically starting audio, we should check if the device is connectable and then ask the upper layers
                        unsigned int ui32sleepTimeOut = 2;
                        unsigned int ui32confirmIdx = 2;

                        do {
                            unsigned int ui32sleepIdx = 1;
                            do {
                                sleep(ui32sleepTimeOut);
                                lenBtrCoreRet = BTRCore_IsDeviceConnectable(ghBTRCoreHdl, lDeviceHandle);
                            } while ((lenBtrCoreRet != enBTRCoreSuccess) && (--ui32sleepIdx));
                        } while ((lenBtrCoreRet != enBTRCoreSuccess) && (--ui32confirmIdx));

                        if (lenBtrCoreRet != enBTRCoreSuccess)
                            continue;

                        if ((btrMgr_GetDevPaired(lDeviceHandle))
                            && ghBTRMgrDevHdlCurStreaming == 0 ) {
                            if (lDeviceHandle == ghBTRMgrDevHdlStreamStartUp) {
                                BTRMGRLOG_INFO("Pairing/Connection in progress for this audio device, so accepting the connection\n");
                            }

                            stBTRCoreBTDevice stDeviceInfo;
                            MEMSET_S(&stDeviceInfo, sizeof(stBTRCoreBTDevice), 0, sizeof(stBTRCoreBTDevice));
                            if (eBTRMgrSuccess != btrMgr_GetDeviceDetails(lDeviceHandle,&stDeviceInfo)) {
                                BTRMGRLOG_ERROR("Could not get device details\n");
                                continue;
                            }
                            BTRMGR_EventMessage_t lstEventMessage;
                            MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));
                            lstEventMessage.m_eventType                            = BTRMGR_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST;
                            lstEventMessage.m_externalDevice.m_deviceHandle        = lDeviceHandle;
                            lstEventMessage.m_externalDevice.m_deviceType          = btrMgr_MapDeviceTypeFromCore(stDeviceInfo.enDeviceType);
                            lstEventMessage.m_externalDevice.m_vendorID            = stDeviceInfo.ui32ModaliasVendorId;
                            lstEventMessage.m_externalDevice.m_isLowEnergyDevice   = 0;
                            strncpy(lstEventMessage.m_externalDevice.m_name, stDeviceInfo.pcDeviceName, BTRMGR_NAME_LEN_MAX - 1);
                            strncpy(lstEventMessage.m_externalDevice.m_deviceAddress, stDeviceInfo.pcDeviceAddress, BTRMGR_NAME_LEN_MAX - 1);

                            //TODO: Check if XRE wants to bring up a Pop-up or Respond
                            if (gfpcBBTRMgrEventOut) {
                                gfpcBBTRMgrEventOut(lstEventMessage);     /* Post a callback */
                            }

                            {   /* Max 200msec timeout - Polled at 50ms second interval */
                                unsigned int ui32sleepIdx = 4;

                                do {
                                    usleep(50000);
                                } while ((gEventRespReceived == 0) && (--ui32sleepIdx));

                            }
                            if (gEventRespReceived == 0) {
                                if (!lbSecondAttempt)
                                {
                                    BTRMGRLOG_INFO("External connection response not received from UI Audio Out device - it is possible UI is not up yet, try again if device is there in 40 seconds\n");
                                    btrMgr_SetAutoconnectOnStartUpTimer();
                                    return BTRMGR_RESULT_SUCCESS;
                                }
                                else
                                {
                                    BTRMGRLOG_WARN("External connection response not received from UI Audio Out device for a second time\n");
                                    api32ConnInAuthResp = 0;
                                }
                            } else {
                                api32ConnInAuthResp = gAcceptConnection;
                                if (gAcceptConnection) {
                                    BTRMGRLOG_INFO ("Incoming Connection accepted for Audio Out device based on the response from UI\n");
                                } else {
                                    BTRMGRLOG_INFO ("Incoming Connection rejected for Audio Out device based on the response from UI\n");
                                }
                                gEventRespReceived = 0;
                            }
                        }
                        else {
                            BTRMGRLOG_ERROR ("Incoming Connection denied\n");
                            api32ConnInAuthResp = 0;
                        }
                        //expect UI to connect to device to start stream
                        if(api32ConnInAuthResp)
                            gIsAudOutStartupInProgress = BTRMGR_STARTUP_AUD_COMPLETED;
                        else
                            gIsAudOutStartupInProgress = BTRMGR_STARTUP_AUD_SKIPPED;
                    }
#else //!AUTO_CONNECT_ENABLED

                        if ((lenBtrMgrResult == BTRMGR_RESULT_GENERIC_FAILURE) ||
                           (!strncmp(lPropValue, BTRMGR_SYS_DIAG_PWRST_ON, strlen(BTRMGR_SYS_DIAG_PWRST_ON)) &&
                           (gIsAudOutStartupInProgress != BTRMGR_STARTUP_AUD_COMPLETED))) {
                            BTRMGRLOG_INFO ("Streaming to Device  = %lld\n", lDeviceHandle);
                            if (btrMgr_StartAudioStreamingOut(0, lDeviceHandle, aenBTRMgrDevConT, 1, 1, 1) != eBTRMgrSuccess) {
                                BTRMGRLOG_ERROR ("btrMgr_StartAudioStreamingOut - Failure\n");
                                lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
                            }
                            ghBTRMgrDevHdlLastConnected = lDeviceHandle;
                            gIsAudOutStartupInProgress = BTRMGR_STARTUP_AUD_COMPLETED;
                        }
                        else {
                            gIsAudOutStartupInProgress = BTRMGR_STARTUP_AUD_SKIPPED;
                        }
                    }
#endif //!AUTO_CONNECT_ENABLED
                    if (lastConnected)
                    {
                        ghBTRMgrDevHdlLastConnected = lDeviceHandle;
                    }
                }
            }
        }

        if (gIsAudOutStartupInProgress == BTRMGR_STARTUP_AUD_INPROGRESS)
            gIsAudOutStartupInProgress = BTRMGR_STARTUP_AUD_UNKNOWN;
    }

    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_StartAudioStreamingOut (
    unsigned char                   aui8AdapterIdx,
    BTRMgrDeviceHandle              ahBTRMgrDevHdl,
    BTRMGR_DeviceOperationType_t    streamOutPref
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }
    else if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!ahBTRMgrDevHdl)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    if (btrMgr_StartAudioStreamingOut(aui8AdapterIdx, ahBTRMgrDevHdl, streamOutPref, 0, 0, 0) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("Failure\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_StopAudioStreamingOut (
    unsigned char       aui8AdapterIdx,
    BTRMgrDeviceHandle  ahBTRMgrDevHdl
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    eBTRMgrRet      lenBtrMgrRet    = eBTRMgrSuccess;


    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


   if (ghBTRMgrDevHdlCurStreaming != ahBTRMgrDevHdl || ghBTRMGRDevHdlTestStreaming == ahBTRMgrDevHdl) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    if ((lenBtrMgrRet = btrMgr_StopCastingAudio()) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("btrMgr_StopCastingAudio = %d\n", lenBtrMgrRet);
    }

    if (btrMgr_IsDevConnected(ahBTRMgrDevHdl) == 1) {
       BTRCore_ReleaseDeviceDataPath (ghBTRCoreHdl, ghBTRMgrDevHdlCurStreaming, enBTRCoreSpeakers);
    }

    ghBTRMgrDevHdlCurStreaming = 0;

    if (gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo) {
        free (gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo);
        gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo = NULL;
    }

    /* We had Reset the ghBTRMgrDevHdlCurStreaming to avoid recursion/looping; so no worries */
    lenBtrMgrResult = BTRMGR_DisconnectFromDevice(aui8AdapterIdx, ahBTRMgrDevHdl);

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_IsAudioStreamingOut (
    unsigned char   aui8AdapterIdx,
    unsigned char*  pStreamingStatus
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;


    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    if(!pStreamingStatus) {
        lenBtrMgrResult = BTRMGR_RESULT_INVALID_INPUT;
        BTRMGRLOG_ERROR ("Input is invalid\n");
    }
    else {
        if (ghBTRMgrDevHdlCurStreaming)
            *pStreamingStatus = 1;
        else
            *pStreamingStatus = 0;

        BTRMGRLOG_INFO ("BTRMGR_IsAudioStreamingOut: Returned status Successfully\n");
    }

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_SetAudioStreamingOutType (
    unsigned char           aui8AdapterIdx,
    BTRMGR_StreamOut_Type_t aenCurrentSoType
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    BTRMGRLOG_INFO ("Audio output - Stored %d - Switching to %d\n", gstBTRMgrStreamingInfo.tBTRMgrSoType, aenCurrentSoType);
    if (gstBTRMgrStreamingInfo.tBTRMgrSoType != aenCurrentSoType) {
        unsigned char ui8StreamingStatus = 0;
        BTRMGR_StreamOut_Type_t lenCurrentSoType = gstBTRMgrStreamingInfo.tBTRMgrSoType;

        gstBTRMgrStreamingInfo.tBTRMgrSoType = aenCurrentSoType;
        if ((BTRMGR_RESULT_SUCCESS == BTRMGR_IsAudioStreamingOut(aui8AdapterIdx, &ui8StreamingStatus)) && ui8StreamingStatus) {
            enBTRCoreRet            lenBtrCoreRet   = enBTRCoreSuccess;
            enBTRCoreDeviceType     lenBTRCoreDevTy = enBTRCoreUnknown;
            enBTRCoreDeviceClass    lenBTRCoreDevCl = enBTRCore_DC_Unknown;

            BTRMGRLOG_WARN ("Its already streaming. lets Switch\n");

            lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ghBTRMgrDevHdlCurStreaming, &lenBTRCoreDevTy, &lenBTRCoreDevCl);
            BTRMGRLOG_DEBUG ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBTRCoreDevTy, lenBTRCoreDevCl);

            if ((lenBTRCoreDevTy == enBTRCoreSpeakers) || (lenBTRCoreDevTy == enBTRCoreHeadSet)) {
                /* Streaming-Out is happening; Lets switch it */
                if (btrMgr_SwitchCastingAudio_AC(aenCurrentSoType) != eBTRMgrSuccess) {
                    gstBTRMgrStreamingInfo.tBTRMgrSoType = lenCurrentSoType;
                    BTRMGRLOG_ERROR ("This device is being Connected n Playing. Failed to switch to %d\n", aenCurrentSoType);
                    BTRMGRLOG_ERROR ("Failed to switch streaming on the current device. Streaming %d\n", gstBTRMgrStreamingInfo.tBTRMgrSoType);
                    if (btrMgr_SwitchCastingAudio_AC(gstBTRMgrStreamingInfo.tBTRMgrSoType) == eBTRMgrSuccess) {
                        BTRMGRLOG_WARN ("Streaming on the current device. Streaming %d\n", gstBTRMgrStreamingInfo.tBTRMgrSoType);
                    }

                    return BTRMGR_RESULT_GENERIC_FAILURE;
                }
            }
        }
    }

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_StartAudioStreamingIn (
    unsigned char                   aui8AdapterIdx,
    BTRMgrDeviceHandle              ahBTRMgrDevHdl,
    BTRMGR_DeviceOperationType_t    connectAs
) {
    BTRMGR_Result_t             lenBtrMgrResult   = BTRMGR_RESULT_SUCCESS;
    BTRMGR_DeviceType_t         lenBtrMgrDevType  = BTRMGR_DEVICE_TYPE_UNKNOWN;
    eBTRMgrRet                  lenBtrMgrRet      = eBTRMgrSuccess;
    enBTRCoreRet                lenBtrCoreRet     = enBTRCoreSuccess;
    enBTRCoreDeviceType         lenBtrCoreDevType = enBTRCoreUnknown;
    stBTRCorePairedDevicesCount listOfPDevices;
    int                         i = 0;
    int                         i32IsFound = 0;
    int                         i32DeviceFD = 0;
    int                         i32DeviceReadMTU = 0;
    int                         i32DeviceWriteMTU = 0;
    unsigned int                ui32deviceDelay = 0xFFFFu;
    eBTRCoreDevMediaType        lenBtrCoreDevInMType = eBTRCoreDevMediaTypeUnknown;
    void*                       lpstBtrCoreDevInMCodecInfo = NULL;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!ahBTRMgrDevHdl)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (ghBTRMgrDevHdlCurStreaming == ahBTRMgrDevHdl) {

        if (gMediaPlaybackStPrev == BTRMGR_EVENT_MEDIA_TRACK_STOPPED) {
            BTRMGRLOG_DEBUG ("Starting Media Playback.\n");
            lenBtrMgrResult = BTRMGR_MediaControl (aui8AdapterIdx, ahBTRMgrDevHdl, BTRMGR_MEDIA_CTRL_PLAY);
        }
        else if (gMediaPlaybackStPrev == BTRMGR_EVENT_MEDIA_TRACK_PAUSED) {
            BTRMGRLOG_DEBUG ("Resuming Media Playback.\n");
            lenBtrMgrResult = BTRMGR_MediaControl (aui8AdapterIdx, ahBTRMgrDevHdl, BTRMGR_MEDIA_CTRL_PLAY);
        }
        else {
            BTRMGRLOG_WARN ("Its already streaming-in in this device.. Check the volume :)\n");
        }

        if (lenBtrMgrResult != BTRMGR_RESULT_SUCCESS) {
            BTRMGRLOG_ERROR ("Failed to perform Media Control!\n");
        }

        return lenBtrMgrResult;
    }


    if ((ghBTRMgrDevHdlCurStreaming != 0) && (ghBTRMgrDevHdlCurStreaming != ahBTRMgrDevHdl)) {
        enBTRCoreDeviceType     lenBTRCoreDevTy = enBTRCoreUnknown;
        enBTRCoreDeviceClass    lenBTRCoreDevCl = enBTRCore_DC_Unknown;

        BTRMGRLOG_WARN ("Its already streaming in. lets stop this and start on other device \n");

        lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ghBTRMgrDevHdlCurStreaming, &lenBTRCoreDevTy, &lenBTRCoreDevCl);
        BTRMGRLOG_DEBUG ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBTRCoreDevTy, lenBTRCoreDevCl);

        if ((lenBTRCoreDevTy == enBTRCoreSpeakers) || (lenBTRCoreDevTy == enBTRCoreHeadSet)) {
            /* Streaming-Out is happening; stop it */
            if ((lenBtrMgrResult = BTRMGR_StopAudioStreamingOut(aui8AdapterIdx, ghBTRMgrDevHdlCurStreaming)) != BTRMGR_RESULT_SUCCESS) {
                BTRMGRLOG_ERROR ("This device is being Connected n Playing. Failed to stop Playback.-Out\n");
                BTRMGRLOG_ERROR ("Failed to stop streaming at the current device..\n");
                return lenBtrMgrResult;
            }
        }
        else if ((lenBTRCoreDevTy == enBTRCoreMobileAudioIn) || (lenBTRCoreDevTy == enBTRCorePCAudioIn)) {
            /* Streaming-In is happening; stop it */
            if ((lenBtrMgrResult = BTRMGR_StopAudioStreamingIn(aui8AdapterIdx, ghBTRMgrDevHdlCurStreaming)) != BTRMGR_RESULT_SUCCESS) {
                BTRMGRLOG_ERROR ("This device is being Connected n Playing. Failed to stop Playback.-In\n");
                BTRMGRLOG_ERROR ("Failed to stop streaming at the current device..\n");
                return lenBtrMgrResult;
            }
        }
    }

    /* Check whether the device is in the paired list */
    MEMSET_S(&listOfPDevices, sizeof(listOfPDevices), 0, sizeof(listOfPDevices));
    if (BTRCore_GetListOfPairedDevices(ghBTRCoreHdl, &listOfPDevices) != enBTRCoreSuccess) {
        BTRMGRLOG_ERROR ("Failed to get the paired devices list\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    if (!listOfPDevices.numberOfDevices) {
        BTRMGRLOG_ERROR ("No device is paired yet; Will not be able to play at this moment\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }


    for (i = 0; i < listOfPDevices.numberOfDevices; i++) {
        if (ahBTRMgrDevHdl == listOfPDevices.devices[i].tDeviceId) {
            i32IsFound = 1;
            break;
        }
    }

    if (!i32IsFound) {
        BTRMGRLOG_ERROR ("Failed to find this device in the paired devices list\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }


    lenBtrMgrDevType = btrMgr_MapDeviceTypeFromCore(listOfPDevices.devices[i].enDeviceType);
    if (lenBtrMgrDevType == BTRMGR_DEVICE_TYPE_SMARTPHONE) {
       lenBtrCoreDevType = enBTRCoreMobileAudioIn;
    }
    else if (lenBtrMgrDevType == BTRMGR_DEVICE_TYPE_TABLET) {
       lenBtrCoreDevType = enBTRCorePCAudioIn; 
    }
    if (!gIsAudioInEnabled && ((lenBtrCoreDevType == enBTRCoreMobileAudioIn) || (lenBtrCoreDevType == enBTRCorePCAudioIn))) {
        BTRMGRLOG_WARN ("StreamingIn Rejected - BTR AudioIn is currently Disabled!\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }


    if (!listOfPDevices.devices[i].bDeviceConnected || (ghBTRMgrDevHdlCurStreaming != listOfPDevices.devices[i].tDeviceId)) {
        if ((lenBtrMgrRet = btrMgr_ConnectToDevice(aui8AdapterIdx, listOfPDevices.devices[i].tDeviceId, connectAs, 0, 1)) == eBTRMgrSuccess) {
            gMediaPlaybackStPrev = BTRMGR_EVENT_MEDIA_TRACK_STOPPED;    //TODO: Bad Bad Bad way of doing this
        }
        else {
            BTRMGRLOG_ERROR ("Failure\n");
            return BTRMGR_RESULT_GENERIC_FAILURE;
        }
    }


    if (gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo) {
        free (gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo);
        gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo = NULL;
    }


    gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo = (void*)malloc((sizeof(stBTRCoreDevMediaPcmInfo) > sizeof(stBTRCoreDevMediaSbcInfo) ? sizeof(stBTRCoreDevMediaPcmInfo) : sizeof(stBTRCoreDevMediaSbcInfo)) > sizeof(stBTRCoreDevMediaMpegInfo) ?
                                                                   (sizeof(stBTRCoreDevMediaPcmInfo) > sizeof(stBTRCoreDevMediaSbcInfo) ? sizeof(stBTRCoreDevMediaPcmInfo) : sizeof(stBTRCoreDevMediaSbcInfo)) : sizeof(stBTRCoreDevMediaMpegInfo));

    lenBtrCoreRet = BTRCore_GetDeviceMediaInfo(ghBTRCoreHdl, listOfPDevices.devices[i].tDeviceId, lenBtrCoreDevType, &gstBtrCoreDevMediaInfo);
    if (lenBtrCoreRet == enBTRCoreSuccess) {
        lenBtrCoreDevInMType      = gstBtrCoreDevMediaInfo.eBtrCoreDevMType;
        lpstBtrCoreDevInMCodecInfo= gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo;

        if (lenBtrCoreDevInMType == eBTRCoreDevMediaTypeSBC) {
            stBTRCoreDevMediaSbcInfo*   lpstBtrCoreDevMSbcInfo = (stBTRCoreDevMediaSbcInfo*)lpstBtrCoreDevInMCodecInfo;

            BTRMGRLOG_INFO ("DevMedInfo SFreq           = %d\n", lpstBtrCoreDevMSbcInfo->ui32DevMSFreq);
            BTRMGRLOG_INFO ("DevMedInfo AChan           = %d\n", lpstBtrCoreDevMSbcInfo->eDevMAChan);
            BTRMGRLOG_INFO ("DevMedInfo SbcAllocMethod  = %d\n", lpstBtrCoreDevMSbcInfo->ui8DevMSbcAllocMethod);
            BTRMGRLOG_INFO ("DevMedInfo SbcSubbands     = %d\n", lpstBtrCoreDevMSbcInfo->ui8DevMSbcSubbands);
            BTRMGRLOG_INFO ("DevMedInfo SbcBlockLength  = %d\n", lpstBtrCoreDevMSbcInfo->ui8DevMSbcBlockLength);
            BTRMGRLOG_INFO ("DevMedInfo SbcMinBitpool   = %d\n", lpstBtrCoreDevMSbcInfo->ui8DevMSbcMinBitpool);
            BTRMGRLOG_INFO ("DevMedInfo SbcMaxBitpool   = %d\n", lpstBtrCoreDevMSbcInfo->ui8DevMSbcMaxBitpool);
            BTRMGRLOG_INFO ("DevMedInfo SbcFrameLen     = %d\n", lpstBtrCoreDevMSbcInfo->ui16DevMSbcFrameLen);
            BTRMGRLOG_INFO ("DevMedInfo SbcBitrate      = %d\n", lpstBtrCoreDevMSbcInfo->ui16DevMSbcBitrate);
        }
        else if (lenBtrCoreDevInMType == eBTRCoreDevMediaTypeAAC) {
            stBTRCoreDevMediaMpegInfo*   lpstBtrCoreDevMAacInfo = (stBTRCoreDevMediaMpegInfo*)lpstBtrCoreDevInMCodecInfo;

            BTRMGRLOG_INFO ("DevMedInfo SFreq           = %d\n", lpstBtrCoreDevMAacInfo->ui32DevMSFreq);
            BTRMGRLOG_INFO ("DevMedInfo AChan           = %d\n", lpstBtrCoreDevMAacInfo->eDevMAChan);
            BTRMGRLOG_INFO ("DevMedInfo AacMpegCrc      = %d\n", lpstBtrCoreDevMAacInfo->ui8DevMMpegCrc);
            BTRMGRLOG_INFO ("DevMedInfo AacMpegLayer    = %d\n", lpstBtrCoreDevMAacInfo->ui8DevMMpegLayer);
            BTRMGRLOG_INFO ("DevMedInfo AacMpegMpf      = %d\n", lpstBtrCoreDevMAacInfo->ui8DevMMpegMpf);
            BTRMGRLOG_INFO ("DevMedInfo AacMpegRfa      = %d\n", lpstBtrCoreDevMAacInfo->ui8DevMMpegRfa);
            BTRMGRLOG_INFO ("DevMedInfo AacMpegFrmLen   = %d\n", lpstBtrCoreDevMAacInfo->ui16DevMMpegFrameLen);
            BTRMGRLOG_INFO ("DevMedInfo AacMpegBitrate  = %d\n", lpstBtrCoreDevMAacInfo->ui16DevMMpegBitrate);
        }
    }

    /* Aquire Device Data Path to start audio reception */
    lenBtrCoreRet = BTRCore_AcquireDeviceDataPath (ghBTRCoreHdl, listOfPDevices.devices[i].tDeviceId, lenBtrCoreDevType, &i32DeviceFD, &i32DeviceReadMTU, &i32DeviceWriteMTU, &ui32deviceDelay);
    if (lenBtrCoreRet == enBTRCoreSuccess) {
        if ((lenBtrMgrRet = btrMgr_StartReceivingAudio(i32DeviceFD, i32DeviceReadMTU, ui32deviceDelay, lenBtrCoreDevInMType, lpstBtrCoreDevInMCodecInfo)) == eBTRMgrSuccess) {
            ghBTRMgrDevHdlCurStreaming = listOfPDevices.devices[i].tDeviceId;
            BTRMGRLOG_INFO("Audio Reception Started.. Enjoy the show..! :)\n");
        }
        else {
            BTRMGRLOG_ERROR ("Failed to read audio now\n");
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }
    }
    else {
        BTRMGRLOG_ERROR ("Failed to get Device Data Path. So Will not be able to stream now\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    
    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_StopAudioStreamingIn (
    unsigned char       aui8AdapterIdx,
    BTRMgrDeviceHandle  ahBTRMgrDevHdl
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    eBTRMgrRet      lenBtrMgrRet    = eBTRMgrSuccess;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    if ((ghBTRMgrDevHdlCurStreaming != ahBTRMgrDevHdl) && (ghBTRMgrDevHdlLastConnected != ahBTRMgrDevHdl)) {
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    if ((lenBtrMgrRet = btrMgr_StopReceivingAudio()) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("btrMgr_StopReceivingAudio = %d\n", lenBtrMgrRet);
    }

    // TODO : determine enBTRCoreDeviceType from get Paired dev list
    BTRCore_ReleaseDeviceDataPath (ghBTRCoreHdl, ghBTRMgrDevHdlCurStreaming, enBTRCoreMobileAudioIn);

    ghBTRMgrDevHdlCurStreaming = 0;

    if (gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo) {
        free (gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo);
        gstBtrCoreDevMediaInfo.pstBtrCoreDevMCodecInfo = NULL;
    }

    /* We had Reset the ghBTRMgrDevHdlCurStreaming to avoid recursion/looping; so no worries */
    lenBtrMgrResult = BTRMGR_DisconnectFromDevice(aui8AdapterIdx, ahBTRMgrDevHdl);

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_IsAudioStreamingIn (
    unsigned char   aui8AdapterIdx,
    unsigned char*  pStreamingStatus
) {
    BTRMGR_Result_t lenBtrMgrRet = BTRMGR_RESULT_SUCCESS;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!pStreamingStatus)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    if (ghBTRMgrDevHdlCurStreaming)
        *pStreamingStatus = 1;
    else
        *pStreamingStatus = 0;

    BTRMGRLOG_INFO ("BTRMGR_IsAudioStreamingIn: Returned status Successfully\n");

    return lenBtrMgrRet;
}
#endif
BTRMGR_Result_t
BTRMGR_SetEventResponse (
    unsigned char           aui8AdapterIdx,
    BTRMGR_EventResponse_t* apstBTRMgrEvtRsp
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || (!apstBTRMgrEvtRsp)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }


    switch (apstBTRMgrEvtRsp->m_eventType) {
    case BTRMGR_EVENT_DEVICE_OUT_OF_RANGE:
        break;
    case BTRMGR_EVENT_DEVICE_DISCOVERY_UPDATE:
        break;
    case BTRMGR_EVENT_DEVICE_DISCOVERY_COMPLETE:
        break;
    case BTRMGR_EVENT_DEVICE_PAIRING_COMPLETE:
        break;
    case BTRMGR_EVENT_DEVICE_UNPAIRING_COMPLETE:
        break;
    case BTRMGR_EVENT_DEVICE_CONNECTION_COMPLETE:
        break;
    case BTRMGR_EVENT_DEVICE_DISCONNECT_COMPLETE:
        break;
    case BTRMGR_EVENT_DEVICE_PAIRING_FAILED:
        break;
    case BTRMGR_EVENT_DEVICE_UNPAIRING_FAILED:
        break;
    case BTRMGR_EVENT_DEVICE_CONNECTION_FAILED:
        break;
    case BTRMGR_EVENT_DEVICE_DISCONNECT_FAILED:
        break;
    case BTRMGR_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST:
        gEventRespReceived = 1;
        if (apstBTRMgrEvtRsp->m_eventResp) {
            gAcceptConnection = 1;
        }
        break;
    case BTRMGR_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST:
        gEventRespReceived = 1;
        gAcceptConnection  = 0;
        if (apstBTRMgrEvtRsp->m_eventResp) {
            gAcceptConnection = 1;
        }
        break;
#ifndef LE_MODE
    case BTRMGR_EVENT_RECEIVED_EXTERNAL_PLAYBACK_REQUEST:
        if (apstBTRMgrEvtRsp->m_eventResp && apstBTRMgrEvtRsp->m_deviceHandle) {
            BTRMGR_DeviceOperationType_t    stream_pref = BTRMGR_DEVICE_OP_TYPE_AUDIO_INPUT;
            lenBtrMgrResult = BTRMGR_StartAudioStreamingIn(aui8AdapterIdx, apstBTRMgrEvtRsp->m_deviceHandle, stream_pref);   
        }
        break;
#endif
    case BTRMGR_EVENT_DEVICE_FOUND:
        break;
    case BTRMGR_EVENT_DEVICE_OP_INFORMATION:
#ifndef LE_MODE
        if (apstBTRMgrEvtRsp->m_eventResp) {
            strncpy(gLeReadOpResponse, apstBTRMgrEvtRsp->m_writeData, BTRMGR_MAX_DEV_OP_DATA_LEN - 1);
            gEventRespReceived = 1;
        }
#endif
        break;
    case BTRMGR_EVENT_MAX:
    default:
        break;
    }


    return lenBtrMgrResult;
}

#ifndef LE_MODE
BTRMGR_Result_t
BTRMGR_MediaControl (
    unsigned char                 aui8AdapterIdx,
    BTRMgrDeviceHandle            ahBTRMgrDevHdl,
    BTRMGR_MediaControlCommand_t  mediaCtrlCmd
) {
    BTRMGR_Result_t             lenBtrMgrResult     = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet                lenBtrCoreRet       = enBTRCoreSuccess;
    enBTRCoreDeviceType         lenBtrCoreDevTy     = enBTRCoreUnknown;
    enBTRCoreDeviceClass        lenBtrCoreDevCl     = enBTRCore_DC_Unknown;
    BTRMGR_MediaDeviceStatus_t  lstMediaDeviceStatus;


    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (!btrMgr_IsDevConnected(ahBTRMgrDevHdl)) {
       BTRMGRLOG_ERROR ("Device Handle(%lld) not connected\n", ahBTRMgrDevHdl);
       return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBtrCoreDevTy, &lenBtrCoreDevCl);
    BTRMGRLOG_DEBUG ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBtrCoreDevTy, lenBtrCoreDevCl);


    lstMediaDeviceStatus.m_ui8mediaDevVolume= BTRMGR_SO_MAX_VOLUME;
    lstMediaDeviceStatus.m_enmediaCtrlCmd   = mediaCtrlCmd;
    if (mediaCtrlCmd == BTRMGR_MEDIA_CTRL_MUTE)
        lstMediaDeviceStatus.m_ui8mediaDevMute  =  1;
    else if (mediaCtrlCmd == BTRMGR_MEDIA_CTRL_UNMUTE)
        lstMediaDeviceStatus.m_ui8mediaDevMute  =  0;
    else
        lstMediaDeviceStatus.m_ui8mediaDevMute  =  BTRMGR_SO_MAX_VOLUME;     // To handle alternate options


    if (btrMgr_MediaControl(aui8AdapterIdx, ahBTRMgrDevHdl, &lstMediaDeviceStatus, lenBtrCoreDevTy, lenBtrCoreDevCl, NULL) != eBTRMgrSuccess)
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;

    return lenBtrMgrResult;
}

eBTRMgrRet
btrMgr_MediaControl (
    unsigned char               aui8AdapterIdx,
    BTRMgrDeviceHandle          ahBTRMgrDevHdl,
    BTRMGR_MediaDeviceStatus_t* apstMediaDeviceStatus,
    enBTRCoreDeviceType         aenBtrCoreDevTy,
    enBTRCoreDeviceClass        aenBtrCoreDevCl,
    stBTRCoreMediaCtData*       apstBtrCoreMediaCData
) {
    enBTRCoreMediaCtrl  lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlUnknown;
    eBTRMgrRet          lenBtrMgrRet        = eBTRMgrFailure;

    switch (apstMediaDeviceStatus->m_enmediaCtrlCmd) {
    case BTRMGR_MEDIA_CTRL_PLAY:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlPlay;
        break;
    case BTRMGR_MEDIA_CTRL_PAUSE:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlPause;
        break;
    case BTRMGR_MEDIA_CTRL_STOP:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlStop;
        break;
    case BTRMGR_MEDIA_CTRL_NEXT:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlNext;
        break;
    case BTRMGR_MEDIA_CTRL_PREVIOUS:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlPrevious;
        break;
    case BTRMGR_MEDIA_CTRL_FASTFORWARD:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlFastForward;
        break;
    case BTRMGR_MEDIA_CTRL_REWIND:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlRewind;
        break;
    case BTRMGR_MEDIA_CTRL_VOLUMEUP:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlVolumeUp;
        break;
    case BTRMGR_MEDIA_CTRL_VOLUMEDOWN:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlVolumeDown;
        break;
    case BTRMGR_MEDIA_CTRL_EQUALIZER_OFF:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlEqlzrOff;
        break;
    case BTRMGR_MEDIA_CTRL_EQUALIZER_ON:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlEqlzrOn;
        break;
    case BTRMGR_MEDIA_CTRL_SHUFFLE_OFF:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlShflOff;
        break;
    case BTRMGR_MEDIA_CTRL_SHUFFLE_ALLTRACKS:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlShflAllTracks;
        break;
    case BTRMGR_MEDIA_CTRL_SHUFFLE_GROUP:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlShflGroup;
        break;
    case BTRMGR_MEDIA_CTRL_REPEAT_OFF:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlRptOff;
        break;
    case BTRMGR_MEDIA_CTRL_REPEAT_SINGLETRACK:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlRptSingleTrack;
        break;
    case BTRMGR_MEDIA_CTRL_REPEAT_ALLTRACKS:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlRptAllTracks;
        break;
    case BTRMGR_MEDIA_CTRL_REPEAT_GROUP:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlRptGroup;
        break;
    case BTRMGR_MEDIA_CTRL_MUTE:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlMute;
        break;
    case BTRMGR_MEDIA_CTRL_UNMUTE:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlUnMute;
        break;
    case BTRMGR_MEDIA_CTRL_UNKNOWN:
    default:
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlUnknown;
    }


    if (lenBTRCoreMediaCtrl != enBTRCoreMediaCtrlUnknown) {

        if ((aenBtrCoreDevTy == enBTRCoreSpeakers) || (aenBtrCoreDevTy == enBTRCoreHeadSet)) {
            if ((ghBTRMgrDevHdlCurStreaming == ahBTRMgrDevHdl) ) {
                BTRMGR_EventMessage_t lstEventMessage;

                MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));

                switch (apstMediaDeviceStatus->m_enmediaCtrlCmd) {
                case BTRMGR_MEDIA_CTRL_VOLUMEUP:
                    if (enBTRCoreSuccess == BTRCore_MediaControl(ghBTRCoreHdl, ahBTRMgrDevHdl, aenBtrCoreDevTy, lenBTRCoreMediaCtrl, apstBtrCoreMediaCData)) {
                        BTRMGRLOG_TRACE ("Media Control Command BTRMGR_MEDIA_CTRL_VOLUMEUP for %llu Success for streamout!!!\n", ahBTRMgrDevHdl);
                        lenBtrMgrRet = eBTRMgrSuccess;
                    }
                    else if (apstMediaDeviceStatus->m_ui8mediaDevMute == BTRMGR_SO_MAX_VOLUME) {
                        unsigned char         ui8CurVolume = 0;

                        BTRMgr_SO_GetVolume(gstBTRMgrStreamingInfo.hBTRMgrSoHdl,&ui8CurVolume);
                        BTRMGRLOG_TRACE ("ui8CurVolume %d \n ",ui8CurVolume);

                        if (apstBtrCoreMediaCData != NULL) {
                            ui8CurVolume = apstBtrCoreMediaCData->m_mediaAbsoluteVolume;
                        }
                        else {
                            if(ui8CurVolume < 5)
                                ui8CurVolume = 5;
                            else if (ui8CurVolume <= 245 && ui8CurVolume >= 5)
                                ui8CurVolume = ui8CurVolume + 10; // Increment steps in 10
                            else
                                ui8CurVolume = BTRMGR_SO_MAX_VOLUME;
                        }

                        if ((lenBtrMgrRet = BTRMgr_SO_SetVolume(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, ui8CurVolume)) == eBTRMgrSuccess) {
                            lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_MEDIA_STATUS;
                            lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevVolume  = ui8CurVolume;
                            lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevMute    = (ui8CurVolume) ? FALSE : TRUE;
                            lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd     = BTRMGR_MEDIA_CTRL_VOLUMEUP;
                            BTRMGRLOG_TRACE (" Volume Up %d \n", ui8CurVolume);
#ifdef RDKTV_PERSIST_VOLUME
                            btrMgr_SetLastVolume(aui8AdapterIdx, ui8CurVolume, ahBTRMgrDevHdl, BTRMGR_A2DP_SINK_PROFILE_ID);
#endif
                        }
                    }
                break;

                case BTRMGR_MEDIA_CTRL_VOLUMEDOWN:
                    if (enBTRCoreSuccess == BTRCore_MediaControl(ghBTRCoreHdl, ahBTRMgrDevHdl, aenBtrCoreDevTy, lenBTRCoreMediaCtrl, apstBtrCoreMediaCData)) {
                        BTRMGRLOG_INFO ("Media Control Command BTRMGR_MEDIA_CTRL_VOLUMEDOWN for %llu Success for streamout!!!\n", ahBTRMgrDevHdl);
                        lenBtrMgrRet = eBTRMgrSuccess;
                    }
                    else if (apstMediaDeviceStatus->m_ui8mediaDevMute == BTRMGR_SO_MAX_VOLUME) {
                        unsigned char         ui8CurVolume = 0;

                        BTRMgr_SO_GetVolume(gstBTRMgrStreamingInfo.hBTRMgrSoHdl,&ui8CurVolume);
                        BTRMGRLOG_TRACE ("ui8CurVolume %d \n ",ui8CurVolume);

                        if (apstBtrCoreMediaCData != NULL) {
                            ui8CurVolume = apstBtrCoreMediaCData->m_mediaAbsoluteVolume;
                        }
                        else {
                            if (ui8CurVolume > 250)
                                ui8CurVolume = 250;
                            else if (ui8CurVolume <= 250 && ui8CurVolume >= 10)
                                ui8CurVolume = ui8CurVolume - 10;   // Decrement steps in 10
                            else
                                ui8CurVolume = 0;
                        }

                        lenBtrMgrRet = BTRMgr_SO_SetVolume(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, ui8CurVolume);

                        if (lenBtrMgrRet == eBTRMgrSuccess) {
                            lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_MEDIA_STATUS;
                            lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevVolume= ui8CurVolume;
                            lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevMute  = (ui8CurVolume) ? FALSE : TRUE;
                            lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd   = BTRMGR_MEDIA_CTRL_VOLUMEDOWN;
                            BTRMGRLOG_TRACE (" Volume Down %d \n", ui8CurVolume);
#ifdef RDKTV_PERSIST_VOLUME
                            btrMgr_SetLastVolume(aui8AdapterIdx, ui8CurVolume, ahBTRMgrDevHdl, BTRMGR_A2DP_SINK_PROFILE_ID);
#endif
                        }
                    }
                break;

                case BTRMGR_MEDIA_CTRL_MUTE:
                    if ((lenBtrMgrRet = BTRMgr_SO_SetMute(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, TRUE)) == eBTRMgrSuccess) {
                        unsigned char         ui8CurVolume = 0;
                        BTRMgr_SO_GetVolume(gstBTRMgrStreamingInfo.hBTRMgrSoHdl,&ui8CurVolume);

                        lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_MEDIA_STATUS;
                        lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevVolume= ui8CurVolume;
                        lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevMute  = TRUE;
                        lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd   = BTRMGR_MEDIA_CTRL_MUTE;
                        BTRMGRLOG_TRACE (" Mute set success \n");
#ifdef RDKTV_PERSIST_VOLUME
                        btrMgr_SetLastMuteState(aui8AdapterIdx, TRUE);
#endif
                    }
                break;

                case BTRMGR_MEDIA_CTRL_UNMUTE:
                    if ((lenBtrMgrRet = BTRMgr_SO_SetMute(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, FALSE)) == eBTRMgrSuccess) {
                        unsigned char         ui8CurVolume = 0;
                        BTRMgr_SO_GetVolume(gstBTRMgrStreamingInfo.hBTRMgrSoHdl,&ui8CurVolume);

                        lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_MEDIA_STATUS;
                        lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevVolume= ui8CurVolume;
                        lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevMute  = FALSE;
                        lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd   = BTRMGR_MEDIA_CTRL_UNMUTE;
                        BTRMGRLOG_TRACE (" UnMute set success \n");
#ifdef RDKTV_PERSIST_VOLUME
                        btrMgr_SetLastMuteState(aui8AdapterIdx, FALSE);
#endif
                    }
                break;

                default:
                    if (enBTRCoreSuccess == BTRCore_MediaControl(ghBTRCoreHdl, ahBTRMgrDevHdl, aenBtrCoreDevTy, lenBTRCoreMediaCtrl, apstBtrCoreMediaCData)) {
                        lenBtrMgrRet = eBTRMgrSuccess;
                    }
                    else  {
                        BTRMGRLOG_ERROR ("Media Control Command for %llu Failed for streamout!!!\n", ahBTRMgrDevHdl);
                        lenBtrMgrRet = eBTRMgrFailure;
                    }
                }

                if ((lenBtrMgrRet == eBTRMgrSuccess) && (gfpcBBTRMgrEventOut)) {
                    lstEventMessage.m_mediaInfo.m_deviceHandle  = ahBTRMgrDevHdl;
                    lstEventMessage.m_mediaInfo.m_deviceType    = btrMgr_MapDeviceTypeFromCore(aenBtrCoreDevCl);
                    for (int j = 0; j <= gListOfPairedDevices.m_numOfDevices; j++) {
                        if (ahBTRMgrDevHdl == gListOfPairedDevices.m_deviceProperty[j].m_deviceHandle) {
                            strncpy(lstEventMessage.m_mediaInfo.m_name, gListOfPairedDevices.m_deviceProperty[j].m_name, BTRMGR_NAME_LEN_MAX -1);
                            break;
                        }
                    }

                    gfpcBBTRMgrEventOut(lstEventMessage); /*  Post a callback */
                }

            } else {
                BTRMGRLOG_ERROR ("pstBtrMgrSoHdl Null  or streaming out not started \n");
                lenBtrMgrRet = eBTRMgrFailure;
            }
        }
        else {
            if (enBTRCoreSuccess != BTRCore_MediaControl(ghBTRCoreHdl, ahBTRMgrDevHdl, aenBtrCoreDevTy, lenBTRCoreMediaCtrl, apstBtrCoreMediaCData)) {
                BTRMGRLOG_ERROR ("Media Control Command for %llu Failed!!!\n", ahBTRMgrDevHdl);
                lenBtrMgrRet = eBTRMgrFailure;
            }
        }
    }
    else {
        BTRMGRLOG_ERROR ("Media Control Command for %llu Unknown!!!\n", ahBTRMgrDevHdl);
        lenBtrMgrRet = eBTRMgrFailure;
    }

    return lenBtrMgrRet;
}

BTRMGR_Result_t
BTRMGR_GetDataPathAndConfigurationForStreamOut(
    unsigned char             aui8AdapterIdx,
    BTRMgrDeviceHandle        ahBTRMgrDevHdl,
    int*                      pi32dataPath,
    int*                      pi32Readmtu,
    int*                      pi32Writemtu,
    unsigned int*             pi32Delay,
    BTRMGR_MediaStreamInfo_t* pstBtrMgrDevStreamInfo
)
{
    stBTRCoreDevMediaInfo pstDevMediaInfo;
    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }
    if (!pi32dataPath || !pi32Readmtu || !pi32Writemtu || !pi32Delay || !pstBtrMgrDevStreamInfo)
    {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }
    pstDevMediaInfo.eBtrCoreDevMType = eBTRCoreDevMediaTypeUnknown;
    pstDevMediaInfo.pstBtrCoreDevMCodecInfo = (void*) &pstBtrMgrDevStreamInfo->m_sbcInfo; //sbc info is largest in union, use for memory address

    if(BTRCore_GetDeviceMediaInfo(ghBTRCoreHdl, ahBTRMgrDevHdl, enBTRCoreSpeakers, &pstDevMediaInfo) != enBTRCoreSuccess)
    {
        BTRMGRLOG_ERROR("Could not get audio configuration\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }
    pstBtrMgrDevStreamInfo->m_codec = (eBTRMGRDevMediaType_t) pstDevMediaInfo.eBtrCoreDevMType;

    if (BTRCore_AcquireDeviceDataPath(ghBTRCoreHdl, ahBTRMgrDevHdl, enBTRCoreSpeakers, pi32dataPath, pi32Readmtu, pi32Writemtu, pi32Delay) != enBTRCoreSuccess)
    {
        BTRMGRLOG_ERROR("Failed to acquire data path\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }
    return BTRMGR_RESULT_SUCCESS;
}

BTRMGR_Result_t
BTRMGR_ReleaseDataPathForStreamOut(
    unsigned char            aui8AdapterIdx,
    BTRMgrDeviceHandle       ahBTRMgrDevHdl
)
{

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }
    if (BTRCore_ReleaseDeviceDataPath(ghBTRCoreHdl,ahBTRMgrDevHdl, enBTRCoreSpeakers) != enBTRCoreSuccess)
    {
        BTRMGRLOG_ERROR("Failed to release data path\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }
    return BTRMGR_RESULT_SUCCESS;
}

BTRMGR_Result_t
BTRMGR_StartSendingAudioFromFile(
    unsigned char             aui8AdapterIdx,
    BTRMgrDeviceHandle        ahBTRMgrDevHdl,
    BTRMGR_MediaStreamInfo_t* pstMediaAudioOutInfo,
    BTRMGR_MediaStreamInfo_t* pstMediaAudioInInfo,
    int                       i32OutFd,
    int                       outMTUSize, 
    unsigned int              outDevDelay,
    char *                    pcAudioInputFilePath
)
{
    stBTRMgrOutASettings    lstBtrMgrAcOutASettings;
    stBTRMgrInASettings     lstBtrMgrSoInASettings;
    stBTRMgrOutASettings    lstBtrMgrSoOutASettings;
    eBTRMgrRet              lenBtrMgrRet = eBTRMgrSuccess;
    int                     inBytesToEncode = 3072; // Corresponds to MTU size of 895
    unsigned char           ui8FlushTimeoutMs = 0;

    if ((ghBTRMgrDevHdlCurStreaming != 0) || (outMTUSize == 0) || !pstMediaAudioOutInfo || !pstMediaAudioInInfo || !pcAudioInputFilePath || i32OutFd < 0) {
        return BTRMGR_RESULT_INVALID_INPUT;
    }
    if (pstMediaAudioInInfo->m_codec != BTRMGR_DEV_MEDIA_TYPE_PCM)
    {
        BTRMGRLOG_ERROR("Input type must be PCM\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }
    if (pstMediaAudioOutInfo->m_codec != BTRMGR_DEV_MEDIA_TYPE_SBC)
    {
        BTRMGRLOG_ERROR("Output type must be SBC\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }
    stBTRMgrPCMInfo       lstBtrMgrPcmInfo;
    stBTRMgrSBCInfo       lstBtrMgrSbcInfo;
    
    /* Reset the buffer */
    MEMSET_S(&gstBTRMgrStreamingInfo, sizeof(gstBTRMgrStreamingInfo), 0, sizeof(gstBTRMgrStreamingInfo));

    MEMSET_S(&lstBtrMgrAcOutASettings,sizeof(lstBtrMgrAcOutASettings), 0, sizeof(lstBtrMgrAcOutASettings));
    MEMSET_S(&lstBtrMgrSoInASettings, sizeof(lstBtrMgrSoInASettings),  0, sizeof(lstBtrMgrSoInASettings));
    MEMSET_S(&lstBtrMgrSoOutASettings,sizeof(lstBtrMgrSoOutASettings), 0, sizeof(lstBtrMgrSoOutASettings));

    /* Init StreamOut module - Create Pipeline */
    if ((lenBtrMgrRet = BTRMgr_SO_Init(&gstBTRMgrStreamingInfo.hBTRMgrSoHdl, btrMgr_SOStatusCb, &gstBTRMgrStreamingInfo)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SO_Init FAILED\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if ((lenBtrMgrRet = BTRMgr_AC_TestInit(&gstBTRMgrStreamingInfo.hBTRMgrAcHdl, gDebugModeEnabled, pcAudioInputFilePath)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_AC_Init FAILED\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    gstBTRMgrStreamingInfo.bitsPerSample = 16;
    gstBTRMgrStreamingInfo.samplerate = 48000;
    gstBTRMgrStreamingInfo.channels = 2;

    lstBtrMgrPcmInfo.eBtrMgrAChan = (eBTRMgrAChan) pstMediaAudioInInfo->m_pcmInfo.m_channelMode;
    switch(pstMediaAudioInInfo->m_pcmInfo.m_format)
    {
        case 8:
            lstBtrMgrPcmInfo.eBtrMgrSFmt = eBTRMgrSFmt8bit;
            break;
        case 16:
            lstBtrMgrPcmInfo.eBtrMgrSFmt = eBTRMgrSFmt16bit;
            break;
        case 24:
            lstBtrMgrPcmInfo.eBtrMgrSFmt = eBTRMgrSFmt24bit;
            break;
        case 32:
            lstBtrMgrPcmInfo.eBtrMgrSFmt = eBTRMgrSFmt32bit;
            break;
        default:
            lstBtrMgrPcmInfo.eBtrMgrSFmt = eBTRMgrSFmtUnknown;
            break;
    }
    switch(pstMediaAudioInInfo->m_pcmInfo.m_freq)
    {
        case 8000:
            lstBtrMgrPcmInfo.eBtrMgrSFreq = eBTRMgrSFreq8K;
            break;
        case 16000:
            lstBtrMgrPcmInfo.eBtrMgrSFreq = eBTRMgrSFreq16K;
            break;
        case 32000:
            lstBtrMgrPcmInfo.eBtrMgrSFreq = eBTRMgrSFreq32K;
            break;
        case 44100:
            lstBtrMgrPcmInfo.eBtrMgrSFreq = eBTRMgrSFreq44_1K;
            break;
        case 48000:
            lstBtrMgrPcmInfo.eBtrMgrSFreq = eBTRMgrSFreq48K;
            break;
        default:
            lstBtrMgrPcmInfo.eBtrMgrSFreq = eBTRMgrSFreqUnknown;
            break;
    }
    
    lstBtrMgrPcmInfo.eBtrMgrSFreq = (eBTRMgrSFreq) pstMediaAudioInInfo->m_pcmInfo.m_freq;
    lstBtrMgrSoInASettings.eBtrMgrInAType = eBTRMgrATypePCM;
    lstBtrMgrSoInASettings.pstBtrMgrInCodecInfo = &lstBtrMgrPcmInfo;
    
    switch(pstMediaAudioOutInfo->m_sbcInfo.m_freq)
    {
        case 8000:
            lstBtrMgrSbcInfo.eBtrMgrSbcSFreq = eBTRMgrSFreq8K;
            break;
        case 16000:
            lstBtrMgrSbcInfo.eBtrMgrSbcSFreq = eBTRMgrSFreq16K;
            break;
        case 32000:
            lstBtrMgrSbcInfo.eBtrMgrSbcSFreq = eBTRMgrSFreq32K;
            break;
        case 44100:
            lstBtrMgrSbcInfo.eBtrMgrSbcSFreq = eBTRMgrSFreq44_1K;
            break;
        case 48000:
            lstBtrMgrSbcInfo.eBtrMgrSbcSFreq = eBTRMgrSFreq48K;
            break;
        default:
            lstBtrMgrSbcInfo.eBtrMgrSbcSFreq = eBTRMgrSFreqUnknown;
            break;
    }

    lstBtrMgrSbcInfo.eBtrMgrSbcAChan = (eBTRMgrAChan) pstMediaAudioOutInfo->m_sbcInfo.m_channelMode;
    lstBtrMgrSbcInfo.ui8SbcAllocMethod = pstMediaAudioOutInfo->m_sbcInfo.m_allocMethod;
    lstBtrMgrSbcInfo.ui8SbcSubbands = pstMediaAudioOutInfo->m_sbcInfo.m_subbands;
    lstBtrMgrSbcInfo.ui8SbcBlockLength = pstMediaAudioOutInfo->m_sbcInfo.m_blockLength;
    lstBtrMgrSbcInfo.ui8SbcMinBitpool = pstMediaAudioOutInfo->m_sbcInfo.m_minBitpool;
    lstBtrMgrSbcInfo.ui8SbcMaxBitpool = pstMediaAudioOutInfo->m_sbcInfo.m_maxBitpool;
    lstBtrMgrSbcInfo.ui16SbcFrameLen = pstMediaAudioOutInfo->m_sbcInfo.m_frameLen;
    lstBtrMgrSbcInfo.ui16SbcBitrate = pstMediaAudioOutInfo->m_sbcInfo.m_bitrate;

    lstBtrMgrSoOutASettings.eBtrMgrOutAType = eBTRMgrATypeSBC;
    lstBtrMgrSoOutASettings.pstBtrMgrOutCodecInfo = &lstBtrMgrSbcInfo;

    lstBtrMgrSoOutASettings.i32BtrMgrDevFd      = i32OutFd;
    lstBtrMgrSoOutASettings.i32BtrMgrDevMtu     = outMTUSize;
    lstBtrMgrSoOutASettings.ui32BtrMgrDevDelay  = outDevDelay;

    if ((lenBtrMgrRet = BTRMgr_SO_GetEstimatedInABufSize(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, &lstBtrMgrSoInASettings, &lstBtrMgrSoOutASettings)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SO_GetEstimatedInABufSize FAILED\n");
        lstBtrMgrSoInASettings.i32BtrMgrInBufMaxSize = inBytesToEncode;
    }
    else {
        gstBTRMgrStreamingInfo.i32BytesToEncode = lstBtrMgrSoInASettings.i32BtrMgrInBufMaxSize;
    }
     if ((lenBtrMgrRet = BTRMgr_SO_Start(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, &lstBtrMgrSoInASettings, &lstBtrMgrSoOutASettings)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SO_Start FAILED\n");
    }

    if (lenBtrMgrRet == eBTRMgrSuccess) {
        lstBtrMgrAcOutASettings.i32BtrMgrOutBufMaxSize = lstBtrMgrSoInASettings.i32BtrMgrInBufMaxSize;
        lstBtrMgrAcOutASettings.ui32BtrMgrDevDelay = outDevDelay;

        if ((lenBtrMgrRet = BTRMgr_AC_TestStart(gstBTRMgrStreamingInfo.hBTRMgrAcHdl,
                                            &lstBtrMgrAcOutASettings,
                                            btrMgr_ACDataReadyCb,
                                            btrMgr_ACStatusCb,
                                            &gstBTRMgrStreamingInfo)) != eBTRMgrSuccess) {
            BTRMGRLOG_ERROR ("BTRMgr_AC_Start FAILED\n");
        }
    }
    if(lenBtrMgrRet == eBTRMgrSuccess)
    {
        ghBTRMgrDevHdlCurStreaming = ahBTRMgrDevHdl;
        ghBTRMGRDevHdlTestStreaming = ahBTRMgrDevHdl;
        BTRMGRLOG_INFO ("Setting flush timeout\n");
        ui8FlushTimeoutMs = (outMTUSize >= BTMGR_LARGE_MTU_THRESHOLD) ? BTMGR_FLUSH_TIMEOUT_LARGE_MTU_INTERVAL_MS : BTMGR_FLUSH_TIMEOUT_INTERVAL_MS;
        if (BTRCore_SetDeviceDataAckTimeout(ghBTRCoreHdl, ui8FlushTimeoutMs) != enBTRCoreSuccess) {
            BTRMGRLOG_WARN ("Failed to set timeout for Audio drop. EXPECT AV SYNC ISSUES!\n");
        }        
    }
    return BTRMGR_RESULT_SUCCESS;
}
BTRMGR_Result_t
BTRMGR_StopSendingAudioFromFile (
    void
) {
    eBTRMgrRet  lenBtrMgrRet = eBTRMgrSuccess;

    if (ghBTRMgrDevHdlCurStreaming == 0 && ghBTRMGRDevHdlTestStreaming == 0) {
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    BTRMGRLOG_INFO("Stopping streaming from file\n");
    if ((lenBtrMgrRet = BTRMgr_AC_TestStop(gstBTRMgrStreamingInfo.hBTRMgrAcHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_AC_Stop FAILED\n");
    }

    if ((lenBtrMgrRet = BTRMgr_SO_SendEOS(gstBTRMgrStreamingInfo.hBTRMgrSoHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SO_SendEOS FAILED\n");
    }

    if ((lenBtrMgrRet = BTRMgr_SO_Stop(gstBTRMgrStreamingInfo.hBTRMgrSoHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SO_Stop FAILED\n");
    }

    if ((lenBtrMgrRet = BTRMgr_AC_TestDeInit(gstBTRMgrStreamingInfo.hBTRMgrAcHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_AC_DeInit FAILED\n");
    }

    if ((lenBtrMgrRet = BTRMgr_SO_DeInit(gstBTRMgrStreamingInfo.hBTRMgrSoHdl)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("BTRMgr_SO_DeInit FAILED\n");
    }

    gstBTRMgrStreamingInfo.hBTRMgrAcHdl = NULL;
    gstBTRMgrStreamingInfo.hBTRMgrSoHdl = NULL;
    ghBTRMgrDevHdlCurStreaming = 0;
    ghBTRMGRDevHdlTestStreaming = 0;	
	if (lenBtrMgrRet == eBTRMgrSuccess)
    	return BTRMGR_RESULT_SUCCESS;
	else
		return BTRMGR_RESULT_GENERIC_FAILURE;
}

BTRMGR_Result_t
BTRMGR_GetDeviceVolumeMute (
    unsigned char                aui8AdapterIdx,
    BTRMgrDeviceHandle           ahBTRMgrDevHdl,
    BTRMGR_DeviceOperationType_t deviceOpType,
    unsigned char*               pui8Volume,
    unsigned char*               pui8Mute
) {
    BTRMGR_Result_t       lenBtrMgrResult       = BTRMGR_RESULT_SUCCESS;
    eBTRMgrRet            lenBtrMgrRet          = eBTRMgrSuccess;
    eBTRMgrRet            lenBtrMgrPersistRet   = eBTRMgrFailure;
    enBTRCoreRet          lenBtrCoreRet         = enBTRCoreSuccess;
    enBTRCoreDeviceType   lenBtrCoreDevTy       = enBTRCoreUnknown;
    enBTRCoreDeviceClass  lenBtrCoreDevCl       = enBTRCore_DC_Unknown;
    unsigned char         lui8CurVolume         = 0;
    gboolean              lbCurMute             = FALSE;
    char                  profileStr[BTRMGR_MAX_STR_LEN] = {'\0'};


#ifdef RDKTV_PERSIST_VOLUME
    unsigned char         lui8PersistedVolume = 0;
    gboolean              lbPersistedMute = FALSE;
#endif

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (!btrMgr_IsDevConnected(ahBTRMgrDevHdl) || (gstBTRMgrStreamingInfo.hBTRMgrSoHdl == NULL)) {
       BTRMGRLOG_ERROR ("Device Handle(%lld) not connected/streaming\n", ahBTRMgrDevHdl);
       return BTRMGR_RESULT_INVALID_INPUT;
    }

    // Currently implemented for audio out only
    if (deviceOpType != BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT) {
       BTRMGRLOG_ERROR ("Device Handle(%lld) not audio out\n", ahBTRMgrDevHdl);
       return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBtrCoreDevTy, &lenBtrCoreDevCl);
    BTRMGRLOG_DEBUG ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBtrCoreDevTy, lenBtrCoreDevCl);

    if ((lenBtrCoreDevTy == enBTRCoreSpeakers) || (lenBtrCoreDevTy == enBTRCoreHeadSet)) {
        MEMCPY_S(profileStr,sizeof(profileStr),BTRMGR_A2DP_SINK_PROFILE_ID,sizeof(BTRMGR_A2DP_SINK_PROFILE_ID));
    } else if ((lenBtrCoreDevTy == enBTRCoreMobileAudioIn) || (lenBtrCoreDevTy == enBTRCorePCAudioIn)) {
        MEMCPY_S(profileStr,sizeof(profileStr),BTRMGR_A2DP_SRC_PROFILE_ID,sizeof(BTRMGR_A2DP_SRC_PROFILE_ID));
    }

#ifdef RDKTV_PERSIST_VOLUME
    if ((lenBtrMgrPersistRet = btrMgr_GetLastVolume(0, &lui8PersistedVolume, ahBTRMgrDevHdl, profileStr)) == eBTRMgrSuccess) {
        lui8CurVolume = lui8PersistedVolume;
    }
    else {
       BTRMGRLOG_WARN ("Device Handle(%lld) Persist audio out volume get fail\n", ahBTRMgrDevHdl);
    }
#endif

    if ((lenBtrMgrPersistRet == eBTRMgrFailure) &&
        ((lenBtrMgrRet = BTRMgr_SO_GetVolume(gstBTRMgrStreamingInfo.hBTRMgrSoHdl,&lui8CurVolume)) != eBTRMgrSuccess)) {
       BTRMGRLOG_ERROR ("Device Handle(%lld) audio out volume get fail\n", ahBTRMgrDevHdl);
       lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

#ifdef RDKTV_PERSIST_VOLUME
    if ((lenBtrMgrPersistRet = btrMgr_GetLastMuteState(0, &lbPersistedMute)) == eBTRMgrSuccess) {
        lbCurMute = lbPersistedMute;
    }
    else {
       BTRMGRLOG_ERROR ("Device Handle(%lld) Persist audio out mute get fail\n", ahBTRMgrDevHdl);
    }
#endif

    if ((lenBtrMgrPersistRet == eBTRMgrFailure) &&
        ((lenBtrMgrRet = BTRMgr_SO_GetMute(gstBTRMgrStreamingInfo.hBTRMgrSoHdl,&lbCurMute)) != eBTRMgrSuccess)) {
       BTRMGRLOG_ERROR ("Device Handle(%lld) audio out mute get fail\n", ahBTRMgrDevHdl);
       lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    if (lenBtrMgrRet == eBTRMgrSuccess)  {
        *pui8Volume = lui8CurVolume;
        *pui8Mute   = (unsigned char)lbCurMute;
    }

    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_SetDeviceVolumeMute (
    unsigned char                aui8AdapterIdx,
    BTRMgrDeviceHandle           ahBTRMgrDevHdl,
    BTRMGR_DeviceOperationType_t deviceOpType,
    unsigned char                aui8Volume,
    unsigned char                aui8Mute
) {
    BTRMGR_Result_t             lenBtrMgrResult     = BTRMGR_RESULT_SUCCESS;
    eBTRMgrRet                  lenBtrMgrRet        = eBTRMgrSuccess;
    enBTRCoreRet                lenBtrCoreRet       = enBTRCoreSuccess;
    enBTRCoreDeviceType         lenBtrCoreDevTy     = enBTRCoreUnknown;
    enBTRCoreDeviceClass        lenBtrCoreDevCl     = enBTRCore_DC_Unknown;
    unsigned char               lui8Volume          = BTRMGR_SO_MAX_VOLUME - 1;
    gboolean                    lbMuted             = FALSE;
    gboolean                    abMuted             = FALSE;
    BTRMGR_MediaDeviceStatus_t  lstMediaDeviceStatus;
    stBTRCoreMediaCtData        lstBTRCoreMediaCData;
    BTRMGR_EventMessage_t       lstEventMessage;


    MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));
    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt() || aui8Mute > 1) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (!btrMgr_IsDevConnected(ahBTRMgrDevHdl) || (gstBTRMgrStreamingInfo.hBTRMgrSoHdl == NULL)) {
        BTRMGRLOG_ERROR ("Device Handle(%lld) not connected/streaming\n", ahBTRMgrDevHdl);
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    // Currently implemented for audio out only
    if (deviceOpType != BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT) {
        BTRMGRLOG_ERROR ("Device Handle(%lld) not audio out\n", ahBTRMgrDevHdl);
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBtrCoreDevTy, &lenBtrCoreDevCl);
    BTRMGRLOG_TRACE ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBtrCoreDevTy, lenBtrCoreDevCl);

#ifdef RDKTV_PERSIST_VOLUME
    if ((lenBtrCoreDevTy == enBTRCoreSpeakers) || (lenBtrCoreDevTy == enBTRCoreHeadSet)) {
        btrMgr_GetLastVolume(aui8AdapterIdx, &lui8Volume, ahBTRMgrDevHdl, BTRMGR_A2DP_SINK_PROFILE_ID);
    } else if ((lenBtrCoreDevTy == enBTRCoreMobileAudioIn) || (lenBtrCoreDevTy == enBTRCorePCAudioIn)) {
        btrMgr_GetLastVolume(aui8AdapterIdx, &lui8Volume, ahBTRMgrDevHdl, BTRMGR_A2DP_SRC_PROFILE_ID);
    }

    btrMgr_GetLastMuteState(0, &lbMuted);
#endif

    lstMediaDeviceStatus.m_enmediaCtrlCmd   = BTRMGR_MEDIA_CTRL_UNKNOWN;
    lstMediaDeviceStatus.m_ui8mediaDevVolume= aui8Volume;
    lstMediaDeviceStatus.m_ui8mediaDevMute  = aui8Mute;

    lstBTRCoreMediaCData.m_mediaAbsoluteVolume  = aui8Volume;

    if (aui8Volume > lui8Volume)
        lstMediaDeviceStatus.m_enmediaCtrlCmd   = BTRMGR_MEDIA_CTRL_VOLUMEUP;
    else if (aui8Volume < lui8Volume)
        lstMediaDeviceStatus.m_enmediaCtrlCmd   = BTRMGR_MEDIA_CTRL_VOLUMEDOWN;
    
    if ((lenBtrMgrRet = btrMgr_MediaControl(aui8AdapterIdx, ahBTRMgrDevHdl, &lstMediaDeviceStatus, lenBtrCoreDevTy, lenBtrCoreDevCl, &lstBTRCoreMediaCData)) == eBTRMgrSuccess) {
        BTRMGRLOG_TRACE ("Device Handle(%lld) AVRCP audio out volume Set Success\n", ahBTRMgrDevHdl);
        if (BTRMgr_SO_SetVolume(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, BTRMGR_SO_MAX_VOLUME) != eBTRMgrSuccess) {
            BTRMGRLOG_ERROR ("Device Handle(%lld) AVRCP audio-out SO volume Set fail\n", ahBTRMgrDevHdl);
        }
    }
    else if ((aui8Volume != lui8Volume) && ((lenBtrMgrRet = BTRMgr_SO_SetVolume(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, aui8Volume)) != eBTRMgrSuccess)) {
        BTRMGRLOG_ERROR ("Device Handle(%lld) audio out volume Set fail\n", ahBTRMgrDevHdl);
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

#ifdef RDKTV_PERSIST_VOLUME
    if (lenBtrMgrRet == eBTRMgrSuccess) {
        if ((lenBtrCoreDevTy == enBTRCoreSpeakers) || (lenBtrCoreDevTy == enBTRCoreHeadSet)) {
            btrMgr_SetLastVolume(aui8AdapterIdx, aui8Volume, ahBTRMgrDevHdl, BTRMGR_A2DP_SINK_PROFILE_ID);
        } else if ((lenBtrCoreDevTy == enBTRCoreMobileAudioIn) || (lenBtrCoreDevTy == enBTRCorePCAudioIn)) {
            btrMgr_SetLastVolume(aui8AdapterIdx, aui8Volume, ahBTRMgrDevHdl, BTRMGR_A2DP_SRC_PROFILE_ID);
        }
    }
#endif

    abMuted = aui8Mute ? TRUE : FALSE;
    if ((lenBtrMgrRet = BTRMgr_SO_SetMute(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, abMuted)) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("Device Handle(%lld) not audio out mute set fail\n", ahBTRMgrDevHdl);
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
#ifdef RDKTV_PERSIST_VOLUME
        btrMgr_SetLastMuteState(aui8AdapterIdx, abMuted);
#endif
    }

    if ((lenBtrMgrResult == BTRMGR_RESULT_SUCCESS) && gfpcBBTRMgrEventOut) {
        lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_MEDIA_STATUS;
        lstEventMessage.m_mediaInfo.m_deviceType = btrMgr_MapDeviceTypeFromCore(lenBtrCoreDevCl);
        lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevVolume = aui8Volume;
        lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevMute   = aui8Mute;

        if (abMuted && (abMuted != lbMuted)) {
            lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd = BTRMGR_MEDIA_CTRL_MUTE;
            BTRMGRLOG_INFO ("Device Mute status changed, Set to Mute\n");
        }
        else if (!abMuted && (abMuted != lbMuted)) {
            lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd = BTRMGR_MEDIA_CTRL_UNMUTE;
            BTRMGRLOG_INFO ("Device Mute status changed, Set to Unmute\n");
        }
        else if (aui8Volume > lui8Volume) {
            lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd = BTRMGR_MEDIA_CTRL_VOLUMEUP;
            BTRMGRLOG_INFO ("Device Volume status changed, VolumeUp Prev Vol:%d Curr Vol:%d\n",lui8Volume, aui8Volume);
        }
        else if (aui8Volume < lui8Volume) {
            lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd = BTRMGR_MEDIA_CTRL_VOLUMEDOWN;
            BTRMGRLOG_INFO ("Device Volume status changed, VolumeDown Prev Vol:%d Curr Vol:%d\n",lui8Volume, aui8Volume);
        }
        else
            lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd = BTRMGR_MEDIA_CTRL_UNKNOWN;

        for (int j = 0; j <= gListOfPairedDevices.m_numOfDevices; j++) {
            if (ahBTRMgrDevHdl == gListOfPairedDevices.m_deviceProperty[j].m_deviceHandle) {
                lstEventMessage.m_mediaInfo.m_deviceHandle = ahBTRMgrDevHdl;
                strncpy(lstEventMessage.m_mediaInfo.m_name, gListOfPairedDevices.m_deviceProperty[j].m_name, BTRMGR_NAME_LEN_MAX -1);
                break;
            }
        }

        gfpcBBTRMgrEventOut(lstEventMessage); /*  Post a callback */
    }

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_GetDeviceDelay (
    unsigned char                aui8AdapterIdx,
    BTRMgrDeviceHandle           ahBTRMgrDevHdl,
    BTRMGR_DeviceOperationType_t deviceOpType,
    unsigned int*                    pui16Delay,
    unsigned int*                    pui16MsInBuffer
) {
    BTRMGR_Result_t       lenBtrMgrResult       = BTRMGR_RESULT_SUCCESS;
    eBTRMgrRet            lenBtrMgrRet          = eBTRMgrSuccess;
    enBTRCoreRet          lenBtrCoreRet         = enBTRCoreSuccess;
    enBTRCoreDeviceType   lenBtrCoreDevTy       = enBTRCoreUnknown;
    enBTRCoreDeviceClass  lenBtrCoreDevCl       = enBTRCore_DC_Unknown;
    unsigned int          lui16CurDelay         = 0;
    unsigned int          lui16CurMsInBuffer    = 0;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (!btrMgr_IsDevConnected(ahBTRMgrDevHdl) || (gstBTRMgrStreamingInfo.hBTRMgrSoHdl == NULL)) {
       BTRMGRLOG_ERROR ("Device Handle(%lld) not connected/streaming\n", ahBTRMgrDevHdl);
       return BTRMGR_RESULT_INVALID_INPUT;
    }

    // Currently implemented for audio out only
    if (deviceOpType != BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT) {
       BTRMGRLOG_ERROR ("Device Handle(%lld) not audio out\n", ahBTRMgrDevHdl);
       return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBtrCoreDevTy, &lenBtrCoreDevCl);
    BTRMGRLOG_INFO ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBtrCoreDevTy, lenBtrCoreDevCl);

    if ((lenBtrMgrRet = BTRMgr_SO_GetDelay(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, &lui16CurDelay, &lui16CurMsInBuffer)) != eBTRMgrSuccess) {
       BTRMGRLOG_ERROR ("Device Handle(%lld) audio out delay get fail\n", ahBTRMgrDevHdl);
       lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    if (lenBtrMgrRet == eBTRMgrSuccess)  {
        *pui16Delay = lui16CurDelay;
        *pui16MsInBuffer   = lui16CurMsInBuffer;
    }

    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_SetDeviceDelay (
    unsigned char                aui8AdapterIdx,
    BTRMgrDeviceHandle           ahBTRMgrDevHdl,
    BTRMGR_DeviceOperationType_t deviceOpType,
    unsigned int                     ui16Delay
) {
    BTRMGR_Result_t             lenBtrMgrResult     = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet                lenBtrCoreRet       = enBTRCoreSuccess;
    enBTRCoreDeviceType         lenBtrCoreDevTy     = enBTRCoreUnknown;
    enBTRCoreDeviceClass        lenBtrCoreDevCl     = enBTRCore_DC_Unknown;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (!btrMgr_IsDevConnected(ahBTRMgrDevHdl) || (gstBTRMgrStreamingInfo.hBTRMgrSoHdl == NULL)) {
        BTRMGRLOG_ERROR ("Device Handle(%lld) not connected/streaming\n", ahBTRMgrDevHdl);
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    // Currently implemented for audio out only
    if (deviceOpType != BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT) {
        BTRMGRLOG_ERROR ("Device Handle(%lld) not audio out\n", ahBTRMgrDevHdl);
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBtrCoreDevTy, &lenBtrCoreDevCl);
    BTRMGRLOG_TRACE ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBtrCoreDevTy, lenBtrCoreDevCl);

    BTRMgr_SO_SetDelay(gstBTRMgrStreamingInfo.hBTRMgrSoHdl, ui16Delay);
    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_GetMediaTrackInfo (
    unsigned char                aui8AdapterIdx,
    BTRMgrDeviceHandle           ahBTRMgrDevHdl,
    BTRMGR_MediaTrackInfo_t      *mediaTrackInfo
) {
    BTRMGR_Result_t              lenBtrMgrResult     = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet                 lenBtrCoreRet       = enBTRCoreSuccess;
    enBTRCoreDeviceType          lenBtrCoreDevTy     = enBTRCoreUnknown;
    enBTRCoreDeviceClass         lenBtrCoreDevCl     = enBTRCore_DC_Unknown;


    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (!btrMgr_IsDevConnected(ahBTRMgrDevHdl)) {
       BTRMGRLOG_ERROR ("Device Handle(%lld) not connected\n", ahBTRMgrDevHdl);
       return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBtrCoreDevTy, &lenBtrCoreDevCl);
    BTRMGRLOG_DEBUG ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBtrCoreDevTy, lenBtrCoreDevCl);

    if (enBTRCoreSuccess != BTRCore_GetMediaTrackInfo(ghBTRCoreHdl, ahBTRMgrDevHdl, lenBtrCoreDevTy, (stBTRCoreMediaTrackInfo*)mediaTrackInfo)) {
       BTRMGRLOG_ERROR ("Get Media Track Information for %llu Failed!!!\n", ahBTRMgrDevHdl);
       lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_GetMediaElementTrackInfo (
    unsigned char                aui8AdapterIdx,
    BTRMgrDeviceHandle           ahBTRMgrDevHdl,
    BTRMgrMediaElementHandle     ahBTRMgrMedElementHdl,
    BTRMGR_MediaTrackInfo_t      *mediaTrackInfo
) {
    BTRMGR_Result_t              lenBtrMgrResult     = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet                 lenBtrCoreRet       = enBTRCoreSuccess;
    enBTRCoreDeviceType          lenBtrCoreDevTy     = enBTRCoreUnknown;
    enBTRCoreDeviceClass         lenBtrCoreDevCl     = enBTRCore_DC_Unknown;


    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (!btrMgr_IsDevConnected(ahBTRMgrDevHdl)) {
       BTRMGRLOG_ERROR ("Device Handle(%lld) not connected\n", ahBTRMgrDevHdl);
       return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBtrCoreDevTy, &lenBtrCoreDevCl);
    BTRMGRLOG_DEBUG ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBtrCoreDevTy, lenBtrCoreDevCl);

    if (enBTRCoreSuccess != BTRCore_GetMediaElementTrackInfo(ghBTRCoreHdl, ahBTRMgrDevHdl,
                            lenBtrCoreDevTy, ahBTRMgrMedElementHdl, (stBTRCoreMediaTrackInfo*)mediaTrackInfo)) {
       BTRMGRLOG_ERROR ("Get Media Track Information for %llu Failed!!!\n", ahBTRMgrDevHdl);
       lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_GetMediaCurrentPosition (
    unsigned char                aui8AdapterIdx,
    BTRMgrDeviceHandle           ahBTRMgrDevHdl,
    BTRMGR_MediaPositionInfo_t  *mediaPositionInfo
) {
    BTRMGR_Result_t             lenBtrMgrResult     = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet                lenBtrCoreRet       = enBTRCoreSuccess;
    enBTRCoreDeviceType         lenBtrCoreDevTy     = enBTRCoreUnknown;
    enBTRCoreDeviceClass        lenBtrCoreDevCl     = enBTRCore_DC_Unknown;


    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (!btrMgr_IsDevConnected(ahBTRMgrDevHdl)) {
       BTRMGRLOG_ERROR ("Device Handle(%lld) not connected\n", ahBTRMgrDevHdl);
       return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBtrCoreDevTy, &lenBtrCoreDevCl);
    BTRMGRLOG_DEBUG ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBtrCoreDevTy, lenBtrCoreDevCl);

    if (enBTRCoreSuccess != BTRCore_GetMediaPositionInfo(ghBTRCoreHdl, ahBTRMgrDevHdl, lenBtrCoreDevTy, (stBTRCoreMediaPositionInfo*)mediaPositionInfo)) {
       BTRMGRLOG_ERROR ("Get Media Current Position for %llu Failed!!!\n", ahBTRMgrDevHdl);
       lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_SetMediaElementActive (
    unsigned char                aui8AdapterIdx,
    BTRMgrDeviceHandle           ahBTRMgrDevHdl,
    BTRMgrMediaElementHandle     ahBTRMgrMedElementHdl,
    BTRMGR_MediaElementType_t    aMediaElementType
) {
    BTRMGR_Result_t              lenBtrMgrResult     = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet                 lenBtrCoreRet       = enBTRCoreSuccess;
    enBTRCoreDeviceType          lenBtrCoreDevTy     = enBTRCoreUnknown;
    enBTRCoreDeviceClass         lenBtrCoreDevCl     = enBTRCore_DC_Unknown;
    eBTRCoreMedElementType       lenMediaElementType = enBTRCoreMedETypeUnknown;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (!btrMgr_IsDevConnected(ahBTRMgrDevHdl)) {
       BTRMGRLOG_ERROR ("Device Handle(%lld) not connected\n", ahBTRMgrDevHdl);
       return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBtrCoreDevTy, &lenBtrCoreDevCl);
    BTRMGRLOG_DEBUG ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBtrCoreDevTy, lenBtrCoreDevCl);

    switch (aMediaElementType) {
    case BTRMGR_MEDIA_ELEMENT_TYPE_ALBUM:
        lenMediaElementType    = enBTRCoreMedETypeAlbum;
        break;
    case BTRMGR_MEDIA_ELEMENT_TYPE_ARTIST:
        lenMediaElementType    = enBTRCoreMedETypeArtist;
        break;
    case BTRMGR_MEDIA_ELEMENT_TYPE_GENRE:
        lenMediaElementType    = enBTRCoreMedETypeGenre;
        break;
    case BTRMGR_MEDIA_ELEMENT_TYPE_COMPILATIONS:
        lenMediaElementType    = enBTRCoreMedETypeCompilation;
        break;
    case BTRMGR_MEDIA_ELEMENT_TYPE_PLAYLIST:
        lenMediaElementType    = enBTRCoreMedETypePlayList;
        break;
    case BTRMGR_MEDIA_ELEMENT_TYPE_TRACKLIST:
        lenMediaElementType    = enBTRCoreMedETypeTrackList;
        break;
    case BTRMGR_MEDIA_ELEMENT_TYPE_TRACK:
        lenMediaElementType    = enBTRCoreMedETypeTrack;
        break;
    default:
        break;
    }

    /* Add intelligence to prefetch MediaElementInfo based on users' interest/navigation */

    if (enBTRCoreSuccess != BTRCore_SetMediaElementActive (ghBTRCoreHdl,
                                                           ahBTRMgrDevHdl,
                                                           ahBTRMgrMedElementHdl,
                                                           lenBtrCoreDevTy,
                                                           lenMediaElementType)) {
       BTRMGRLOG_ERROR ("Set Active Media Element(%llu) List for Dev %llu Failed!!!\n", ahBTRMgrMedElementHdl, ahBTRMgrDevHdl);
       lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_GetMediaElementList (
    unsigned char                   aui8AdapterIdx,
    BTRMgrDeviceHandle              ahBTRMgrDevHdl,
    BTRMgrMediaElementHandle        ahBTRMgrMedElementHdl,
    unsigned short                  aui16MediaElementStartIdx,
    unsigned short                  aui16MediaElementEndIdx,
    unsigned char                   abMediaElementListDepth,
    BTRMGR_MediaElementType_t       aMediaElementType,
    BTRMGR_MediaElementListInfo_t*  aMediaElementListInfo
) {
    BTRMGR_Result_t               lenBtrMgrResult     = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet                  lenBtrCoreRet       = enBTRCoreSuccess;
    enBTRCoreDeviceType           lenBtrCoreDevTy     = enBTRCoreUnknown;
    enBTRCoreDeviceClass          lenBtrCoreDevCl     = enBTRCore_DC_Unknown;
    stBTRCoreMediaElementInfoList lpstBTRCoreMediaElementInfoList;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt() || !aMediaElementListInfo ||
        aui16MediaElementStartIdx > aui16MediaElementEndIdx) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (aui16MediaElementEndIdx - aui16MediaElementStartIdx > BTRMGR_MEDIA_ELEMENT_COUNT_MAX -1) {
        aui16MediaElementEndIdx = aui16MediaElementStartIdx + BTRMGR_MEDIA_ELEMENT_COUNT_MAX -1;
    }

    if (!btrMgr_IsDevConnected(ahBTRMgrDevHdl)) {
       BTRMGRLOG_ERROR ("Device Handle(%lld) not connected\n", ahBTRMgrDevHdl);
       return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBtrCoreDevTy, &lenBtrCoreDevCl);
    BTRMGRLOG_DEBUG ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBtrCoreDevTy, lenBtrCoreDevCl);

    MEMSET_S(&lpstBTRCoreMediaElementInfoList, sizeof(stBTRCoreMediaElementInfoList), 0, sizeof(stBTRCoreMediaElementInfoList));

    /* TODO Retrive required List from the populated DataBase */
    /* In below passing argument aMediaElementType is not transitioning now due to
     * BTRMGR_MediaElementType_t and eBTRCoreMedElementType both enums are same.
     * In future if change the any enums it will impact */
    if (enBTRCoreSuccess != BTRCore_GetMediaElementList (ghBTRCoreHdl,
                                                         ahBTRMgrDevHdl,
                                                         ahBTRMgrMedElementHdl,
                                                         aui16MediaElementStartIdx,
                                                         aui16MediaElementEndIdx,
                                                         lenBtrCoreDevTy,
                                                         aMediaElementType,
                                                         &lpstBTRCoreMediaElementInfoList)) {
        BTRMGRLOG_ERROR ("Get Media Element(%llu) List for Dev %llu Failed!!!\n", ahBTRMgrMedElementHdl, ahBTRMgrDevHdl);
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        unsigned short               ui16LoopIdx = 0;
        stBTRCoreMediaElementInfo*   srcElement  = 0;
        BTRMGR_MediaElementInfo_t*   desElement  = 0;

        if (lpstBTRCoreMediaElementInfoList.m_numOfElements > BTRMGR_MEDIA_ELEMENT_COUNT_MAX) {
            lpstBTRCoreMediaElementInfoList.m_numOfElements = BTRMGR_MEDIA_ELEMENT_COUNT_MAX;
        }

        while (ui16LoopIdx < lpstBTRCoreMediaElementInfoList.m_numOfElements) {
            srcElement = &lpstBTRCoreMediaElementInfoList.m_mediaElementInfo[ui16LoopIdx];
            desElement = &aMediaElementListInfo->m_mediaElementInfo[ui16LoopIdx++];

            desElement->m_mediaElementHdl  = srcElement->ui32MediaElementId;
            desElement->m_IsPlayable       = srcElement->bIsPlayable;
            strncpy (desElement->m_mediaElementName, srcElement->m_mediaElementName, BTRMGR_MAX_STR_LEN -1);
            MEMCPY_S(&desElement->m_mediaTrackInfo, sizeof(desElement->m_mediaTrackInfo), &srcElement->m_mediaTrackInfo, sizeof(BTRMGR_MediaTrackInfo_t));
        }
        aMediaElementListInfo->m_numberOfElements   = lpstBTRCoreMediaElementInfoList.m_numOfElements;
    }

    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_SelectMediaElement (
    unsigned char                aui8AdapterIdx,
    BTRMgrDeviceHandle           ahBTRMgrDevHdl,
    BTRMgrMediaElementHandle     ahBTRMgrMedElementHdl,
    BTRMGR_MediaElementType_t    aMediaElementType
) {
    BTRMGR_Result_t              lenBtrMgrResult     = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet                 lenBtrCoreRet       = enBTRCoreSuccess;
    enBTRCoreDeviceType          lenBtrCoreDevTy     = enBTRCoreUnknown;
    enBTRCoreDeviceClass         lenBtrCoreDevCl     = enBTRCore_DC_Unknown;
    eBTRCoreMedElementType       lenMediaElementType = enBTRCoreMedETypeUnknown;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (!btrMgr_IsDevConnected(ahBTRMgrDevHdl)) {
       BTRMGRLOG_ERROR ("Device Handle(%lld) not connected\n", ahBTRMgrDevHdl);
       return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, ahBTRMgrDevHdl, &lenBtrCoreDevTy, &lenBtrCoreDevCl);
    BTRMGRLOG_DEBUG ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBtrCoreDevTy, lenBtrCoreDevCl);
   

    switch (aMediaElementType) {
    case BTRMGR_MEDIA_ELEMENT_TYPE_ALBUM:
        lenMediaElementType    = enBTRCoreMedETypeAlbum;
        break;
    case BTRMGR_MEDIA_ELEMENT_TYPE_ARTIST:
        lenMediaElementType    = enBTRCoreMedETypeArtist;
        break;
    case BTRMGR_MEDIA_ELEMENT_TYPE_GENRE:
        lenMediaElementType    = enBTRCoreMedETypeGenre;
        break;
    case BTRMGR_MEDIA_ELEMENT_TYPE_COMPILATIONS:
        lenMediaElementType    = enBTRCoreMedETypeCompilation;
        break;
    case BTRMGR_MEDIA_ELEMENT_TYPE_PLAYLIST:
        lenMediaElementType    = enBTRCoreMedETypePlayList;
        break;
    case BTRMGR_MEDIA_ELEMENT_TYPE_TRACKLIST:
        lenMediaElementType    = enBTRCoreMedETypeTrackList;
        break;
    case BTRMGR_MEDIA_ELEMENT_TYPE_TRACK:
        lenMediaElementType    = enBTRCoreMedETypeTrack;
        break;
    default:
        break;
    }

    if (enBTRCoreSuccess != BTRCore_SelectMediaElement (ghBTRCoreHdl,
                                                        ahBTRMgrDevHdl,
                                                        ahBTRMgrMedElementHdl,
                                                        lenBtrCoreDevTy,
                                                        lenMediaElementType)) {
       BTRMGRLOG_ERROR ("Select Media Element(%llu) on Dev %llu Failed!!!\n", ahBTRMgrMedElementHdl, ahBTRMgrDevHdl);
       lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return lenBtrMgrResult;
}
#endif


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
        return "XBB";
    else
        return "UNKNOWN DEVICE";
}

#ifndef LE_MODE
BTRMGR_Result_t
BTRMGR_SetAudioInServiceState (
    unsigned char   aui8AdapterIdx,
    unsigned char   aui8State
) {
    if ((gIsAudioInEnabled = aui8State)) {
        BTRMGRLOG_INFO ("AudioIn Service is Enabled.\n");
    }
    else {
        BTRMGRLOG_INFO ("AudioIn Service is Disabled.\n");
    }
    return BTRMGR_RESULT_SUCCESS;
}

BTRMGR_Result_t
BTRMGR_SetHidGamePadServiceState (
    unsigned char   aui8AdapterIdx,
    unsigned char   aui8State
) {
    if ((gIsHidGamePadEnabled = aui8State)) {
        BTRMGRLOG_INFO ("HID GamePad Service is Enabled.\n");
    }
    else {
        BTRMGRLOG_INFO ("HID GamePad Service is Disabled.\n");
    }
    return BTRMGR_RESULT_SUCCESS;
}

BTRMGR_Result_t
BTRMGR_SetBtmgrDebugModeState (
    unsigned char   aui8State
) {
    struct stat pstDebugDirInfo = {0};
    if ((gDebugModeEnabled = aui8State)) {
        BTRMGRLOG_INFO ("Debug mode is Enabled!\n");
        if (stat(BTRMGR_DEBUG_DIRECTORY, &pstDebugDirInfo) == -1)
        {
            mkdir(BTRMGR_DEBUG_DIRECTORY, 0775); 
        }
        
        //start pcaps and increase logging to trace
        BTRMGRLOG_INFO ("Starting packet capture\n");
        btrMgr_StartPacketCapture(BTRMGR_DEBUG_DIRECTORY);
        btrMgr_StartHidEventMonitor();
#ifndef BUILD_FOR_PI
        btrMgr_SetProcessLogLevel(BTMGR_PROCESS_NAME, RDK_LOGGER_BTMGR_NAME, LOG_LEVEL_TRACE);
        btrMgr_SetProcessLogLevel(BTMGR_PROCESS_NAME, RDK_LOGGER_BTCORE_NAME, LOG_LEVEL_TRACE);
#endif  //BUILD_FOR_PI
            }
    else {
        BTRMGRLOG_INFO ("Debug mode is Disabled.\n");
        //stop pcaps and increase log level
        btrMgr_StopPacketCapture();
        btrMgr_StopHidEventMonitor();
#ifndef BUILD_FOR_PI
        btrMgr_SetProcessLogLevel(BTMGR_PROCESS_NAME, RDK_LOGGER_BTMGR_NAME, LOG_LEVEL_DEBUG);
        btrMgr_SetProcessLogLevel(BTMGR_PROCESS_NAME, RDK_LOGGER_BTCORE_NAME, LOG_LEVEL_DEBUG);
#endif  //BUILD_FOR_PI

        
    }
    return BTRMGR_RESULT_SUCCESS;
}
#endif

BTRMGR_Result_t
BTRMGR_GetLimitBeaconDetection (
    unsigned char   aui8AdapterIdx,
    unsigned char   *pLimited
) {

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt())) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    BTRMGR_Beacon_PersistentData_t BeaconPersistentData;
    if (BTRMgr_PI_GetLEBeaconLimitingStatus(&BeaconPersistentData) == eBTRMgrFailure) {
        BTRMGRLOG_INFO ("Failed to get limit for beacon detection from json.\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    if (pLimited != NULL) {
        *pLimited =  (unsigned char)((!strncasecmp(BeaconPersistentData.limitBeaconDetection, "true", 4)) ? 1 : 0 );
        BTRMGRLOG_INFO ("the beacon detection detection : %s\n", *pLimited? "true":"false");
    }  //CID:45164 - Reverse_inull
    else {
        BTRMGRLOG_INFO ("Failed to get limit for beacon detection.\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return BTRMGR_RESULT_SUCCESS;
}

BTRMGR_Result_t
BTRMGR_SetLimitBeaconDetection (
    unsigned char   aui8AdapterIdx,
    unsigned char   limited
) {
    errno_t rc = -1;
    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt())) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    BTRMGR_Beacon_PersistentData_t BeaconPersistentData;
    rc = sprintf_s(BeaconPersistentData.limitBeaconDetection, sizeof(BeaconPersistentData.limitBeaconDetection), "%s", limited ? "true" : "false");
    if (rc < EOK) {
        ERR_CHK(rc);
    }

    if (BTRMgr_PI_SetLEBeaconLimitingStatus(&BeaconPersistentData) == eBTRMgrFailure) {
        BTRMGRLOG_ERROR ("Failed to set limit for beacon detection from json.\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    if (limited) {
        BTRMGRLOG_INFO ("Limiting the beacon detection.\n");
     }
     else {
        BTRMGRLOG_INFO ("Removing the limit for beacon detection\n");
     }
    return BTRMGR_RESULT_SUCCESS;
}

STATIC eBTRMgrRet
btrMgr_SetLastConnectionStatus (
    unsigned char   aui8AdapterIdx,
    int   ConStatus,
    BTRMgrDeviceHandle deviceID,
    const char *ProfileStr
) {
   if ((aui8AdapterIdx > btrMgr_GetAdapterCnt())) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return eBTRMgrFailInArg;
    }

    BTRMGR_ConStatus_PersistentData_t ConPersistentData;
    ConPersistentData.ConStatus = ConStatus;

    if (BTRMgr_PI_SetConnectionStatus(&ConPersistentData,ProfileStr,deviceID) == eBTRMgrFailure) {
        BTRMGRLOG_ERROR ("Failed to set con status from json.\n");
        return eBTRMgrFailure;
    }

    BTRMGRLOG_TRACE ("set conn status %d.\n",ConStatus);
    return eBTRMgrSuccess;

}

#ifdef RDKTV_PERSIST_VOLUME
STATIC eBTRMgrRet
btrMgr_SetLastVolume (
    unsigned char   aui8AdapterIdx,
    unsigned char   volume,
    BTRMgrDeviceHandle deviceID,
    const char *ProfileStr
) {
    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt())) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return eBTRMgrFailInArg;
    }

    BTRMGR_Volume_PersistentData_t VolumePersistentData;
    VolumePersistentData.Volume =  volume;
    if (BTRMgr_PI_SetVolume(&VolumePersistentData,ProfileStr,deviceID) == eBTRMgrFailure) {
        BTRMGRLOG_ERROR ("Failed to set volume from json.\n");
        return eBTRMgrFailure;
    }

    BTRMGRLOG_TRACE ("set volume %d.\n",volume);
    return eBTRMgrSuccess;
}

STATIC eBTRMgrRet
btrMgr_GetLastVolume (
    unsigned char   aui8AdapterIdx,
    unsigned char   *pVolume,
    BTRMgrDeviceHandle deviceID,
    const char *ProfileStr
) {

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || !(pVolume)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return eBTRMgrFailInArg;
    }

    BTRMGR_Volume_PersistentData_t VolumePersistentData;
    if (BTRMgr_PI_GetVolume(&VolumePersistentData,ProfileStr,deviceID) == eBTRMgrFailure) {
        BTRMGRLOG_INFO ("Failed to get volume detection from json.\n");
        return eBTRMgrFailure;
    }

    *pVolume = VolumePersistentData.Volume;
    BTRMGRLOG_TRACE ("get volume %d \n",*pVolume);
    return eBTRMgrSuccess;
}

STATIC eBTRMgrRet
btrMgr_SetLastMuteState (
    unsigned char   aui8AdapterIdx,
    gboolean        muted
) {
    errno_t rc = -1;
    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt())) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return eBTRMgrFailInArg;
    }

    BTRMGR_Mute_PersistentData_t MutePersistentData;
    rc = sprintf_s(MutePersistentData.Mute, sizeof(MutePersistentData.Mute), "%s", muted ? "true" : "false");
    if (rc < EOK) {
        ERR_CHK(rc);
    }

    if (BTRMgr_PI_SetMute(&MutePersistentData) == eBTRMgrFailure) {
        BTRMGRLOG_ERROR ("Failed to set mute from json.\n");
        return eBTRMgrFailure;
    }

    BTRMGRLOG_TRACE (" set mute %s.\n", muted ? "true":"false");
    return eBTRMgrSuccess;
}

STATIC eBTRMgrRet
btrMgr_GetLastMuteState (
    unsigned char   aui8AdapterIdx,
    gboolean        *pMute
) {

    if ((aui8AdapterIdx > btrMgr_GetAdapterCnt()) || !(pMute)) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return eBTRMgrFailInArg;
    }

    BTRMGR_Mute_PersistentData_t MutePersistentData;
    if (BTRMgr_PI_GetMute(&MutePersistentData) == eBTRMgrFailure) {
        BTRMGRLOG_INFO ("Failed to get mute detection from json.\n");
        return eBTRMgrFailure;
    }

    *pMute =  (unsigned char)((!strncasecmp(MutePersistentData.Mute, "true", 4)) ? 1 : 0 );
    BTRMGRLOG_TRACE (" get mute %s.\n", *pMute ? "true":"false");
    return eBTRMgrSuccess;
}
#endif

BTRMGR_Result_t
BTRMGR_GetLeProperty (
    unsigned char                aui8AdapterIdx,
    BTRMgrDeviceHandle           ahBTRMgrDevHdl,
    const char*                  apBtrPropUuid,
    BTRMGR_LeProperty_t          aenLeProperty,
    void*                        vpPropValue
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt() || !apBtrPropUuid) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (eBTRMgrSuccess != btrMgr_PreCheckDiscoveryStatus(aui8AdapterIdx, BTRMGR_DEVICE_OP_TYPE_LE)) {
        BTRMGRLOG_ERROR ("Pre Check Discovery State Rejected !!!\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    enBTRCoreLeProp lenBTRCoreLeProp = enBTRCoreLePropUnknown;

    switch(aenLeProperty) {
    case BTRMGR_LE_PROP_UUID:
        lenBTRCoreLeProp = enBTRCoreLePropGUUID;
        break;
    case BTRMGR_LE_PROP_PRIMARY:
        lenBTRCoreLeProp = enBTRCoreLePropGPrimary;
        break;
    case BTRMGR_LE_PROP_DEVICE:
        lenBTRCoreLeProp = enBTRCoreLePropGDevice;
        break;
    case BTRMGR_LE_PROP_SERVICE:
        lenBTRCoreLeProp = enBTRCoreLePropGService;
        break;
    case BTRMGR_LE_PROP_VALUE:
        lenBTRCoreLeProp = enBTRCoreLePropGValue;
        break;
    case BTRMGR_LE_PROP_NOTIFY:
        lenBTRCoreLeProp = enBTRCoreLePropGNotifying;
        break;
    case BTRMGR_LE_PROP_FLAGS:
        lenBTRCoreLeProp = enBTRCoreLePropGFlags;
        break;
    case BTRMGR_LE_PROP_CHAR:
        lenBTRCoreLeProp = enBTRCoreLePropGChar;
        break;
    default:
        break;
    }

    if (enBTRCoreSuccess != BTRCore_GetLEProperty(ghBTRCoreHdl, ahBTRMgrDevHdl, apBtrPropUuid, lenBTRCoreLeProp, vpPropValue)) {
       BTRMGRLOG_ERROR ("Get LE Property %d for Device/UUID  %llu/%s Failed!!!\n", lenBTRCoreLeProp, ahBTRMgrDevHdl, apBtrPropUuid);
       lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_PerformLeOp (
    unsigned char                aui8AdapterIdx,
    BTRMgrDeviceHandle           ahBTRMgrDevHdl,
    const char*                  aBtrLeUuid,
    BTRMGR_LeOp_t                aLeOpType,
    char*                        aLeOpArg,
    char*                        rOpResult
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    BTRMGR_ConnectedDevicesList_t listOfCDevices;
    unsigned char isConnected  = 0;
    unsigned short ui16LoopIdx = 0;
    enBTRCoreLeOp aenBTRCoreLeOp =  enBTRCoreLeOpUnknown;


    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR ("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt() || !aBtrLeUuid) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    if (eBTRMgrSuccess != btrMgr_PreCheckDiscoveryStatus(aui8AdapterIdx, BTRMGR_DEVICE_OP_TYPE_LE)) {
        BTRMGRLOG_ERROR ("Pre Check Discovery State Rejected !!!\n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    /* Check whether the device is in the Connected list */ 
    if(!BTRMGR_GetConnectedDevices (aui8AdapterIdx, &listOfCDevices)) {
        BTRMGRLOG_TRACE ("BTRMGR_GetConnectedDevices is error !!!\n");
    }  //CID:95101 - Checked return

    for ( ;ui16LoopIdx < listOfCDevices.m_numOfDevices; ui16LoopIdx++) {
        if (listOfCDevices.m_deviceProperty[ui16LoopIdx].m_deviceHandle == ahBTRMgrDevHdl) {
            isConnected = listOfCDevices.m_deviceProperty[ui16LoopIdx].m_isConnected;
            break;
        }
    }

    if (!isConnected) {
#if 0
        //TODO: Check if ahBTRMgrDevHdl corresponds to the current adapter
        BTRMGRLOG_ERROR ("LE Device %lld is not connected to perform LE Op!!!\n", ahBTRMgrDevHdl);
        return BTRMGR_RESULT_GENERIC_FAILURE;
#endif
    }


    switch (aLeOpType) {
    case BTRMGR_LE_OP_READY:
        aenBTRCoreLeOp = enBTRCoreLeOpGReady;
        break;
    case BTRMGR_LE_OP_READ_VALUE:
        aenBTRCoreLeOp = enBTRCoreLeOpGReadValue;
        break;
    case BTRMGR_LE_OP_WRITE_VALUE:
        aenBTRCoreLeOp = enBTRCoreLeOpGWriteValue;
        break;
    case BTRMGR_LE_OP_START_NOTIFY:
        aenBTRCoreLeOp = enBTRCoreLeOpGStartNotify;
        break;
    case BTRMGR_LE_OP_STOP_NOTIFY:
        aenBTRCoreLeOp = enBTRCoreLeOpGStopNotify;
        break;
    case BTRMGR_LE_OP_UNKNOWN:
    default:
        aenBTRCoreLeOp = enBTRCoreLeOpGReady;
        break;
    }

    if (enBTRCoreSuccess != BTRCore_PerformLEOp (ghBTRCoreHdl, ahBTRMgrDevHdl, aBtrLeUuid, aenBTRCoreLeOp, aLeOpArg, rOpResult)) {
       BTRMGRLOG_ERROR ("Perform LE Op %d for device  %llu Failed!!!\n", aLeOpType, ahBTRMgrDevHdl);
       lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_LE_StartAdvertisement (
    unsigned char                   aui8AdapterIdx,
    BTRMGR_LeCustomAdvertisement_t* pstBTMGR_LeCustomAdvt
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

    char *lAdvtType = "peripheral";
    int lLenServiceUUID = 0;
    int lComcastFlagType = 0;
    int lLenManfData = 0;
    unsigned short lManfId = 0;
    unsigned char laDeviceDetails[BTRMGR_DEVICE_COUNT_MAX] = {'\0'};
    unsigned char lManfType;
    char lAdvtBeaconName[BTRMGR_MAX_STR_LEN + 20] = {'\0'};
    char lPropertyValue[BTRMGR_MAX_STR_LEN] = {'\0'};
    char lDefaultBeaconName[BTRMGR_STR_LEN] = {'\0'};

#ifndef BUILD_FOR_PI
    RFC_ParamData_t param = {0};
    WDMP_STATUS status = getRFCParameter("BTRMGR", "Device.DeviceInfo.ProductClass", &param);
    if (status == WDMP_SUCCESS)
    {
        BTRMGRLOG_DEBUG("name = %s, type = %d, value = %s\n", param.name, param.type, param.value);
        snprintf(lDefaultBeaconName, BTRMGR_STR_LEN, "%.16s", param.value);
    }
    else
    {
        BTRMGRLOG_ERROR("getRFCParameter Failed : %s\n", getRFCErrorString(status));
    }
#else // BUILD_FOR_PI - certain RDKB or RPI builds
        FILE * fp = NULL;
        //if no beacon name given, try and use Product class to replace it
        fp = popen("dmcli eRT getv Device.DeviceInfo.ProductClass | grep value | awk '{print $5}'", "r");
        if (fp){
            fgets(lDefaultBeaconName, sizeof(lDefaultBeaconName) - 1, fp);
            BTRMGRLOG_DEBUG("Name = %s\n", lDefaultBeaconName);
            BTRMGRLOG_WARN("No Beacon Name set, use default name %s\n", lDefaultBeaconName);
            pclose(fp);
        }
        else
        {
            BTRMGRLOG_ERROR("Could not default beacon name to Product Class\n");
        }
#endif

    if (BTRMGR_SysDiagInfo(0, BTRMGR_SYSTEM_ID_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE) == BTRMGR_RESULT_SUCCESS)
    {
       if (!lPropertyValue[0])
       {
            BTRMGRLOG_WARN("Empty CM Mac address, use default beacon Name. \n");
            snprintf(lAdvtBeaconName, BTRMGR_MAX_STR_LEN + 20, "%.8s", lDefaultBeaconName);
       }
       else
       {
            btrMgr_RemoveChars(lPropertyValue, ':');
            btrMgr_SetCMMac(pstBTMGR_LeCustomAdvt->device_mac, lPropertyValue);
            snprintf(lAdvtBeaconName, BTRMGR_MAX_STR_LEN + 20, "%.8s_%s", lDefaultBeaconName, lPropertyValue);
       }
    }
    else
    {
       BTRMGRLOG_WARN("Unable to get CM Mac address, use default beacon Name. \n");
       snprintf(lAdvtBeaconName, BTRMGR_MAX_STR_LEN + 20, "%.8s", lDefaultBeaconName);
    }

    /* Parse the adv data  */

    /* Set advertisement Info */
    BTRMGRLOG_INFO("Adv Type : %s, Adv name :%s \n", lAdvtType, lAdvtBeaconName);
    BTRCore_SetAdvertisementInfo(ghBTRCoreHdl, lAdvtType, lAdvtBeaconName);

    /* Parse and set the services supported by the device */
    lLenServiceUUID = (int)pstBTMGR_LeCustomAdvt->len_comcastflags;

    lComcastFlagType = (int)pstBTMGR_LeCustomAdvt->type_comcastflags;
    lLenServiceUUID -= 1;

    if ((2 == lComcastFlagType) || (3 == lComcastFlagType)) { /* TODO use macro/enum */
        char lUUID[10];
        unsigned short lu16UUID = ((unsigned short)pstBTMGR_LeCustomAdvt->deviceInfo_UUID_HI << 8) | (unsigned short)pstBTMGR_LeCustomAdvt->deviceInfo_UUID_LO;
        snprintf(lUUID, sizeof(lUUID), "%x", lu16UUID);
        if (enBTRCoreSuccess != BTRCore_SetServiceUUIDs(ghBTRCoreHdl, lUUID)) {
            lenBtrMgrResult = BTRMGR_RESULT_INVALID_INPUT;
        }

#ifndef RBUS_RPC_ENABLED
        lu16UUID = ((unsigned short)pstBTMGR_LeCustomAdvt->rdk_diag_UUID_HI << 8) | (unsigned short)pstBTMGR_LeCustomAdvt->rdk_diag_UUID_LO;
        snprintf(lUUID, sizeof(lUUID), "%x", lu16UUID);
        if (enBTRCoreSuccess != BTRCore_SetServiceUUIDs(ghBTRCoreHdl, lUUID)) {
            lenBtrMgrResult = BTRMGR_RESULT_INVALID_INPUT;
        }
#endif
    }

    /* Parse and set the manufacturer specific data for the device */
    lLenManfData = (int)pstBTMGR_LeCustomAdvt->len_manuf;
    BTRMGRLOG_INFO("Length of manf data is %d \n", lLenManfData);

    lManfType = pstBTMGR_LeCustomAdvt->type_manuf;
    lLenManfData -= sizeof(lManfType);

    lManfId = ((unsigned short)pstBTMGR_LeCustomAdvt->company_HI << 8) | ((unsigned short)pstBTMGR_LeCustomAdvt->company_LO);
    lLenManfData -= sizeof(lManfId);
    BTRMGRLOG_INFO("manf id is %d \n", lManfId);

    int index = 0;
    laDeviceDetails[index + 1] = pstBTMGR_LeCustomAdvt->device_model & 0xF;
    laDeviceDetails[index] = (pstBTMGR_LeCustomAdvt->device_model >> 8) & 0xF;
    index += sizeof(pstBTMGR_LeCustomAdvt->device_model);
#ifndef RBUS_RPC_ENABLED
    for (int count = 0; count < BTRMGR_DEVICE_MAC_LEN; count++) {
        laDeviceDetails[index] = pstBTMGR_LeCustomAdvt->device_mac[count];
        index += 1;
    }
#else
    for (int count = 0; count < BTRMGR_SERIAL_NUM_LEN; count++) {
         laDeviceDetails[index] = pstBTMGR_LeCustomAdvt->serial_number[count];
         index += 1;
    }

#endif
    BTRCore_SetManufacturerData(ghBTRCoreHdl, lManfId, laDeviceDetails, lLenManfData);

    /* Set the Tx Power */
    BTRCore_SetEnableTxPower(ghBTRCoreHdl, TRUE);

    gIsAdvertisementSet = TRUE;

    /* Add standard advertisement Gatt Info */
    //btrMgr_AddStandardAdvGattInfo();

    /* Start advertising */
    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR("BTRCore is not Inited\n");
        lenBtrMgrResult = BTRMGR_RESULT_INIT_FAILED;
    }
    else if (FALSE == gIsAdvertisementSet) {
        BTRMGRLOG_ERROR("Advertisement data has not been set\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else if (TRUE == gIsDeviceAdvertising) {
        BTRMGRLOG_ERROR("Device is already advertising\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        /* start advertisement */
        if (enBTRCoreSuccess != BTRCore_StartAdvertisement(ghBTRCoreHdl)) {
            BTRMGRLOG_ERROR("Starting advertisement has failed\n");
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }
        else {
            BTRMGRLOG_INFO("Device is advertising\n");
            gIsDeviceAdvertising = TRUE;
        }
    }

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_LE_StopAdvertisement (
    unsigned char aui8AdapterIdx
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR("BTRCore is not Inited\n");
        lenBtrMgrResult = BTRMGR_RESULT_INIT_FAILED;
    }
    else if (FALSE == gIsDeviceAdvertising) {
        BTRMGRLOG_ERROR("Device is not advertising yet\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        /* stop advertisement */
        BTRMGRLOG_INFO("Stopping advertisement\n");
        if (enBTRCoreSuccess != BTRCore_StopAdvertisement(ghBTRCoreHdl)) {
            BTRMGRLOG_ERROR("Stopping advertisement has failed\n");
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }

#ifdef LE_MODE
        BTRMGR_LE_RemoveServiceInfo(0, BTRMGR_DEVICE_INFORMATION_UUID);
        if (gCellularModemIsOnline == TRUE) {
            BTRMGR_LE_RemoveServiceInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP);
        }
#endif

        gIsDeviceAdvertising = FALSE;
    }

    return lenBtrMgrResult;
}

STATIC BTRMGR_Result_t
BTRMGR_LE_ReleaseAdvertisement (
    unsigned char aui8AdapterIdx
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

        /* release advertisement */
        BTRMGRLOG_INFO("Releasing advertisement\n");
        if (enBTRCoreSuccess != BTRCore_ReleaseAdvertisement(ghBTRCoreHdl)) {
            BTRMGRLOG_ERROR("Releasing advertisement has failed\n");
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }

#ifdef LE_MODE
        BTRMGR_LE_RemoveServiceInfo(0, BTRMGR_DEVICE_INFORMATION_UUID);
        if (gCellularModemIsOnline == TRUE) {
            BTRMGR_LE_RemoveServiceInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP);
        }
#endif

        gIsDeviceAdvertising = FALSE;

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_LE_GetPropertyValue (
    unsigned char       aui8AdapterIdx,
    char*               aUUID,
    char*               aValue,
    BTRMGR_LeProperty_t aElement
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    char lPropValue[BTRMGR_MAX_STR_LEN] = "";

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR("BTRCore is not Inited\n");
        lenBtrMgrResult = BTRMGR_RESULT_INIT_FAILED;
    }
    else if (FALSE == gIsDeviceAdvertising) {
        BTRMGRLOG_ERROR("Device is not advertising yet\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        if (enBTRCoreSuccess != BTRCore_GetPropertyValue(ghBTRCoreHdl, aUUID, lPropValue, aElement)) {
            BTRMGRLOG_ERROR("Getting property value has failed\n");
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }
        else {
            strncpy(aValue, lPropValue, (BTRMGR_MAX_STR_LEN - 1));
        }
    }

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_LE_SetServiceUUIDs (
    unsigned char   aui8AdapterIdx,
    char*           aUUID
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;


    if (enBTRCoreSuccess != BTRCore_SetServiceUUIDs(ghBTRCoreHdl, aUUID)) {
        lenBtrMgrResult = BTRMGR_RESULT_INVALID_INPUT;
    }

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_LE_SetServiceInfo (
    unsigned char   aui8AdapterIdx,
    char*           aUUID,
    unsigned char   aServiceType
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

    BTRMGRLOG_INFO("Calling BTRCore_SetServiceInfo\n");
    if (enBTRCoreSuccess != BTRCore_SetServiceInfo(ghBTRCoreHdl, aUUID, (BOOLEAN)aServiceType)) {
        BTRMGRLOG_ERROR("Could not add service info\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return lenBtrMgrResult;
}

#ifdef LE_MODE
static BTRMGR_Result_t
BTRMGR_LE_RemoveServiceInfo(
    unsigned char aui8AdapterIdx,
    char *aUUID) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

    BTRMGRLOG_INFO("Calling BTRCore_RemoveServiceInfo\n");
    if (enBTRCoreSuccess != BTRCore_RemoveServiceInfo(ghBTRCoreHdl, aUUID)) {
        BTRMGRLOG_ERROR("Could not remove service info\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return lenBtrMgrResult;
}
#endif

BTRMGR_Result_t
BTRMGR_LE_SetGattInfo (
    unsigned char       aui8AdapterIdx,
    char*               aParentUUID,
    char*               aCharUUID,
    unsigned short      aFlags,
    char*               aValue,
    BTRMGR_LeProperty_t aElement
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

    BTRMGRLOG_INFO("Calling BTRCore_SetGatInfo\n");
    if (enBTRCoreSuccess != BTRCore_SetGattInfo(ghBTRCoreHdl, aParentUUID, aCharUUID, aFlags, aValue, aElement)) {
        BTRMGRLOG_ERROR("Could not add gatt info\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }

    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_LE_SetGattPropertyValue (
    unsigned char       aui8AdapterIdx,
    char*               aUUID,
    char*               aValue,
    BTRMGR_LeProperty_t aElement
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR("BTRCore is not Inited\n");
        lenBtrMgrResult = BTRMGR_RESULT_INIT_FAILED;
    }
    else if (FALSE == gIsDeviceAdvertising) {
        BTRMGRLOG_ERROR("Device is not advertising yet\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    else {
        BTRMGRLOG_INFO("Value is %s\n", aValue);

#if 1
        if (enBTRCoreSuccess != BTRCore_SetPropertyValue(ghBTRCoreHdl, aUUID, aValue, aElement)) {
            BTRMGRLOG_ERROR("Setting property value has failed\n");
            lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        }
#else
        BTRCore_PerformLEOp
#endif
    }

    return lenBtrMgrResult;
}


BTRMGR_Result_t
BTRMGR_SysDiagInfo(
    unsigned char aui8AdapterIdx,
    char *apDiagElement,
    char *apValue,
    BTRMGR_LeOp_t aOpType
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    int lenDiagElement = 0;
    char lPropValue[BTRMGR_LE_STR_LEN_MAX] = {'\0'};   //CID:135225 - Overrurn

    if (!ghBTRCoreHdl) {
        BTRMGRLOG_ERROR("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt() || !apDiagElement) {
        BTRMGRLOG_ERROR("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenDiagElement = btrMgr_MapUUIDtoDiagElement(apDiagElement);
    BTRMGRLOG_INFO("Printing lenDiagElement = %d apDiagElement = %s\n",lenDiagElement,apDiagElement);

    if ((BTRMGR_SYS_DIAG_BEGIN < (BTRMGR_SysDiagChar_t)lenDiagElement) && (BTRMGR_SYS_DIAG_END > (BTRMGR_SysDiagChar_t)lenDiagElement)) {
        if (BTRMGR_LE_OP_READ_VALUE == aOpType) {
            if (eBTRMgrSuccess != BTRMGR_SD_GetData(ghBTRMgrSdHdl, lenDiagElement, lPropValue)) {
                BTRMGRLOG_ERROR("Could not get diagnostic data\n");
                lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
            }
            else {
                strncpy(apValue, lPropValue, BTRMGR_MAX_STR_LEN - 1);
                //CID 341849: String not null terminated (STRING_NULL)
                apValue[BTRMGR_MAX_STR_LEN - 1] = '\0';
            }
        }
    }
    else if ((BTRMGR_SYSDIAG_COLUMBO_BEGIN < (BTRMGR_ColumboChar_t)lenDiagElement) && (BTRMGR_SYSDIAG_COLUMBO_END > (BTRMGR_ColumboChar_t)lenDiagElement)) {
        if (BTRMGR_LE_OP_READ_VALUE == aOpType) {
            if (eBTRMgrSuccess != BTRMGR_Columbo_GetData(lenDiagElement, lPropValue)) {
                BTRMGRLOG_ERROR("Could not get diagnostic data\n");
                lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
            }
            else {
                strncpy(apValue, lPropValue, BTRMGR_MAX_STR_LEN - 1);
            }
        }
        else if (BTRMGR_LE_OP_WRITE_VALUE == aOpType) {
            if (eBTRMgrSuccess != BTRMGR_Columbo_SetData(lenDiagElement, lPropValue)) {
                BTRMGRLOG_ERROR("Could not get diagnostic data\n");
                lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
            }
        }
    }
    else if ((BTRMGR_LE_ONBRDG_BEGIN < (BTRMGR_LeOnboardingChar_t)lenDiagElement) && (BTRMGR_LE_ONBRDG_END > (BTRMGR_LeOnboardingChar_t)lenDiagElement)) {
        if (BTRMGR_LE_OP_READ_VALUE == aOpType) {
            if (eBTRMgrSuccess != BTRMGR_LeOnboarding_GetData(lenDiagElement, lPropValue)) {
                BTRMGRLOG_ERROR("Could not get diagnostic data\n");
                lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
            }
            else {
                strncpy(apValue, lPropValue, BTRMGR_MAX_STR_LEN - 1);
            }
        }
        else if (BTRMGR_LE_OP_WRITE_VALUE == aOpType) {
            if (eBTRMgrSuccess != BTRMGR_LeOnboarding_SetData(lenDiagElement, apValue)) {
                BTRMGRLOG_ERROR("Could not set diagnostic data\n");
                lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
            }
            else {
                
            }
        }
    }
    else {
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
    }
    
    
    return lenBtrMgrResult;
}

BTRMGR_Result_t
BTRMGR_ConnectToWifi(
    unsigned char   aui8AdapterIdx,
    char*           apSSID,
    char*           apPassword,
    int             aSecMode
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

    if (!ghBTRCoreHdl || !ghBTRMgrSdHdl) {
        BTRMGRLOG_ERROR("BTRCore is not Inited\n");
        return BTRMGR_RESULT_INIT_FAILED;
    }

    if (aui8AdapterIdx > btrMgr_GetAdapterCnt()) {
        BTRMGRLOG_ERROR("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    lenBtrMgrResult = BTRMGR_SD_ConnectToWifi(ghBTRMgrSdHdl, apSSID, apPassword, aSecMode);

    return lenBtrMgrResult;
  }

// Outgoing callbacks Registration Interfaces
BTRMGR_Result_t
BTRMGR_RegisterEventCallback (
    BTRMGR_EventCallback    afpcBBTRMgrEventOut
) {
    BTRMGR_Result_t lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;

    if (!afpcBBTRMgrEventOut) {
        BTRMGRLOG_ERROR ("Input is invalid\n");
        return BTRMGR_RESULT_INVALID_INPUT;
    }

    gfpcBBTRMgrEventOut = afpcBBTRMgrEventOut;
    BTRMGRLOG_INFO ("BTRMGR_RegisterEventCallback : Success\n");

    return lenBtrMgrResult;
}


/*  Local Op Threads Prototypes */
static gpointer
btrMgr_g_main_loop_Task (
    gpointer appvMainLoop
) {
    GMainLoop* pMainLoop = (GMainLoop*)appvMainLoop;
    if (!pMainLoop) {
        BTRMGRLOG_INFO ("GMainLoop Error - In arguments Exiting\n");
        return NULL;
    }

    g_main_context_push_thread_default (gmainContext);

    BTRMGRLOG_INFO ("GMainLoop Running - %p - %p\n", pMainLoop, appvMainLoop);
    g_main_loop_run (pMainLoop);
    BTRMGRLOG_INFO ("GMainLoop Quitting - %p - %p\n", pMainLoop, appvMainLoop);

    g_main_context_pop_thread_default (gmainContext);

    return appvMainLoop;
}


/*  Incoming Callbacks */
static gboolean
btrMgr_DiscoveryHoldOffTimerCb (
    gpointer    gptr
) {
    unsigned char lui8AdapterIdx = 0;

    BTRMGRLOG_DEBUG ("CB context Invoked for btrMgr_DiscoveryHoldOffTimerCb || TimeOutReference - %u\n", gTimeOutRef);

    if (gptr) {
        lui8AdapterIdx = *(unsigned char*)gptr;
    } else {
        BTRMGRLOG_WARN ("CB context received NULL arg!\n");
    }

    gTimeOutRef = 0;

    if (btrMgr_PostCheckDiscoveryStatus (lui8AdapterIdx, BTRMGR_DEVICE_OP_TYPE_UNKNOWN) != eBTRMgrSuccess) {
        BTRMGRLOG_ERROR ("Post Check Disc Status Failed!\n");
    }

    return FALSE;
}

static gboolean btrMgr_CheckDisconnectionStatus (gpointer user_data)
{
    /* Making the device id as 0 after 10 secs of disconnect */
    BTRMGRLOG_INFO("Resetting the disconnect status \n");
    ghBTRMgrDevHdlLastDisconnected = 0;
    btrMgr_ClearDisconnectStatusHoldOffTimer();
    return G_SOURCE_REMOVE;
}
#ifdef RDKTV_PERSIST_VOLUME
/* Cleared the flag to update the MediaStatusInfo to UI*/
static gboolean ResetVolumeUpdateFlagCb(gpointer data)
{
    BTRMGRLOG_INFO("Resetting the volume update flag \n");
    ghBTRMgrDevHdlVolSetupInProgress = 0;
    gSkipVolumeUpdate = FALSE;
    btrMgr_ClearSkipVolumeUpdateTimer();
    return G_SOURCE_REMOVE;
}

static gboolean btrMgr_SetVolumeHoldOffTimerCb(gpointer user_data)
{
    stBTRCoreMediaStatusCBInfo*  mediaStatusCB = (stBTRCoreMediaStatusCBInfo*)user_data;
    unsigned char   BtrMgrPersistVolume = BTRMGR_SO_MAX_VOLUME - 1;
    int BtrMgrActualVolume = 0;
    enBTRCoreDeviceType     lenBTRCoreDevTy = enBTRCoreUnknown;
    enBTRCoreDeviceClass    lenBTRCoreDevCl = enBTRCore_DC_Unknown;
    enBTRCoreRet            lenBtrCoreRet   = enBTRCoreSuccess;

    if (mediaStatusCB == NULL) {
        BTRMGRLOG_ERROR("Media status user data is empty, can't proceed\n");
        ghBTRMgrDevHdlVolSetupInProgress = 0;
        btrMgr_ClearVolumeHoldOffTimer();
        return G_SOURCE_REMOVE;
    }
    stBTRCoreMediaStatusUpdate* mediaStatus = &mediaStatusCB->m_mediaStatusUpdate;
    lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, mediaStatusCB->deviceId, &lenBTRCoreDevTy, &lenBTRCoreDevCl);
    BTRMGRLOG_DEBUG ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBTRCoreDevTy, lenBTRCoreDevCl);
    if (lenBtrCoreRet != enBTRCoreSuccess) {
        BTRMGRLOG_INFO("Getting the device info failed \n");
    }
    if (btrMgr_GetLastVolume(0, &BtrMgrPersistVolume,mediaStatusCB->deviceId,BTRMGR_A2DP_SINK_PROFILE_ID) == eBTRMgrSuccess) {
        BTRMGRLOG_INFO("Read the volume from persistent data - %u \n",BtrMgrPersistVolume);
    } else {
        BTRMGRLOG_ERROR("Reading the persistent volume failed ...\n");
        ghBTRMgrDevHdlVolSetupInProgress = 0;
        btrMgr_ClearVolumeHoldOffTimer();
        return G_SOURCE_REMOVE;
    }

    enBTRCoreMediaCtrl  lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlUnknown;
    stBTRCoreMediaCtData        lstBTRCoreMediaCData;
    if (mediaStatus->m_mediaPlayerVolume > BtrMgrPersistVolume) {
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlVolumeDown;
        for (BtrMgrActualVolume = mediaStatus->m_mediaPlayerVolume - 10; (BtrMgrActualVolume > BtrMgrPersistVolume) && (BtrMgrActualVolume >= 0) ; BtrMgrActualVolume -= 10) {
            lstBTRCoreMediaCData.m_mediaAbsoluteVolume = BtrMgrActualVolume;
            if (enBTRCoreSuccess == BTRCore_MediaControl(ghBTRCoreHdl, mediaStatusCB->deviceId, lenBTRCoreDevTy, lenBTRCoreMediaCtrl, &lstBTRCoreMediaCData)) {
                BTRMGRLOG_INFO("Media control Volume DOWN command succes for devid - %lld AV - %d PV - %d\n",mediaStatusCB->deviceId,BtrMgrActualVolume,BtrMgrPersistVolume);
            }
        }
        if (BtrMgrPersistVolume == 0 && BtrMgrActualVolume < 0) {
            lstBTRCoreMediaCData.m_mediaAbsoluteVolume = BtrMgrPersistVolume;
            if (enBTRCoreSuccess == BTRCore_MediaControl(ghBTRCoreHdl, mediaStatusCB->deviceId, lenBTRCoreDevTy, lenBTRCoreMediaCtrl, &lstBTRCoreMediaCData)) {
                BTRMGRLOG_INFO("Media control MIN Volume  command succes ....\n");
            }
        }
    } else if (mediaStatus->m_mediaPlayerVolume < BtrMgrPersistVolume) {
        lenBTRCoreMediaCtrl = enBTRCoreMediaCtrlVolumeUp;
        for (BtrMgrActualVolume = mediaStatus->m_mediaPlayerVolume + 10;  (BtrMgrActualVolume < BtrMgrPersistVolume) && (BtrMgrActualVolume <= 255) ; BtrMgrActualVolume += 10) {
            lstBTRCoreMediaCData.m_mediaAbsoluteVolume = BtrMgrActualVolume;
            if (enBTRCoreSuccess == BTRCore_MediaControl(ghBTRCoreHdl, mediaStatusCB->deviceId, lenBTRCoreDevTy, lenBTRCoreMediaCtrl, &lstBTRCoreMediaCData)) {
                BTRMGRLOG_INFO("Media control Volume UP command succes for devid - %lld AV - %d PV - %d\n",mediaStatusCB->deviceId,BtrMgrActualVolume,BtrMgrPersistVolume);
            }
        }
        if (BtrMgrPersistVolume == BTRMGR_SO_MAX_VOLUME && BtrMgrActualVolume > BTRMGR_SO_MAX_VOLUME) {
            lstBTRCoreMediaCData.m_mediaAbsoluteVolume = BtrMgrPersistVolume;
            if (enBTRCoreSuccess == BTRCore_MediaControl(ghBTRCoreHdl, mediaStatusCB->deviceId, lenBTRCoreDevTy, lenBTRCoreMediaCtrl, &lstBTRCoreMediaCData)) {
                BTRMGRLOG_INFO("Media control MAX Volume command succes ....\n");
            }
        }
    } else {
        /* Not required */
    }
    ghBTRMgrDevHdlVolSetupInProgress = 0;
    btrMgr_ClearVolumeHoldOffTimer();
    return G_SOURCE_REMOVE;
}
#endif
static gboolean btrMgr_PostDeviceFoundEvtTimerCb (gpointer user_data)
{
    BTRMGR_EventMessage_t   lstEventMessage;
    stBTRCoreDevStatusCBInfo *cb_data = (stBTRCoreDevStatusCBInfo*)user_data;
    BTRMGRLOG_DEBUG("Posting Device Found Event ..\n");
    btrMgr_MapDevstatusInfoToEventInfo ((void *)cb_data, &lstEventMessage, BTRMGR_EVENT_DEVICE_FOUND);
    if (gbGamepadStandbyMode && ((lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HID) ||
        (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HID_GAMEPAD)))
    {
        BTRMGRLOG_WARN("Device is in standby mode, we won't post to the upper layers if a device is found\n");
        return G_SOURCE_REMOVE;
    }
    if (gfpcBBTRMgrEventOut) {
        gfpcBBTRMgrEventOut(lstEventMessage);
    }
    return G_SOURCE_REMOVE;
}

static gboolean btrMgr_PostOutOfRangeHoldOffTimerCb(gpointer user_data)
{
    BTRMGR_EventMessage_t   lstEventMessage;
    stBTRCoreDevStatusCBInfo *cb_data = (stBTRCoreDevStatusCBInfo*)user_data;
    btrMgr_MapDevstatusInfoToEventInfo ((void *)cb_data, &lstEventMessage, BTRMGR_EVENT_DEVICE_OUT_OF_RANGE);
    if (gEliteIncomCon != 1 && (btrMgr_IsDevConnected(lstEventMessage.m_pairedDevice.m_deviceHandle) != 1)) {
        BTRMGRLOG_INFO("Generated out of range event for elite\n");
        if (gfpcBBTRMgrEventOut) {
            gfpcBBTRMgrEventOut(lstEventMessage);
        }
    }
    btrMgr_ClearDeviceOORHoldOffTimer();
    return G_SOURCE_REMOVE;
}

STATIC gboolean
btrmgr_DisconnectDeviceTimerCb (
    gpointer    gptr
) {
    enBTRCoreDeviceType lenBTRCoreDeviceType= enBTRCoreUnknown;
    tBTRCoreDevId aBTRCoreDevId;

    BTRMGRLOG_DEBUG ("CB context Invoked for btrmgr_DisconnectDeviceTimerCb || TimeOutReference - %u\n", gAuthDisconnDevTimeOutRef);

    if (gptr) {
        aBTRCoreDevId = *(tBTRCoreDevId*)gptr;
    } else {
        BTRMGRLOG_WARN ("CB context received NULL arg!\n");
        return FALSE;
    }

    gAuthDisconnDevTimeOutRef = 0; // TODO: Is this really required ? Should we call btrMgr_ClearDisconnDevHoldOffTimer to perform g_source_destroy
                                   // Can we even call a g_source_destroy from the context of the timer Cb associated with the same Id/Ref ?
    if (ghBTRMgrDevHdlCurStreaming != aBTRCoreDevId && ghBTRMgrDevHdlStreamStartUp != aBTRCoreDevId) //check device hasn't started streaming since timer was set ( don't want to disconnect incorrectly)
    {
        if (BTRCore_DisconnectDevice (ghBTRCoreHdl, aBTRCoreDevId, lenBTRCoreDeviceType) != enBTRCoreSuccess) {
            BTRMGRLOG_ERROR ("Post Device disconnect Cb timeout Failed!\n");
        }
    }
    else 
    {
        BTRMGRLOG_INFO("Not disconnecting device, as device has started streaming or is starting up streaming\n");
    }
    return FALSE;
}

static gboolean
btrMgr_ConnPwrStChangeTimerCb (
    gpointer gptr
) {
#ifndef LE_MODE
    unsigned char lui8AdapterIdx = 0;
#endif
    BTRMGRLOG_DEBUG ("CB context Invoked for btrMgr_ConnPwrStChangeTimerCb || TimeOutReference - %u\n", gConnPwrStChangeTimeOutRef);
#ifndef LE_MODE
    if (gptr) {
        lui8AdapterIdx = *(unsigned char*)gptr;
    } else {
        BTRMGRLOG_WARN ("CB context received NULL arg!\n");
    }

    gConnPwrStChangeTimeOutRef = 0;

    if ( ghBTRMgrDevHdlCurStreaming == 0 ) {
            if( BTRMGR_StartAudioStreamingOut_StartUp(lui8AdapterIdx, BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT) != BTRMGR_RESULT_SUCCESS) {
                     BTRMGRLOG_ERROR ("ConnPwrStChange - BTRMGR_StartAudioStreamingOut_StartUp Failed!\n");
            }
            if( BTRMGR_ConnectGamepads_StartUp(lui8AdapterIdx, BTRMGR_DEVICE_OP_TYPE_HID) != BTRMGR_RESULT_SUCCESS) {
                     BTRMGRLOG_ERROR ("ConnPwrStChange - BTRMGR_ConnectGamepads_StartUp Failed!\n");
            }
            //gamepads should connect back now
            BTRCore_refreshLEActionListForGamepads(ghBTRCoreHdl);
    }

#endif
    return FALSE;
}

STATIC gboolean
btrMgr_PairCompleteRstTimerCb (
    gpointer gptr
) {
    unsigned char lui8AdapterIdx = 0;

    BTRMGRLOG_DEBUG ("CB context Invoked for btrMgr_PairCompleteRstTimerCb || TimeOutReference - %u\n", gConnPairCompRstTimeOutRef);

    if (gptr) {
        lui8AdapterIdx = *(unsigned char*)gptr;
        (void)lui8AdapterIdx;
    } else {
        BTRMGRLOG_WARN ("CB context received NULL arg!\n");
    }

    gConnPairCompRstTimeOutRef  = 0;
    ghBTRMgrDevHdlPairingInProgress = 0;

    return FALSE;
}

#ifndef LE_MODE
STATIC eBTRMgrRet
btrMgr_ACDataReadyCb (
    void*           apvAcDataBuf,
    unsigned int    aui32AcDataLen,
    void*           apvUserData
) {
    eBTRMgrRet              leBtrMgrAcRet       = eBTRMgrSuccess;
    stBTRMgrStreamingInfo*  lstBTRMgrStrmInfo   = (stBTRMgrStreamingInfo*)apvUserData; 

    if (lstBTRMgrStrmInfo) {
        if ((leBtrMgrAcRet = BTRMgr_SO_SendBuffer(lstBTRMgrStrmInfo->hBTRMgrSoHdl, apvAcDataBuf, aui32AcDataLen)) != eBTRMgrSuccess) {
            BTRMGRLOG_ERROR ("cbBufferReady: BTRMgr_SO_SendBuffer FAILED\n");
        }
        lstBTRMgrStrmInfo->bytesWritten += aui32AcDataLen;
    }   //CID:23337 - Forward null

    return leBtrMgrAcRet;
}


STATIC eBTRMgrRet
btrMgr_ACStatusCb (
    stBTRMgrMediaStatus*    apstBtrMgrAcStatus,
    void*                   apvUserData
) {
    eBTRMgrRet              leBtrMgrAcRet = eBTRMgrSuccess;
    stBTRMgrStreamingInfo*  lpstBTRMgrStrmInfo  = (stBTRMgrStreamingInfo*)apvUserData; 

    if (lpstBTRMgrStrmInfo && apstBtrMgrAcStatus) {
        stBTRMgrMediaStatus lstBtrMgrSoStatus;

        //TODO; Dont just memcpy from AC Status to SO Status, map it correctly in the future.
        BTRMGRLOG_WARN ("STATUS CHANGED\n");
        MEMCPY_S(&lstBtrMgrSoStatus,sizeof(lstBtrMgrSoStatus), apstBtrMgrAcStatus, sizeof(stBTRMgrMediaStatus));
        if ((leBtrMgrAcRet = BTRMgr_SO_SetStatus(lpstBTRMgrStrmInfo->hBTRMgrSoHdl, &lstBtrMgrSoStatus)) != eBTRMgrSuccess) {
            BTRMGRLOG_ERROR ("BTRMgr_SO_SetStatus FAILED = %d\n", leBtrMgrAcRet);
        }
    }

    return leBtrMgrAcRet;
}


STATIC eBTRMgrRet
btrMgr_SOStatusCb (
    stBTRMgrMediaStatus*    apstBtrMgrSoStatus,
    void*                   apvUserData
) {
    eBTRMgrRet              leBtrMgrSoRet = eBTRMgrSuccess;
    stBTRMgrStreamingInfo*  lpstBTRMgrStrmInfo   = (stBTRMgrStreamingInfo*)apvUserData; 

    if (lpstBTRMgrStrmInfo && apstBtrMgrSoStatus) {
        BTRMGRLOG_INFO ("Entering\n");

        //TODO: Not happy that we are doing in the context of the callback.
        //      If possible move to a task thread
        //TODO: Rather than giving up on Streaming Out altogether, think about a retry implementation
        if (apstBtrMgrSoStatus->eBtrMgrState == eBTRMgrStateError) {
            if (ghBTRMgrDevHdlCurStreaming && lpstBTRMgrStrmInfo->hBTRMgrSoHdl) { /* Connected device. AC extablished; Release and Disconnect it */
                BTRMGRLOG_ERROR ("Error - ghBTRMgrDevHdlCurStreaming = %lld\n", ghBTRMgrDevHdlCurStreaming);
                if (ghBTRMgrDevHdlCurStreaming) { /* The streaming is happening; stop it */
#if 0
                    //TODO: DONT ENABLE Just a Reference of what we are trying to acheive
                    if (BTRMGR_StopAudioStreamingOut(0, ghBTRMgrDevHdlCurStreaming) != BTRMGR_RESULT_SUCCESS) {
                        BTRMGRLOG_ERROR ("Streamout is failed to stop\n");
                        leBtrMgrSoRet = eBTRMgrFailure;
                    }
#endif
                }
            }
        }
    }

    return leBtrMgrSoRet;
}

#ifdef STREAM_IN_SUPPORTED
STATIC eBTRMgrRet
btrMgr_SIStatusCb (
    stBTRMgrMediaStatus*    apstBtrMgrSiStatus,
    void*                   apvUserData
) {
    eBTRMgrRet              leBtrMgrSiRet = eBTRMgrSuccess;
    stBTRMgrStreamingInfo*  lpstBTRMgrStrmInfo   = (stBTRMgrStreamingInfo*)apvUserData; 

    if (lpstBTRMgrStrmInfo && apstBtrMgrSiStatus) {
        BTRMGRLOG_INFO ("Entering\n");

        //TODO: Not happy that we are doing in the context of the callback.
        //      If possible move to a task thread
        //TODO: Rather than giving up on Streaming In altogether, think about a retry implementation
        if (apstBtrMgrSiStatus->eBtrMgrState == eBTRMgrStateError) {
            if (ghBTRMgrDevHdlCurStreaming && lpstBTRMgrStrmInfo->hBTRMgrSiHdl) { /* Connected device. AC extablished; Release and Disconnect it */
                BTRMGRLOG_ERROR ("Error - ghBTRMgrDevHdlCurStreaming = %lld\n", ghBTRMgrDevHdlCurStreaming);
                if (ghBTRMgrDevHdlCurStreaming) { /* The streaming is happening; stop it */
#if 0
                    //TODO: DONT ENABLE Just a Reference of what we are trying to acheive
                    if (BTRMGR_StopAudioStreamingIn(0, ghBTRMgrDevHdlCurStreaming) != BTRMGR_RESULT_SUCCESS) {
                        BTRMGRLOG_ERROR ("Streamin is failed to stop\n");
                        leBtrMgrSiRet = eBTRMgrFailure;
                    }
#endif
                }
            }
        }
    }

    return leBtrMgrSiRet;
}
#endif
#endif
STATIC eBTRMgrRet
btrMgr_SDStatusCb (
    stBTRMgrSysDiagStatus*  apstBtrMgrSdStatus,
    void*                   apvUserData
) {
    eBTRMgrRet                      leBtrMgrSdRet = eBTRMgrSuccess;
    
    BTRMGR_Result_t                 lenBtrMgrResult   = BTRMGR_RESULT_SUCCESS;
    enBTRCoreRet                    lenBtrCoreRet     = enBTRCoreSuccess;
    unsigned short                  ui16LoopIdx       = 0;
    BTRMGR_ConnectedDevicesList_t  *lstConnectedDevices;
    unsigned int                    ui32sleepTimeOut = 1;
    gboolean                        isRemoteDev = FALSE;

    if(apstBtrMgrSdStatus == NULL || apvUserData == NULL)
    {
	BTRMGRLOG_ERROR("Invalid memory\n");
        return eBTRMgrFailure;	
    }
    if ((apstBtrMgrSdStatus != NULL) &&
#ifndef LE_MODE
        (gIsAudOutStartupInProgress != BTRMGR_STARTUP_AUD_INPROGRESS) &&
#endif
        (apstBtrMgrSdStatus->enSysDiagChar == BTRMGR_SYS_DIAG_POWERSTATE) &&
        !strncmp(apstBtrMgrSdStatus->pcSysDiagRes, BTRMGR_SYS_DIAG_PWRST_ON, strlen(BTRMGR_SYS_DIAG_PWRST_ON))) {

        gConnPwrStChTimeOutCbData = 0; // TODO: Change when this entire file is made re-entrant

        if (gConnPwrStChangeTimeOutRef) {
            BTRMGRLOG_DEBUG ("Cancelling previous Power state Cb : %u\n", gConnPwrStChangeTimeOutRef);
            g_source_destroy(g_main_context_find_source_by_id(gmainContext, gConnPwrStChangeTimeOutRef));
            gConnPwrStChangeTimeOutRef = 0;
        }

        GSource* source = g_timeout_source_new(BTRMGR_DEVCONN_PWRST_CHANGE_TIME * 1000);
        g_source_set_priority(source, G_PRIORITY_DEFAULT);
        g_source_set_callback(source, (GSourceFunc)btrMgr_ConnPwrStChangeTimerCb , NULL, NULL);

        gConnPwrStChangeTimeOutRef = g_source_attach(source, gmainContext);
        g_source_unref(source);
        if (gConnPwrStChangeTimeOutRef)
            leBtrMgrSdRet = eBTRMgrSuccess;
    }
    
    if ((apstBtrMgrSdStatus != NULL) && (apstBtrMgrSdStatus->enSysDiagChar == BTRMGR_SYS_DIAG_POWERSTATE))
    {
        if (!strncmp(apstBtrMgrSdStatus->pcSysDiagRes, BTRMGR_SYS_DIAG_PWRST_ON, strlen(BTRMGR_SYS_DIAG_PWRST_ON)))
        {
            BTRMGRLOG_INFO("Exited standby mode, allowing gamepads to connect\n");
            gbGamepadStandbyMode = FALSE;
        }
        else if (!strncmp(apstBtrMgrSdStatus->pcSysDiagRes, BTRMGR_SYS_DIAG_PWRST_STANDBY, strlen(BTRMGR_SYS_DIAG_PWRST_STANDBY)) || 
                 !strncmp(apstBtrMgrSdStatus->pcSysDiagRes, BTRMGR_SYS_DIAG_PWRST_STDBY_LIGHT_SLEEP, strlen(BTRMGR_SYS_DIAG_PWRST_STDBY_LIGHT_SLEEP)))
        {
            BTRMGRLOG_INFO("Entered standby mode, disconnecting gamepads\n");
            gbGamepadStandbyMode = TRUE;
            //stop any gamepads from connecting whilst in standby mode
            BTRCore_clearLEActionListForGamepads(ghBTRCoreHdl);
            //Disconnect all connected gamepads
            lstConnectedDevices = malloc(sizeof(BTRMGR_ConnectedDevicesList_t));
            if (!lstConnectedDevices)
            {
                BTRMGRLOG_ERROR("Run out memory for connected devices list\n");
                return eBTRMgrFailure;
            }
            if ((lenBtrMgrResult = BTRMGR_GetConnectedDevices(0, lstConnectedDevices)) == BTRMGR_RESULT_SUCCESS) {
                BTRMGRLOG_DEBUG ("Connected Devices = %d\n", lstConnectedDevices->m_numOfDevices);

                for (ui16LoopIdx = 0; ui16LoopIdx < lstConnectedDevices->m_numOfDevices; ui16LoopIdx++) {
                    unsigned int            ui32confirmIdx  = 2;
                    enBTRCoreDeviceType     lenBtrCoreDevTy = enBTRCoreUnknown;
                    enBTRCoreDeviceClass    lenBtrCoreDevCl = enBTRCore_DC_Unknown;
                    isRemoteDev = btrMgr_IsDeviceRdkRcu(&lstConnectedDevices->m_deviceProperty[ui16LoopIdx].m_serviceInfo, lstConnectedDevices->m_deviceProperty[ui16LoopIdx].m_ui16DevAppearanceBleSpec);
                    BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, lstConnectedDevices->m_deviceProperty[ui16LoopIdx].m_deviceHandle, &lenBtrCoreDevTy, &lenBtrCoreDevCl);

                    if (!isRemoteDev && ((lstConnectedDevices->m_deviceProperty[ui16LoopIdx].m_deviceType == BTRMGR_DEVICE_TYPE_HID) ||
                    (lstConnectedDevices->m_deviceProperty[ui16LoopIdx].m_deviceType == BTRMGR_DEVICE_TYPE_HID_GAMEPAD)))
                    {
                        if (BTRCore_DisconnectDevice(ghBTRCoreHdl, lstConnectedDevices->m_deviceProperty[ui16LoopIdx].m_deviceHandle, lenBtrCoreDevTy) != enBTRCoreSuccess) {
                            BTRMGRLOG_ERROR ("Failed to Disconnect - %llu\n", lstConnectedDevices->m_deviceProperty[ui16LoopIdx].m_deviceHandle);
                        }

                        do {
                            unsigned int ui32sleepIdx = 2;

                            do {
                                sleep(ui32sleepTimeOut);
                                lenBtrCoreRet = BTRCore_GetDeviceDisconnected(ghBTRCoreHdl, lstConnectedDevices->m_deviceProperty[ui16LoopIdx].m_deviceHandle, lenBtrCoreDevTy);
                            } while ((lenBtrCoreRet != enBTRCoreSuccess) && (--ui32sleepIdx));
                        } while (--ui32confirmIdx);
                    }
                }
            }
            free(lstConnectedDevices);
        } 
    }

    return leBtrMgrSdRet;
}

void btrMgr_IncomingConnectionAuthentication(stBTRCoreDevStatusCBInfo* p_StatusCB, int *auth)
{
    BTRMGR_EventMessage_t lstEventMessage;
    MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));

    lstEventMessage.m_adapterIndex = 0;
    lstEventMessage.m_eventType    = BTRMGR_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST;
    lstEventMessage.m_externalDevice.m_deviceHandle = ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceId;
    lstEventMessage.m_externalDevice.m_deviceType = btrMgr_MapDeviceTypeFromCore(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->eDeviceClass);
    lstEventMessage.m_externalDevice.m_isLowEnergyDevice = 0;
    lstEventMessage.m_externalDevice.m_vendorID = 0;
    strncpy(lstEventMessage.m_externalDevice.m_name, ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceName,
            strlen(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceName) < BTRMGR_NAME_LEN_MAX ? strlen (((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceName) : BTRMGR_NAME_LEN_MAX - 1);
    strncpy(lstEventMessage.m_externalDevice.m_deviceAddress, ((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceAddress,
            strlen(((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceAddress) < BTRMGR_NAME_LEN_MAX ? strlen (((stBTRCoreDevStatusCBInfo*)p_StatusCB)->deviceAddress) : BTRMGR_NAME_LEN_MAX - 1);

    if (gfpcBBTRMgrEventOut) {
        gfpcBBTRMgrEventOut(lstEventMessage);
    }

    BTRMGR_GetPairedDevices (lstEventMessage.m_adapterIndex, &gListOfPairedDevices);
    BTRMGRLOG_INFO("Wating for the external connection response from UI for LE HID device\n");
    unsigned int ui32sleepIdx = 40;
    do {
        usleep(500000);
    } while ((gEventRespReceived == 0) && (--ui32sleepIdx));
    if (gEventRespReceived == 0) {
        BTRMGRLOG_INFO("External connection response not received from UI for LE HID device, So disconnecting the device.\n");
        *auth = 0;
        if (BTRCore_DisconnectDevice(ghBTRCoreHdl, p_StatusCB->deviceId, enBTRCoreHID) == enBTRCoreSuccess) {
             BTRMGRLOG_INFO("Disconnected an LE HID device successfully\n");
        }
    } else {
        *auth = gAcceptConnection;
        if (gAcceptConnection) {
            BTRMGRLOG_INFO ("Incoming Connection accepted for LE HID device based on the response from UI\n");
        } else {
            BTRMGRLOG_INFO ("Incoming Connection rejected for LE HID device based on the response from UI, So disconnecting it\n");
            if (BTRCore_DisconnectDevice(ghBTRCoreHdl, p_StatusCB->deviceId, enBTRCoreHID) == enBTRCoreSuccess) {
                BTRMGRLOG_INFO("Disconnected an LE HID device successfully\n");
            }
        }
    gEventRespReceived = 0;
    }
}

STATIC enBTRCoreRet
btrMgr_DeviceStatusCb (
    stBTRCoreDevStatusCBInfo*   p_StatusCB,
    void*                       apvUserData
) {
    enBTRCoreRet            lenBtrCoreRet   = enBTRCoreSuccess;
    BTRMGR_EventMessage_t   lstEventMessage;
    BTRMGR_DeviceType_t     lBtrMgrDevType  = BTRMGR_DEVICE_TYPE_UNKNOWN;
    stBTRCoreBTDevice       stDeviceInfo;
#ifndef LE_MODE
    static guint8           ui8BtrMgrAvdtpReconnections = 0;
#endif
    MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));

    BTRMGRLOG_DEBUG ("Received status callback\n");

    if (p_StatusCB) {

        lBtrMgrDevType = btrMgr_MapDeviceTypeFromCore(p_StatusCB->eDeviceClass);
        BTRMGRLOG_INFO (" Received status callback device type %d PrevState %d State %d\n",lBtrMgrDevType, p_StatusCB->eDevicePrevState, p_StatusCB->eDeviceCurrState);

        if (!gIsHidGamePadEnabled &&
            (lBtrMgrDevType == BTRMGR_DEVICE_TYPE_HID_GAMEPAD)) {
             BTRMGRLOG_WARN ("Rejecting status callback - BTR HID Gamepad is currently Disabled!\n");
             return lenBtrCoreRet;
        }

        if ((p_StatusCB->ui16DevAppearanceBleSpec == BTRMGR_HID_GAMEPAD_LE_APPEARANCE) &&
            (lBtrMgrDevType == BTRMGR_DEVICE_TYPE_HID_GAMEPAD || lBtrMgrDevType == BTRMGR_DEVICE_TYPE_HID)) {
            //btrMgr_GetDeviceDetails method call communicate with bluez.
            if((p_StatusCB->eDeviceCurrState == enBTRCoreDevStPaired) && (p_StatusCB->ui32VendorId == 0)) {
                unsigned int ui32retrycount = BTRMGR_MODALIAS_RETRY_ATTEMPTS;

                do {
                    btrMgr_GetDeviceDetails(p_StatusCB->deviceId,&stDeviceInfo);
                    BTRMGRLOG_INFO ("btrMgr_DeviceStatusCb callaback v%04Xp%04Xd%04X\n", stDeviceInfo.ui32ModaliasVendorId,
                                    stDeviceInfo.ui32ModaliasProductId, stDeviceInfo.ui32ModaliasDeviceId);

                    //wait some time to update the modalias from bluez to btmgr. here only retry for paired event
                    if ((stDeviceInfo.ui32ModaliasVendorId == 0) && ui32retrycount) {
                        BTRMGRLOG_INFO("usleep executed to update the modalias...\n");
                        usleep(100000); // Delay for 100 milliseconds
                    }
                    else {
                        break;
                    }
                } while (ui32retrycount--); //300 ms

                p_StatusCB->ui32VendorId = stDeviceInfo.ui32ModaliasVendorId;
                p_StatusCB->ui32ProductId = stDeviceInfo.ui32ModaliasProductId;
                p_StatusCB->ui32DeviceId = stDeviceInfo.ui32ModaliasDeviceId;
            }

            BTRMGRLOG_INFO ("btrMgr_DeviceStatusCb callaback v%04Xp%04Xd%04X\n", p_StatusCB->ui32VendorId, p_StatusCB->ui32ProductId, p_StatusCB->ui32DeviceId);

            if(BTRCore_IsUnsupportedGamepad(p_StatusCB->ui32VendorId, p_StatusCB->ui32ProductId, p_StatusCB->ui32DeviceId)) {
                if (p_StatusCB->eDeviceCurrState != enBTRCoreDevStUnsupported) {
                    BTRMGRLOG_INFO ("Ignored notification to UI for incompatible device v%04Xp%04Xd%04X\n",
                    p_StatusCB->ui32VendorId, p_StatusCB->ui32ProductId, p_StatusCB->ui32DeviceId);

                    return lenBtrCoreRet;
                }
            }
        }
        switch (p_StatusCB->eDeviceCurrState) {
        case enBTRCoreDevStPaired:
            /* Post this event only for HID Devices and Audio-In Devices */
            if ((p_StatusCB->eDeviceType == enBTRCoreHID) ||
                (p_StatusCB->eDeviceType == enBTRCoreMobileAudioIn) ||
                (p_StatusCB->eDeviceType == enBTRCorePCAudioIn)) {

                if ((p_StatusCB->eDeviceType == enBTRCoreHID) &&
                    (p_StatusCB->deviceId == ghBTRMgrDevHdlLastPaired) &&
                    (p_StatusCB->ui16DevAppearanceBleSpec == BTRMGR_HID_GAMEPAD_LE_APPEARANCE)) {
                    if (BTRCore_UnPairDevice(ghBTRCoreHdl, ghBTRMgrDevHdlLastPaired) != enBTRCoreSuccess) {
                        BTRMGRLOG_ERROR ("Failed to Unpair - %llu\n", ghBTRMgrDevHdlLastPaired);
                    }
                    ghBTRMgrDevHdlLastPaired = 0;
                    btrMgr_ClearLastPairedDeviceStatusHoldOffTimer();
                    BTRMGRLOG_INFO("Unpaired the last pairing failed device ...\n");
                    break;
                }

                btrMgr_GetDiscoveredDevInfo (p_StatusCB->deviceId, &lstEventMessage.m_discoveredDevice);

                lstEventMessage.m_discoveredDevice.m_deviceHandle       = p_StatusCB->deviceId;
                lstEventMessage.m_discoveredDevice.m_deviceType         = (p_StatusCB->eDeviceType == enBTRCoreHID) ? BTRMGR_DEVICE_TYPE_HID : lBtrMgrDevType;
                lstEventMessage.m_discoveredDevice.m_isPairedDevice     = p_StatusCB->isPaired;
                lstEventMessage.m_discoveredDevice.m_ui32DevClassBtSpec = p_StatusCB->ui32DevClassBtSpec;
                lstEventMessage.m_discoveredDevice.m_ui16DevAppearanceBleSpec = p_StatusCB->ui16DevAppearanceBleSpec;
                if (p_StatusCB->eDevicePrevState == enBTRCoreDevStConnected) {
                    lstEventMessage.m_discoveredDevice.m_isConnected = 1;
                }

                strncpy(lstEventMessage.m_discoveredDevice.m_name, p_StatusCB->deviceName,
                            strlen(p_StatusCB->deviceName) < BTRMGR_NAME_LEN_MAX ? strlen (p_StatusCB->deviceName) : BTRMGR_NAME_LEN_MAX - 1);
                strncpy(lstEventMessage.m_discoveredDevice.m_deviceAddress, p_StatusCB->deviceAddress,
                            strlen(p_StatusCB->deviceAddress) < BTRMGR_NAME_LEN_MAX ? strlen (p_StatusCB->deviceAddress) : BTRMGR_NAME_LEN_MAX - 1);


                lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_PAIRING_COMPLETE;
                if (gfpcBBTRMgrEventOut) {
                    gfpcBBTRMgrEventOut(lstEventMessage);  /* Post a callback */
                }

                if (p_StatusCB->eDeviceType == enBTRCoreHID) {
                    BTRMGR_GetPairedDevices (gDefaultAdapterContext.adapter_number, &gListOfPairedDevices);
                }
            }
            break;
        case enBTRCoreDevStInitialized:
            break;
        case enBTRCoreDevStConnecting:
            break;
        case enBTRCoreDevStSuspended:
#ifndef LE_MODE
            //there could be a disconnect in progress, but not complete, this could be why the transport goes idle
            if (ghBTRMgrDevHdlCurStreaming == p_StatusCB->deviceId)
            {
                BTRMGRLOG_WARN("AVDTP stream has been suspended by external device, recover by disconnecting and reconnecting\n");
                if (ui8BtrMgrAvdtpReconnections < BTMGR_AVDTP_SUSPEND_MAX_RETRIES)
                {
                    ui8BtrMgrAvdtpReconnections++;
                    if (BTRCore_DisconnectDevice(ghBTRCoreHdl, p_StatusCB->deviceId, enBTRCoreSpeakers) == enBTRCoreSuccess)
                    {
                        BTRMGRLOG_INFO("Disconnected successfully from external device\n");
                        btrMgr_ConnectBackToDevice(0, p_StatusCB->deviceId, BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT);
                    }
                    else
                    {
                        BTRMGRLOG_ERROR("Could not disconnect from external device\n");
                    }
                }
                else
                {
                    BTRMGRLOG_WARN("Stream is suspended, not trying reconnect\n");
                }
            }
#endif
            break;
        case enBTRCoreDevStConnected:               /*  notify user device back   */
            if (enBTRCoreDevStLost == p_StatusCB->eDevicePrevState || enBTRCoreDevStPaired == p_StatusCB->eDevicePrevState) {
#ifndef LE_MODE
                if (gIsAudOutStartupInProgress != BTRMGR_STARTUP_AUD_INPROGRESS) {
#endif
                    unsigned char doPostCheck = 0;
                    btrMgr_MapDevstatusInfoToEventInfo ((void*)p_StatusCB, &lstEventMessage, BTRMGR_EVENT_DEVICE_FOUND);

                    if ((lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_WEARABLE_HEADSET)  ||
                        (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HANDSFREE)         ||
                        (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_LOUDSPEAKER)       ||
                        (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HEADPHONES)        ||
                        (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_PORTABLE_AUDIO)    ||
                        (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_CAR_AUDIO)         ||
                        (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HIFI_AUDIO_DEVICE) ){

                        if (lstEventMessage.m_pairedDevice.m_isLastConnectedDevice) {
                            btrMgr_PreCheckDiscoveryStatus (0, BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT);
                            doPostCheck = 1;
                        }
#ifdef RDKTV_PERSIST_VOLUME
                        unsigned char   lui8Volume = BTRMGR_SO_MAX_VOLUME - 1;
                        if (btrMgr_GetLastVolume(0, &lui8Volume, lstEventMessage.m_pairedDevice.m_deviceHandle, BTRMGR_A2DP_SINK_PROFILE_ID) == eBTRMgrSuccess) {
                            BTRMGRLOG_INFO("Device connection in progress ...\n");
                            ghBTRMgrDevHdlVolSetupInProgress = lstEventMessage.m_pairedDevice.m_deviceHandle;
                        }
#endif
                    }
                    else if ((lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HID) ||
                             (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HID_GAMEPAD)) {
#ifdef AUTO_CONNECT_ENABLED
                        BTRMGRLOG_DEBUG("HID Device Found ui16DevAppearanceBleSpec - %d \n",p_StatusCB->ui16DevAppearanceBleSpec);
                        if ((p_StatusCB->ui16DevAppearanceBleSpec == BTRMGR_HID_GAMEPAD_LE_APPEARANCE) &&
                            (enBTRCoreDevStLost == p_StatusCB->eDevicePrevState ||
                             enBTRCoreDevStPaired == p_StatusCB->eDevicePrevState) &&
                            (lstEventMessage.m_pairedDevice.m_deviceHandle != ghBTRMgrDevHdlConnInProgress) &&
                            (lstEventMessage.m_pairedDevice.m_deviceHandle != ghBTRMgrDevHdlPairingInProgress)) {
                            int auth = 0;
                            btrMgr_IncomingConnectionAuthentication(p_StatusCB,&auth);
                            if (!auth)
                                break;
                        }
#endif //AUTO_CONNECT_ENABLED
                        if (ghBTRMgrDevHdlLastDisconnected == lstEventMessage.m_pairedDevice.m_deviceHandle) {
                            BTRMGRLOG_INFO("Auto connection rejection in progress after disconnection, skipped posting the device found\n");
                            break;
                        }
                        lstEventMessage.m_pairedDevice.m_deviceType = BTRMGR_DEVICE_TYPE_HID;
                        btrMgr_SetDevConnected(lstEventMessage.m_pairedDevice.m_deviceHandle, 1);
                        BTRCore_newBatteryLevelDevice(ghBTRCoreHdl);
                    }
                    else if ((lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_SMARTPHONE) ||
                              (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_TABLET)) {
                        btrMgr_AddPersistentEntry(0, lstEventMessage.m_pairedDevice.m_deviceHandle, BTRMGR_A2DP_SRC_PROFILE_ID, 1);
                    }
#ifdef LE_MODE
                    else if (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_UNKNOWN) {
                              btrMgr_SetDevConnected(lstEventMessage.m_pairedDevice.m_deviceHandle, 1);
                              lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_CONNECTION_COMPLETE;
                    }
#endif
                    /* TODO : Generic API should be created to create and destroy the timers in different context */
                    if (gDeviceFoundEvtTimeOutRef) {
                        BTRMGRLOG_DEBUG ("Cancelling previous Device Found Event TimeOut Session : %u\n", gDeviceFoundEvtTimeOutRef);
                        g_source_destroy(g_main_context_find_source_by_id(gmainContext, gDeviceFoundEvtTimeOutRef));
                        gDeviceFoundEvtTimeOutRef = 0;
                    }

                    GSource* source = g_timeout_source_new(BTRMGR_POST_DEVICE_FOUND_EVENT_WAIT_TIME * 1000);
                    g_source_set_priority(source, G_PRIORITY_DEFAULT);
                    g_source_set_callback(source, (GSourceFunc)btrMgr_PostDeviceFoundEvtTimerCb , (gpointer)p_StatusCB, NULL);

                    gDeviceFoundEvtTimeOutRef = g_source_attach(source, gmainContext);
                    g_source_unref(source);

                    if (gDeviceFoundEvtTimeOutRef!= 0) {
                        BTRMGRLOG_DEBUG ("Triggered Post Device Found Event Wait time : %u\n", gDeviceFoundEvtTimeOutRef);
                    }
                    BTRMGRLOG_INFO("Device Found event posted Device Type - %d p_StatusCB->eDeviceType - %d \n",lstEventMessage.m_pairedDevice.m_deviceType,p_StatusCB->eDeviceType);

                    if (doPostCheck) {
                        btrMgr_PostCheckDiscoveryStatus (0, BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT);
                    }
#ifndef LE_MODE
                }
#endif
            }
            else if(enBTRCoreDevStDisconnecting == p_StatusCB->eDevicePrevState && enBTRCoreDevStConnected == p_StatusCB->eDeviceCurrState && p_StatusCB->eDeviceType == enBTRCoreHID && p_StatusCB->ui16DevAppearanceBleSpec == BTRMGR_HID_GAMEPAD_LE_APPEARANCE) {
               BTRMGRLOG_INFO("Disconnect Process is going on for this HID device, So not required to post connection completion event ...\n");
               break;
            }
            else if (enBTRCoreDevStInitialized != p_StatusCB->eDevicePrevState && (p_StatusCB->deviceId != ghBTRMgrDevHdlLastDisconnected || ghBTRMgrDevHdlConnInProgress == p_StatusCB->deviceId)) {
                btrMgr_MapDevstatusInfoToEventInfo ((void*)p_StatusCB, &lstEventMessage, BTRMGR_EVENT_DEVICE_CONNECTION_COMPLETE);
                lstEventMessage.m_pairedDevice.m_isConnected = 1;

                if ((lstEventMessage.m_pairedDevice.m_deviceType != BTRMGR_DEVICE_TYPE_WEARABLE_HEADSET) &&
                    (lstEventMessage.m_pairedDevice.m_deviceType != BTRMGR_DEVICE_TYPE_HANDSFREE) &&
                    (lstEventMessage.m_pairedDevice.m_deviceType != BTRMGR_DEVICE_TYPE_LOUDSPEAKER) &&
                    (lstEventMessage.m_pairedDevice.m_deviceType != BTRMGR_DEVICE_TYPE_HEADPHONES) &&
                    (lstEventMessage.m_pairedDevice.m_deviceType != BTRMGR_DEVICE_TYPE_PORTABLE_AUDIO) &&
                    (lstEventMessage.m_pairedDevice.m_deviceType != BTRMGR_DEVICE_TYPE_CAR_AUDIO) &&
                    (lstEventMessage.m_pairedDevice.m_deviceType != BTRMGR_DEVICE_TYPE_HIFI_AUDIO_DEVICE)) {

                    /* Update the flag as the Device is Connected */
                    if ((lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HID) ||
                        (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HID_GAMEPAD)) {
                        lstEventMessage.m_pairedDevice.m_deviceType = BTRMGR_DEVICE_TYPE_HID;
                        /*Skipped posting the connection completion event here if the device tries to
                        *auto-connect post disconnection from UI. Based on the connect wrapper initiated
                        *from UI, the connection completion event will be posted*/
                        if(p_StatusCB->eDevicePrevState == enBTRCoreDevStDisconnected)
                        break;
                        /* After connection failure, get confirmation from the UI to connect back.
			 * For Classsic HID devices ,Connect method will be triggered from UI based
			 * on the connect request event on connection authorization process, so not
			 * required to post connection completion event. */
                        if (p_StatusCB->eDevicePrevState == enBTRCoreDevStConnecting &&
                            lstEventMessage.m_pairedDevice.m_deviceHandle != ghBTRMgrDevHdlConnInProgress) {
                            if (p_StatusCB->ui16DevAppearanceBleSpec == BTRMGR_HID_GAMEPAD_LE_APPEARANCE) {
                                int auth = 0;
                                btrMgr_IncomingConnectionAuthentication(p_StatusCB,&auth);
                                if (!auth)
                                    break;
                            } else {
                                BTRMGRLOG_INFO("Connect method will be triggered from UI based on the connect request event on connection authorization\n");
                                break;
                            }
                        }

                        BTRMGRLOG_WARN ("Sending Event for HID \n");
                        btrMgr_SetDevConnected(lstEventMessage.m_pairedDevice.m_deviceHandle, 1);
                        BTRCore_newBatteryLevelDevice(ghBTRCoreHdl);
                    }
                    else if ((lstEventMessage.m_pairedDevice.m_deviceType != BTRMGR_DEVICE_TYPE_TILE) &&
                        (lstEventMessage.m_pairedDevice.m_deviceType != BTRMGR_DEVICE_TYPE_XBB)) {
                        btrMgr_SetDevConnected(lstEventMessage.m_pairedDevice.m_deviceHandle, 1);
                        ghBTRMgrDevHdlLastConnected = lstEventMessage.m_pairedDevice.m_deviceHandle;
                        if ((lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_SMARTPHONE) ||
                            (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_TABLET)) {
                            btrMgr_AddPersistentEntry(0, lstEventMessage.m_pairedDevice.m_deviceHandle, BTRMGR_A2DP_SRC_PROFILE_ID, 1);
                        }
                    }

                    if (gfpcBBTRMgrEventOut) {
                        gfpcBBTRMgrEventOut(lstEventMessage);  /* Post a callback */
                    }
                }
            }
            break;
        case enBTRCoreDevStDisconnected:
            if (enBTRCoreDevStConnecting != p_StatusCB->eDevicePrevState) {
                btrMgr_MapDevstatusInfoToEventInfo ((void*)p_StatusCB, &lstEventMessage, BTRMGR_EVENT_DEVICE_DISCONNECT_COMPLETE);

               if ((lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_SMARTPHONE) ||
                   (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_TABLET)) {
                    btrMgr_SetLastConnectionStatus(0, 0, lstEventMessage.m_pairedDevice.m_deviceHandle, BTRMGR_A2DP_SRC_PROFILE_ID);
                }
                /* external modules like thunder,servicemanager yet to define
                 * HID sub types hence type is sending as BTRMGR_DEVICE_TYPE_HID in events
                 */
                if (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HID_GAMEPAD)
                    lstEventMessage.m_pairedDevice.m_deviceType = BTRMGR_DEVICE_TYPE_HID;

                if (gfpcBBTRMgrEventOut) {
                    gfpcBBTRMgrEventOut(lstEventMessage);    /* Post a callback */
                }

                BTRMGRLOG_INFO ("lstEventMessage.m_pairedDevice.m_deviceType = %d\n", lstEventMessage.m_pairedDevice.m_deviceType);
                if (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_TILE) {
                    /* update the flags as the LE device is NOT Connected */
                    gIsLeDeviceConnected = 0;
                }
#ifdef LE_MODE
                else if (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_XBB) {
                    gIsLeDeviceConnected = 0;
                    gXbbConnected = FALSE;
                }
#endif
                else if ((lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HID) ||
                         (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HID_GAMEPAD)) {
                    btrMgr_SetDevConnected(lstEventMessage.m_pairedDevice.m_deviceHandle, 0);
                }
                else if ((ghBTRMgrDevHdlCurStreaming != 0) && (ghBTRMgrDevHdlCurStreaming == p_StatusCB->deviceId)) {
                    /* update the flags as the device is NOT Connected */
                    btrMgr_SetDevConnected(lstEventMessage.m_pairedDevice.m_deviceHandle, 0);

                    if (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_SMARTPHONE ||
                        lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_TABLET) {
                        btrMgr_SetLastConnectionStatus(0, 0, p_StatusCB->deviceId, BTRMGR_A2DP_SRC_PROFILE_ID);
                        /* Stop the playback which already stopped internally but to free up the memory */
                        if(!BTRMGR_StopAudioStreamingIn(0, ghBTRMgrDevHdlCurStreaming))
                        {
                            BTRMGRLOG_ERROR ("BTRMGR_StopAudioStreamingIn error \n ");  //CID:41286 - Checked return
                        }
                        ghBTRMgrDevHdlLastConnected = 0;
                    }
                    else {
                        /* Stop the playback which already stopped internally but to free up the memory */
                        if (BTRMGR_RESULT_SUCCESS != BTRMGR_StopAudioStreamingOut(0, ghBTRMgrDevHdlCurStreaming)) {
                            BTRMGRLOG_ERROR ("BTRMGR_StopAudioStreamingOut error \n ");  //CID:41354 - Checked return
                        }
                    }
                }
                else if ((btrMgr_IsDevConnected(lstEventMessage.m_pairedDevice.m_deviceHandle) == 1) &&
                         (ghBTRMgrDevHdlLastConnected == lstEventMessage.m_pairedDevice.m_deviceHandle)) {

                    if (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_SMARTPHONE ||
                        lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_TABLET) {
                        ghBTRMgrDevHdlLastConnected = 0;
                    }
                    else {
                        //TODO: Add what to do for other device types
                    }
                }
            }
            break;
        case enBTRCoreDevStLost:
            if( !gIsUserInitiated ) {
                btrMgr_MapDevstatusInfoToEventInfo ((void*)p_StatusCB, &lstEventMessage, BTRMGR_EVENT_DEVICE_OUT_OF_RANGE);
                if ((lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_WEARABLE_HEADSET)   ||
                    (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HANDSFREE)          ||
                    (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_LOUDSPEAKER)        ||
                    (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HEADPHONES)         ||
                    (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_PORTABLE_AUDIO)     ||
                    (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_CAR_AUDIO)          ||
                    (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_HIFI_AUDIO_DEVICE)) {

                    btrMgr_PreCheckDiscoveryStatus (0, BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT);

                    if (gfpcBBTRMgrEventOut) {
                        gfpcBBTRMgrEventOut(lstEventMessage);    /* Post a callback */
                    }

                    if ((ghBTRMgrDevHdlCurStreaming != 0) && (ghBTRMgrDevHdlCurStreaming == p_StatusCB->deviceId)) {
                        /* update the flags as the device is NOT Connected */
                        btrMgr_SetDevConnected(lstEventMessage.m_pairedDevice.m_deviceHandle, 0);
                        btrMgr_SetLastConnectionStatus(0, 0,lstEventMessage.m_pairedDevice.m_deviceHandle, BTRMGR_A2DP_SINK_PROFILE_ID);

                        BTRMGRLOG_INFO ("lstEventMessage.m_pairedDevice.m_deviceType = %d\n", lstEventMessage.m_pairedDevice.m_deviceType);
                        /* Stop the playback which already stopped internally but to free up the memory */
                        if (BTRMGR_RESULT_SUCCESS != BTRMGR_StopAudioStreamingOut (0, ghBTRMgrDevHdlCurStreaming)) {
                            BTRMGRLOG_ERROR ("BTRMGR_StopAudioStreamingOut error \n ");  //CID:41354 - Checked return
                        }
                    }
                    btrMgr_PostCheckDiscoveryStatus (0, BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT);
                }
                else if ((BTRMGR_DEVICE_TYPE_HID == lstEventMessage.m_pairedDevice.m_deviceType) ||
                         (BTRMGR_DEVICE_TYPE_HID_GAMEPAD == lstEventMessage.m_pairedDevice.m_deviceType)) {
                    unsigned int ui32MProductId = 0, MVendorId = 0;
                    if (eBTRMgrSuccess == btrMgr_GetDeviceProductDetails(lstEventMessage.m_pairedDevice.m_deviceHandle, &ui32MProductId, &MVendorId)) {
                        if ((BTRMGR_XBOX_ELITE_VENDOR_ID == MVendorId) && (BTRMGR_XBOX_ELITE_PRODUCT_ID == ui32MProductId)) {
                            gEliteIncomCon = 0;
                            btrMgr_PostDeviceOORHoldOffTimer(p_StatusCB);
                        } else {
                            if (gfpcBBTRMgrEventOut) {
                                gfpcBBTRMgrEventOut(lstEventMessage);    /* Post a callback */
                            }
                        }
                    } else {
                        if (gfpcBBTRMgrEventOut) {
                            gfpcBBTRMgrEventOut(lstEventMessage);
                        }
                    }
                    btrMgr_SetDevConnected(lstEventMessage.m_pairedDevice.m_deviceHandle, 0);
                }
#ifdef LE_MODE
                else if (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_XBB) {
                   BTRMGRLOG_INFO("Battery device connection lost ....\n");
                   gXbbConnected = FALSE;
                }
#endif
                else if (lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_SMARTPHONE ||
                         lstEventMessage.m_pairedDevice.m_deviceType == BTRMGR_DEVICE_TYPE_TABLET) {
                    /* Stop the playback which already stopped internally but to free up the memory */
                    if ((ghBTRMgrDevHdlCurStreaming != 0) && (ghBTRMgrDevHdlCurStreaming == p_StatusCB->deviceId)) {
                        btrMgr_SetLastConnectionStatus(0, 0,lstEventMessage.m_pairedDevice.m_deviceHandle, BTRMGR_A2DP_SRC_PROFILE_ID);
                        //CID 41286: Unchecked return value (CHECKED_RETURN)
                        if (BTRMGR_RESULT_SUCCESS != BTRMGR_StopAudioStreamingIn(0, ghBTRMgrDevHdlCurStreaming)) {
                            BTRMGRLOG_ERROR ("BTRMGR_StopAudioStreamingIn error \n ");
                        }
                        ghBTRMgrDevHdlLastConnected = 0;
                    }
                }
            }
            gIsUserInitiated = 0;
            break;
        case enBTRCoreDevStPlaying:
#ifndef LE_MODE
            ui8BtrMgrAvdtpReconnections = 0;
#endif
            if (btrMgr_MapDeviceTypeFromCore(p_StatusCB->eDeviceClass) == BTRMGR_DEVICE_TYPE_SMARTPHONE ||
                btrMgr_MapDeviceTypeFromCore(p_StatusCB->eDeviceClass) == BTRMGR_DEVICE_TYPE_TABLET) {
                btrMgr_MapDevstatusInfoToEventInfo ((void*)p_StatusCB, &lstEventMessage, BTRMGR_EVENT_RECEIVED_EXTERNAL_PLAYBACK_REQUEST);

                if (gfpcBBTRMgrEventOut) {
                    gfpcBBTRMgrEventOut(lstEventMessage);    /* Post a callback */
                }
            }
            break;
        case enBTRCoreDevStOpReady:
            if ((btrMgr_MapDeviceTypeFromCore(p_StatusCB->eDeviceClass) == BTRMGR_DEVICE_TYPE_TILE) ||
                (btrMgr_MapDeviceTypeFromCore(p_StatusCB->eDeviceClass) == BTRMGR_DEVICE_TYPE_XBB)) {
                MEMSET_S(lstEventMessage.m_deviceOpInfo.m_deviceAddress, BTRMGR_NAME_LEN_MAX, '\0', BTRMGR_NAME_LEN_MAX);
                MEMSET_S(lstEventMessage.m_deviceOpInfo.m_name, BTRMGR_NAME_LEN_MAX, '\0', BTRMGR_NAME_LEN_MAX);
                MEMSET_S(lstEventMessage.m_deviceOpInfo.m_uuid, BTRMGR_MAX_STR_LEN, '\0', BTRMGR_MAX_STR_LEN);
                btrMgr_MapDevstatusInfoToEventInfo ((void*)p_StatusCB, &lstEventMessage, BTRMGR_EVENT_DEVICE_OP_READY);
                //Not a big fan of devOpResponse. We should think of a better way to do this
                MEMSET_S(lstEventMessage.m_deviceOpInfo.m_notifyData, BTRMGR_MAX_DEV_OP_DATA_LEN, '\0', BTRMGR_MAX_DEV_OP_DATA_LEN);
                strncpy(lstEventMessage.m_deviceOpInfo.m_notifyData, p_StatusCB->devOpResponse,
                            strlen(p_StatusCB->devOpResponse) < BTRMGR_MAX_DEV_OP_DATA_LEN ? strlen(p_StatusCB->devOpResponse) : BTRMGR_MAX_DEV_OP_DATA_LEN - 1);

                /* Post a callback */
                if (gfpcBBTRMgrEventOut) {
                    gfpcBBTRMgrEventOut(lstEventMessage);
                }
            }
            break;
        case enBTRCoreDevStOpInfo:
            BTRMGRLOG_INFO(">>>>> %d <<<<\n",(BTRMGR_LeOp_t)p_StatusCB->eCoreLeOper);
            if (btrMgr_MapDeviceTypeFromCore(p_StatusCB->eDeviceClass) == BTRMGR_DEVICE_TYPE_TILE) {
                MEMSET_S(lstEventMessage.m_deviceOpInfo.m_deviceAddress, BTRMGR_NAME_LEN_MAX, '\0', BTRMGR_NAME_LEN_MAX);
                MEMSET_S(lstEventMessage.m_deviceOpInfo.m_name, BTRMGR_NAME_LEN_MAX, '\0', BTRMGR_NAME_LEN_MAX);
                MEMSET_S(lstEventMessage.m_deviceOpInfo.m_uuid, BTRMGR_MAX_STR_LEN, '\0', BTRMGR_MAX_STR_LEN);
                btrMgr_MapDevstatusInfoToEventInfo ((void*)p_StatusCB, &lstEventMessage, BTRMGR_EVENT_DEVICE_OP_INFORMATION);
                //Not a big fan of devOpResponse. We should think of a better way to do this
                MEMSET_S(lstEventMessage.m_deviceOpInfo.m_notifyData, BTRMGR_MAX_DEV_OP_DATA_LEN, '\0', BTRMGR_MAX_DEV_OP_DATA_LEN);
                strncpy(lstEventMessage.m_deviceOpInfo.m_notifyData, p_StatusCB->devOpResponse,
                            strlen(p_StatusCB->devOpResponse) < BTRMGR_MAX_DEV_OP_DATA_LEN ? strlen(p_StatusCB->devOpResponse) : BTRMGR_MAX_DEV_OP_DATA_LEN - 1);

                /* Post a callback */
                if (gfpcBBTRMgrEventOut) {
                    gfpcBBTRMgrEventOut(lstEventMessage);
                }
            }
            else if (btrMgr_MapDeviceTypeFromCore(p_StatusCB->eDeviceClass) == BTRMGR_DEVICE_TYPE_XBB) {
                if (BTRMGR_LE_OP_READ_VALUE == (BTRMGR_LeOp_t)p_StatusCB->eCoreLeOper) {
                    BTRMGRLOG_INFO("Battery Notification value : %s \n",p_StatusCB->devOpResponse);
                    btrMgr_MapDevstatusInfoToEventInfo((void*)p_StatusCB, &lstEventMessage, BTRMGR_EVENT_BATTERY_INFO);
                    MEMSET_S(lstEventMessage.m_batteryInfo.m_notifyData, BTRMGR_MAX_DEV_OP_DATA_LEN, '\0', BTRMGR_MAX_DEV_OP_DATA_LEN);

                    strncpy(lstEventMessage.m_batteryInfo.m_notifyData, p_StatusCB->devOpResponse,
                            strlen(p_StatusCB->devOpResponse) < BTRMGR_MAX_DEV_OP_DATA_LEN ? strlen(p_StatusCB->devOpResponse) : BTRMGR_MAX_DEV_OP_DATA_LEN - 1);

                    /* Post a callback */
                    if (gfpcBBTRMgrEventOut) {
                        gfpcBBTRMgrEventOut(lstEventMessage);
                    }
                } else if (BTRMGR_LE_OP_START_NOTIFY == (BTRMGR_LeOp_t)p_StatusCB->eCoreLeOper) {
                    BTRMGRLOG_INFO("Storing start notify UUID : %s \n",p_StatusCB->uuid);

#ifdef LE_MODE
                    if (eBTRMgrSuccess != BTRMgr_BatteryStartNotifyChar(gstBTRMgrBatteryInfo.hBTRMgrBatteryHdl,p_StatusCB->uuid)) {
                        BTRMGRLOG_INFO ("Failed to store BTRMgr Battery Start Notify characteristics \n");
                    } else {
                        BTRMGRLOG_INFO ("Stored BTRMgr Battery Start Notify characteristics succesfully\n");
                    }
#endif
                }
            }
            else if (BTRMGR_LE_OP_READ_VALUE == (BTRMGR_LeOp_t)p_StatusCB->eCoreLeOper) {
                btrMgr_MapDevstatusInfoToEventInfo((void*)p_StatusCB, &lstEventMessage, BTRMGR_EVENT_DEVICE_OP_INFORMATION);
                /* Post a callback */
                if (gfpcBBTRMgrEventOut) {
                    gfpcBBTRMgrEventOut(lstEventMessage);
                }

#ifndef LE_MODE
                /* Max 10 sec timeout - Polled at 100ms second interval */
                {
                    unsigned int ui32sleepIdx = 100;

                    do {
                        usleep(100000);
                    } while ((gEventRespReceived == 0) && (--ui32sleepIdx));

                }
                strncpy(p_StatusCB->devOpResponse, gLeReadOpResponse, BTRMGR_MAX_STR_LEN - 1);
#else
                BTRMGR_SysDiagInfo(0, lstEventMessage.m_deviceOpInfo.m_uuid, lstEventMessage.m_deviceOpInfo.m_readData, lstEventMessage.m_deviceOpInfo.m_leOpType);
                BTRMGRLOG_INFO("reading  %s for UUID %s\n", lstEventMessage.m_deviceOpInfo.m_readData, lstEventMessage.m_deviceOpInfo.m_uuid);
                strncpy(p_StatusCB->devOpResponse, lstEventMessage.m_deviceOpInfo.m_readData, BTRMGR_MAX_DEV_OP_DATA_LEN - 1);
#endif
            }
            else if(BTRMGR_LE_OP_WRITE_VALUE == (BTRMGR_LeOp_t)p_StatusCB->eCoreLeOper) {
                btrMgr_MapDevstatusInfoToEventInfo((void*)p_StatusCB, &lstEventMessage, BTRMGR_EVENT_DEVICE_OP_INFORMATION);
                MEMSET_S(lstEventMessage.m_deviceOpInfo.m_writeData, BTRMGR_MAX_DEV_OP_DATA_LEN, '\0', BTRMGR_MAX_DEV_OP_DATA_LEN);
                strncpy(lstEventMessage.m_deviceOpInfo.m_writeData, p_StatusCB->devOpResponse,
                            strlen(p_StatusCB->devOpResponse) < BTRMGR_MAX_DEV_OP_DATA_LEN ? strlen(p_StatusCB->devOpResponse) : BTRMGR_MAX_DEV_OP_DATA_LEN - 1);

                /* Post a callback */
                if (gfpcBBTRMgrEventOut) {
                    gfpcBBTRMgrEventOut(lstEventMessage);
                }
            }
            else if (BTRMGR_LE_OP_START_NOTIFY == (BTRMGR_LeOp_t)p_StatusCB->eCoreLeOper)
            {

                BTRMGRLOG_TRACE("Start Notify uuid : %s\n", p_StatusCB->uuid);

#ifdef LE_MODE
                // Is it provision notify?
                if (!g_strcmp0(BTRMGR_UUID_PROVISION_STATUS, p_StatusCB->uuid))
                {
                    if (gProvisionNotifyTimerHdl != 0)  {
                        g_source_destroy(g_main_context_find_source_by_id(gmainContext, gProvisionNotifyTimerHdl));
                        BTRMGRLOG_TRACE("Previous Stop Notify for provision uuid : %s.\n", p_StatusCB->uuid);
                        gProvisionNotifyTimerHdl = 0;
                    }

                    BTRMGRLOG_TRACE("Start Notify for privision status.\n");
                    GSource* source = g_timeout_source_new(5000);
                    g_source_set_priority(source, G_PRIORITY_DEFAULT);
                    g_source_set_callback(source, (GSourceFunc)btrMgr_ProvisionNotifyUpdateCb, p_StatusCB->uuid, NULL);

                    gProvisionNotifyTimerHdl = g_source_attach(source, gmainContext);
                    g_source_unref(source);

                    BTRMGRLOG_TRACE("Add provision notify timer ID : %d\n", gProvisionNotifyTimerHdl);
                }
                else
                {
                    BTRMGRLOG_TRACE("No corresponding action for the notify!\n");
                }
#endif
            }
            else if (BTRMGR_LE_OP_STOP_NOTIFY == (BTRMGR_LeOp_t)p_StatusCB->eCoreLeOper)
            {
#ifdef LE_MODE
                if (!g_strcmp0(BTRMGR_UUID_PROVISION_STATUS, p_StatusCB->uuid) && gProvisionNotifyTimerHdl != 0)
                {
                    g_source_destroy(g_main_context_find_source_by_id(gmainContext, gProvisionNotifyTimerHdl));
                    BTRMGRLOG_TRACE("Stop Notify for provision uuid : %s.\n", p_StatusCB->uuid);
                    gProvisionNotifyTimerHdl = 0;
                }
#endif
            }
            else {
            }
            break;
        case enBTRCoreDevStUnsupported:
            BTRMGRLOG_INFO("enBTRCoreDevStUnsupported event notification!\n");
            btrMgr_GetDiscoveredDevInfo (p_StatusCB->deviceId, &lstEventMessage.m_discoveredDevice);
            lstEventMessage.m_discoveredDevice.m_deviceHandle       = p_StatusCB->deviceId;
            lstEventMessage.m_discoveredDevice.m_deviceType         = (p_StatusCB->eDeviceType == enBTRCoreHID) ? BTRMGR_DEVICE_TYPE_HID : lBtrMgrDevType;
            lstEventMessage.m_discoveredDevice.m_isPairedDevice     = p_StatusCB->isPaired;
            lstEventMessage.m_discoveredDevice.m_ui32DevClassBtSpec = p_StatusCB->ui32DevClassBtSpec;
            lstEventMessage.m_discoveredDevice.m_ui16DevAppearanceBleSpec = p_StatusCB->ui16DevAppearanceBleSpec;
            strncpy(lstEventMessage.m_discoveredDevice.m_name, p_StatusCB->deviceName,
                        strlen(p_StatusCB->deviceName) < BTRMGR_NAME_LEN_MAX ? strlen (p_StatusCB->deviceName) : BTRMGR_NAME_LEN_MAX - 1);
            strncpy(lstEventMessage.m_discoveredDevice.m_deviceAddress, p_StatusCB->deviceAddress,
                        strlen(p_StatusCB->deviceAddress) < BTRMGR_NAME_LEN_MAX ? strlen (p_StatusCB->deviceAddress) : BTRMGR_NAME_LEN_MAX - 1);
            lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_UNSUPPORTED;

            if (gfpcBBTRMgrEventOut) {
                gfpcBBTRMgrEventOut(lstEventMessage);  /* Post a callback */
            }
            break;
        default:
            break;
        }
    }

    return lenBtrCoreRet;
}

#ifdef LE_MODE
static gboolean 
btrMgr_ProvisionNotifyUpdateCb(gpointer usrData)
{
    char lPropertyValue[BTRMGR_MAX_STR_LEN] = {'\0'};
    BTRMGR_SysDiagInfo(0, (char *)usrData, lPropertyValue, BTRMGR_LE_OP_READ_VALUE);
    BTRMGRLOG_TRACE("Read value from sysdiag in notify : %s\n", lPropertyValue);

    // for testing
    static int num = 11;
    static char lPropertyValueTest[BTRMGR_MAX_STR_LEN] = "";
    snprintf(lPropertyValueTest, sizeof(lPropertyValueTest), "%d", num++);
    BTRMGR_LE_SetGattPropertyValue(0, (char *)usrData, lPropertyValueTest, BTRMGR_LE_PROP_CHAR);

    /* Add condition to notify only if different from the last */
    if (strncmp(gPrePropertyValue, lPropertyValue, BTRMGR_MAX_STR_LEN - 1))
    {
        // after testing, enable here, and delete the test code!!!
        // BTRMGR_LE_SetGattPropertyValue(0, (char *) usrData, lPropertyValue, BTRMGR_LE_PROP_CHAR);
        strncpy(gPrePropertyValue, lPropertyValue, BTRMGR_MAX_STR_LEN - 1);
    }

    return G_SOURCE_CONTINUE;
}
#endif

STATIC enBTRCoreRet
btrMgr_DeviceDiscoveryCb (
    stBTRCoreDiscoveryCBInfo*   astBTRCoreDiscoveryCbInfo,
    void*                       apvUserData
) {
    enBTRCoreRet        lenBtrCoreRet   = enBTRCoreSuccess;

    BTRMGRLOG_TRACE ("callback type = %d\n", astBTRCoreDiscoveryCbInfo->type);

    if (astBTRCoreDiscoveryCbInfo->type == enBTRCoreOpTypeDevice) {
        BTRMGR_DiscoveryHandle_t* ldiscoveryHdl  = NULL;

        if ((ldiscoveryHdl = btrMgr_GetDiscoveryInProgress()) || (astBTRCoreDiscoveryCbInfo->device.bFound == FALSE)) { /* Not a big fan of this */
            if (ldiscoveryHdl &&
                (btrMgr_GetDiscoveryDeviceType(ldiscoveryHdl) == BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT)) {
                // Acceptable hack for DELIA-39526
                // We would have already reported a list of discovered devices to XRE/Client of BTRMgr if
                // they have requested for the same. But this list will be invalidated by the Resume and if
                // a Pairing Op is requested based on this stale information it will fail, but we will not end up
                // in a loop repeating the same sequence of events as we understand that on the first resume
                // Bluez/BTRCore will correctly give us the device Name. But if we get a new Audio-Out device which exhibits the
                // same behavior then we will repeat this again. As we do the above only for Audio-Out devices
                // non-Audio-Out devices cannot trigger this logic. So we will perform a focussed Audio-Out internal rescan
                if (((astBTRCoreDiscoveryCbInfo->device.enDeviceType == enBTRCore_DC_WearableHeadset)   ||
                     (astBTRCoreDiscoveryCbInfo->device.enDeviceType == enBTRCore_DC_Loudspeaker)       ||
                     (astBTRCoreDiscoveryCbInfo->device.enDeviceType == enBTRCore_DC_PortableAudio)     ||
                     (astBTRCoreDiscoveryCbInfo->device.enDeviceType == enBTRCore_DC_CarAudio)          ||
                     (astBTRCoreDiscoveryCbInfo->device.enDeviceType == enBTRCore_DC_HIFIAudioDevice)   ||
                     (astBTRCoreDiscoveryCbInfo->device.enDeviceType == enBTRCore_DC_Headphones)) &&
                    (strlen(astBTRCoreDiscoveryCbInfo->device.pcDeviceName) == strlen(astBTRCoreDiscoveryCbInfo->device.pcDeviceAddress)) &&
                    btrMgr_IsDevNameSameAsAddress(astBTRCoreDiscoveryCbInfo->device.pcDeviceName, astBTRCoreDiscoveryCbInfo->device.pcDeviceAddress, strlen(astBTRCoreDiscoveryCbInfo->device.pcDeviceName)) &&
                    !btrMgr_CheckIfDevicePrevDetected(astBTRCoreDiscoveryCbInfo->device.tDeviceId)) {

                    BTRMGR_DiscoveredDevicesList_t  lstDiscDevices;
                    eBTRMgrRet                      lenBtrMgrRet    = eBTRMgrFailure;
                    BTRMGR_Result_t                 lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;

                    gIsDiscoveryOpInternal = TRUE;
                    lenBtrMgrRet = btrMgr_PauseDeviceDiscovery(0, ldiscoveryHdl);
                    BTRMGRLOG_WARN ("Called btrMgr_PauseDeviceDiscovery = %d\n", lenBtrMgrRet);

                    lenBtrMgrResult = BTRMGR_GetDiscoveredDevices_Internal(0, &lstDiscDevices);
                    BTRMGRLOG_WARN ("Stored BTRMGR_GetDiscoveredDevices_Internal = %d\n", lenBtrMgrResult);

                    lenBtrMgrRet = btrMgr_ResumeDeviceDiscovery(0, ldiscoveryHdl);
                    BTRMGRLOG_WARN ("Called btrMgr_ResumeDeviceDiscovery = %d\n", lenBtrMgrRet);
                }
                else {
                    BTRMGR_EventMessage_t lstEventMessage;
                    MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));

                    gIsDiscoveryOpInternal = FALSE;
                    btrMgr_MapDevstatusInfoToEventInfo ((void*)&astBTRCoreDiscoveryCbInfo->device, &lstEventMessage, BTRMGR_EVENT_DEVICE_DISCOVERY_UPDATE);

                    if (gfpcBBTRMgrEventOut) {
                        gfpcBBTRMgrEventOut(lstEventMessage); /*  Post a callback */
                    }
                }
            }
            else {
                BTRMGR_EventMessage_t lstEventMessage;
                MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));

                if (!gIsHidGamePadEnabled &&
                     (astBTRCoreDiscoveryCbInfo->device.enDeviceType == enBTRCore_DC_HID_GamePad)) {
                     BTRMGRLOG_WARN ("BTR HID Gamepad is currently Disabled!\n");
                     return lenBtrCoreRet;
                }

                btrMgr_MapDevstatusInfoToEventInfo ((void*)&astBTRCoreDiscoveryCbInfo->device, &lstEventMessage, BTRMGR_EVENT_DEVICE_DISCOVERY_UPDATE);

                if (gfpcBBTRMgrEventOut) {
                    gfpcBBTRMgrEventOut(lstEventMessage); /*  Post a callback */
                }
            }
        }
    }
    else if (astBTRCoreDiscoveryCbInfo->type == enBTRCoreOpTypeAdapter) {
        BTRMGRLOG_INFO ("adapter number = %d, discoverable = %d, discovering = %d\n",
                astBTRCoreDiscoveryCbInfo->adapter.adapter_number,
                astBTRCoreDiscoveryCbInfo->adapter.discoverable,
                astBTRCoreDiscoveryCbInfo->adapter.bDiscovering);

        gIsAdapterDiscovering = astBTRCoreDiscoveryCbInfo->adapter.bDiscovering;

        if(astBTRCoreDiscoveryCbInfo->adapter.bAdvertisingTimedout) {
            BTRMGRLOG_INFO("Advertisement timeout!!\n");
            BTRMGR_LE_ReleaseAdvertisement(0);
            astBTRCoreDiscoveryCbInfo->adapter.bAdvertisingTimedout = FALSE;
        }
#if 0
        if (gfpcBBTRMgrEventOut && (gIsDiscoveryOpInternal == FALSE)) {
            BTRMGR_EventMessage_t lstEventMessage;
            MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));

            lstEventMessage.m_adapterIndex = astBTRCoreDiscoveryCbInfo->adapter.adapter_number;
            if (astBTRCoreDiscoveryCbInfo->adapter.bDiscovering) {
                lstEventMessage.m_eventType    = BTRMGR_EVENT_DEVICE_DISCOVERY_STARTED;
            }
            else {
                lstEventMessage.m_eventType    = BTRMGR_EVENT_DEVICE_DISCOVERY_COMPLETE;
            }

            gfpcBBTRMgrEventOut(lstEventMessage); /*  Post a callback */
        }
#endif

    }

    return lenBtrCoreRet;
}


STATIC enBTRCoreRet
btrMgr_ConnectionInIntimationCb (
    stBTRCoreConnCBInfo*    apstConnCbInfo,
    int*                    api32ConnInIntimResp,
    void*                   apvUserData
) {
    enBTRCoreRet            lenBtrCoreRet   = enBTRCoreSuccess;
    BTRMGR_Result_t         lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
    eBTRMgrRet              lenBtrMgrRet    = eBTRMgrSuccess;
    BTRMGR_DeviceType_t     lBtrMgrDevType  = BTRMGR_DEVICE_TYPE_UNKNOWN;
    BTRMGR_Events_t         lBtMgrOutEvent  = -1;
    unsigned char           lui8AdapterIdx  = 0;
    BTRMGR_EventMessage_t   lstEventMessage;

     if (!apstConnCbInfo) {
        BTRMGRLOG_ERROR ("Invaliid argument\n");
        return enBTRCoreInvalidArg;
    }

    lBtrMgrDevType = btrMgr_MapDeviceTypeFromCore(apstConnCbInfo->stFoundDevice.enDeviceType);
    BTRMGRLOG_INFO("Printing lBtrMgrDevType = %d",lBtrMgrDevType);
#ifdef LE_MODE
    /* checking on to identify the correct flag to differentiate between smart phone settings call back or from app */
    if( (TRUE == gIsDeviceAdvertising) &&
        ((BTRMGR_DEVICE_TYPE_SMARTPHONE == lBtrMgrDevType) || (BTRMGR_DEVICE_TYPE_TABLET == lBtrMgrDevType) ||
        (BTRMGR_DEVICE_TYPE_UNKNOWN == lBtrMgrDevType)) ) {
        BTRMGRLOG_INFO ("Accept Incoming Connection for LE !\n");
        *api32ConnInIntimResp = 1;
        return lenBtrCoreRet;
    }
#endif

    if(!((BTRMGR_DEVICE_TYPE_HID == lBtrMgrDevType) || (BTRMGR_DEVICE_TYPE_HID_GAMEPAD == lBtrMgrDevType))) {
        if (!gIsAudioInEnabled) {
            BTRMGRLOG_WARN ("Incoming Connection Rejected - BTR AudioIn is currently Disabled!\n");
            *api32ConnInIntimResp = 0;
            return lenBtrCoreRet;
        }
    }

    lenBtrMgrRet = btrMgr_PreCheckDiscoveryStatus(lui8AdapterIdx, lBtrMgrDevType);

    if (eBTRMgrSuccess != lenBtrMgrRet) {
        BTRMGRLOG_ERROR ("Pre Check Discovery State Rejected !!!\n");
        return enBTRCoreFailure;
    }

    if (apstConnCbInfo->ui32devPassKey) {
        BTRMGRLOG_WARN ("Incoming Connection passkey = %06d\n", apstConnCbInfo->ui32devPassKey);
    }

    MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));
    btrMgr_MapDevstatusInfoToEventInfo ((void*)apstConnCbInfo, &lstEventMessage, BTRMGR_EVENT_RECEIVED_EXTERNAL_PAIR_REQUEST);

    /* We mustn't need this conditional check; We must always reset the globals before invoking the Callbacks. But as per code review comments, resetting it only for HID */
    if ((BTRMGR_DEVICE_TYPE_HID == lBtrMgrDevType) || (BTRMGR_DEVICE_TYPE_HID_GAMEPAD == lBtrMgrDevType)) {
        gEventRespReceived = 0;
        gAcceptConnection  = 0;
    }

    if (gfpcBBTRMgrEventOut) {
        gfpcBBTRMgrEventOut(lstEventMessage); /* Post a callback */
    }

    /* Used for PIN DISPLAY request */
    if (!apstConnCbInfo->ucIsReqConfirmation) {
        BTRMGRLOG_WARN ("This paring request does not require a confirmation BUT it might need you to enter the PIN at the specified device\n");
        /* Set the return to true; just in case */
        *api32ConnInIntimResp = 1;
        btrMgr_PostCheckDiscoveryStatus (lui8AdapterIdx, lBtrMgrDevType);
        return lenBtrCoreRet;
    }

    /* Max 25 sec timeout - Polled at 500ms second interval */
    {
        unsigned int ui32sleepIdx = 50;

        do {
            usleep(500000);
        } while ((gEventRespReceived == 0) && (--ui32sleepIdx));

    }

    BTRMGRLOG_ERROR ("you picked %d\n", gAcceptConnection);
    if (gEventRespReceived && gAcceptConnection == 1) {
        BTRMGRLOG_ERROR ("Pin-Passkey accepted\n");
        gAcceptConnection = 0;  //reset variabhle for the next connection
        *api32ConnInIntimResp = 1;
    }
    else {
        BTRMGRLOG_ERROR ("Pin-Passkey Rejected\n");
        gAcceptConnection = 0;  //reset variabhle for the next connection
        *api32ConnInIntimResp = 0;
    }

    gEventRespReceived = 0;

    if (*api32ConnInIntimResp == 1) {
        BTRMGRLOG_INFO ("Paired Successfully\n");
        lenBtrMgrResult = BTRMGR_RESULT_SUCCESS;
        lBtMgrOutEvent  = BTRMGR_EVENT_DEVICE_PAIRING_COMPLETE;
    }
    else {
        BTRMGRLOG_ERROR ("Failed to pair a device\n");
        lenBtrMgrResult = BTRMGR_RESULT_GENERIC_FAILURE;
        lBtMgrOutEvent  = BTRMGR_EVENT_DEVICE_PAIRING_FAILED;
    }


    lstEventMessage.m_adapterIndex = lui8AdapterIdx;
    lstEventMessage.m_eventType    = lBtMgrOutEvent;

    if (gfpcBBTRMgrEventOut) {
        gfpcBBTRMgrEventOut(lstEventMessage); /*  Post a callback */
    }

    btrMgr_PostCheckDiscoveryStatus (lui8AdapterIdx, lBtrMgrDevType);

    (void)lenBtrMgrResult;

    return lenBtrCoreRet;
}


STATIC enBTRCoreRet
btrMgr_ConnectionInAuthenticationCb (
    stBTRCoreConnCBInfo*    apstConnCbInfo,
    int*                    api32ConnInAuthResp,
    void*                   apvUserData
) {
    enBTRCoreRet            lenBtrCoreRet   = enBTRCoreSuccess;
    eBTRMgrRet              lenBtrMgrRet    = eBTRMgrSuccess;
    BTRMGR_DeviceType_t     lBtrMgrDevType  = BTRMGR_DEVICE_TYPE_UNKNOWN;
    unsigned char           lui8AdapterIdx  = 0;


    if (!apstConnCbInfo) {
        BTRMGRLOG_ERROR ("Invaliid argument\n");
        return enBTRCoreInvalidArg;
    }

    lBtrMgrDevType = btrMgr_MapDeviceTypeFromCore(apstConnCbInfo->stKnownDevice.enDeviceType);
    /* Move this check into DeviceClass scope? */
    lenBtrMgrRet = btrMgr_PreCheckDiscoveryStatus(lui8AdapterIdx, lBtrMgrDevType);

    if (eBTRMgrSuccess != lenBtrMgrRet) {
        BTRMGRLOG_ERROR ("Pre Check Discovery State Rejected !!!\n");
        return enBTRCoreFailure;
    }


    if (apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_SmartPhone ||
        apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_Tablet) {

#ifdef LE_MODE
        if(TRUE == gIsDeviceAdvertising)
        {
            BTRMGRLOG_INFO ("Accept Incoming Connection form BT SmartPhone/Tablet \n");
            *api32ConnInAuthResp = 1;
            btrMgr_PostCheckDiscoveryStatus (lui8AdapterIdx, lBtrMgrDevType);
            return lenBtrCoreRet;
        }
#endif

        if (!gIsAudioInEnabled) {
            BTRMGRLOG_WARN ("Incoming Connection Rejected - BTR AudioIn is currently Disabled!\n");
            *api32ConnInAuthResp = 0;
            btrMgr_PostCheckDiscoveryStatus (lui8AdapterIdx, lBtrMgrDevType);
            return lenBtrCoreRet;
        }

        BTRMGRLOG_WARN ("Incoming Connection from BT SmartPhone/Tablet\n");

        if (apstConnCbInfo->stKnownDevice.tDeviceId != ghBTRMgrDevHdlLastConnected) {
            BTRMGR_EventMessage_t lstEventMessage;

            MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));
            btrMgr_MapDevstatusInfoToEventInfo ((void*)apstConnCbInfo, &lstEventMessage, BTRMGR_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST);

            if (gfpcBBTRMgrEventOut) {
                gfpcBBTRMgrEventOut(lstEventMessage);     /* Post a callback */
            }

            /* PairedList updation is necessary for Connect event than Disconnect event */
            BTRMGR_GetPairedDevices (lstEventMessage.m_adapterIndex, &gListOfPairedDevices);

            
            {   /* Max 25 sec timeout - Polled at 500ms second interval */
                unsigned int ui32sleepIdx = 50;

                do {
                    usleep(500000);
                } while ((gEventRespReceived == 0) && (--ui32sleepIdx));
            }


            if (gEventRespReceived == 1) {
                BTRMGRLOG_ERROR ("you picked %d\n", gAcceptConnection);
                if (gAcceptConnection == 1) {
                    BTRMGRLOG_WARN ("Incoming Connection accepted\n");

                    if (ghBTRMgrDevHdlLastConnected) {
                        CheckStatusRetryCount = 0;
                        BTRMGRLOG_INFO ("Disconnecting from previous AudioIn connection(%llu)!\n", ghBTRMgrDevHdlLastConnected);

                        if (BTRMGR_RESULT_SUCCESS != BTRMGR_DisconnectFromDevice(0, ghBTRMgrDevHdlLastConnected)) {
                            BTRMGRLOG_INFO("Initating different thread to check the disconnection status of the device ..\n");
                            btrMgr_SetDeviceDisStatusHoldOffTimer();
                            //return lenBtrCoreRet;
                        }
                    }
                    ghBTRMgrDevHdlDisConStatusCheck = ghBTRMgrDevHdlLastConnected;
                    ghBTRMgrDevHdlLastConnected = lstEventMessage.m_externalDevice.m_deviceHandle;
                }
                else {
                    BTRMGRLOG_ERROR ("Incoming Connection denied\n");
                }

                *api32ConnInAuthResp = gAcceptConnection;
            }
            else {
                BTRMGRLOG_ERROR ("Incoming Connection Rejected\n");
                *api32ConnInAuthResp = 0;
            }

            gEventRespReceived = 0;
        }
        else {
            BTRMGRLOG_ERROR ("Incoming Connection From Dev = %lld Status %d LastConnectedDev = %lld\n", apstConnCbInfo->stKnownDevice.tDeviceId, gAcceptConnection, ghBTRMgrDevHdlLastConnected);
            *api32ConnInAuthResp = gAcceptConnection;
        }
    }
    else if ((apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_WearableHeadset)   ||
             (apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_Loudspeaker)       ||
             (apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_PortableAudio)     ||
             (apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_CarAudio)          ||
             (apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_HIFIAudioDevice)   ||
             (apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_Headphones)) {

        BTRMGRLOG_WARN ("Incoming Connection from BT Speaker/Headset PrevDeviceState : %d CurrDeviceState : %d\n",apstConnCbInfo->eDevicePrevState,apstConnCbInfo->eDeviceCurrState);

#ifdef AUTO_CONNECT_ENABLED
        if ((btrMgr_GetDevPaired(apstConnCbInfo->stKnownDevice.tDeviceId))
             && ghBTRMgrDevHdlCurStreaming == 0 ) {
#else
        if ((btrMgr_GetDevPaired(apstConnCbInfo->stKnownDevice.tDeviceId))
             && ghBTRMgrDevHdlCurStreaming == 0 &&
             (enBTRCoreDevStLost == apstConnCbInfo->eDevicePrevState ||
             enBTRCoreDevStPaired == apstConnCbInfo->eDevicePrevState ||
             enBTRCoreDevStConnecting == apstConnCbInfo->eDeviceCurrState)) {
#endif
            BTRMGRLOG_INFO ("Paired - Last Connected device...\n");
#ifndef LE_MODE
            if (gIsAudOutStartupInProgress != BTRMGR_STARTUP_AUD_INPROGRESS) {
#endif
                BTRMGR_EventMessage_t lstEventMessage;

                MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));
                btrMgr_MapDevstatusInfoToEventInfo ((void*)apstConnCbInfo, &lstEventMessage, BTRMGR_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST);

                //TODO: Check if XRE wants to bring up a Pop-up or Respond
                if (gfpcBBTRMgrEventOut) {
                    gfpcBBTRMgrEventOut(lstEventMessage);     /* Post a callback */
                }

                {   /* Max 200msec timeout - Polled at 50ms second interval */
                    unsigned int ui32sleepIdx = 4;

                    do {
                        usleep(50000);
                    } while ((gEventRespReceived == 0) && (--ui32sleepIdx));

#ifndef AUTO_CONNECT_ENABLED
                    gEventRespReceived = 0;
#endif
                }
#ifndef LE_MODE
            }
#endif
#ifdef AUTO_CONNECT_ENABLED
        if (gEventRespReceived == 0) {
            BTRMGRLOG_INFO("External connection response not received from UI Audio Out device, So rejecting the incoming connection\n");
            *api32ConnInAuthResp = 0;
        } else {
            *api32ConnInAuthResp = gAcceptConnection;
            if (gAcceptConnection) {
                BTRMGRLOG_INFO ("Incoming Connection accepted for Audio Out device based on the response from UI\n");
            } else {
                BTRMGRLOG_INFO ("Incoming Connection rejected for Audio Out device based on the response from UI\n");
            }
            gEventRespReceived = 0;
        }
#else
        BTRMGRLOG_WARN ("Incoming Connection accepted\n");
        *api32ConnInAuthResp = 1;
#endif
        }
        else {
            BTRMGRLOG_ERROR ("Incoming Connection denied\n");
            if (strstr(apstConnCbInfo->stKnownDevice.pcDeviceName, "AirPods") || (apstConnCbInfo->stKnownDevice.ui32VendorId == 834)) {
                btrMgr_ClearDisconnDevHoldOffTimer();
                btrMgr_SetDisconnDevHoldOffTimer(apstConnCbInfo->stKnownDevice.tDeviceId);
            }
            *api32ConnInAuthResp = 0;
         }
    }
    else if ((apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_HID_Keyboard)      ||
             (apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_HID_Mouse)         ||
             (apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_HID_MouseKeyBoard) || 
             (apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_HID_AudioRemote)   ||
             (apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_HID_Joystick)      ||
             (apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_HID_GamePad)) {


        if ((!gIsHidGamePadEnabled) && (apstConnCbInfo->stKnownDevice.enDeviceType == enBTRCore_DC_HID_GamePad)) {
            BTRMGRLOG_WARN ("Incoming Connection Rejected - BTR GamePad is currently Disabled!\n");
            *api32ConnInAuthResp = 0;
            btrMgr_PostCheckDiscoveryStatus (lui8AdapterIdx, lBtrMgrDevType);
            return lenBtrCoreRet;
        }

        BTRMGRLOG_WARN ("Incoming Connection from BT HID device\n");
        /* PairedList updation is necessary for Connect event than Disconnect event */
        BTRMGR_GetPairedDevices (gDefaultAdapterContext.adapter_number, &gListOfPairedDevices);

        unsigned int ui32MProductId = 0, MVendorId = 0;
        if (eBTRMgrSuccess == btrMgr_GetDeviceProductDetails(apstConnCbInfo->stKnownDevice.tDeviceId, &ui32MProductId, &MVendorId)) {
            BTRMGRLOG_INFO("i32MProductId - %u MVendorId - %u\n",ui32MProductId,MVendorId);
        } else {
            BTRMGRLOG_INFO("Getting the product details failed ... \n");
        }

        if (btrMgr_GetDevPaired(apstConnCbInfo->stKnownDevice.tDeviceId) &&
            !((ghBTRMgrDevHdlLastDisconnected == apstConnCbInfo->stKnownDevice.tDeviceId) &&
            ((BTRMGR_XBOX_ELITE_VENDOR_ID == MVendorId && BTRMGR_XBOX_ELITE_PRODUCT_ID == ui32MProductId) ||
            (BTRMGR_XBOX_GAMESIR_VENDOR_ID == MVendorId && BTRMGR_XBOX_GAMESIR_PRODUCT_ID == ui32MProductId) ||
            (BTRMGR_NINTENDO_GAMESIR_VENDOR_ID == MVendorId && BTRMGR_NINTENDO_GAMESIR_PRODUCT_ID == ui32MProductId) ||
            (BTRMGR_XBOX_ADAPTIVE_VENDOR_ID == MVendorId && BTRMGR_XBOX_ADAPTIVE_PRODUCT_ID == ui32MProductId))) &&
            isDeinitInProgress != TRUE &&
            !gbGamepadStandbyMode) {

            if (apstConnCbInfo->stKnownDevice.tDeviceId != ghBTRMgrDevHdlPairingInProgress &&
                apstConnCbInfo->stKnownDevice.tDeviceId != ghBTRMgrDevHdlConnInProgress) {
                BTRMGR_EventMessage_t lstEventMessage;
                MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));
                btrMgr_MapDevstatusInfoToEventInfo ((void*)apstConnCbInfo, &lstEventMessage, BTRMGR_EVENT_RECEIVED_EXTERNAL_CONNECT_REQUEST);

                //TODO: Check if XRE wants to bring up a Pop-up or Respond
                if (gfpcBBTRMgrEventOut) {
                    gfpcBBTRMgrEventOut(lstEventMessage);     /* Post a callback */
                }

#ifdef AUTO_CONNECT_ENABLED
                BTRMGRLOG_INFO ("Wating for the external connection response from UI for HID device\n");
                {   /* Max 2 sec timeout - Polled at 50ms second interval */
                    unsigned int ui32sleepIdx = 40;

                    do {
                        usleep(50000);
                    } while ((gEventRespReceived == 0) && (--ui32sleepIdx));


                    if (gEventRespReceived == 0) {
                        BTRMGRLOG_INFO ("External connection response not received from UI for HID device, So rejecting the incoming connection\n");
                        *api32ConnInAuthResp = 0;
                    } else {
                        *api32ConnInAuthResp = gAcceptConnection;
                        if (gAcceptConnection) {
                            BTRMGRLOG_INFO ("Incoming Connection accepted for HID device based on the response from UI\n");
                        } else {
                            BTRMGRLOG_INFO ("Incoming Connection rejected for HID device based on the response from UI\n");
                        }
                    }
                }
#else
                *api32ConnInAuthResp = gAcceptConnection;
#endif //AUTO_CONNECT_ENABLED

                /* Updating this flag here to allow posting the out of range event since the HID device
                 * is getting autoconnected after disconnect when it has been powered ON.
                 */
                 if (*api32ConnInAuthResp == 1) {
                     gIsUserInitiated = 0;
                 }

                 gEventRespReceived = 0;
            } else {
                BTRMGRLOG_INFO ("Pairing or Connection in progress, so accepting the incoming connection from external HID device ...\n");
                *api32ConnInAuthResp = 1;
            }

            if ((BTRMGR_XBOX_ELITE_VENDOR_ID == MVendorId) && (BTRMGR_XBOX_ELITE_PRODUCT_ID == ui32MProductId)) {
                gEliteIncomCon = 1;
            }

            BTRMGRLOG_WARN ("Incoming Connection accepted - %d\n", *api32ConnInAuthResp);
        }
        else {
            BTRMGRLOG_ERROR ("Incoming Connection denied\n");
            *api32ConnInAuthResp = 0;
            if (!gbGamepadStandbyMode)
            {
                if (ghBTRMgrDevHdlLastDisconnected == apstConnCbInfo->stKnownDevice.tDeviceId) {
                    BTRMGRLOG_INFO("Restarting the timer to clear the disconnection status\n");
                    btrMgr_ClearDisconnectStatusHoldOffTimer();
                    btrMgr_SetDisconnectStatusHoldOffTimer();
                }
            }
        }
    }
    else
    {
        BTRMGRLOG_ERROR ("Incoming Connection Auth Callback\n");
    }

    btrMgr_PostCheckDiscoveryStatus (lui8AdapterIdx, lBtrMgrDevType);

    return lenBtrCoreRet;
}

#ifndef LE_MODE
STATIC enBTRCoreRet
btrMgr_MediaStatusCb (
    stBTRCoreMediaStatusCBInfo*  mediaStatusCB,
    void*                        apvUserData
) {
    enBTRCoreRet            lenBtrCoreRet   = enBTRCoreSuccess;
    BTRMGR_EventMessage_t   lstEventMessage;
    enBTRCoreDeviceType     lenBTRCoreDevTy = enBTRCoreUnknown;
    enBTRCoreDeviceClass    lenBTRCoreDevCl = enBTRCore_DC_Unknown;

    MEMSET_S(&lstEventMessage, sizeof(lstEventMessage), 0, sizeof(lstEventMessage));

    BTRMGRLOG_INFO ("Received media status callback\n");

    if (mediaStatusCB) {
        stBTRCoreMediaStatusUpdate* mediaStatus = &mediaStatusCB->m_mediaStatusUpdate;

        lstEventMessage.m_mediaInfo.m_deviceHandle = mediaStatusCB->deviceId;
        lstEventMessage.m_mediaInfo.m_deviceType   = btrMgr_MapDeviceTypeFromCore(mediaStatusCB->eDeviceClass);
        strncpy (lstEventMessage.m_mediaInfo.m_name, mediaStatusCB->deviceName, BTRMGR_NAME_LEN_MAX -1);
        lstEventMessage.m_mediaInfo.m_name[BTRMGR_NAME_LEN_MAX -1] = '\0';  //CID:136544 - Buffer size warning

        switch (mediaStatus->eBTMediaStUpdate) {
        case eBTRCoreMediaTrkStStarted:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_TRACK_STARTED;
            gMediaPlaybackStPrev = lstEventMessage.m_eventType;
            MEMCPY_S(&lstEventMessage.m_mediaInfo.m_mediaPositionInfo,sizeof(lstEventMessage.m_mediaInfo.m_mediaPositionInfo), &mediaStatus->m_mediaPositionInfo, sizeof(BTRMGR_MediaPositionInfo_t));
            break;
        case eBTRCoreMediaTrkStPlaying: 
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_TRACK_PLAYING;
            gMediaPlaybackStPrev = lstEventMessage.m_eventType;
            MEMCPY_S(&lstEventMessage.m_mediaInfo.m_mediaPositionInfo,sizeof(lstEventMessage.m_mediaInfo.m_mediaPositionInfo), &mediaStatus->m_mediaPositionInfo, sizeof(BTRMGR_MediaPositionInfo_t));
            break;
        case eBTRCoreMediaTrkStPaused:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_TRACK_PAUSED;
            gMediaPlaybackStPrev = lstEventMessage.m_eventType;
            MEMCPY_S(&lstEventMessage.m_mediaInfo.m_mediaPositionInfo,sizeof(lstEventMessage.m_mediaInfo.m_mediaPositionInfo), &mediaStatus->m_mediaPositionInfo, sizeof(BTRMGR_MediaPositionInfo_t));
            break;
        case eBTRCoreMediaTrkStStopped:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_TRACK_STOPPED;
            gMediaPlaybackStPrev = lstEventMessage.m_eventType;
            MEMCPY_S(&lstEventMessage.m_mediaInfo.m_mediaPositionInfo,sizeof(lstEventMessage.m_mediaInfo.m_mediaPositionInfo), &mediaStatus->m_mediaPositionInfo, sizeof(BTRMGR_MediaPositionInfo_t));
            break;
        case eBTRCoreMediaTrkPosition:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_TRACK_POSITION;
            gMediaPlaybackStPrev = lstEventMessage.m_eventType;
            MEMCPY_S(&lstEventMessage.m_mediaInfo.m_mediaPositionInfo,sizeof(lstEventMessage.m_mediaInfo.m_mediaPositionInfo), &mediaStatus->m_mediaPositionInfo, sizeof(BTRMGR_MediaPositionInfo_t));
            break;
        case eBTRCoreMediaTrkStChanged:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_TRACK_CHANGED;
            gMediaPlaybackStPrev = lstEventMessage.m_eventType;
            MEMCPY_S(&lstEventMessage.m_mediaInfo.m_mediaTrackInfo,sizeof(lstEventMessage.m_mediaInfo.m_mediaTrackInfo), &mediaStatus->m_mediaTrackInfo, sizeof(BTRMGR_MediaTrackInfo_t));
            break;
        case eBTRCoreMediaPlaybackEnded:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_PLAYBACK_ENDED;
            gMediaPlaybackStPrev = lstEventMessage.m_eventType;
            break;
        case eBTRCoreMediaPlyrName:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_PLAYER_NAME;
            strncpy (lstEventMessage.m_mediaInfo.m_mediaPlayerName, mediaStatus->m_mediaPlayerName, BTRMGR_MAX_STR_LEN -1);
            break;
        case eBTRCoreMediaPlyrEqlzrStOff:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_PLAYER_EQUALIZER_OFF;
            break;
        case eBTRCoreMediaPlyrEqlzrStOn:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_PLAYER_EQUALIZER_ON;
            break;
        case eBTRCoreMediaPlyrShflStOff:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_PLAYER_SHUFFLE_OFF;
            break;
        case eBTRCoreMediaPlyrShflStAllTracks:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_PLAYER_SHUFFLE_ALLTRACKS;
            break;
        case eBTRCoreMediaPlyrShflStGroup:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_PLAYER_SHUFFLE_GROUP;
            break;
        case eBTRCoreMediaPlyrRptStOff:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_PLAYER_REPEAT_OFF;
            break;
        case eBTRCoreMediaPlyrRptStSingleTrack:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_PLAYER_REPEAT_SINGLETRACK;
            break;
        case eBTRCoreMediaPlyrRptStAllTracks:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_PLAYER_REPEAT_ALLTRACKS;
            break;
        case eBTRCoreMediaPlyrRptStGroup:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_PLAYER_REPEAT_GROUP;
            break;
        case eBTRCoreMediaPlyrVolume:
            if ((lstEventMessage.m_mediaInfo.m_deviceType == BTRMGR_DEVICE_TYPE_WEARABLE_HEADSET)  ||
                (lstEventMessage.m_mediaInfo.m_deviceType == BTRMGR_DEVICE_TYPE_HANDSFREE)         ||
                (lstEventMessage.m_mediaInfo.m_deviceType == BTRMGR_DEVICE_TYPE_LOUDSPEAKER)       ||
                (lstEventMessage.m_mediaInfo.m_deviceType == BTRMGR_DEVICE_TYPE_HEADPHONES)        ||
                (lstEventMessage.m_mediaInfo.m_deviceType == BTRMGR_DEVICE_TYPE_PORTABLE_AUDIO)    ||
                (lstEventMessage.m_mediaInfo.m_deviceType == BTRMGR_DEVICE_TYPE_CAR_AUDIO)         ||
                (lstEventMessage.m_mediaInfo.m_deviceType == BTRMGR_DEVICE_TYPE_HIFI_AUDIO_DEVICE)) {

                lenBtrCoreRet = BTRCore_GetDeviceTypeClass(ghBTRCoreHdl, mediaStatusCB->deviceId, &lenBTRCoreDevTy, &lenBTRCoreDevCl);
                BTRMGRLOG_DEBUG ("Status = %d\t Device Type = %d\t Device Class = %x\n", lenBtrCoreRet, lenBTRCoreDevTy, lenBTRCoreDevCl);

                gboolean        lbMuted = FALSE;
                unsigned char   lui8Volume = BTRMGR_SO_MAX_VOLUME - 1;
#ifdef RDKTV_PERSIST_VOLUME
                if (btrMgr_GetLastMuteState(0, &lbMuted) == eBTRMgrSuccess) {
                }

               /* Skipped the Media Status info to UI here for 4 seconds, since the volume will be applied to BT device based on the persistent memory.
                */
               if (gSkipVolumeUpdate != FALSE) {
                   BTRMGRLOG_INFO("Skipping the update, since the volume changed based on persistent data ...\n");
                   return enBTRCoreSuccess;
               }

               /* For AUDIO OUT devices, updating the media status info based on the persistent memory and initiated a timer to set the volume */
               if ((ghBTRMgrDevHdlVolSetupInProgress == mediaStatusCB->deviceId) && (lenBTRCoreDevTy == enBTRCoreSpeakers || lenBTRCoreDevTy == enBTRCoreHeadSet) && (btrMgr_GetLastVolume(0, &lui8Volume,mediaStatusCB->deviceId,BTRMGR_A2DP_SINK_PROFILE_ID) == eBTRMgrSuccess) && (lui8Volume != mediaStatus->m_mediaPlayerVolume)) {
                   BTRMGRLOG_INFO("Setting the volume based on the persistent data - %d MP Vol - %d ...\n",lui8Volume,mediaStatus->m_mediaPlayerVolume);
                   gSkipVolumeUpdate = TRUE;
                   btrMgr_ResetSkipVolumeUpdateTimer();
                   btrMgr_SetVolumeHoldOffTimer(mediaStatusCB);
                   lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_MEDIA_STATUS;
                   lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevVolume = lui8Volume;
                   lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevMute = (unsigned char)lbMuted;
                   gui8IsSoDevAvrcpSupported = 1;

                   if (lui8Volume > mediaStatus->m_mediaPlayerVolume)
                       lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd = BTRMGR_MEDIA_CTRL_VOLUMEUP;
                   else if (lui8Volume < mediaStatus->m_mediaPlayerVolume)
                       lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd = BTRMGR_MEDIA_CTRL_VOLUMEDOWN;
                   else
                       lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd = BTRMGR_MEDIA_CTRL_UNKNOWN;
                   break;
               } else {
                   if ((lenBTRCoreDevTy == enBTRCoreSpeakers) || (lenBTRCoreDevTy == enBTRCoreHeadSet)) {
                       if (btrMgr_GetLastVolume(0, &lui8Volume,mediaStatusCB->deviceId,BTRMGR_A2DP_SINK_PROFILE_ID) == eBTRMgrSuccess) {
                       }

                       if (btrMgr_SetLastVolume(0, mediaStatus->m_mediaPlayerVolume,mediaStatusCB->deviceId,BTRMGR_A2DP_SINK_PROFILE_ID) == eBTRMgrSuccess) {
                       }
                   } else if ((lenBTRCoreDevTy == enBTRCoreMobileAudioIn) || (lenBTRCoreDevTy == enBTRCorePCAudioIn)) {
                       if (btrMgr_GetLastVolume(0, &lui8Volume,mediaStatusCB->deviceId,BTRMGR_A2DP_SRC_PROFILE_ID) == eBTRMgrSuccess) {
                       }

                       if (btrMgr_SetLastVolume(0, mediaStatus->m_mediaPlayerVolume,mediaStatusCB->deviceId,BTRMGR_A2DP_SRC_PROFILE_ID) == eBTRMgrSuccess) {
                       }
                   }
                   lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_MEDIA_STATUS;
                   lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevVolume = mediaStatus->m_mediaPlayerVolume;
                   lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevMute = (unsigned char)lbMuted;
                   gui8IsSoDevAvrcpSupported = 1;      // TODO: Find a better way to do this

                   if (mediaStatus->m_mediaPlayerVolume > lui8Volume)
                       lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd = BTRMGR_MEDIA_CTRL_VOLUMEUP;
                   else if (mediaStatus->m_mediaPlayerVolume < lui8Volume)
                       lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd = BTRMGR_MEDIA_CTRL_VOLUMEDOWN;
                   else
                       lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd = BTRMGR_MEDIA_CTRL_UNKNOWN;
            }
#else
               lstEventMessage.m_eventType = BTRMGR_EVENT_DEVICE_MEDIA_STATUS;
               lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevVolume = mediaStatus->m_mediaPlayerVolume;
               lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_ui8mediaDevMute = (unsigned char)lbMuted;
               gui8IsSoDevAvrcpSupported = 1;      // TODO: Find a better way to do this

               if (mediaStatus->m_mediaPlayerVolume > lui8Volume)
                   lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd = BTRMGR_MEDIA_CTRL_VOLUMEUP;
               else if (mediaStatus->m_mediaPlayerVolume < lui8Volume)
                   lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd = BTRMGR_MEDIA_CTRL_VOLUMEDOWN;
               else
                   lstEventMessage.m_mediaInfo.m_mediaDevStatus.m_enmediaCtrlCmd = BTRMGR_MEDIA_CTRL_UNKNOWN;
#endif
            }
            else {
                lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_PLAYER_VOLUME;
                lstEventMessage.m_mediaInfo.m_mediaPlayerVolume = mediaStatus->m_mediaPlayerVolume;
            }
            break;
        case eBTRCoreMediaPlyrDelay:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_PLAYER_DELAY;
            lstEventMessage.m_mediaInfo.m_mediaPlayerDelay = mediaStatus->m_mediaPlayerDelay;
            stBTRCoreBTDevice   pstMediaDeviceInfo;
            MEMSET_S(&pstMediaDeviceInfo, sizeof(pstMediaDeviceInfo), 0, sizeof(pstMediaDeviceInfo));
            btrMgr_GetDeviceDetails (mediaStatusCB->deviceId, &pstMediaDeviceInfo);
            if (!strstr(pstMediaDeviceInfo.pcDeviceName, "Lindy"))
            {
                btrMgr_UpdateDynamicDelay(mediaStatus->m_mediaPlayerDelay);
            }
            else {
                BTRMGRLOG_INFO("Refusing dynamic delay requst for Lindy device\n");
            }
            break;
        case eBTRCoreMediaElementInScope:
            switch (mediaStatus->m_mediaElementInfo.eAVMedElementType) {
            case enBTRCoreMedETypeAlbum:
                lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_ALBUM_INFO;
                break;
            case enBTRCoreMedETypeArtist:
                lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_ARTIST_INFO;
                break;
            case enBTRCoreMedETypeGenre:
                lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_GENRE_INFO;
                break;
            case enBTRCoreMedETypeCompilation:
                lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_COMPILATION_INFO;
                break;
            case enBTRCoreMedETypePlayList:
                lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_PLAYLIST_INFO;
                break;
            case enBTRCoreMedETypeTrackList:
                lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_TRACKLIST_INFO;
                break;
            case enBTRCoreMedETypeTrack:
                lstEventMessage.m_eventType = BTRMGR_EVENT_MEDIA_TRACK_INFO;
                break;
            default:
                break;
            }

            lstEventMessage.m_mediaInfo.m_mediaTrackListInfo.m_numberOfElements = 1;
            lstEventMessage.m_mediaInfo.m_mediaTrackListInfo.m_mediaElementInfo[0].m_mediaElementHdl  = mediaStatus->m_mediaElementInfo.ui32MediaElementId;
            
            if ((lstEventMessage.m_mediaInfo.m_mediaTrackListInfo.m_mediaElementInfo[0].m_IsPlayable = mediaStatus->m_mediaElementInfo.bIsPlayable)) {
                MEMCPY_S(&lstEventMessage.m_mediaInfo.m_mediaTrackListInfo.m_mediaElementInfo[0].m_mediaTrackInfo,sizeof(lstEventMessage.m_mediaInfo.m_mediaTrackListInfo.m_mediaElementInfo[0].m_mediaTrackInfo), &mediaStatus->m_mediaElementInfo.m_mediaTrackInfo, sizeof(BTRMGR_MediaTrackInfo_t));
            }
            else {
                strncpy (lstEventMessage.m_mediaInfo.m_mediaTrackListInfo.m_mediaElementInfo[0].m_mediaElementName, mediaStatus->m_mediaElementInfo.m_mediaElementName, BTRMGR_MAX_STR_LEN -1);
            }
            break;
        case eBTRCoreMediaElementOofScope:
            lstEventMessage.m_eventType = BTRMGR_EVENT_MAX;
            break;

        default:
            break;
        }


        if (gfpcBBTRMgrEventOut) {
            gfpcBBTRMgrEventOut(lstEventMessage);    /* Post a callback */
        }


        switch (mediaStatus->eBTMediaStUpdate) {
        case eBTRCoreMediaTrkStStarted:
            break;
        case eBTRCoreMediaTrkStPlaying: 
            break;
        case eBTRCoreMediaTrkStPaused:
            break;
        case eBTRCoreMediaTrkStStopped:
            break;
        case eBTRCoreMediaTrkPosition:
            break;
        case eBTRCoreMediaTrkStChanged:
            break;
        case eBTRCoreMediaPlaybackEnded:
            break;
        case eBTRCoreMediaPlyrName:
            break;
        case eBTRCoreMediaPlyrEqlzrStOff:
            break;
        case eBTRCoreMediaPlyrEqlzrStOn:
            break;
        case eBTRCoreMediaPlyrShflStOff:
            break;
        case eBTRCoreMediaPlyrShflStAllTracks:
            break;
        case eBTRCoreMediaPlyrShflStGroup:
            break;
        case eBTRCoreMediaPlyrRptStOff:
            break;
        case eBTRCoreMediaPlyrRptStSingleTrack:
            break;
        case eBTRCoreMediaPlyrRptStAllTracks:
            break;
        case eBTRCoreMediaPlyrRptStGroup:
            break;
        case eBTRCoreMediaPlyrVolume:
            {
#ifndef STREAM_IN_SUPPORTED
                BTRMGRLOG_INFO("Streaming in not supported\n");
                return eBTRMgrFailure;

#else
                 stBTRMgrMediaStatus lstBtrMgrSiStatus;

                lstBtrMgrSiStatus.eBtrMgrState  = eBTRMgrStateUnknown;
                lstBtrMgrSiStatus.eBtrMgrSFreq  = eBTRMgrSFreqUnknown;
                lstBtrMgrSiStatus.eBtrMgrSFmt   = eBTRMgrSFmtUnknown;
                lstBtrMgrSiStatus.eBtrMgrAChan  = eBTRMgrAChanUnknown;
                lstBtrMgrSiStatus.ui8Volume     = mediaStatus->m_mediaPlayerVolume;
                if (gstBTRMgrStreamingInfo.hBTRMgrSiHdl &&
                    ((lstEventMessage.m_mediaInfo.m_deviceType == BTRMGR_DEVICE_TYPE_SMARTPHONE) ||
                     (lstEventMessage.m_mediaInfo.m_deviceType == BTRMGR_DEVICE_TYPE_TABLET))) {
                    if (BTRMgr_SI_SetStatus(gstBTRMgrStreamingInfo.hBTRMgrSiHdl, &lstBtrMgrSiStatus) != eBTRMgrSuccess) {
                        BTRMGRLOG_WARN ("BTRMgr_SI_SetStatus FAILED - eBTRCoreMediaPlyrVolume\n");
                    }
                }
#endif
            }
            break;
        case eBTRCoreMediaElementInScope:
            break;
        case eBTRCoreMediaElementOofScope:
            break;
        default:
            break;
        }

    }

    return lenBtrCoreRet;
}
#endif

#ifdef LE_MODE
BTRMGR_Result_t
BTRMGR_SetLTEServiceState (
    unsigned char   aui8AdapterIdx,
    unsigned char   aui8State
) {
    int rc;

    if(aui8State == 255) {
        gCellularModemTstOverride = FALSE;
        // It will check the online info anyway, so set to false as default after test reset.
        gCellularModemIsOnline = FALSE;
    } else {
        gCellularModemTstOverride = TRUE;
        gCellularModemIsOnline = ((aui8State == 0) ? FALSE : TRUE);
        BTRMGRLOG_INFO("LTE Service is : %s.\n", gCellularModemIsOnline ? "Enabled" : "Disabled");
    }

    if (gIsDeviceAdvertising == TRUE) {
        rc = BTRMGR_LE_StopAdvertisement(0);
        if (rc == BTRMGR_RESULT_SUCCESS) {
            sleep(10);
            BTRMGRLOG_INFO("Stopping advertisement Success\n");
        }
        else {
            BTRMGRLOG_INFO("Stopping advertisement Failed\n");
        }

    }

    return BTRMGR_RESULT_SUCCESS;
}

BTRMGR_Result_t
BTRMGR_StartLEDeviceActivation (
    void
) {
    if (!gIsBroadcastEnable)
    {
        BTRMGRLOG_INFO("Broadcast is NOT enabled, unable to active the device. \n");
        return BTRMGR_RESULT_GENERIC_FAILURE;
    }

    BTRMGR_Result_t  rc = BTRMGR_RESULT_SUCCESS;
    char lPropertyValue[BTRMGR_MAX_STR_LEN] = {"\0"};
    char modelNumber[BTRMGR_STR_LEN] = {"\0"};
    char serialNumber[BTRMGR_STR_LEN] = {"\0"};
    char deviceMac[BTRMGR_STR_LEN] = {"\0"};
    char ImeiNumber[BTRMGR_STR_LEN] = {"\0"};
    unsigned char discoverable_status = 0;
    int timeout = 0;
    BTRMGR_LeCustomAdvertisement_t stCustomAdv;
    memset(&stCustomAdv, 0, sizeof(BTRMGR_LeCustomAdvertisement_t));

    stCustomAdv.len_flags           = 0x02;
    stCustomAdv.type_flags          = 0x01;
    stCustomAdv.val_flags           = 0x06;
    stCustomAdv.len_comcastflags    = 0x05;
    stCustomAdv.type_comcastflags   = 0x03;
    stCustomAdv.deviceInfo_UUID_LO  = 0x0A;
    stCustomAdv.deviceInfo_UUID_HI  = 0x18;
    stCustomAdv.rdk_diag_UUID_LO    = 0xB0;
    stCustomAdv.rdk_diag_UUID_HI    = 0x01;
    stCustomAdv.len_manuf           = 0x11;
    stCustomAdv.type_manuf          = 0xFF;
    stCustomAdv.company_LO          = 0xA3;
    stCustomAdv.company_HI          = 0x07;
    stCustomAdv.device_model        = 0x0301;

#if 0
    //TODO: Use atoi, to convert the MAC address string into Hex bytes. strncpy will not work here
    if (gListOfAdapters.number_of_adapters != 0) {
        strncpy((char*)stCustomAdv.device_mac, gListOfAdapters.adapterAddr[0], BTRMGR_DEVICE_MAC_LEN);
    }
#endif

    rc = BTRMGR_IsAdapterDiscoverable(0, &discoverable_status);
    if(rc == BTRMGR_RESULT_SUCCESS) {
        if (discoverable_status) {
            printf("<<< Adapter is already discoverable >>>\n");
        }
        else {
            rc = BTRMGR_SetAdapterDiscoverable(0, 1, timeout);
            if (BTRMGR_RESULT_SUCCESS != rc) {
                printf ("SetAdapterDiscoverable failed\n");
            }
            else {
                discoverable_status = 1;
                printf ("SetAdapterDiscoverable Success;\n");
            }
        }
    }

    if(discoverable_status) {

        printf("<<<<invoking BTRMGR_LE_SetServiceInfo from btrMgrBus\n");
        BTRMGR_LE_SetServiceInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, 1);

        printf("Adding characterstic values \n");
        /* Get system ID - device MAC */
        if (BTRMGR_SysDiagInfo(0, BTRMGR_SYSTEM_ID_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE) == BTRMGR_RESULT_SUCCESS) {
            strncpy(deviceMac, lPropertyValue, BTRMGR_STR_LEN - 1);

            //btrMgr_SetCMMac(stCustomAdv.device_mac, lPropertyValue);
            BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_SYSTEM_ID_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
        }

        /* model number */
        if (BTRMGR_SysDiagInfo(0, BTRMGR_MODEL_NUMBER_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE) == BTRMGR_RESULT_SUCCESS) {
            strncpy(modelNumber, lPropertyValue, BTRMGR_STR_LEN - 1);
            BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_MODEL_NUMBER_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
        }

        /*Get HW revision*/
        if (BTRMGR_SysDiagInfo(0, BTRMGR_HARDWARE_REVISION_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE) == BTRMGR_RESULT_SUCCESS) {
            BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_HARDWARE_REVISION_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
        }

        /* Get serial number */
        if (BTRMGR_SysDiagInfo(0, BTRMGR_SERIAL_NUMBER_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE) == BTRMGR_RESULT_SUCCESS) {
            strncpy(serialNumber, lPropertyValue, BTRMGR_STR_LEN - 1);
            strncpy((char*)stCustomAdv.serial_number , lPropertyValue, strlen(lPropertyValue)+1);
            BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_SERIAL_NUMBER_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
        }

        /* Get firmware/software revision */
        if (BTRMGR_SysDiagInfo(0, BTRMGR_FIRMWARE_REVISION_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE) == BTRMGR_RESULT_SUCCESS) {
            BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_FIRMWARE_REVISION_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
            BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_SOFTWARE_REVISION_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
        }

        /* Get manufacturer name */
        if (BTRMGR_SysDiagInfo(0, BTRMGR_MANUFACTURER_NAME_UUID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE) == BTRMGR_RESULT_SUCCESS) {
            BTRMGR_LE_SetGattInfo(0, BTRMGR_DEVICE_INFORMATION_UUID, BTRMGR_MANUFACTURER_NAME_UUID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
        }

#ifdef LE_MODE
    if (gCellularModemIsOnline == TRUE)
        {
           BTRMGR_LE_SetServiceInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, 1);
           
           /* Modem IMEI */
           if (BTRMGR_SysDiagInfo(0, BTRMGR_UUID_MODEM_IMEI, lPropertyValue, BTRMGR_LE_OP_READ_VALUE) == BTRMGR_RESULT_SUCCESS) {
               strncpy(ImeiNumber, lPropertyValue, BTRMGR_STR_LEN - 1);
               BTRMGR_LE_SetGattInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, BTRMGR_UUID_MODEM_IMEI, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
           }

           /* QR CODE */
#if 0
           if (BTRMGR_SysDiagInfo(0, BTRMGR_UUID_QR_CODE, lPropertyValue, BTRMGR_LE_OP_READ_VALUE) == BTRMGR_RESULT_SUCCESS)
#else
           if (strlen(modelNumber) && strlen(serialNumber) && strlen (deviceMac) && strlen(ImeiNumber))
#endif
           {
               snprintf(lPropertyValue, BTRMGR_MAX_STR_LEN-1, "MN:%s.SN:%s.DM:%s.IM:%s", modelNumber, serialNumber, deviceMac, ImeiNumber);
               printf("<<< Qr_code = %s >>>\n", lPropertyValue);
               BTRMGR_LE_SetGattInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, BTRMGR_UUID_QR_CODE, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
           }

           /* Provison status */
           if (BTRMGR_SysDiagInfo(0, BTRMGR_UUID_PROVISION_STATUS, lPropertyValue, BTRMGR_LE_OP_READ_VALUE) == BTRMGR_RESULT_SUCCESS) {
               BTRMGR_LE_SetGattInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, BTRMGR_UUID_PROVISION_STATUS, 0x101, lPropertyValue, BTRMGR_LE_PROP_CHAR);
           }

           /* Sim ICCID */
           if (BTRMGR_SysDiagInfo(0, BTRMGR_UUID_SIM_ICCID, lPropertyValue, BTRMGR_LE_OP_READ_VALUE) == BTRMGR_RESULT_SUCCESS) {
               BTRMGR_LE_SetGattInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, BTRMGR_UUID_SIM_ICCID, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
           }


           /* CELLULAR SIGNAL STRENGTH */
           if (BTRMGR_SysDiagInfo(0, BTRMGR_UUID_CELLULAR_SIGNAL_STRENGTH, lPropertyValue, BTRMGR_LE_OP_READ_VALUE) == BTRMGR_RESULT_SUCCESS) {
               BTRMGR_LE_SetGattInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, BTRMGR_UUID_CELLULAR_SIGNAL_STRENGTH, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
           }

           /* MESH BACKHAUL TYPE */
           if (BTRMGR_SysDiagInfo(0, BTRMGR_UUID_MESH_BACKHAUL_TYPE, lPropertyValue, BTRMGR_LE_OP_READ_VALUE) == BTRMGR_RESULT_SUCCESS) {
               BTRMGR_LE_SetGattInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, BTRMGR_UUID_MESH_BACKHAUL_TYPE, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
           }

           /* WIFI BACKHAUL STATS */
           if (BTRMGR_SysDiagInfo(0, BTRMGR_UUID_WIFI_BACKHAUL_STATS, lPropertyValue, BTRMGR_LE_OP_READ_VALUE) == BTRMGR_RESULT_SUCCESS) {
               BTRMGR_LE_SetGattInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP, BTRMGR_UUID_WIFI_BACKHAUL_STATS, 0x1, lPropertyValue, BTRMGR_LE_PROP_CHAR);
           }
        } else{
            BTRMGRLOG_WARN("Remove Gatt Service : %s \n", BTRMGR_RDK_SERVICE_UUID_SETUP);
            BTRMGR_LE_RemoveServiceInfo(0, BTRMGR_RDK_SERVICE_UUID_SETUP);
        }
#endif /*LE_MODE*/

        printf("Starting the ad\n");
        if ((rc = BTRMGR_LE_StartAdvertisement(0, &stCustomAdv)) == BTRMGR_RESULT_SUCCESS) {
            BTRMGRLOG_INFO("Success\n");
        }
        else {
            BTRMGRLOG_ERROR("Failed; RetCode = %d\n", rc);
        }
    }
    else {
        rc = BTRMGR_RESULT_GENERIC_FAILURE;
        printf("Adapter is not in discoverable mode\n");
    }

    return rc;
}

#ifdef LE_MODE
BTRMGR_Result_t
BTRMGR_SetBatteryOpsState (
    unsigned char   aui8AdapterIdx,
    unsigned char   aui8State
) {
    if ((gIsBatteryOperationsEnabled = aui8State)) {
        BTRMGRLOG_INFO ("Battery Operations are Enabled.\n");
    }
    else {
        BTRMGRLOG_INFO ("Battery Operations are Disabled.\n");
    }

    return BTRMGR_RESULT_SUCCESS;
}

STATIC eBTRMgrRet
btrMgr_BatteryOperations(void)
{
    int i = 0,rc;
    gboolean PairedBatteryDevFound = FALSE;
    eBTRMgrRet lenBtrMgrRet    = eBTRMgrSuccess;
    BTRMGRLOG_INFO(" Initiated Battery Operations\n");

    rc = BTRMgr_BatteryModInit(&gstBTRMgrBatteryInfo.hBTRMgrBatteryHdl);

    if (rc == eBTRMgrSuccess) {
        BTRMGRLOG_INFO ("Initiaized battery module sucessfully\n");
    } else {
        BTRMGRLOG_INFO ("Failed to initialize battery module\n");
    }

    /* Check whether the device is already paired */
    for (; i<gListOfPairedDevices.m_numOfDevices; i++) {
        BTRMGRLOG_INFO("Paired device type - %d \n",gListOfPairedDevices.m_deviceProperty[i].m_deviceType);
        if (gListOfPairedDevices.m_deviceProperty[i].m_deviceType == BTRMGR_DEVICE_TYPE_XBB) {
            PairedBatteryDevFound = TRUE;
            BTRMGRLOG_INFO("Battery is already paired, Trying to connect Handle - %lld\n",gListOfPairedDevices.m_deviceProperty[i].m_deviceHandle);
            gBatteryDevHandle = gListOfPairedDevices.m_deviceProperty[i].m_deviceHandle;
            btrMgr_SetConnectBatteryHoldOffTimer();
            break;
        }
    }

    if (PairedBatteryDevFound != TRUE) {
        rc = BTRMGR_StartDeviceDiscovery(gDefaultAdapterContext.adapter_number, BTRMGR_DEVICE_OP_TYPE_LE);
        if (BTRMGR_RESULT_SUCCESS == rc) {
            BTRMGRLOG_INFO (" Started Device Discovery for battery\n");
        } else {
            BTRMGRLOG_INFO ("Failed to start device discovery for battery\n");
            return eBTRMgrFailure;
        }

        btrMgr_SetDiscoverbatteryDevicesHoldOffTimer();
    }

    return lenBtrMgrRet;
}
#endif

static _enBTRMGR_Mode_t modeVal()
{
    FILE *fp = NULL;
    char line[BTRMGR_MODELINE_MAX_LEN]; //Buffer to hold mode line + '\0'
    _enBTRMGR_Mode_t mode_value = BTRMGR_MODE_UNKNOWN;

    // initialization
    memset(line, '\0', sizeof(line));

    BTRMGRLOG_ERROR("Checking Mesh Active status\n");

    /* Open the command for reading. */
    fp = popen("ovsh s AW_Bluetooth_Config", "r");
    if (fp == NULL) {
        BTRMGRLOG_ERROR("Failed to run ovsh command for AW_Bluetooth_Config\n");
        return BTRMGR_MODE_UNKNOWN;
    }

    /* Read the output a line at a time. */
    while (fgets(line, sizeof(line), fp) != NULL) {
        /* Assign last char to NULL, read only BTRMGR_MODELINE_MAX_LEN chars */
        line[BTRMGR_MODELINE_MAX_LEN - 1] = '\0';

        if (strstr(line, "mode") != NULL) {
            if(strstr(line, "on")) {
                mode_value = BTRMGR_MODE_ON;
            }
            else if(strstr(line, "off")) {
                mode_value = BTRMGR_MODE_OFF;
            }
            /* by default, mode_value is BTRMGR_MODE_UNKNOWN. So no need to
             * assign it again, here in else case. */
            BTRMGRLOG_INFO ("Mesh Mode Value: %d\n", mode_value);
            break; // Exit loop after finding the mode
        }
    }

    BTRMGRLOG_INFO ("Line checked: %s\n", line);

    /* close */
    pclose(fp);
    fp = NULL;
    return mode_value;
}

static gboolean btrMgr_CheckDeviceActivationStatus(gpointer user_data)
{
    char unitActStatus[BTRMGR_MAX_STR_LEN] = {"\0"};
    char lte_wan_status[BTRMGR_MAX_STR_LEN] = {"\0"};
    bool lte_enable;
    static bool isLteEnabled = 0;
    bool newLteStatus = 0;
    int rc;
    _enBTRMGR_Mode_t mode_val = BTRMGR_MODE_UNKNOWN;

    // check broadcast status
    btrMgr_CheckBroadcastAvailability();

    // higher priority to do with broadcast state
    if ((TRUE == gIsDeviceAdvertising) && (FALSE == gIsBroadcastEnable)) {
        BTRMGRLOG_INFO("Broadcast is NOT enabled, stop the device adv. \n");
        BTRMGR_LE_StopAdvertisement(0);
        return G_SOURCE_CONTINUE;
    }

    BTRMGRLOG_INFO("Checking unit activation Status\n");

    mode_val = modeVal();

    BTRMGRLOG_INFO ("mode_val: %d\n", mode_val);

#ifdef LE_MODE
    BTRMGR_SysDiagInfo(0, BTRMGR_UUID_PROVISION_STATUS, unitActStatus, BTRMGR_LE_OP_READ_VALUE);
#endif

    if(unitActStatus[0] != '\0') {
        BTRMGRLOG_INFO ("unitActStatus: %d\n", atoi(unitActStatus));
    }
    else {
        BTRMGRLOG_INFO ("unitActStatus is NULL\n");
    }

    if (((unitActStatus[0] != '\0') && (atoi(unitActStatus) == 1)) && (BTRMGR_MODE_OFF == mode_val)) {

        BTRMGRLOG_INFO("Device is Activated - gIsDeviceAdvertising - %d \n",gIsDeviceAdvertising);
        if (TRUE == gIsDeviceAdvertising) {
            rc = BTRMGR_LE_StopAdvertisement(0);
            if (rc == BTRMGR_RESULT_SUCCESS) {
                BTRMGRLOG_INFO(" stopping advertisement is success \n");
            }
            else {
                BTRMGRLOG_INFO("stopping advertisement is Failed\n");
            }
        }

        /* Initiated battery operations, once the device is activated
         * and stopped the advertisement.
         */
        if (gIsBatteryOperationsEnabled == TRUE) {
            rc = btrMgr_BatteryOperations();
            if (rc == BTRMGR_RESULT_SUCCESS) {
                BTRMGRLOG_INFO("Initiated Battery Operations \n");
            } else {
                BTRMGRLOG_INFO("Initiating Battery Operations Failed\n");
            }
        }
        else {
            //TODO: Disconnect the Battery and if these two timers are active, disable them
            // i.e. g_timeout_add_seconds(BTRMGR_BATTERY_CONNECT_TIME_INTERVAL,btrMgr_ConnectBatteryDevices,NULL);
            // &    g_timeout_add_seconds(BTRMGR_BATTERY_DISCOVERY_TIME_INTERVAL,btrMgr_GetDiscoveredBatteryDevices,NULL);
        }

        return false;
    }
    else {
        if (FALSE == gIsDeviceAdvertising) {
            BTRMGRLOG_INFO(" Starting Device Activation\n");

            if (gCellularModemTstOverride == FALSE) {
                BTRMGR_SD_Check_Cellularmanager_ISOnline(lte_wan_status, &lte_enable);
                if(( 0 == strncmp( lte_wan_status, "CONNECTED", 9 )) && (lte_enable == 1)) {
                    gCellularModemIsOnline = TRUE;
                    isLteEnabled = 1;
                }
            }
            else {
                isLteEnabled = (gCellularModemIsOnline == TRUE) ? 1 : 0;
            }

            rc = BTRMGR_StartLEDeviceActivation();
            if (rc == BTRMGR_RESULT_SUCCESS) {
                BTRMGRLOG_INFO("StartLEDeviceActivation success \n");
            }
            else {
                BTRMGRLOG_INFO("StartLEDeviceActivation Failed\n");
            }
        }
        else {
            BTRMGRLOG_INFO("Checking CellularModem status \n");

            if (gCellularModemTstOverride == FALSE) {
                BTRMGR_SD_Check_Cellularmanager_ISOnline(lte_wan_status, &lte_enable);
                if ((0 == strncmp(lte_wan_status, "CONNECTED", 9)) && (lte_enable == 1)) {
                    BTRMGRLOG_INFO("CellularModem is online ... \n");
                    gCellularModemIsOnline = TRUE;
                    newLteStatus = 1;
                }
                else {
                    BTRMGRLOG_INFO("CellularModem is offline ... \n");
                    gCellularModemIsOnline = FALSE;
                    newLteStatus = 0;
                }
            }
            else {
                newLteStatus = (gCellularModemIsOnline == TRUE) ? 1 : 0;
            }

            if (isLteEnabled != newLteStatus) {
                BTRMGRLOG_INFO(" CellularModem status is changed so stopping and restarting advertisement ...\n");

                isLteEnabled = newLteStatus; // setting to new status
                rc = BTRMGR_LE_StopAdvertisement(0);
                if (rc == BTRMGR_RESULT_SUCCESS) {
                    sleep(10);
                    rc = BTRMGR_StartLEDeviceActivation();
                    if (rc == BTRMGR_RESULT_SUCCESS) {
                        BTRMGRLOG_INFO("Restarting advertisement success...\n");
                    }
                    else {
                        BTRMGRLOG_INFO("StartLEDeviceActivation Failed\n");
                    }
                }
                else {
                    BTRMGRLOG_INFO("stopping advertisement is Failed\n");
                }
            }
            else
                BTRMGRLOG_INFO("Device is already advertising...\n");
        }
    }
    return G_SOURCE_CONTINUE;
}

#ifdef LE_MODE
BTRMGR_Result_t
BTRMGR_LEDeviceActivation (
    void
) {
    int rc = BTRMGR_RESULT_GENERIC_FAILURE;

    btrMgr_CheckBroadcastAvailability();

    if (gdeviceActstChangeTimeOutRef) {
        BTRMGRLOG_DEBUG ("Cancelling previous LEDeviceActivation : %u\n", gdeviceActstChangeTimeOutRef);
        g_source_destroy(g_main_context_find_source_by_id(gmainContext, gdeviceActstChangeTimeOutRef));
        gdeviceActstChangeTimeOutRef = 0;
    }

    GSource* source = g_timeout_source_new(BTRMGR_UNITACTIVATION_STATUS_CHECK_TIME_INTERVAL * 1000);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(source, btrMgr_CheckDeviceActivationStatus , NULL, NULL);

    gdeviceActstChangeTimeOutRef = g_source_attach(source, gmainContext);
    g_source_unref(source);

    if (gdeviceActstChangeTimeOutRef) {
       rc = BTRMGR_RESULT_SUCCESS;
    }

    return rc;
}
#endif /*LE_MODE*/
#endif
/* End of File */



