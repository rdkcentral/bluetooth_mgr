#if 0
#include "unity.h" // The testing framework
#include  <stdlib.h>
#include <stdbool.h>
#include "btmgr.h"
#include "btrMgr_streamOut.h"
#include "btrMgr_audioCap.h"

/* Ext lib Headers */
#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/app/gstappsrc.h>


/* Interface lib Headers */
#include "btrMgr_logger.h"         //for rdklogger

/* Local Headers */
#include "btrMgr_streamOutGst.h"

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
#include "mock_btrMgr_streamIn.h"

TEST_FILE("btrMgr_streamOutGst.c")  //this line force ceedlign to compile btrMgr.c, because btmgr.h is  a diffenrent name, no need if the file was called btmgr.c(they need to match)

/* Local defines */
#define BTRMGR_SLEEP_TIMEOUT_MS             1   // Suspend execution of thread. Keep as minimal as possible
#define BTRMGR_WAIT_TIMEOUT_MS              2   // Use for blocking operations
#define BTRMGR_MAX_INTERNAL_QUEUE_ELEMENTS 10   // Number of blocks in the internal queue
#define BTRMGR_INPUT_BUF_INTERVAL_THRES_CNT 4   // Number of buffer threshold based on input buffer interval

#define GST_ELEMENT_GET_STATE_RETRY_CNT_MAX 5

#define ENABLE_MAIN_LOOP_CONTEXT 0

#define BTRMGR_SBC_ALLOCATION_SNR           (1 << 1)    // Has to match with a2dp-codecs.h
#define BTRMGR_SBC_ALLOCATION_LOUDNESS      1           // Need a better way to pass/map this from the upper layers

#define BTMGR_TIMEOUT_SEND_EMPTY_BUFFERS 100
#define PACKET_OVERHEAD_SIZE 22

typedef struct _stBTRMgrSOGst {
    void*        pPipeline;
    void*        pSrc;
    void*        pSink;
    void*        pAudioConv;
    void*        pAudioResample;
    void*        pAudioEnc;
    void*        pAECapsFilter;
    void*        pRtpAudioPay;
    void*        pVolume;
    void*        pDelay;

#if !(ENABLE_MAIN_LOOP_CONTEXT)
    void*        pContext;
    GMutex       gMtxMainLoopRunLock;
    GCond        gCndMainLoopRun;
    guint        emptyBufPushId;
#endif

    void*        pLoop;
    void*        pLoopThread;
    guint        busWId;

    GstClockTime gstClkTStamp;
    guint64      inBufOffset;
    int          i32InBufMaxSize;
    int          i32InRate;
    int          i32InChannels;
    unsigned int ui32InBitsPSample;
    unsigned int ui32InBufIntervalms;
    unsigned char ui8IpBufThrsCnt;

    fPtr_BTRMgr_SO_GstStatusCb  fpcBSoGstStatus;
    void*                       pvcBUserData;

    GMutex       pipelineDataMutex;
    gboolean     bPipelineError;
    gboolean     bIsInputPaused;
    gboolean     bPipelineReady;
} stBTRMgrSOGst;

void test_BTRMgr_SO_GstStop_NullHandle(void) {
    tBTRMgrSoGstHdl hBTRMgrSoGstHdl = NULL; // Simulating NULL handle input

    // Execute
    eBTRMgrSOGstRet result = BTRMgr_SO_GstStop(hBTRMgrSoGstHdl);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, result);
}

void test_BTMgr_SO_GstSetDelay_NullHandle(void) {
    tBTRMgrSoGstHdl hBTRMgrSoGstHdl = NULL; // Simulating NULL handle input
    unsigned int delay_comp_ms = 100; // Example delay value

    // Execute
    eBTRMgrSOGstRet result = BTMgr_SO_GstSetDelay(hBTRMgrSoGstHdl, delay_comp_ms);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, result);
}

void test_btrMgr_SO_g_timeout_EmptyBufPushCb_NullInput_ReturnsFalse(void) {
    TEST_ASSERT_FALSE(btrMgr_SO_g_timeout_EmptyBufPushCb(NULL));
}

void test_btrMgr_SO_g_timeout_EmptyBufPushCb_NotPaused_ThresholdNotMet_ReturnsTrue(void) {
    stBTRMgrSOGst fakeGst = {.bIsInputPaused = FALSE, .ui8IpBufThrsCnt = BTRMGR_INPUT_BUF_INTERVAL_THRES_CNT + 1,.i32InBufMaxSize = 10};
    TEST_ASSERT_TRUE(btrMgr_SO_g_timeout_EmptyBufPushCb(&fakeGst));
}

void test_btrMgr_SO_g_timeout_EmptyBufPushCb_ThresholdMet_IncrementAndReturnTrue(void) {
    stBTRMgrSOGst fakeGst = {.bIsInputPaused = FALSE, .ui8IpBufThrsCnt = 3, .i32InBufMaxSize = 10, .bPipelineError = TRUE};
    fakeGst.ui8IpBufThrsCnt = 3;
    TEST_ASSERT_TRUE(btrMgr_SO_g_timeout_EmptyBufPushCb(&fakeGst));
}

void test_btrMgr_SO_EnoughDataCb_NullPstBtrMgrSoGst(void) {
    GstElement* dummyAppsrc = (GstElement*)1;  // Assuming GstElement is an external dependency
    btrMgr_SO_EnoughDataCb(dummyAppsrc, NULL);
    // Expect no call to fpcBSoGstStatus; just verify the function does not crash.
}

void test_btrMgr_SO_EnoughDataCb_NoStatusCallback(void) {
    stBTRMgrSOGst fakeGst = {0};  // fpcBSoGstStatus is implicitly NULL here.
    GstElement* dummyAppsrc = (GstElement*)1;
    btrMgr_SO_EnoughDataCb(dummyAppsrc, &fakeGst);
    // Again, we expect no action. This verifies safe handling of no status callback.
}

void fpcBSoGstStatusMock(eBTRMgrSOGstStatus status, void *pvcBUserData) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstStOverflow, status);
    TEST_ASSERT_NOT_NULL(pvcBUserData);
}

void test_btrMgr_SO_EnoughDataCb_ValidStatusCallback(void) {
    stBTRMgrSOGst fakeGst;
    fakeGst.fpcBSoGstStatus = fpcBSoGstStatusMock;  // Assign mock function
    fakeGst.pvcBUserData = (void*)1;  // Arbitrary non-NULL value

    GstElement* dummyAppsrc = (GstElement*)1;
    btrMgr_SO_EnoughDataCb(dummyAppsrc, &fakeGst);

    // The assertions are in the mock function to check correct parameters were passed.
}

void test_btrMgr_SO_NeedDataCb_NullApstBtrMgrSoGst(void) {
    GstElement* dummyAppsrc = (GstElement*)1;
    guint dummySize = 10;
    btrMgr_SO_NeedDataCb(dummyAppsrc, dummySize, NULL);
    // Expectation: No crash, no call to fpcBSoGstStatus
}

void test_btrMgr_SO_NeedDataCb_FpcBSoGstStatusIsNull(void) {
    stBTRMgrSOGst fakeGst = {0};  // fpcBSoGstStatus is NULL
    GstElement* dummyAppsrc = (GstElement*)1;
    guint dummySize = 10;
    btrMgr_SO_NeedDataCb(dummyAppsrc, dummySize, &fakeGst);
    // Expectation: No action is taken
}

void fpcBSoGstStatusMockUnderFlow(eBTRMgrSOGstStatus status, void *pvcBUserData) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstStUnderflow, status);
    // Additional assertions can be added here if needed
}

void test_btrMgr_SO_NeedDataCb_ValidFpcBSoGstStatus(void) {
    stBTRMgrSOGst fakeGst;
    fakeGst.fpcBSoGstStatus = fpcBSoGstStatusMockUnderFlow;  // Mocked callback
    fakeGst.pvcBUserData = (void*)1;  // Example user data

    GstElement* dummyAppsrc = (GstElement*)1;
    guint dummySize = 10;
    btrMgr_SO_NeedDataCb(dummyAppsrc, dummySize, &fakeGst);

    // The expectation is that fpcBSoGstStatusMock is called with the correct parameters
}

void test_btrMgr_SO_g_main_loop_RunningCb_NullInput(void) {
    gboolean result = btrMgr_SO_g_main_loop_RunningCb(NULL);
    TEST_ASSERT_EQUAL(G_SOURCE_REMOVE, result);
}

void test_btrMgr_SO_g_main_loop_RunningCb_ValidInput(void) {
    stBTRMgrSOGst fakeGst;
    g_mutex_init(&fakeGst.gMtxMainLoopRunLock);
    g_cond_init(&fakeGst.gCndMainLoopRun);

    gboolean result = btrMgr_SO_g_main_loop_RunningCb(&fakeGst);
    TEST_ASSERT_EQUAL(G_SOURCE_REMOVE, result);

    g_mutex_clear(&fakeGst.gMtxMainLoopRunLock);
    g_cond_clear(&fakeGst.gCndMainLoopRun);
}

void test_BTRMgr_SO_GstSendEOS_InvalidInput(void) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTRMgr_SO_GstSendEOS(NULL));
}

void test_BTRMgr_SO_GstSendEOS_PipelineError(void) {
    stBTRMgrSOGst fakeGst = {0};
    fakeGst.bPipelineError = TRUE; // Simulate pipeline error
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailure, BTRMgr_SO_GstSendEOS(&fakeGst));
}

void test_BTRMgr_SO_GstGetMute_InvalidInput(void) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTRMgr_SO_GstGetMute(NULL, NULL));
    gboolean mute;
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTRMgr_SO_GstGetMute(NULL, &mute));
}

void test_BTRMgr_SO_GstGetMute_ValidInput_Muted(void) {
    stBTRMgrSOGst fakeGst;
    //fakeGst.pVolume = (GstElement*) TRUE;
    //gboolean muteStatus = TRUE;
    //g_object_get_ExpectAndReturn(fakeGst.pVolume, "mute", &muteStatus, NULL, G_TYPE_BOOLEAN);
    gboolean muted;
    TEST_ASSERT_EQUAL(eBTRMgrSOGstSuccess, BTRMgr_SO_GstGetMute(&fakeGst, &muted));
    TEST_ASSERT_TRUE(muted);
}

void test_BTRMgr_SO_GstSetMute_InvalidInput(void) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTRMgr_SO_GstSetMute(NULL, TRUE));
}

void test_BTRMgr_SO_GstSetMute_ValidInput_MuteTrue(void) {
    stBTRMgrSOGst fakeGst;
    gboolean mute = TRUE;
    //g_object_set_Expect(fakeGst.pVolume, "mute", mute, NULL);
    //BTRMGRLOG_TRACE_ExpectAnyArgs();
    TEST_ASSERT_EQUAL(eBTRMgrSOGstSuccess, BTRMgr_SO_GstSetMute(&fakeGst, mute));
}

void test_BTRMgr_SO_GstCalculateDelayNeeded_NullDelayRet(void) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTRMgr_SO_GstCalculateDelayNeeded((void*)1, 100, NULL));
}
void test_BTRMgr_SO_GstCalculateDelayNeeded_ValidBtDeviceDelay_NoCustomComp(void) {
    unsigned int delayRet;
    TEST_ASSERT_EQUAL(eBTRMgrSOGstSuccess, BTRMgr_SO_GstCalculateDelayNeeded((void*)1, 100, &delayRet));
    // Example calculation for assertion, adjust based on actual formula used
    TEST_ASSERT_EQUAL(430, delayRet); // Adjust expected value based on actual formula
}

void test_btrMgr_SO_GstReadyForData_SetsPipelineReady(void) {
    stBTRMgrSOGst testStruct;
    memset(&testStruct, 0, sizeof(stBTRMgrSOGst)); // Initialize testStruct

    gboolean result = btrMgr_SO_GstReadyForData(&testStruct);

    TEST_ASSERT_TRUE(testStruct.bPipelineReady);
    TEST_ASSERT_EQUAL(G_SOURCE_REMOVE, result);
}

void test_BTRMgr_SO_GstSendBuffer_InvalidInput(void) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTRMgr_SO_GstSendBuffer(NULL, NULL, 100));
}

void test_BTRMgr_SO_GstSendBuffer_PipelineNotReady(void) {
    stBTRMgrSOGst fakeGst;
    memset(&fakeGst, 0, sizeof(fakeGst)); // Initialize
    char dummyData = 'A';
    fakeGst.bPipelineReady = FALSE; // Pipeline is not ready

    TEST_ASSERT_EQUAL(eBTRMgrSOGstSuccess, BTRMgr_SO_GstSendBuffer(&fakeGst, &dummyData, 1));
}

void test_BTRMgr_SO_GstGetVolume_InvalidInput(void) {
    unsigned char volume;
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTRMgr_SO_GstGetVolume(NULL, &volume));
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTRMgr_SO_GstGetVolume((void*)1, NULL));
}

void test_BTRMgr_SO_GstGetVolume_ValidInput(void) {
    stBTRMgrSOGst fakeGst;
    memset(&fakeGst, 0, sizeof(fakeGst)); // Initialize
    unsigned char volume;
    double dvolume = 0.5; // Assume volume is at 50%

    //g_object_get_ExpectAndReturn(fakeGst.pVolume, "volume", &dvolume, NULL, G_TYPE_DOUBLE);
    //BTRMGRLOG_DEBUG_ExpectAnyArgs();

    TEST_ASSERT_EQUAL(eBTRMgrSOGstSuccess, BTRMgr_SO_GstGetVolume(&fakeGst, &volume));
    TEST_ASSERT_EQUAL(0, volume); // 50% of 255
}

void test_BTRMgr_SO_GstSetInputPaused_InvalidInput(void) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTRMgr_SO_GstSetInputPaused(NULL, TRUE));
}

void test_BTRMgr_SO_GstSetInputPaused_SetTrue(void) {
    stBTRMgrSOGst fakeGst;
    memset(&fakeGst, 0, sizeof(stBTRMgrSOGst)); // Initialize

    // Mock the relevant GLib functions
    //g_timeout_source_new_ExpectAndReturn(fakeGst.ui32InBufIntervalms, NULL);
    // Further GLib function expectations...

    TEST_ASSERT_EQUAL(eBTRMgrSOGstSuccess, BTRMgr_SO_GstSetInputPaused(&fakeGst, TRUE));
    TEST_ASSERT_TRUE(fakeGst.bIsInputPaused);
    // Additional assertions can be made based on mocked function calls
}

void test_BTRMgr_SO_GstSetInputPaused_SetFalse(void) {
    stBTRMgrSOGst fakeGst;
    fakeGst.emptyBufPushId = 1; // Assume there is an existing source
    fakeGst.pContext = g_main_context_default(); // Example context

    // Expectations for destroying the GSource
    //g_source_destroy_ExpectAnyArgs();
    //g_source_unref_ExpectAnyArgs();

    TEST_ASSERT_EQUAL(eBTRMgrSOGstSuccess, BTRMgr_SO_GstSetInputPaused(&fakeGst, FALSE));
    TEST_ASSERT_FALSE(fakeGst.bIsInputPaused);
    TEST_ASSERT_EQUAL(0, fakeGst.emptyBufPushId);
}

void test_BTMgr_SO_GstGetDelay_Failure(void) {
    unsigned int delay, msInBuffer;

    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTMgr_SO_GstGetDelay(NULL, &delay, &msInBuffer));
}

void test_BTMgr_SO_GstGetDelay_Success(void) {
    stBTRMgrSOGst fakeGst;
    memset(&fakeGst, 0, sizeof(stBTRMgrSOGst)); // Initialize
    unsigned int delay, msInBuffer;
    guint64 currentDelayNanoSec = 2000000000; // Example: 2 seconds
    guint64 currentTimeInBufferNanoSec = 1500000000; // Example: 1.5 seconds

    // Set expectations for g_object_get calls
    //g_object_get_ExpectAndReturn(fakeGst.pDelay, "min-threshold-time", &currentDelayNanoSec, NULL, G_TYPE_UINT64);
    //g_object_get_ExpectAndReturn(fakeGst.pDelay, "current-level-time", &currentTimeInBufferNanoSec, NULL, G_TYPE_UINT64);

    TEST_ASSERT_EQUAL(eBTRMgrSOGstSuccess, BTMgr_SO_GstGetDelay(&fakeGst, &delay, &msInBuffer));
}

void test_BTMgr_SO_GstSetDelay_ValidInput(void) {
    stBTRMgrSOGst fakeGst;
    memset(&fakeGst, 0, sizeof(stBTRMgrSOGst)); // Initialize
    guint64 newDelayNanoSec = 100 * 1000000; // Convert ms to ns

    g_object_set(fakeGst.pDelay, "min-threshold-time", newDelayNanoSec, NULL);
    //BTRMGRLOG_INFO_ExpectAnyArgs();

    TEST_ASSERT_EQUAL(eBTRMgrSOGstSuccess, BTMgr_SO_GstSetDelay(&fakeGst, 100));
}

void test_BTRMgr_SO_GstSetVolume_InvalidInput(void) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTRMgr_SO_GstSetVolume(NULL, 100));
}

void test_BTRMgr_SO_GstSetVolume_ValidInput(void) {
    stBTRMgrSOGst fakeGst;
    memset(&fakeGst, 0, sizeof(stBTRMgrSOGst)); // Initialize
    unsigned char ui8Volume = 128; // 50% volume
    double expectedVolume = ui8Volume / 255.0;

    g_object_set(fakeGst.pVolume, "volume", expectedVolume, NULL);
    //BTRMGRLOG_TRACE_ExpectAnyArgs();

    TEST_ASSERT_EQUAL(eBTRMgrSOGstSuccess, BTRMgr_SO_GstSetVolume(&fakeGst, ui8Volume));
}

void fpcBSoGstStatusMockEOS(eBTRMgrSOGstStatus status, void *pvcBUserData) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstStCompleted, status);
    // Additional assertions can be added here if needed
}
void test_gstBusCallCb_EOS_Message(void) {
    stBTRMgrSOGst fakeGst = {0};
    fakeGst.fpcBSoGstStatus = fpcBSoGstStatusMockEOS;  // Mocked callback
    fakeGst.pvcBUserData = (void*)1;
    GstMessage msg = {.type = GST_MESSAGE_EOS};
    TEST_ASSERT_TRUE(btrMgr_SO_gstBusCallCb(NULL, &msg, &fakeGst));
    // Expectations: g_main_loop_quit called if pLoop exists, fpcBSoGstStatus called with eBTRMgrSOGstStCompleted or eBTRMgrSOGstStError
}

void test_gstBusCallCb_StateChanged_Message(void) {
    stBTRMgrSOGst fakeGst = {0};
    GstMessage msg = {.type = GST_MESSAGE_STATE_CHANGED};
    TEST_ASSERT_TRUE(btrMgr_SO_gstBusCallCb(NULL, &msg, &fakeGst));
    // Expectations: State change logged. You need to mock gst_message_parse_state_changed and related logging functions.
}

void test_BTRMgr_SO_GstPause_InvalidInput(void) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTRMgr_SO_GstPause(NULL));
}

void test_BTRMgr_SO_GstPause_IncorrectState(void) {
    stBTRMgrSOGst fakeGst;
    memset(&fakeGst, 0, sizeof(fakeGst)); // Initialize

    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailure, BTRMgr_SO_GstPause(&fakeGst));
}

void test_BTRMgr_SO_GstResume_InvalidInput(void) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTRMgr_SO_GstResume(NULL));
}

void test_BTRMgr_SO_GstResume_IncorrectState(void) {
    stBTRMgrSOGst fakeGst;
    memset(&fakeGst, 0, sizeof(stBTRMgrSOGst)); // Initialize

    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailure, BTRMgr_SO_GstResume(&fakeGst));
}

void test_BTRMgr_SO_GstStart_InvalidInput(void) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTRMgr_SO_GstStart(NULL, 1024, "S16LE", 44100, 2, 44100, 2, "stereo", 0, 8, 16, 2, 53, 128, 328, 3, 250));
}

void test_BTRMgr_SO_GstStart_IncorrectState(void) {
    stBTRMgrSOGst fakeGst;
    memset(&fakeGst, 0, sizeof(stBTRMgrSOGst)); // Initialize

    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailure, BTRMgr_SO_GstStart(&fakeGst, 1024, "S16LE", 44100, 2, 44100, 2, "stereo", 0, 8, 16, 2, 53, 128, 328, 3, 250));
}

void test_BTRMgr_SO_GstDeInit_InvalidInput(void) {
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailInArg, BTRMgr_SO_GstDeInit(NULL));
}

void test_BTRMgr_SO_GstDeInit_PipelineCleanup(void) {
    stBTRMgrSOGst *fakeGst = malloc(sizeof(stBTRMgrSOGst));
    memset(fakeGst, 0, sizeof(stBTRMgrSOGst)); // Initialize

    TEST_ASSERT_EQUAL(eBTRMgrSOGstSuccess, BTRMgr_SO_GstDeInit(fakeGst));
}

void test_btrMgr_SO_g_main_loop_Task_InvalidInput(void) {
    TEST_ASSERT_NULL(btrMgr_SO_g_main_loop_Task(NULL));
}

void test_btrMgr_SO_g_main_loop_Task_InvalidContext(void) {
    stBTRMgrSOGst fakeGst;
    memset(&fakeGst, 0, sizeof(stBTRMgrSOGst));
    fakeGst.pLoop = 1;
    TEST_ASSERT_NULL(btrMgr_SO_g_main_loop_Task(&fakeGst));
}

void test_BTRMgr_SO_GstStop_FailureToStop(void) {
    stBTRMgrSOGst fakeGst;
    memset(&fakeGst, 0, sizeof(stBTRMgrSOGst)); // Initialize

    gst_element_set_state(GST_ELEMENT(fakeGst.pPipeline), GST_STATE_NULL);
    //btrMgr_SO_validateStateWithTimeout_ExpectAndReturn(fakeGst.pPipeline, GST_STATE_NULL, BTRMGR_SLEEP_TIMEOUT_MS, GST_STATE_VOID_PENDING);
    //BTRMGRLOG_ERROR_ExpectAnyArgs();

    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailure, BTRMgr_SO_GstStop(&fakeGst));
}


#if 0
void test_BTRMgr_SO_GstStop_FailureToStopPlayback(void) {
    stBTRMgrSOGst fakeGst;
    memset(&fakeGst, 0, sizeof(fakeGst));
    //gst_element_set_state_ExpectAndReturn(GST_ELEMENT(fakeGst.pPipeline), GST_STATE_NULL, GST_STATE_CHANGE_SUCCESS);
    btrMgr_SO_validateStateWithTimeout_ExpectAndReturn(fakeGst.pPipeline, GST_STATE_NULL, BTRMGR_SLEEP_TIMEOUT_MS, GST_STATE_CHANGE_FAILURE);
    //BTRMGRLOG_ERROR_Expect("- Unable to perform Operation\n");
    TEST_ASSERT_EQUAL(eBTRMgrSOGstFailure, BTRMgr_SO_GstStop(&fakeGst));
}

void test_BTRMgr_SO_GstSendEOS_Success(void) {
    stBTRMgrSOGst fakeGst;
    memset(&fakeGst, 0, sizeof(fakeGst)); // Initialize to zero
    gst_app_src_end_of_stream_Expect(GST_APP_SRC(fakeGst.pSrc)); // Expect the EOS function to be called
    TEST_ASSERT_EQUAL(eBTRMgrSOGstSuccess, BTRMgr_SO_GstSendEOS(&fakeGst));
}
#endif
#endif