#ifndef UNIT_TEST
#define UNIT_TEST
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include "mock_btrMgr_logger.h"
#include "btrMgr_Types.h"
#include "btrMgr_mediaTypes.h"
#include "btrMgr_streamOut.h"
#include "mock_btrMgr_streamOutGst.h"

TEST_FILE("btrMgr_streamOut.c") 

typedef struct _stBTRMgrSOHdl {
    stBTRMgrMediaStatus     lstBtrMgrSoStatus;
    eBTRMgrState            leBtrMgrSoInState;
    fPtr_BTRMgr_SO_StatusCb fpcBSoStatus;
    void*                   pvcBUserData;
#ifdef USE_GST1
    tBTRMgrSoGstHdl         hBTRMgrSoGstHdl;
#endif
} stBTRMgrSOHdl;


void test_BTRMgr_SO_GetDefaultSettingsFailure(void) {


    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, BTRMgr_SO_GetDefaultSettings(NULL));
}

void test_BTRMgr_SO_GetDefaultSettings_NotInitialized(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = NULL;

    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, BTRMgr_SO_GetDefaultSettings(hBTRMgrSoHdl));
}

void test_BTRMgr_SO_GetDefaultSettings_Success(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(1);  

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_SO_GetDefaultSettings(hBTRMgrSoHdl));

    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}
void test_BTRMgr_SO_GetCurrentSettings_NotInitialized(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = NULL;

    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, BTRMgr_SO_GetCurrentSettings(hBTRMgrSoHdl));
}

void test_BTRMgr_SO_GetCurrentSettings_Success(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(1);  

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_SO_GetCurrentSettings(hBTRMgrSoHdl));

    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}
void test_BTRMgr_SO_GetStatus_NotInitialized(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = NULL;
    stBTRMgrMediaStatus mediaStatus;

    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, BTRMgr_SO_GetStatus(hBTRMgrSoHdl, &mediaStatus));
}

void test_BTRMgr_SO_GetStatus_Success(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(1);  
    stBTRMgrMediaStatus mediaStatus;

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_SO_GetStatus(hBTRMgrSoHdl, &mediaStatus));

    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_SetStatus_NotInitialized(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = NULL;
    stBTRMgrMediaStatus mediaStatus;

    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, BTRMgr_SO_SetStatus(hBTRMgrSoHdl, &mediaStatus));
}

void test_BTRMgr_SO_SetStatus_Null(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(1);  
    stBTRMgrMediaStatus* mediaStatus = NULL;

    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, BTRMgr_SO_SetStatus(hBTRMgrSoHdl, mediaStatus));

    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_SetStatus_Invalid_State(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(sizeof(stBTRMgrSOHdl));  
    stBTRMgrMediaStatus mediaStatus;
    mediaStatus.eBtrMgrState = eBTRMgrStatePlaying;  // Set a state which should result in an error
    ((stBTRMgrSOHdl*)hBTRMgrSoHdl)->hBTRMgrSoGstHdl= malloc(1);
    ((stBTRMgrSOHdl*)hBTRMgrSoHdl)->leBtrMgrSoInState = eBTRMgrStatePlaying;
    BTRMgr_SO_GstSetVolume_ExpectAndReturn(((stBTRMgrSOHdl*)hBTRMgrSoHdl)->hBTRMgrSoGstHdl, mediaStatus.ui8Volume ,eBTRMgrSOGstFailure);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_SO_SetStatus(hBTRMgrSoHdl, &mediaStatus));
    // Issue caused after the code added at 536
    free(((stBTRMgrSOHdl*)hBTRMgrSoHdl)->hBTRMgrSoGstHdl);
    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_SetStatus_Invalid_Volume(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(sizeof(stBTRMgrSOHdl));  
    stBTRMgrMediaStatus mediaStatus;
    mediaStatus.ui8Volume = 129;  // Set an invalid volume
    mediaStatus.eBtrMgrState = eBTRMgrStateInitialized;
    ((stBTRMgrSOHdl*)hBTRMgrSoHdl)->hBTRMgrSoGstHdl= malloc(1);
    ((stBTRMgrSOHdl*)hBTRMgrSoHdl)->leBtrMgrSoInState = eBTRMgrStatePlaying;
    BTRMgr_SO_GstSetVolume_ExpectAndReturn(((stBTRMgrSOHdl*)hBTRMgrSoHdl)->hBTRMgrSoGstHdl, mediaStatus.ui8Volume ,eBTRMgrSOGstFailure);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_SO_SetStatus(hBTRMgrSoHdl, &mediaStatus));
    free(((stBTRMgrSOHdl*)hBTRMgrSoHdl)->hBTRMgrSoGstHdl);
    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}
void test_BTRMgr_SO_SetStatusPlaying_Success(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(sizeof(stBTRMgrSOHdl));
    stBTRMgrMediaStatus mediaStatus;
    mediaStatus.eBtrMgrState = eBTRMgrStateInitialized;
    ((stBTRMgrSOHdl*)hBTRMgrSoHdl)->hBTRMgrSoGstHdl= malloc(1);
    ((stBTRMgrSOHdl*)hBTRMgrSoHdl)->leBtrMgrSoInState = eBTRMgrStatePlaying;  // Set a state which should not result in an error
    mediaStatus.ui8Volume = 64;  // Set a valid volume
    BTRMgr_SO_GstSetVolume_ExpectAndReturn(((stBTRMgrSOHdl*)hBTRMgrSoHdl)->hBTRMgrSoGstHdl, mediaStatus.ui8Volume ,eBTRMgrSOGstSuccess);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_SO_SetStatus(hBTRMgrSoHdl, &mediaStatus));
    free(((stBTRMgrSOHdl*)hBTRMgrSoHdl)->hBTRMgrSoGstHdl);
    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_SetStatusPaused_Success(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(sizeof(stBTRMgrSOHdl));  
    stBTRMgrMediaStatus mediaStatus;
    mediaStatus.eBtrMgrState = eBTRMgrStatePaused;
    ((stBTRMgrSOHdl*)hBTRMgrSoHdl)->hBTRMgrSoGstHdl= malloc(1);
    ((stBTRMgrSOHdl*)hBTRMgrSoHdl)->leBtrMgrSoInState = eBTRMgrStatePlaying;  // Set a state which should not result in an error
    mediaStatus.ui8Volume = 64;  // Set a valid volume
    BTRMgr_SO_GstSetInputPaused_ExpectAndReturn(((stBTRMgrSOHdl*)hBTRMgrSoHdl)->hBTRMgrSoGstHdl, 1 ,eBTRMgrSOGstSuccess);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_SO_SetStatus(hBTRMgrSoHdl, &mediaStatus));
    free(((stBTRMgrSOHdl*)hBTRMgrSoHdl)->hBTRMgrSoGstHdl);
    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_SetVolume_BTRMgr_SO_GstSetVolume_Fails(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(sizeof(stBTRMgrSOHdl));  
    stBTRMgrSOHdl* pstBtrMgrSoHdl = (stBTRMgrSOHdl*)hBTRMgrSoHdl;
    unsigned char volume = 64;

    BTRMgr_SO_GstSetVolume_ExpectAndReturn(pstBtrMgrSoHdl->hBTRMgrSoGstHdl, volume, eBTRMgrSOGstFailure);

    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_SO_SetVolume(hBTRMgrSoHdl, volume));
    //free(pstBtrMgrSoHdl);
    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_SetVolume_Success(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(sizeof(stBTRMgrSOHdl));  
    stBTRMgrSOHdl* pstBtrMgrSoHdl = (stBTRMgrSOHdl*)hBTRMgrSoHdl;
    unsigned char volume = 64;

    BTRMgr_SO_GstSetVolume_ExpectAndReturn(pstBtrMgrSoHdl->hBTRMgrSoGstHdl, volume, eBTRMgrSOGstSuccess);

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_SO_SetVolume(hBTRMgrSoHdl, volume));
    TEST_ASSERT_EQUAL(pstBtrMgrSoHdl->lstBtrMgrSoStatus.ui8Volume, volume);
   
    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_GetVolume_Null_Pointers(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = NULL;
    unsigned char* volume = NULL;

    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_SO_GetVolume(hBTRMgrSoHdl, volume));
}

void test_BTRMgr_SO_GetVolume_BTRMgr_SO_GstGetVolume_Fails(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(sizeof(hBTRMgrSoHdl));  
    stBTRMgrSOHdl* pstBtrMgrSoHdl = (stBTRMgrSOHdl*)hBTRMgrSoHdl;
    unsigned char volume ;
    BTRMgr_SO_GstGetVolume_ExpectAndReturn(pstBtrMgrSoHdl->hBTRMgrSoGstHdl, &volume, eBTRMgrSOGstFailure);

    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_SO_GetVolume(hBTRMgrSoHdl, &volume));
  
    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}
void test_BTRMgr_SO_GetVolume_Success(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(sizeof(hBTRMgrSoHdl));  
    stBTRMgrSOHdl* pstBtrMgrSoHdl = (stBTRMgrSOHdl*)hBTRMgrSoHdl;
    unsigned char volume ;

    BTRMgr_SO_GstGetVolume_ExpectAndReturn(pstBtrMgrSoHdl->hBTRMgrSoGstHdl, &volume, eBTRMgrSOGstSuccess);

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_SO_GetVolume(hBTRMgrSoHdl, &volume));
    
    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_SetDelay_null_hBTRMgrSoHdl() {
    tBTRMgrSoHdl hBTRMgrSoHdl = NULL;
    unsigned int delay = 123;

    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, BTRMgr_SO_SetDelay(hBTRMgrSoHdl, delay));
}

void test_BTRMgr_SO_SetDelay_BTMgr_SO_GstSetDelay_fails() {
    unsigned int delay = 123;
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(sizeof(hBTRMgrSoHdl));  
    stBTRMgrSOHdl* pstBtrMgrSoHdl = (stBTRMgrSOHdl*)hBTRMgrSoHdl;
    BTMgr_SO_GstSetDelay_ExpectAndReturn(pstBtrMgrSoHdl->hBTRMgrSoGstHdl, delay, eBTRMgrSOGstFailure);

    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_SO_SetDelay(hBTRMgrSoHdl, delay));

    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_SetDelay_success() {
    unsigned int delay = 123;
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(sizeof(hBTRMgrSoHdl));  
    stBTRMgrSOHdl* pstBtrMgrSoHdl = (stBTRMgrSOHdl*)hBTRMgrSoHdl;
    BTMgr_SO_GstSetDelay_ExpectAndReturn(pstBtrMgrSoHdl->hBTRMgrSoGstHdl, delay, eBTRMgrSOGstSuccess);

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_SO_SetDelay(hBTRMgrSoHdl, delay));
    
    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_GetDelay_Null(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = NULL;
    unsigned int* delay = NULL;
    unsigned int* msInQueue = NULL;

    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_SO_GetDelay(hBTRMgrSoHdl, delay, msInQueue));
}

void test_BTRMgr_SO_GetDelay_BTMgr_SO_GstGetDelay_Fails(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(sizeof(tBTRMgrSoHdl));  
    stBTRMgrSOHdl* pstBtrMgrSoHdl = (stBTRMgrSOHdl*)hBTRMgrSoHdl;
    unsigned int delay =0;
    unsigned int msInQueue =0; 

    BTMgr_SO_GstGetDelay_ExpectAndReturn(pstBtrMgrSoHdl->hBTRMgrSoGstHdl, &delay, &msInQueue, eBTRMgrSOGstFailure);

    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_SO_GetDelay(hBTRMgrSoHdl, &delay, &msInQueue));
    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_GetDelay_Success(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(1);  
    stBTRMgrSOHdl* pstBtrMgrSoHdl = (stBTRMgrSOHdl*)hBTRMgrSoHdl;
    unsigned int delay = 0;
    unsigned int msInQueue = 0;
    BTMgr_SO_GstGetDelay_ExpectAndReturn(pstBtrMgrSoHdl->hBTRMgrSoGstHdl, &delay, &msInQueue, eBTRMgrSOGstSuccess);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_SO_GetDelay(hBTRMgrSoHdl, &delay, &msInQueue));
    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}
#define DUMMY_DEVICE_DELAY  123
#define DUMMY_NEW_DELAY     321

void test_BTRMgr_SO_UpdateDelayDynamically_Success(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(1);  
    unsigned int deviceDelay = DUMMY_DEVICE_DELAY;
    unsigned int newDelay = 0;
    BTRMgr_SO_GstCalculateDelayNeeded_IgnoreAndReturn( eBTRMgrSOGstSuccess);
    BTMgr_SO_GstSetDelay_IgnoreAndReturn( eBTRMgrSOGstSuccess);

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_SO_UpdateDelayDynamically(hBTRMgrSoHdl, deviceDelay));
    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}
void test_BTRMgr_SO_SetMute_BTRMgr_SO_GstSetMute_Fails(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(1);  
    stBTRMgrSOHdl* pstBtrMgrSoHdl = (stBTRMgrSOHdl*)hBTRMgrSoHdl;
    gboolean mute = TRUE;

    BTRMgr_SO_GstSetMute_ExpectAndReturn(pstBtrMgrSoHdl->hBTRMgrSoGstHdl, mute, eBTRMgrSOGstFailure);

    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_SO_SetMute(hBTRMgrSoHdl, mute));

    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_SetMute_Success(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(1);  
    stBTRMgrSOHdl* pstBtrMgrSoHdl = (stBTRMgrSOHdl*)hBTRMgrSoHdl;
    gboolean mute = TRUE;

    BTRMgr_SO_GstSetMute_ExpectAndReturn(pstBtrMgrSoHdl->hBTRMgrSoGstHdl, mute, eBTRMgrSOGstSuccess);

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_SO_SetMute(hBTRMgrSoHdl, mute));

    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}


void test_BTRMgr_SO_GetMute_Null(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = NULL;
    gboolean* mute = NULL;

    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_SO_GetMute(hBTRMgrSoHdl, mute));
}

void test_BTRMgr_SO_GetMute_BTRMgr_SO_GstGetMute_Fails(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(1);  
    stBTRMgrSOHdl* pstBtrMgrSoHdl = (stBTRMgrSOHdl*)hBTRMgrSoHdl;
    gboolean mute = 0;

    BTRMgr_SO_GstGetMute_ExpectAndReturn(pstBtrMgrSoHdl->hBTRMgrSoGstHdl, &mute, eBTRMgrSOGstFailure);

    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_SO_GetMute(hBTRMgrSoHdl, &mute));

    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_GetMute_Success(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(1);  
    stBTRMgrSOHdl* pstBtrMgrSoHdl = (stBTRMgrSOHdl*)hBTRMgrSoHdl;
    gboolean mute = 0;

    BTRMgr_SO_GstGetMute_ExpectAndReturn(pstBtrMgrSoHdl->hBTRMgrSoGstHdl, &mute, eBTRMgrSOGstSuccess);

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_SO_GetMute(hBTRMgrSoHdl, &mute));

    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_GetEstimatedInABufSize_Null(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = NULL;
    stBTRMgrInASettings inASettings;
    stBTRMgrOutASettings outASettings;

    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, BTRMgr_SO_GetEstimatedInABufSize(hBTRMgrSoHdl, &inASettings, &outASettings));
}

void test_BTRMgr_SO_GetEstimatedInABufSize_Null_ASettings(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(1);  
    stBTRMgrInASettings* inASettings = NULL;
    stBTRMgrOutASettings* outASettings = NULL;

    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, BTRMgr_SO_GetEstimatedInABufSize(hBTRMgrSoHdl, inASettings, outASettings));

    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}

void test_BTRMgr_SO_GetEstimatedInABufSize_Null_Args(void) {
    tBTRMgrSoHdl hBTRMgrSoHdl = (tBTRMgrSoHdl*)malloc(1);  
    stBTRMgrInASettings* inSettings = NULL;
    stBTRMgrOutASettings* outSettings = NULL;

    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, BTRMgr_SO_GetEstimatedInABufSize(hBTRMgrSoHdl, inSettings, outSettings));
    
    inSettings = malloc(1000);
    inSettings->eBtrMgrInAType = eBTRMgrATypePCM;

    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, BTRMgr_SO_GetEstimatedInABufSize(hBTRMgrSoHdl, inSettings, outSettings));

    outSettings = malloc(1000);
    outSettings->eBtrMgrOutAType = eBTRMgrATypeSBC;

    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, BTRMgr_SO_GetEstimatedInABufSize(hBTRMgrSoHdl, inSettings, outSettings));

    free(inSettings);
    free(hBTRMgrSoHdl);
    hBTRMgrSoHdl = NULL;
}
/*/
void test_BTRMgr_SO_GetEstimatedInABufSize(void){
    // Init handler and necessary structures with necessary fields
    tBTRMgrSoHdl hBTRMgrSoHdl = = (tBTRMgrSoHdl*)malloc(1);  // Init
    stBTRMgrInASettings apstBtrMgrSoInASettings;
    // Needed fields from the structure
    apstBtrMgrSoInASettings.pstBtrMgrInCodecInfo = 
    apstBtrMgrSoInASettings.eBtrMgrInAType       = eBTRMgrATypePCM;
    // Initialize fields in pstBtrMgrSoInASettings...

    stBTRMgrOutASettings apstBtrMgrSoOutASettings; 
    // Needed fields from the structure
    apstBtrMgrSoOutASettings.pstBtrMgrOutCodecInfo = ...
    apstBtrMgrSoOutASettings.i32BtrMgrDevMtu         = 100; // Example value
    apstBtrMgrSoOutASettings.eBtrMgrOutAType         = eBTRMgrATypeSBC;
    // Initialize fields in apstBtrMgrSoOutASettings...

    eBTRMgrRet result = BTRMgr_SO_GetEstimatedInABufSize(hBTRMgrSoHdl, &apstBtrMgrSoInASettings, &apstBtrMgrSoOutASettings);
    
    // Verify that the function returns the success condition
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}
*
void test_BTRMgr_SO_GetEstimatedInABufSize_HandleIsNull(void) {
    eBTRMgrRet result = BTRMgr_SO_GetEstimatedInABufSize(NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}
*/

void test_BTRMgr_SO_Start_HandleIsNull_ReturnsNotInitialized(void) {
    eBTRMgrRet result = BTRMgr_SO_Start(NULL, NULL, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(eBTRMgrNotInitialized, result, "Expected eBTRMgrNotInitialized when handle is NULL");
}
void test_BTRMgr_SO_Start_InputOutputSettingsAreNull_ReturnsFailInArg(void) {
    tBTRMgrSoHdl mockHandle = (void*)0x01; // Mocked handle
    eBTRMgrRet result = BTRMgr_SO_Start(mockHandle, NULL, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(eBTRMgrFailInArg, result, "Expected eBTRMgrFailInArg when input/output settings are NULL");
}

void test_BTRMgr_SO_Start_ValidInputs_GstStartSuccess_ReturnsSuccess(void) {
    stBTRMgrSOHdl tmpHdl ;
    tBTRMgrSoHdl mockHandle = (tBTRMgrSoHdl)&tmpHdl; 

    stBTRMgrPCMInfo pstBtrMgrSoInPcmInfo = {
        .eBtrMgrSFmt = eBTRMgrSFreq8K,
        .eBtrMgrAChan = eBTRMgrAChanDualChannel,
        .eBtrMgrSFreq = eBTRMgrSFmt8bit
    };
    // Mock input settings initialization
    stBTRMgrInASettings mockInSettings = {
        .eBtrMgrInAType = eBTRMgrATypePCM, 
        .pstBtrMgrInCodecInfo = &pstBtrMgrSoInPcmInfo,
        // Initialize other members of mockInSettings as needed...
    };

    stBTRMgrSBCInfo pstBtrMgrSoOutSbcInfo = {
        .eBtrMgrSbcAChan = eBTRMgrAChanStereo,
        .eBtrMgrSbcSFreq = eBTRMgrSFreq16K,
        .ui8SbcAllocMethod = 0,
        .ui8SbcSubbands =  0,
        .ui8SbcBlockLength = 1,
        .ui8SbcMinBitpool = 0,
        .ui8SbcMaxBitpool =  0,
        .ui16SbcFrameLen =  2,
    };

    // Mock output settings initialization
    stBTRMgrOutASettings mockOutSettings = {
        .eBtrMgrOutAType = eBTRMgrATypeSBC, 
        .pstBtrMgrOutCodecInfo = &pstBtrMgrSoOutSbcInfo
    };


     BTRMgr_SO_GstStart_IgnoreAndReturn(eBTRMgrSOGstSuccess);

    eBTRMgrRet result = BTRMgr_SO_Start(mockHandle, &mockInSettings, &mockOutSettings);

    // Verify the result
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}

void test_BTRMgr_SO_Stop_NullHandle_ReturnsNotInitialized(void) {
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, BTRMgr_SO_Stop(NULL));
}



void test_BTRMgr_SO_Stop_SuccessfulStop_ReturnsSuccess(void) 
{
    //tBTRMgrSoHdl mockHandle = (tBTRMgrSoHdl)0x01; // Example mock handle, replace with actual initialization
    stBTRMgrSOHdl*  mockHandle =(stBTRMgrSOHdl*)malloc(sizeof(stBTRMgrSOHdl));
   BTRMgr_SO_GstStop_ExpectAndReturn(mockHandle->hBTRMgrSoGstHdl, eBTRMgrSOGstSuccess);
    eBTRMgrRet result = BTRMgr_SO_Stop(mockHandle);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
   free(mockHandle);
}

void test_BTRMgr_SO_Stop_FailureInStopping_ReturnsFailure(void) {
    //tBTRMgrSoHdl mockHandle = (tBTRMgrSoHdl)0x02; // Example mock handle, replace with actual initialization
     stBTRMgrSOHdl*  mockHandle = (stBTRMgrSOHdl*)malloc(sizeof(stBTRMgrSOHdl));
    // Setup mock function call expectation
    BTRMgr_SO_GstStop_ExpectAndReturn(mockHandle->hBTRMgrSoGstHdl, eBTRMgrSOGstFailure);

    eBTRMgrRet result = BTRMgr_SO_Stop(mockHandle);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
     free(mockHandle);
}

void test_BTRMgr_SO_Pause_NullHandle_ReturnsNotInitialized(void) {
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, BTRMgr_SO_Pause(NULL));
}

void test_BTRMgr_SO_Pause_SuccessfulPause_ReturnsSuccess(void) {
    //tBTRMgrSoHdl mockHandle = (tBTRMgrSoHdl)1; // Mock handle creation
     stBTRMgrSOHdl*  mockHandle = (stBTRMgrSOHdl*)malloc(sizeof(stBTRMgrSOHdl));;
    BTRMgr_SO_GstPause_ExpectAndReturn(mockHandle->hBTRMgrSoGstHdl, eBTRMgrSOGstSuccess);
    eBTRMgrRet result = BTRMgr_SO_Pause(mockHandle);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
     free(mockHandle);
}

void test_BTRMgr_SO_Pause_FailureInPausing_ReturnsFailure(void) {
   // tBTRMgrSoHdl mockHandle = (tBTRMgrSoHdl)2; // Mock handle creation
     stBTRMgrSOHdl*  mockHandle = (stBTRMgrSOHdl*)malloc(sizeof(stBTRMgrSOHdl));;
    // Mock the failure pause operation
    BTRMgr_SO_GstPause_ExpectAndReturn(mockHandle->hBTRMgrSoGstHdl, eBTRMgrSOGstFailure);

    eBTRMgrRet result = BTRMgr_SO_Pause(mockHandle);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
     free(mockHandle);
}

void test_BTRMgr_SO_Resume_NullHandle_ReturnsNotInitialized(void) {
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, BTRMgr_SO_Resume(NULL));
}

void test_BTRMgr_SO_Resume_SuccessfulResume_ReturnsSuccess(void) {
   // stBTRMgrSoHdl mockHandle = (stBTRMgrSoHdl)malloc(sizeof(stBTRMgrSOHdl));; // Example, replace with your handle creation
    stBTRMgrSOHdl*  mockHandle = (stBTRMgrSOHdl*)malloc(sizeof(stBTRMgrSOHdl));
    BTRMgr_SO_GstResume_ExpectAndReturn(mockHandle->hBTRMgrSoGstHdl, eBTRMgrSOGstSuccess);

    eBTRMgrRet result = BTRMgr_SO_Resume(mockHandle);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
     free(mockHandle);
}

void test_BTRMgr_SO_Resume_FailureInResuming_ReturnsFailure(void) {
    //tBTRMgrSoHdl mockHandle = (tBTRMgrSoHdl)malloc(sizeof(stBTRMgrSOHdl));; // Example, replace with your handle creation
    stBTRMgrSOHdl*  mockHandle = (stBTRMgrSOHdl*)malloc(sizeof(stBTRMgrSOHdl));
    BTRMgr_SO_GstResume_ExpectAndReturn(mockHandle->hBTRMgrSoGstHdl, eBTRMgrSOGstFailure);

    eBTRMgrRet result = BTRMgr_SO_Resume(mockHandle);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
     free(mockHandle);
}

void test_BTRMgr_SO_SendBuffer_NullHandle_ReturnsNotInitialized(void) {
    char dummyBuffer[10];
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, BTRMgr_SO_SendBuffer(NULL, dummyBuffer, sizeof(dummyBuffer)));
}

void test_BTRMgr_SO_SendBuffer_SuccessfulSend_ReturnsSuccess(void) {
   // tBTRMgrSoHdl mockHandle = (tBTRMgrSoHdl)1; // Example, replace with your handle creation
    stBTRMgrSOHdl*  mockHandle = (stBTRMgrSOHdl*)malloc(sizeof(stBTRMgrSOHdl));
    char dummyBuffer[10];

    BTRMgr_SO_GstSendBuffer_ExpectAndReturn(mockHandle->hBTRMgrSoGstHdl, dummyBuffer, sizeof(dummyBuffer), eBTRMgrSOGstSuccess);

    eBTRMgrRet result = BTRMgr_SO_SendBuffer(mockHandle, dummyBuffer, sizeof(dummyBuffer));
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
     free(mockHandle);
}

void test_BTRMgr_SO_SendBuffer_FailureInSending_ReturnsFailure(void) {
    //tBTRMgrSoHdl mockHandle = (tBTRMgrSoHdl)2; // Example, replace with your handle creation
    stBTRMgrSOHdl*  mockHandle = (stBTRMgrSOHdl*)malloc(sizeof(stBTRMgrSOHdl));
    char dummyBuffer[20];

    BTRMgr_SO_GstSendBuffer_ExpectAndReturn(mockHandle->hBTRMgrSoGstHdl, dummyBuffer, sizeof(dummyBuffer), eBTRMgrSOGstFailure);

    eBTRMgrRet result = BTRMgr_SO_SendBuffer(mockHandle, dummyBuffer, sizeof(dummyBuffer));
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
     free(mockHandle);
}

void test_BTRMgr_SO_SendEOS_NullHandle_ReturnsNotInitialized(void) {
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, BTRMgr_SO_SendEOS(NULL));
}
/*#
void test_btrMgr_SO_GstStatusCb_NullUserData_ReturnsSuccess(void) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstSuccess, btrMgr_SO_GstStatusCb(eBTRMgrSOGstStInitialized, NULL));
}


void test_btrMgr_SO_GstStatusCb_Underflow_IncrementsUnderflowCount(void) {
    stBTRMgrSOHdl mockUserData = {0};
    mockUserData.lstBtrMgrSoStatus.ui32UnderFlowCnt = 0;

    btrMgr_SO_GstStatusCb(eBTRMgrSOGstStUnderflow, &mockUserData);

    TEST_ASSERT_EQUAL(1, mockUserData.lstBtrMgrSoStatus.ui32UnderFlowCnt);
}
/*
void test_btrMgr_SO_GstStatusCb_Warning_SetsStateToWarning(void) {
    stBTRMgrSOHdl mockUserData = {0};
    // Assuming a way to set fpcBSoStatus to a non-null value, if applicable

    btrMgr_SO_GstStatusCb(eBTRMgrSOGstStWarning, &mockUserData);

    TEST_ASSERT_EQUAL(eBTRMgrStateWarning, mockUserData.lstBtrMgrSoStatus.eBtrMgrState);
}*/
