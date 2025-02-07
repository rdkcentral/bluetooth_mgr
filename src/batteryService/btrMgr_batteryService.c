/* System Headers */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* Ext lib Headers */
#include <glib.h>

#include "btrMgr_logger.h"
#ifdef RDK_LOGGER_ENABLED
int b_rdk_logger_enabled = 0;
#endif

/* Local Headers */
#include "btrCore.h"
#include "btrMgr_Types.h"
#include "btrMgr_batteryService.h"

eBTRMgrRet
BTRMgr_BatteryModInit (
       tBTRMgrBatteryHdl* phBTRMgrBatteryHdl
) {
    eBTRMgrRet          leBtrMgrAcRet  = eBTRMgrSuccess;
    stBTRMgrBatteryHdl* pstBtrMgrBatteryHdl = NULL;

    if ((pstBtrMgrBatteryHdl = (stBTRMgrBatteryHdl*)g_malloc0 (sizeof(stBTRMgrBatteryHdl))) == NULL) {
        BTRMGRLOG_ERROR("Unable to allocate memory\n");
        return eBTRMgrFailure;
    }

    *phBTRMgrBatteryHdl = (tBTRMgrBatteryHdl)pstBtrMgrBatteryHdl;
    return leBtrMgrAcRet;
}

eBTRMgrRet
BTRMgr_BatteryStartNotifyChar(stBTRMgrBatteryHdl* BatteryInfo,
                char *uuid)
{
    eBTRMgrRet leBtrMgrAcRet  = eBTRMgrSuccess;

    BTRMGRLOG_INFO("Storing Battery Notify characteristic UUIDs\n");

    if (BatteryInfo == NULL) {
        BTRMGRLOG_ERROR("Battery Structure not initialized\n");
        return eBTRMgrFailure;
    }

    if (!strcmp(uuid,BTRMGR_UUID_BATTERY_LEVEL)) {
        strncpy (BatteryInfo->stBtrMgrNotifyUuidList.BatteryLevel,uuid,(strlen(uuid) < (BTRMGR_UUID_STR_LEN_MAX - 1)) ? strlen(uuid) : BTRMGR_UUID_STR_LEN_MAX - 1);
    } else if (!strcmp(uuid,BTRMGR_UUID_BATTERY_ERROR_VALUES)) {
        strncpy (BatteryInfo->stBtrMgrNotifyUuidList.ErrorStatus,uuid,(strlen(uuid) < (BTRMGR_UUID_STR_LEN_MAX - 1)) ? strlen(uuid) : BTRMGR_UUID_STR_LEN_MAX - 1);
    } else if (!strcmp(uuid,BTRMGR_UUID_BATTERY_FLAGS)) {
        strncpy (BatteryInfo->stBtrMgrNotifyUuidList.BatteryFlags,uuid,(strlen(uuid) < (BTRMGR_UUID_STR_LEN_MAX - 1)) ? strlen(uuid) : BTRMGR_UUID_STR_LEN_MAX - 1);
    }

    return leBtrMgrAcRet;
}

eBTRMgrRet
BTRMgr_TriggerBatteryStartNotify(BTRMgrDeviceHandle ahBTRMgrDevHdl,
                stBTRMgrBatteryHdl* BatteryInfo,
                tBTRCoreHandle ghBTRCoreHdl)
{
    eBTRMgrRet lenBtrMgrAcRet  = eBTRMgrSuccess;

    if (!strcmp(BatteryInfo->stBtrMgrNotifyUuidList.BatteryLevel,BTRMGR_UUID_BATTERY_LEVEL)) {
        if (enBTRCoreSuccess != BTRCore_PerformLEOp(ghBTRCoreHdl,ahBTRMgrDevHdl,BatteryInfo->stBtrMgrNotifyUuidList.BatteryLevel,enBTRCoreLeOpGStartNotify,NULL,NULL)) {
            BTRMGRLOG_ERROR ("Perform LE Op Start Notify for Battery level on for device  %llu Failed!!!\n", ahBTRMgrDevHdl);
            lenBtrMgrAcRet = eBTRMgrFailure;
        }
    }

    if (!strcmp(BatteryInfo->stBtrMgrNotifyUuidList.ErrorStatus,BTRMGR_UUID_BATTERY_ERROR_VALUES)) {
        if (enBTRCoreSuccess != BTRCore_PerformLEOp(ghBTRCoreHdl,ahBTRMgrDevHdl,BatteryInfo->stBtrMgrNotifyUuidList.ErrorStatus,enBTRCoreLeOpGStartNotify,NULL,NULL)) {
            BTRMGRLOG_ERROR ("Perform LE Op Start Notify for Error Status on for device  %llu Failed!!!\n", ahBTRMgrDevHdl);
            lenBtrMgrAcRet = eBTRMgrFailure;
        }
    }

    if (!strcmp(BatteryInfo->stBtrMgrNotifyUuidList.BatteryFlags,BTRMGR_UUID_BATTERY_FLAGS)) {
        if (enBTRCoreSuccess != BTRCore_PerformLEOp(ghBTRCoreHdl,ahBTRMgrDevHdl,BatteryInfo->stBtrMgrNotifyUuidList.BatteryFlags,enBTRCoreLeOpGStartNotify,NULL,NULL)) {
            BTRMGRLOG_ERROR ("Perform LE Op Start Notify for Battery Flags on for device  %llu Failed!!!\n", ahBTRMgrDevHdl);
            lenBtrMgrAcRet = eBTRMgrFailure;
        }
    }

    return lenBtrMgrAcRet;
}
