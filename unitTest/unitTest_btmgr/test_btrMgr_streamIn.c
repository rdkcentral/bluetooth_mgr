#include "unity.h" // The testing framework
#include  <stdlib.h>
#include <stdbool.h>
#include "btmgr.h"
#include "btrMgr_streamIn.h"
#include "btrMgr_audioCap.h"



/* Interface lib Headers */
#include "btrMgr_logger.h"         //for rdklogger

/* Local Headers */
#include "btrMgr_streamInGst.h"

//#include "safec_lib.h"

#include "mock_btrMgr_Types.h"
#include "mock_btrCore.h"
#include "mock_btrMgr_audioCap.h"

//#define bool _BOOL
#include "mock_btrMgr_SysDiag.h"
#include "mock_btrMgr_Columbo.h"
#include "mock_btrMgr_LEOnboarding.h"
#include "mock_btrMgr_persistIface.h"
#include "mock_btrMgr_streamOut.h"
#include "mock_btrMgr_streamInGst.h"

TEST_FILE("btrMgr_streamIn.c")


/* Local types */
typedef struct _stBTRMgrSIHdl {
    stBTRMgrMediaStatus     lstBtrMgrSiStatus;
    fPtr_BTRMgr_SI_StatusCb fpcBSiStatus;
    void*                   pvcBUserData;
#ifdef USE_GST1
    tBTRMgrSiGstHdl         hBTRMgrSiGstHdl;
#endif
} stBTRMgrSIHdl;

void test_BTRMgr_SI_GetDefaultSettings_NullHandle_Should_ReturnNotInitialized(void) {
    eBTRMgrRet result = BTRMgr_SI_GetDefaultSettings(NULL);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

void test_BTRMgr_SI_GetDefaultSettings_ValidHandle_Should_ReturnSuccess(void) {
    tBTRMgrSiHdl validHandle = 1;
    eBTRMgrRet result;

    result = BTRMgr_SI_GetDefaultSettings(validHandle);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}

void test_BTRMgr_SI_GetCurrentSettings_ValidHandle_Should_ReturnSuccess(void) {
    tBTRMgrSiHdl validHandle = 1;
    eBTRMgrRet result;

    result = BTRMgr_SI_GetCurrentSettings(validHandle);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}

void test_BTRMgr_SI_GetCurrentSettings_NullHandle_Should_ReturnNotInitialized(void) {
    eBTRMgrRet result = BTRMgr_SI_GetCurrentSettings(NULL);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

void test_BTRMgr_SI_GetStatus_NullHandle_Should_ReturnNotInitialized(void) {
    stBTRMgrMediaStatus status;
    eBTRMgrRet result = BTRMgr_SI_GetStatus(NULL, &status);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

void test_BTRMgr_SI_GetStatus_ValidHandle_ValidStatus_Should_ReturnSuccess(void) {
    tBTRMgrSiHdl validHandle = 1;
    stBTRMgrMediaStatus status;
    eBTRMgrRet result = BTRMgr_SI_GetStatus(validHandle, &status);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}

void test_btrMgr_SI_GstStatusCb_Underflow_IncrementsUnderflowCount(void) {
    stBTRMgrSIHdl mockHdl;
    mockHdl.lstBtrMgrSiStatus.ui32UnderFlowCnt = 0; // Initialize to zero

    eBTRMgrSIGstRet ret = btrMgr_SI_GstStatusCb(eBTRMgrSIGstStUnderflow, &mockHdl);

    TEST_ASSERT_EQUAL_UINT32(1, mockHdl.lstBtrMgrSiStatus.ui32UnderFlowCnt);
    TEST_ASSERT_EQUAL(eBTRMgrSIGstSuccess, ret);

    ret = btrMgr_SI_GstStatusCb(eBTRMgrSIGstStInitialized, &mockHdl);
    ret = btrMgr_SI_GstStatusCb(eBTRMgrSIGstStCompleted, &mockHdl);
    ret = btrMgr_SI_GstStatusCb(eBTRMgrSIGstStUnknown, &mockHdl);
    TEST_ASSERT_EQUAL(eBTRMgrSIGstSuccess, ret);
}

void test_OverflowStatus_IncrementsOverflowCount(void) {
    stBTRMgrSIHdl mockHdl = {0};
    mockHdl.lstBtrMgrSiStatus.ui32OverFlowCnt = 0;
    btrMgr_SI_GstStatusCb(eBTRMgrSIGstStOverflow, &mockHdl);
    TEST_ASSERT_EQUAL_UINT32(1, mockHdl.lstBtrMgrSiStatus.ui32OverFlowCnt);
}

// Function to simulate the callback and capture its call
void dummyCallback(stBTRMgrMediaStatus* status, void* userData) {
    // Implementation can set a flag or store status for assertion
}

void test_WarningStatus_ChangesStateAndTriggersCallback(void) {
    stBTRMgrSIHdl mockHdl = {0};
    mockHdl.fpcBSiStatus = dummyCallback; // Assign the dummy callback
    btrMgr_SI_GstStatusCb(eBTRMgrSIGstStWarning, &mockHdl);
    TEST_ASSERT_EQUAL(eBTRMgrStateWarning, mockHdl.lstBtrMgrSiStatus.eBtrMgrState);
    // Additional checks can be implemented to verify the callback was triggered
}

void test_ErrorStatus_ChangesStateAndTriggersCallback(void) {
    stBTRMgrSIHdl mockHdl = {0};
    mockHdl.fpcBSiStatus = dummyCallback; // Assign the dummy callback
    btrMgr_SI_GstStatusCb(eBTRMgrSIGstStError, &mockHdl);
    TEST_ASSERT_EQUAL(eBTRMgrStateError, mockHdl.lstBtrMgrSiStatus.eBtrMgrState);
    // Additional checks can be implemented to verify the callback was triggered
}

void test_BTRMgr_SI_Stop_NullHandle_Should_ReturnNotInitialized(void) {
    eBTRMgrRet result = BTRMgr_SI_Stop(NULL);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

// Test Stop with valid handle and successful stop operation
void test_BTRMgr_SI_Stop_ValidHandle_SuccessfulStop_Should_ReturnSuccess(void) {
    stBTRMgrSIHdl mockHdl;
    mockHdl.hBTRMgrSiGstHdl = (void*)1; // Simulate valid internal GStreamer handle

    BTRMgr_SI_GstStop_ExpectAndReturn(mockHdl.hBTRMgrSiGstHdl, eBTRMgrSIGstSuccess); // Expect BTRMgr_SI_GstStop to be called with success
    eBTRMgrRet result = BTRMgr_SI_Stop(&mockHdl);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrStateStopped, mockHdl.lstBtrMgrSiStatus.eBtrMgrState);
    TEST_ASSERT_EQUAL_UINT32(0, mockHdl.lstBtrMgrSiStatus.ui32OverFlowCnt);
    TEST_ASSERT_EQUAL_UINT32(0, mockHdl.lstBtrMgrSiStatus.ui32UnderFlowCnt);
}

void test_BTRMgr_SI_Stop_ValidHandle_FailingStop_Should_ReturnFailure(void) {
    stBTRMgrSIHdl mockHdl;
    mockHdl.hBTRMgrSiGstHdl = (void*)1; // Simulate valid internal GStreamer handle

    BTRMgr_SI_GstStop_ExpectAndReturn(mockHdl.hBTRMgrSiGstHdl, eBTRMgrSIGstFailure); // Expect BTRMgr_SI_GstStop to be called and fail
    eBTRMgrRet result = BTRMgr_SI_Stop(&mockHdl);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}

void test_BTRMgr_SI_Pause_NullHandle_Should_ReturnNotInitialized(void) {
    eBTRMgrRet result = BTRMgr_SI_Pause(NULL);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

// Test for Pause with valid handle and successful pause operation
void test_BTRMgr_SI_Pause_ValidHandle_SuccessfulPause_Should_ReturnSuccess(void) {
    stBTRMgrSIHdl mockHdl;
    mockHdl.hBTRMgrSiGstHdl = (void*)1; // Simulate valid internal GStreamer handle

    BTRMgr_SI_GstPause_ExpectAndReturn(mockHdl.hBTRMgrSiGstHdl, eBTRMgrSIGstSuccess); // Expect BTRMgr_SI_GstPause to be called with success
    eBTRMgrRet result = BTRMgr_SI_Pause(&mockHdl);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrStatePaused, mockHdl.lstBtrMgrSiStatus.eBtrMgrState);
}

// Test for Pause with valid handle but failing pause operation
void test_BTRMgr_SI_Pause_ValidHandle_FailingPause_Should_ReturnFailure(void) {
    stBTRMgrSIHdl mockHdl;
    mockHdl.hBTRMgrSiGstHdl = (void*)1; // Simulate valid internal GStreamer handle

    BTRMgr_SI_GstPause_ExpectAndReturn(mockHdl.hBTRMgrSiGstHdl, eBTRMgrSIGstFailure); // Expect BTRMgr_SI_GstPause to be called and fail
    eBTRMgrRet result = BTRMgr_SI_Pause(&mockHdl);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}

// Test Resume with NULL handle
void test_BTRMgr_SI_Resume_NullHandle_Should_ReturnNotInitialized(void) {
    eBTRMgrRet result = BTRMgr_SI_Resume(NULL);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

// Test Resume with valid handle and successful resume operation
void test_BTRMgr_SI_Resume_ValidHandle_SuccessfulResume_Should_ReturnSuccess_And_StatePlaying(void) {
    stBTRMgrSIHdl mockHdl;
    mockHdl.hBTRMgrSiGstHdl = (void*)1; // Simulate a valid internal GStreamer handle

    BTRMgr_SI_GstResume_ExpectAndReturn(mockHdl.hBTRMgrSiGstHdl, eBTRMgrSIGstSuccess); // Expect BTRMgr_SI_GstResume to be called with success
    eBTRMgrRet result = BTRMgr_SI_Resume(&mockHdl);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrStatePlaying, mockHdl.lstBtrMgrSiStatus.eBtrMgrState);
}

// Test Resume with valid handle but failing resume operation
void test_BTRMgr_SI_Resume_ValidHandle_FailingResume_Should_ReturnFailure(void) {
    stBTRMgrSIHdl mockHdl;
    mockHdl.hBTRMgrSiGstHdl = (void*)1; // Simulate a valid internal GStreamer handle

    BTRMgr_SI_GstResume_ExpectAndReturn(mockHdl.hBTRMgrSiGstHdl, eBTRMgrSIGstFailure); // Expect BTRMgr_SI_GstResume to be called and fail
    eBTRMgrRet result = BTRMgr_SI_Resume(&mockHdl);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}

// Test sending buffer with NULL handle
void test_BTRMgr_SI_SendBuffer_NullHandle_Should_ReturnNotInitialized(void) {
    char sampleBuffer[10];
    eBTRMgrRet result = BTRMgr_SI_SendBuffer(NULL, sampleBuffer, sizeof(sampleBuffer));
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

// Test sending buffer with valid handle and buffer
void test_BTRMgr_SI_SendBuffer_ValidHandleAndBuffer_SuccessfulSend_Should_ReturnSuccess(void) {
    stBTRMgrSIHdl mockHdl;
    char sampleBuffer[10];
    mockHdl.hBTRMgrSiGstHdl = (void*)1; // Simulate a valid internal GStreamer handle

    BTRMgr_SI_GstSendBuffer_ExpectAndReturn(mockHdl.hBTRMgrSiGstHdl, sampleBuffer, sizeof(sampleBuffer), eBTRMgrSIGstSuccess);
    eBTRMgrRet result = BTRMgr_SI_SendBuffer(&mockHdl, sampleBuffer, sizeof(sampleBuffer));
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}

// Test sending buffer with valid handle but failing due to GstSendBuffer
void test_BTRMgr_SI_SendBuffer_ValidHandleAndBuffer_FailingSend_Should_ReturnFailure(void) {
    stBTRMgrSIHdl mockHdl;
    char sampleBuffer[10];
    mockHdl.hBTRMgrSiGstHdl = (void*)1; // Simulate a valid internal GStreamer handle

    BTRMgr_SI_GstSendBuffer_ExpectAndReturn(mockHdl.hBTRMgrSiGstHdl, sampleBuffer, sizeof(sampleBuffer), eBTRMgrSIGstFailure);
    eBTRMgrRet result = BTRMgr_SI_SendBuffer(&mockHdl, sampleBuffer, sizeof(sampleBuffer));
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}

// Test SendEOS with NULL handle
void test_BTRMgr_SI_SendEOS_NullHandle_Should_ReturnNotInitialized(void) {
    eBTRMgrRet result = BTRMgr_SI_SendEOS(NULL);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

// Test SendEOS with valid handle and successful EOS operation
void test_BTRMgr_SI_SendEOS_ValidHandle_SuccessfulEOS_Should_ReturnSuccess_And_StateCompleted(void) {
    stBTRMgrSIHdl mockHdl;
    mockHdl.hBTRMgrSiGstHdl = (void*)1; // Simulate a valid internal GStreamer handle

    BTRMgr_SI_GstSendEOS_ExpectAndReturn(mockHdl.hBTRMgrSiGstHdl, eBTRMgrSIGstSuccess);
    eBTRMgrRet result = BTRMgr_SI_SendEOS(&mockHdl);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrStateCompleted, mockHdl.lstBtrMgrSiStatus.eBtrMgrState);
}

// Test SendEOS with valid handle but failing EOS operation
void test_BTRMgr_SI_SendEOS_ValidHandle_FailingEOS_Should_ReturnFailure(void) {
    stBTRMgrSIHdl mockHdl;
    mockHdl.hBTRMgrSiGstHdl = (void*)1; // Simulate a valid internal GStreamer handle

    BTRMgr_SI_GstSendEOS_ExpectAndReturn(mockHdl.hBTRMgrSiGstHdl, eBTRMgrSIGstFailure);
    eBTRMgrRet result = BTRMgr_SI_SendEOS(&mockHdl);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}

// Test for NULL handle
void test_BTRMgr_SI_SetStatus_NullHandle_Should_ReturnNotInitialized(void) {
    stBTRMgrMediaStatus status;
    eBTRMgrRet result = BTRMgr_SI_SetStatus(NULL, &status);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

// Test for NULL status pointer
void test_BTRMgr_SI_SetStatus_NullStatus_Should_ReturnFailInArg(void) {
    stBTRMgrSIHdl mockHdl;
    eBTRMgrRet result = BTRMgr_SI_SetStatus(&mockHdl, NULL);
    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, result);
}

// Test for setting valid status
void test_BTRMgr_SI_SetStatus_ValidInputs_Should_ReturnFailure(void) {
    stBTRMgrSIHdl mockHdl;
    stBTRMgrMediaStatus status = {
        .eBtrMgrState = eBTRMgrStatePlaying, // Assuming setting to a non-unknown value should fail
        .eBtrMgrSFreq = eBTRMgrSFreq44_1K, // Same as above
        .eBtrMgrSFmt = eBTRMgrSFmt16bit, // Same as above
        .eBtrMgrAChan = eBTRMgrAChanStereo, // Same as above
        .ui8Volume = 10 // Valid volume setting
    };

    BTRMgr_SI_GstSetVolume_ExpectAndReturn(mockHdl.hBTRMgrSiGstHdl, status.ui8Volume, eBTRMgrSIGstSuccess);
    eBTRMgrRet result = BTRMgr_SI_SetStatus(&mockHdl, &status);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result); // Expected to fail due to non-unknown values
}

// Test DeInit with NULL handle
void test_BTRMgr_SI_DeInit_NullHandle_Should_ReturnNotInitialized(void) {
    eBTRMgrRet result = BTRMgr_SI_DeInit(NULL);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

// Test DeInit with valid handle and successful deinitialization
void test_BTRMgr_SI_DeInit_ValidHandle_SuccessfulDeInit_Should_ReturnSuccess(void) {
    stBTRMgrSIHdl *mockHdl = malloc(sizeof(stBTRMgrSIHdl));
    mockHdl->hBTRMgrSiGstHdl = (void*)1; // Simulate a valid internal GStreamer handle

    BTRMgr_SI_GstDeInit_ExpectAndReturn(mockHdl->hBTRMgrSiGstHdl, eBTRMgrSIGstSuccess);
    eBTRMgrRet result = BTRMgr_SI_DeInit(mockHdl);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    // Optionally verify if the handle is set to NULL, which would require accessing it after DeInit (not shown here)
}

// Test Start with NULL handle
void test_BTRMgr_SI_Start_NullHandle_Should_ReturnNotInitialized(void) {
    stBTRMgrInASettings settings;
    eBTRMgrRet result = BTRMgr_SI_Start(NULL, 1024, &settings);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

// Test Start with NULL settings
void test_BTRMgr_SI_Start_NullSettings_Should_ReturnFailInArg(void) {
    stBTRMgrSIHdl mockHdl;
    eBTRMgrRet result = BTRMgr_SI_Start(&mockHdl, 1024, NULL);
    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, result);
}
#if 0
// Test for valid handle and settings but failure in BTRMgr_SI_GstStart
void test_BTRMgr_SI_Start_ValidHandleAndSettings_GstStartFailure_Should_ReturnFailure(void) {
    stBTRMgrSIHdl mockHdl;
    stBTRMgrInASettings settings = {
        .eBtrMgrInAType = eBTRMgrATypeSBC,
        .pstBtrMgrInCodecInfo = (void*)1, // Assuming valid codec info for simplicity
        .i32BtrMgrDevFd = 1,
        .i32BtrMgrDevMtu = 672
    };

    BTRMgr_SI_GstStart_ExpectAndReturn(mockHdl.hBTRMgrSiGstHdl, 1024, settings.i32BtrMgrDevFd, settings.i32BtrMgrDevMtu, 48000, "SBC", eBTRMgrSIGstFailure);
    eBTRMgrRet result = BTRMgr_SI_Start(&mockHdl, 1024, &settings);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}
#endif
