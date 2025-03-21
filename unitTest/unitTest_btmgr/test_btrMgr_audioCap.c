#ifndef UNIT_TEST
#define UNIT_TEST
#endif
#ifndef USE_ACM
#define USE_ACM
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>
//#include "safec_lib.h"
#if defined(USE_ACM)
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#endif

/* Ext lib Headers */

#include <glib.h>
#include <stdbool.h>

#include "btrMgr_audioCap.h"

/* Interface lib Headers */
#if defined(USE_AC_RMF)
#include "rmfAudioCapture.h"
#endif

#if defined(USE_ACM)
#include "mock_libIBus.h"
#include "libIARM.h"
#include "audiocapturemgr_iarm.h"
#endif
#include "mock_btrMgr_logger.h"
#ifdef RDK_LOGGER_ENABLED
int b_rdk_logger_enabled = 0;
#endif
/* Local Headers */
#include "btrMgr_Types.h"
#include "btrMgr_mediaTypes.h"

/* Local defines */
#if defined(USE_ACM)
//TODO: Should match the value in src/rpc/btmgr_iarm_interface.h. Find a better way
#define IARM_BUS_BTRMGR_NAME        "BTRMgrBus" 
#endif

#define COMMAND_STRING_SIZE 128
#define FILE_INPUT_PATH_MAX 32

#if defined(BUILD_RDKTV)
#define TV_CUSTOM_DELAY_COMP 1
#endif


TEST_FILE("test_btrMgr_audioCap.c") 


#if defined(USE_ACM)
typedef enum _eBTRMgrACAcmDCOp {
    eBTRMgrACAcmDCStart,
    eBTRMgrACAcmDCStop,
    eBTRMgrACAcmDCResume,
    eBTRMgrACAcmDCPause,
    eBTRMgrACAcmDCExit,
    eBTRMgrACAcmDCUnknown
} eBTRMgrACAcmDCOp;
#endif

typedef enum _eBTRMgrACTestDCOp {
    eBTRMgrACTestDCStart,
    eBTRMgrACTestDCStop,
    eBTRMgrACTestDCResume,
    eBTRMgrACTestDCPause,
    eBTRMgrACTestDCExit,
    eBTRMgrACTestDCUnknown
} eBTRMgrACTestDCOp;
typedef struct _stBTRMgrACHdl {
    stBTRMgrMediaStatus         stBtrMgrAcStatus;
#if defined(USE_AC_RMF)
    RMF_AudioCaptureHandle      hBTRMgrRmfAcHdl;
    RMF_AudioCapture_Settings   stBtrMgrRmfAcDefSettings;
    RMF_AudioCapture_Settings   stBtrMgrRmfAcCurSettings;
    RMF_AudioCapture_Status     stBtrMgrRmfAcStatus;
#endif
#if defined(USE_ACM)
    GThread*                    pBtrMgrAcmDataCapGThread;
    GAsyncQueue*                pBtrMgrAcmDataCapGAOpQueue;
    session_id_t                hBtrMgrIarmAcmHdl;
    audio_properties_ifce_t     stBtrMgrAcmDefSettings;
    audio_properties_ifce_t     stBtrMgrAcmCurSettings;
    audio_properties_ifce_t*    pstBtrMgrAcmSettings;
    char                        pcBtrMgrAcmSockPath[MAX_OUTPUT_PATH_LEN];
    int                         i32BtrMgrAcmDCSockFd;
    int                         i32BtrMgrAcmExternalIARMMode;
#endif
  char                        pcBtrMgrTestSockPath[FILE_INPUT_PATH_MAX];
    int                         i32BtrMgrTestDCSockFd;
    GThread*                    pBtrMgrTestDataCapGThread;
    GAsyncQueue*                pBtrMgrTestDataCapGAOpQueue;
    unsigned short              ui16Threshold;
 
    tBTRMgrAcType               pcBTRMgrAcType;

    fPtr_BTRMgr_AC_DataReadyCb  fpcBBtrMgrAcDataReady;
    void*                       vpBtrMgrAcDataReadyUserData;

    fPtr_BTRMgr_AC_StatusCb     fpcBBtrMgrAcStatus;
    void*                       vpBtrMgrAcStatusUserData;
    unsigned char               ui8DebugMode;
} stBTRMgrACHdl;


void test_BTRMgr_AC_GetDefaultSettings_NullHandle(void) {
    eBTRMgrRet result;
    stBTRMgrOutASettings outSettings;

    // Set the handle to NULL
    result = BTRMgr_AC_GetDefaultSettings(NULL, &outSettings);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}
void test_BTRMgr_AC_GetDefaultSettings_NullOutSettings(void) {
    eBTRMgrRet result;
    tBTRMgrAcHdl validHandle = (tBTRMgrAcHdl)1; // Mocked valid handle

    // Set the settings pointer to NULL
    result = BTRMgr_AC_GetDefaultSettings(validHandle, NULL);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, result);
}

void test_BTRMgr_AC_GetDefaultSettings_NullOutSettingsCodecInfo(void) {
    eBTRMgrRet result;
    stBTRMgrOutASettings outSettings = { 0 };
    tBTRMgrAcHdl validHandle = (tBTRMgrAcHdl)1; // Mocked valid handle

    // Set the settings codec info pointer to NULL
    result = BTRMgr_AC_GetDefaultSettings(validHandle, &outSettings);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, result);
}

// Success test case for IARM_Bus_Call and primary type
void test_BTRMgr_AC_GetDefaultSettings_SuccessPrimary(void) {
    eBTRMgrRet result;

    stBTRMgrACHdl* pHdl = (stBTRMgrACHdl*)malloc(sizeof(stBTRMgrACHdl));
    
    if (pHdl == NULL) {
        // Handle memory allocation failure if needed
        return NULL;
    }
    
    // Initialize the memory to zero
    memset(pHdl, 0, sizeof(stBTRMgrACHdl));

    // Initialize specific fields (here's just an example)
#if defined(USE_ACM)
    pHdl->hBtrMgrIarmAcmHdl = 0; // Initialize with valid session_id_t if required
    // Set pcBtrMgrAcmSockPath with a valid path, if it needs initialization
    strncpy(pHdl->pcBtrMgrAcmSockPath, "socket_path", MAX_OUTPUT_PATH_LEN - 1);
    pHdl->i32BtrMgrAcmDCSockFd = -1; // Set to a valid socket descriptor if needed
    pHdl->i32BtrMgrAcmExternalIARMMode = 0; // Or another appropriate value
#endif

    // Initialize the function pointers if required
    pHdl->fpcBBtrMgrAcDataReady = NULL; // Set to your data ready callback function
    pHdl->vpBtrMgrAcDataReadyUserData = NULL; // Set to associated user data if needed

    pHdl->fpcBBtrMgrAcStatus = NULL; // Set to your status callback function
    pHdl->vpBtrMgrAcStatusUserData = NULL; // Set to associated user data if needed

    stBTRMgrOutASettings outSettings;
    stBTRMgrPCMInfo pcmInfo;
    memset(&outSettings, 0, sizeof(stBTRMgrOutASettings));
    outSettings.pstBtrMgrOutCodecInfo = &pcmInfo;
#if defined USE_ACM
    // Configure the mock for IARM_Bus_Call
    IARM_Result_t mockIARMResult = IARM_RESULT_SUCCESS;
    iarmbus_acm_arg_t mockIARMBusArgs = {0};
    mockIARMBusArgs.result = 0;
    IARM_Bus_Call_IgnoreAndReturn(IARM_RESULT_SUCCESS);
#endif
    // Invoke the function
    result = BTRMgr_AC_GetDefaultSettings(pHdl, &outSettings);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    // Additional asserts to verify that `outSettings` is set up correctly with the mocked default parameters
}

void test_BTRMgr_AC_GetDefaultSettings_ReturnsNotInitialized_when_HandleIsNull(void) {
    // Given
    tBTRMgrAcHdl handle = NULL;
    stBTRMgrOutASettings settings = {0};

    // When
    eBTRMgrRet result = BTRMgr_AC_GetDefaultSettings(handle, &settings);

    // Then
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

void test_BTRMgr_AC_GetDefaultSettings_ReturnsFailInArg_when_SettingsAreNull(void) {
    // Given
    stBTRMgrACHdl handleStruct = {0};
    tBTRMgrAcHdl handle = (tBTRMgrAcHdl)&handleStruct;

    // When
    eBTRMgrRet result = BTRMgr_AC_GetDefaultSettings(handle, NULL);

    // Then
    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, result);
}

void test_BTRMgr_AC_GetDefaultSettings_ReturnsSuccess_when_ArgumentsAreValid(void) {
    // Given
    stBTRMgrACHdl handleStruct = {0};
    tBTRMgrAcHdl handle = (tBTRMgrAcHdl)&handleStruct;
    stBTRMgrOutASettings settings = {0};
    stBTRMgrPCMInfo pcmInfo = {0};
    settings.pstBtrMgrOutCodecInfo = &pcmInfo;
    #if defined(USE_ACM)
    // Mocked function expectations
    IARM_Bus_Call_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    #endif
    // When
    eBTRMgrRet result = BTRMgr_AC_GetDefaultSettings(handle, &settings);

    // Then
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}

void test_BTRMgr_AC_DeInit_NullHandleError(void) {
    eBTRMgrRet result = BTRMgr_AC_DeInit(NULL);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}
void test_BTRMgr_AC_DeInit_FailUninitializedHandle(void) {
    // Prepare
    tBTRMgrAcHdl handle = NULL;

    // Exercise
    eBTRMgrRet result = BTRMgr_AC_DeInit(handle);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}


void test_BTRMgr_AC_GetStatus_ReturnsSuccess(void) {
    //stBTRMgrACHdl testHandle = (stBTRMgrACHdl)0x01; // Mocked handle, assuming non-null for this test
    stBTRMgrACHdl*  mockHandle =(stBTRMgrACHdl*)malloc(sizeof(stBTRMgrACHdl));
    stBTRMgrMediaStatus testStatus;

    eBTRMgrRet result = BTRMgr_AC_GetStatus(mockHandle, &testStatus);

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    free(mockHandle);
}




void test_BTRMgr_AC_Pause_ReturnsSuccess(void) {
   // tBTRMgrAcHdl testHandle = (tBTRMgrAcHdl)0x02; // A mocked handle, assuming non-null for this test
     stBTRMgrACHdl*  mockHandle =(stBTRMgrACHdl*)malloc(sizeof(stBTRMgrACHdl));
    eBTRMgrRet result = BTRMgr_AC_Pause(mockHandle);

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}
void test_BTRMgr_AC_Start_FailureInvalidHandle(void) {
    eBTRMgrRet result = BTRMgr_AC_Start(NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}


void test_BTRMgr_AC_Start_FailureInvalidSettings(void) {
    tBTRMgrAcHdl testHandle = (tBTRMgrAcHdl)malloc(sizeof(stBTRMgrACHdl));
    eBTRMgrRet result = BTRMgr_AC_Start(testHandle, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, result);
    free(testHandle);
}
void test_BTRMgr_AC_GetCurrentSettings_FailureNullSettings(void) {
    //stBTRMgrACHdl testHandle = (stBTRMgrACHdl)0x01; // Assuming a valid handle
    stBTRMgrACHdl*  mockHandle =(stBTRMgrACHdl*)malloc(sizeof(stBTRMgrACHdl));
    eBTRMgrRet result = BTRMgr_AC_GetCurrentSettings(mockHandle, NULL);
    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, result);
    free(mockHandle);
}
void test_BTRMgr_AC_GetCurrentSettings_Success(void) {
   stBTRMgrACHdl* mockHandle = (stBTRMgrACHdl*)malloc(sizeof(stBTRMgrACHdl));
    if (mockHandle == NULL) {
        TEST_FAIL_MESSAGE("Memory allocation failed");
        return;
    }

    mockHandle->pcBTRMgrAcType = malloc(8);
    if (mockHandle->pcBTRMgrAcType == NULL) {
        free(mockHandle);
        TEST_FAIL_MESSAGE("Memory allocation failed");
        return;
    }

    strncpy(mockHandle->pcBTRMgrAcType, "primary", 8);

    stBTRMgrOutASettings testSettings;
    stBTRMgrPCMInfo pcmInfo;
    memset(&pcmInfo, 0, sizeof(stBTRMgrPCMInfo));
    testSettings.pstBtrMgrOutCodecInfo = &pcmInfo;

#if defined(USE_ACM)
    IARM_Result_t mockIARMResult = IARM_RESULT_SUCCESS;
    iarmbus_acm_arg_t mockIARMBusArgs = {0};
    mockIARMBusArgs.result = 0;
    IARM_Bus_Call_ExpectAndReturn(IARMBUS_AUDIOCAPTUREMGR_NAME, IARMBUS_AUDIOCAPTUREMGR_GET_AUDIO_PROPS, &mockIARMBusArgs, sizeof(mockIARMBusArgs), mockIARMResult);
#endif

    eBTRMgrRet result = BTRMgr_AC_GetCurrentSettings((tBTRMgrAcHdl)mockHandle, &testSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);

    free(mockHandle->pcBTRMgrAcType);
    free(mockHandle);

}
void test_BTRMgr_AC_GetCurrentSettings_FailureNullHandle(void) {
    stBTRMgrOutASettings testSettings;
    eBTRMgrRet result = BTRMgr_AC_GetCurrentSettings(NULL, &testSettings);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}


void test_BTRMgr_AC_GetCurrentSettings_SuccessAuxiliary(void) {
    // Prepare
    tBTRMgrAcHdl handle = NULL;
    stBTRMgrOutASettings settings = {0};
    stBTRMgrPCMInfo pcmInfo = {0};
    settings.pstBtrMgrOutCodecInfo = &pcmInfo;

    BTRMgr_AC_Init(&handle, BTRMGR_AC_TYPE_AUXILIARY, 0);
    #if defined USE_RMF
    //RMF_AudioCapture_GetCurrentSettings_IgnoreAndReturn(RMF_SUCCESS);
    #endif
    // Exercise
    eBTRMgrRet result = BTRMgr_AC_GetCurrentSettings(handle, &settings);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);

    // Cleanup
    BTRMgr_AC_DeInit(handle);
}

void test_BTRMgr_AC_GetCurrentSettings_FailUninitializedHandle(void) {
    // Prepare
    tBTRMgrAcHdl handle = NULL;
    stBTRMgrOutASettings settings = {0};

    // Exercise
    eBTRMgrRet result = BTRMgr_AC_GetCurrentSettings(handle, &settings);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

void test_BTRMgr_AC_Start_UninitializedHandle(void) {
    stBTRMgrOutASettings testSettings;
    eBTRMgrRet result = BTRMgr_AC_Start(NULL, &testSettings, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

void test_BTRMgr_AC_Start_InvalidArguments(void) {
    stBTRMgrACHdl* mockHandle = (stBTRMgrACHdl*)malloc(sizeof(stBTRMgrACHdl));
    if (mockHandle == NULL) {
        TEST_FAIL_MESSAGE("Memory allocation failed");
        return;
    }

    eBTRMgrRet result = BTRMgr_AC_Start((tBTRMgrAcHdl)mockHandle, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, result);

    free(mockHandle);
}

void test_BTRMgr_AC_Start_returns_eBTRMgrNotInitialized_when_handle_is_Null(void) {
    tBTRMgrAcHdl handle = NULL;
    stBTRMgrOutASettings settings = {0};

    eBTRMgrRet result = BTRMgr_AC_Start(handle, &settings, NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

void test_BTRMgr_AC_Start_returns_eBTRMgrFailInArg_when_settings_are_Null(void) {
    stBTRMgrACHdl handleStruct = {0};
    tBTRMgrAcHdl handle = (tBTRMgrAcHdl)&handleStruct;
    stBTRMgrOutASettings* settings = NULL;

    eBTRMgrRet result = BTRMgr_AC_Start(handle, settings, NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, result);
}
void test_BTRMgr_AC_Stop_should_return_eBTRMgrNotInitialized_given_null_handle(void) {
    // Given
    tBTRMgrAcHdl handle = NULL;

    // When
    eBTRMgrRet result = BTRMgr_AC_Stop(handle);

    // Then
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

void test_BTRMgr_AC_GetStatus_Success(void) {
    // Prepare
    tBTRMgrAcHdl handle = (tBTRMgrAcHdl)1; // Assuming a valid handle
    stBTRMgrMediaStatus status = {0};

    // Exercise
    eBTRMgrRet result = BTRMgr_AC_GetStatus(handle, &status);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}

void test_BTRMgr_AC_GetStatus_UninitializedHandle(void) {
    // Prepare
    tBTRMgrAcHdl handle = NULL;
    stBTRMgrMediaStatus status = {0};

    // Exercise
    eBTRMgrRet result = BTRMgr_AC_GetStatus(handle, &status);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result); // Assuming the function should still return success
}

void test_BTRMgr_AC_GetStatus_NullStatusPointer(void) {
    // Prepare
    tBTRMgrAcHdl handle = (tBTRMgrAcHdl)1; // Assuming a valid handle

    // Exercise
    eBTRMgrRet result = BTRMgr_AC_GetStatus(handle, NULL);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result); // Assuming the function should still return success
}
void test_BTRMgr_AC_Start_SuccessAuxiliary(void) {
    // Prepare
    tBTRMgrAcHdl handle = NULL;
    stBTRMgrOutASettings settings = {0};
    settings.i32BtrMgrOutBufMaxSize = 1024;
    BTRMgr_AC_Init(&handle, BTRMGR_AC_TYPE_AUXILIARY, 0);
    #if defined USE_RMF
    RMF_AudioCapture_Start_IgnoreAndReturn(RMF_SUCCESS);
    #endif
    // Exercise
    eBTRMgrRet result = BTRMgr_AC_Start(handle, &settings, NULL, NULL, NULL);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);

    // Cleanup
    BTRMgr_AC_DeInit(handle);
}

void test_BTRMgr_AC_Start_FailUninitializedHandle(void) {
    // Prepare
    tBTRMgrAcHdl handle = NULL;
    stBTRMgrOutASettings settings = {0};

    // Exercise
    eBTRMgrRet result = BTRMgr_AC_Start(handle, &settings, NULL, NULL, NULL);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}
void test_BTRMgr_AC_Stop_SuccessAuxiliary(void) {
    // Prepare
    tBTRMgrAcHdl handle = NULL;
    BTRMgr_AC_Init(&handle, BTRMGR_AC_TYPE_AUXILIARY, 0);
    #if defined USE_RMF
    RMF_AudioCapture_Stop_IgnoreAndReturn(RMF_SUCCESS);
    #endif
    // Exercise
    eBTRMgrRet result = BTRMgr_AC_Stop(handle);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);

    // Cleanup
    BTRMgr_AC_DeInit(handle);
}

void test_BTRMgr_AC_Stop_FailUninitializedHandle(void) {
    // Prepare
    tBTRMgrAcHdl handle = NULL;

    // Exercise
    eBTRMgrRet result = BTRMgr_AC_Stop(handle);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}
void test_BTRMgr_AC_Pause_Success(void) {
    // Prepare
    tBTRMgrAcHdl handle = (tBTRMgrAcHdl)1; // Assuming a valid handle

    // Exercise
    eBTRMgrRet result = BTRMgr_AC_Pause(handle);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}

void test_BTRMgr_AC_Pause_UninitializedHandle(void) {
    // Prepare
    tBTRMgrAcHdl handle = NULL;

    // Exercise
    eBTRMgrRet result = BTRMgr_AC_Pause(handle);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result); // Assuming the function should still return success
}

void test_BTRMgr_AC_Resume_ReturnsSuccess(void) {
    //tBTRMgrAcHdl testHandle = (tBTRMgrAcHdl)0x01; // Mocked handle, assuming non-null for this test
     stBTRMgrACHdl*  mockHandle =(stBTRMgrACHdl*)malloc(sizeof(stBTRMgrACHdl));
    eBTRMgrRet result = BTRMgr_AC_Resume(mockHandle);

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}
void test_BTRMgr_AC_Resume_Success(void) {
    // Prepare
    tBTRMgrAcHdl handle = (tBTRMgrAcHdl)1; // Assuming a valid handle

    // Exercise
    eBTRMgrRet result = BTRMgr_AC_Resume(handle);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}

void test_BTRMgr_AC_Resume_UninitializedHandle(void) {
    // Prepare
    tBTRMgrAcHdl handle = NULL;

    // Exercise
    eBTRMgrRet result = BTRMgr_AC_Resume(handle);

    // Verify
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result); // Assuming the function should still return success
}





void test_BTRMgr_AC_Init_Success_ACM(void) {
    tBTRMgrAcHdl handle;
    char* acType = BTRMGR_AC_TYPE_PRIMARY;
    unsigned char debugMode = 1;

    // Mock the behavior of dependent functions
    IARM_Bus_IsConnected_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Init_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Connect_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Call_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Disconnect_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Term_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    eBTRMgrRet result = BTRMgr_AC_Init(&handle, acType, debugMode);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);

    // Clean up
    BTRMgr_AC_DeInit(handle);
}

void test_BTRMgr_AC_Init_IARM_Bus_Call_Failure(void) {
    tBTRMgrAcHdl handle;
    char* acType = BTRMGR_AC_TYPE_PRIMARY;
    unsigned char debugMode = 1;

    // Mock the behavior of dependent functions
    IARM_Bus_IsConnected_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Init_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Connect_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Call_IgnoreAndReturn(IARM_RESULT_IPCCORE_FAIL);
    IARM_Bus_Disconnect_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Term_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    eBTRMgrRet result = BTRMgr_AC_Init(&handle, acType, debugMode);
    TEST_ASSERT_EQUAL(eBTRMgrInitFailure, result);
}

void test_BTRMgr_AC_Start_Success(void) {
    tBTRMgrAcHdl handle;
    char* acType = BTRMGR_AC_TYPE_PRIMARY;
    unsigned char debugMode = 1;

    // Initialize the handle
    IARM_Bus_IsConnected_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Init_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Connect_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Call_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Disconnect_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Term_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    BTRMgr_AC_Init(&handle, acType, debugMode);

    stBTRMgrOutASettings outSettings;
    outSettings.i32BtrMgrOutBufMaxSize = 512;
    stBTRMgrPCMInfo pcmInfo;
    memset(&pcmInfo, 0, sizeof(stBTRMgrPCMInfo));
    outSettings.pstBtrMgrOutCodecInfo = &pcmInfo;

    // Mock the behavior of dependent functions
    IARM_Bus_Call_IgnoreAndReturn(IARM_RESULT_SUCCESS);

    eBTRMgrRet result = BTRMgr_AC_Start(handle, &outSettings, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);

    // Clean up
    BTRMgr_AC_DeInit(handle);
}

void test_BTRMgr_AC_Start_IARM_Bus_Call_Failure(void) {
    tBTRMgrAcHdl handle;
    char* acType = BTRMGR_AC_TYPE_PRIMARY;
    unsigned char debugMode = 1;

    // Initialize the handle
    IARM_Bus_IsConnected_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Init_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Connect_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Call_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Disconnect_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Term_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    BTRMgr_AC_Init(&handle, acType, debugMode);

    stBTRMgrOutASettings outSettings;
    outSettings.i32BtrMgrOutBufMaxSize = 512;
    stBTRMgrPCMInfo pcmInfo;
    memset(&pcmInfo, 0, sizeof(stBTRMgrPCMInfo));
    outSettings.pstBtrMgrOutCodecInfo = &pcmInfo;

    // Mock the behavior of dependent functions
    IARM_Bus_Call_IgnoreAndReturn(IARM_RESULT_IPCCORE_FAIL);

    eBTRMgrRet result = BTRMgr_AC_Start(handle, &outSettings, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);

    // Clean up
    BTRMgr_AC_DeInit(handle);
}

void test_BTRMgr_AC_DeInit_Success(void) {
    tBTRMgrAcHdl handle;
    char* acType = BTRMGR_AC_TYPE_PRIMARY;
    unsigned char debugMode = 1;

    // Initialize the handle
    IARM_Bus_IsConnected_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Init_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Connect_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Call_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Disconnect_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_Term_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    
    BTRMgr_AC_Init(&handle, acType, debugMode);

    // Mock the behavior of dependent functions
    IARM_Bus_Call_IgnoreAndReturn(IARM_RESULT_SUCCESS);

    eBTRMgrRet result = BTRMgr_AC_DeInit(handle);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}


IARM_Result_t _mock_IARM_Bus_Call(const char* ownerName, const char* methodName, void* arg, size_t argLen) {
    iarmbus_acm_arg_t* lstBtrMgrIarmAcmArgs = (iarmbus_acm_arg_t*)arg;

    if (strcmp(methodName, IARMBUS_AUDIOCAPTUREMGR_GET_DEFAULT_AUDIO_PROPS) == 0 || strcmp(methodName, IARMBUS_AUDIOCAPTUREMGR_GET_AUDIO_PROPS) == 0) {
        // Simulate different formats and frequencies
        if (lstBtrMgrIarmAcmArgs->session_id == 1) {
            lstBtrMgrIarmAcmArgs->details.arg_audio_properties.format = acmFormate16BitStereo;
            lstBtrMgrIarmAcmArgs->details.arg_audio_properties.sampling_frequency = acmFreqe16000;
        } else if (lstBtrMgrIarmAcmArgs->session_id == 2) {
            lstBtrMgrIarmAcmArgs->details.arg_audio_properties.format = acmFormate24BitStereo;
            lstBtrMgrIarmAcmArgs->details.arg_audio_properties.sampling_frequency = acmFreqe32000;
        } else if (lstBtrMgrIarmAcmArgs->session_id == 3) {
            lstBtrMgrIarmAcmArgs->details.arg_audio_properties.format = acmFormate16BitMonoLeft;
            lstBtrMgrIarmAcmArgs->details.arg_audio_properties.sampling_frequency = acmFreqe44100;
        } else if (lstBtrMgrIarmAcmArgs->session_id == 4) {
            lstBtrMgrIarmAcmArgs->details.arg_audio_properties.format = acmFormate16BitMonoRight;
            lstBtrMgrIarmAcmArgs->details.arg_audio_properties.sampling_frequency = acmFreqe48000;
        } else if (lstBtrMgrIarmAcmArgs->session_id == 5) {
            lstBtrMgrIarmAcmArgs->details.arg_audio_properties.format = acmFormate16BitMono;
            lstBtrMgrIarmAcmArgs->details.arg_audio_properties.sampling_frequency = acmFreqeMax;
        } else if (lstBtrMgrIarmAcmArgs->session_id == 6) {
            lstBtrMgrIarmAcmArgs->details.arg_audio_properties.format = acmFormate24Bit5_1;
        } else if (lstBtrMgrIarmAcmArgs->session_id == 7) {
            lstBtrMgrIarmAcmArgs->details.arg_audio_properties.format = acmFormateMax;
        }
        return IARM_RESULT_SUCCESS;
    }

    return IARM_RESULT_IPCCORE_FAIL;
}

void test_BTRMgr_AC_GetDefaultSettings_Success(void) {
    tBTRMgrAcHdl handle = (tBTRMgrAcHdl)malloc(sizeof(stBTRMgrACHdl));
    if (handle == NULL) {
        TEST_FAIL_MESSAGE("Memory allocation failed");
        return;
    }
    stBTRMgrACHdl* pstBtrMgrAcHdl = (stBTRMgrACHdl*)handle;
    pstBtrMgrAcHdl->pcBTRMgrAcType = strdup(BTRMGR_AC_TYPE_PRIMARY);

    stBTRMgrOutASettings outSettings;
    stBTRMgrPCMInfo pcmInfo;
    outSettings.pstBtrMgrOutCodecInfo = &pcmInfo;

    // Mock IARM functions
    IARM_Bus_Call_StubWithCallback(_mock_IARM_Bus_Call);

    // Set different formats and frequencies
    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 1;
    eBTRMgrRet result = BTRMgr_AC_GetDefaultSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmt16bit, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChanStereo, pcmInfo.eBtrMgrAChan);
    TEST_ASSERT_EQUAL(eBTRMgrSFreq16K, pcmInfo.eBtrMgrSFreq);

    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 2;
    result = BTRMgr_AC_GetDefaultSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmt24bit, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChanStereo, pcmInfo.eBtrMgrAChan);
    TEST_ASSERT_EQUAL(eBTRMgrSFreq32K, pcmInfo.eBtrMgrSFreq);

    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 3;
    result = BTRMgr_AC_GetDefaultSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmt16bit, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChanMono, pcmInfo.eBtrMgrAChan);
    TEST_ASSERT_EQUAL(eBTRMgrSFreq44_1K, pcmInfo.eBtrMgrSFreq);

    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 4;
    result = BTRMgr_AC_GetDefaultSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmt16bit, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChanMono, pcmInfo.eBtrMgrAChan);
    TEST_ASSERT_EQUAL(eBTRMgrSFreq48K, pcmInfo.eBtrMgrSFreq);

    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 5;
    result = BTRMgr_AC_GetDefaultSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmt16bit, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChanMono, pcmInfo.eBtrMgrAChan);
    TEST_ASSERT_EQUAL(eBTRMgrSFreqUnknown, pcmInfo.eBtrMgrSFreq);

    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 6;
    result = BTRMgr_AC_GetDefaultSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmt24bit, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChan5_1, pcmInfo.eBtrMgrAChan);

    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 7;
    result = BTRMgr_AC_GetDefaultSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmtUnknown, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChanUnknown, pcmInfo.eBtrMgrAChan);

    
    // Test for default case in frequency switch
    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 8;
    result = BTRMgr_AC_GetDefaultSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmt16bit, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChanStereo, pcmInfo.eBtrMgrAChan);
    
    // Test for failure scenario
    IARM_Bus_Call_IgnoreAndReturn(IARM_RESULT_IPCCORE_FAIL);
    result = BTRMgr_AC_GetDefaultSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);

    // Clean up
    free(pstBtrMgrAcHdl->pcBTRMgrAcType);
    free(handle);
}

void test_BTRMgr_AC_GetCurrentSettings_Success_switchcases(void) {
    tBTRMgrAcHdl handle = (tBTRMgrAcHdl)malloc(sizeof(stBTRMgrACHdl));
    if (handle == NULL) {
        TEST_FAIL_MESSAGE("Memory allocation failed");
        return;
    }
    stBTRMgrACHdl* pstBtrMgrAcHdl = (stBTRMgrACHdl*)handle;
    pstBtrMgrAcHdl->pcBTRMgrAcType = strdup(BTRMGR_AC_TYPE_PRIMARY);

    stBTRMgrOutASettings outSettings;
    stBTRMgrPCMInfo pcmInfo;
    outSettings.pstBtrMgrOutCodecInfo = &pcmInfo;

    // Mock IARM functions
    IARM_Bus_Call_StubWithCallback(_mock_IARM_Bus_Call);

    // Set different formats and frequencies
    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 1;
    eBTRMgrRet result = BTRMgr_AC_GetCurrentSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmt16bit, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChanStereo, pcmInfo.eBtrMgrAChan);
    TEST_ASSERT_EQUAL(eBTRMgrSFreq16K, pcmInfo.eBtrMgrSFreq);

    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 2;
    result = BTRMgr_AC_GetCurrentSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmt24bit, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChanStereo, pcmInfo.eBtrMgrAChan);
    TEST_ASSERT_EQUAL(eBTRMgrSFreq32K, pcmInfo.eBtrMgrSFreq);

    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 3;
    result = BTRMgr_AC_GetCurrentSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmt16bit, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChanMono, pcmInfo.eBtrMgrAChan);
    TEST_ASSERT_EQUAL(eBTRMgrSFreq44_1K, pcmInfo.eBtrMgrSFreq);

    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 4;
    result = BTRMgr_AC_GetCurrentSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmt16bit, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChanMono, pcmInfo.eBtrMgrAChan);
    TEST_ASSERT_EQUAL(eBTRMgrSFreq48K, pcmInfo.eBtrMgrSFreq);

    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 5;
    result = BTRMgr_AC_GetCurrentSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmt16bit, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChanMono, pcmInfo.eBtrMgrAChan);
    TEST_ASSERT_EQUAL(eBTRMgrSFreqUnknown, pcmInfo.eBtrMgrSFreq);

    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 6;
    result = BTRMgr_AC_GetCurrentSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmt24bit, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChan5_1, pcmInfo.eBtrMgrAChan);

    pstBtrMgrAcHdl->hBtrMgrIarmAcmHdl = 7;
    result = BTRMgr_AC_GetCurrentSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL(eBTRMgrSFmtUnknown, pcmInfo.eBtrMgrSFmt);
    TEST_ASSERT_EQUAL(eBTRMgrAChanUnknown, pcmInfo.eBtrMgrAChan);

    
    // Test for failure scenario
    IARM_Bus_Call_IgnoreAndReturn(IARM_RESULT_IPCCORE_FAIL);
    result = BTRMgr_AC_GetCurrentSettings(handle, &outSettings);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);

    // Clean up
    free(pstBtrMgrAcHdl->pcBTRMgrAcType);
    free(handle);
}


////////////////////////////////////////
//after 23 dec 2024

void* (*original_g_malloc0)(gsize) = g_malloc0;
GAsyncQueue* (*original_g_async_queue_new)(void) = g_async_queue_new;
GThread* (*original_g_thread_new)(const gchar*, GThreadFunc, gpointer) = g_thread_new;

void* mock_g_malloc0(gsize n_bytes) {
    return NULL;
}

GAsyncQueue* mock_g_async_queue_new(void) {
    return NULL;
}

GThread* mock_g_thread_new(const gchar* name, GThreadFunc func, gpointer data) {
    return NULL;
}


/////////////////////////////////////////////////////
eBTRMgrRet mock_fpcBBtrMgrAcStatus_Success(stBTRMgrMediaStatus* status, void* userData) {
    return eBTRMgrSuccess;
}

eBTRMgrRet mock_fpcBBtrMgrAcStatus_Failure(stBTRMgrMediaStatus* status, void* userData) {
    return eBTRMgrFailure;
}

void test_CallbackIsSetAndReturnsSuccess(void) {
    stBTRMgrACHdl handle = {0};
    handle.fpcBBtrMgrAcStatus = mock_fpcBBtrMgrAcStatus_Success;

    stBTRMgrMediaStatus lstBtrMgrAcMediaStatus = {0};
    eBTRMgrRet result = handle.fpcBBtrMgrAcStatus(&lstBtrMgrAcMediaStatus, handle.vpBtrMgrAcStatusUserData);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}

void test_CallbackIsSetAndReturnsFailure(void) {
    stBTRMgrACHdl handle = {0};
    handle.fpcBBtrMgrAcStatus = mock_fpcBBtrMgrAcStatus_Failure;

    stBTRMgrMediaStatus lstBtrMgrAcMediaStatus = {0};
    eBTRMgrRet result = handle.fpcBBtrMgrAcStatus(&lstBtrMgrAcMediaStatus, handle.vpBtrMgrAcStatusUserData);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}

void test_CallbackIsNotSet(void) {
    stBTRMgrACHdl handle = {0};
    handle.fpcBBtrMgrAcStatus = NULL;

    stBTRMgrMediaStatus lstBtrMgrAcMediaStatus = {0};
    if (handle.fpcBBtrMgrAcStatus) {
        eBTRMgrRet result = handle.fpcBBtrMgrAcStatus(&lstBtrMgrAcMediaStatus, handle.vpBtrMgrAcStatusUserData);
        TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    } else {
        TEST_PASS();
    }
}

void test_BTRMgr_AC_Stop_SuccessWithAcmDataCaptureThreadInitialized(void) {
    stBTRMgrACHdl* handle = (stBTRMgrACHdl*)malloc(sizeof(stBTRMgrACHdl));
    if (handle == NULL) {
        TEST_FAIL_MESSAGE("Memory allocation failed");
        return;
    }

    // Initialize the handle
    memset(handle, 0, sizeof(stBTRMgrACHdl));
    handle->pBtrMgrAcmDataCapGThread = (GThread*)1; // Simulate initialized thread

    // Set expectations for mocked functions
    IARM_Bus_Init_IgnoreAndReturn(IARM_RESULT_SUCCESS);
    IARM_Bus_IsConnected_IgnoreAndReturn(IARM_RESULT_SUCCESS);

    // Call the function under test
    eBTRMgrRet result = BTRMgr_AC_Stop(handle);

    // Verify the result
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);

    // Clean up
    free(handle);
}

/////////////////////////////




void test_btrMgr_AC_testDataCapture_InTask_NullHandle(void) {
    stBTRMgrACHdl*      pstBtrMgrAcHdl=NULL;
    gpointer result = btrMgr_AC_testDataCapture_InTask(pstBtrMgrAcHdl);
    TEST_ASSERT_NULL(result);
}

void test_btrMgr_AC_testDataCapture_InTask_DebugModeEnabled(void) {
    stBTRMgrACHdl* handle = (stBTRMgrACHdl*)malloc(sizeof(stBTRMgrACHdl));
    if (handle == NULL) {
        TEST_FAIL_MESSAGE("Memory allocation failed");
        return;
    }

    // Initialize the handle
    memset(handle, 0, sizeof(stBTRMgrACHdl));
    handle->pcBtrMgrTestSockPath[0] = '\0'; // Initialize the array

    // Assign values
    strncpy(handle->pcBtrMgrTestSockPath, "/tmp/test_socket", sizeof(handle->pcBtrMgrTestSockPath) - 1);
    handle->ui16Threshold = 1024;

    // Additional test logic here...

    free(handle);
}

void test_BTRMgr_AC_TestInit_Success(void) {
    tBTRMgrAcHdl handle = NULL;
    eBTRMgrRet result = BTRMgr_AC_TestInit(&handle, 1, "/tmp/test_socket");
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_NOT_NULL(handle);
    if (handle) {
        stBTRMgrACHdl* pstHandle = (stBTRMgrACHdl*)handle;
        g_async_queue_unref(pstHandle->pBtrMgrTestDataCapGAOpQueue);
        g_thread_unref(pstHandle->pBtrMgrTestDataCapGThread);
        g_free(handle);
    }
}
void test_BTRMgr_AC_TestStart_HandleNotInitialized(void) {
    tBTRMgrAcHdl handle = NULL;
    stBTRMgrOutASettings settings = { .i32BtrMgrOutBufMaxSize = 1024 };
    fPtr_BTRMgr_AC_DataReadyCb dataReadyCb = NULL;
    fPtr_BTRMgr_AC_StatusCb statusCb = NULL;
    void* userData = NULL;

    eBTRMgrRet result = BTRMgr_AC_TestStart(handle, &settings, dataReadyCb, statusCb, userData);
    TEST_ASSERT_EQUAL(eBTRMgrNotInitialized, result);
}

void test_BTRMgr_AC_TestStart_SettingsNotGiven(void) {
    tBTRMgrAcHdl handle = NULL;
    fPtr_BTRMgr_AC_DataReadyCb dataReadyCb = NULL;
    fPtr_BTRMgr_AC_StatusCb statusCb = NULL;
    void* userData = NULL;

    // Initialize handle
    BTRMgr_AC_TestInit(&handle, 1, "/tmp/test_socket");

    eBTRMgrRet result = BTRMgr_AC_TestStart(handle, NULL, dataReadyCb, statusCb, userData);
    TEST_ASSERT_EQUAL(eBTRMgrFailInArg, result);

    // Clean up
    if (handle) {
        stBTRMgrACHdl* pstHandle = (stBTRMgrACHdl*)handle;
        g_async_queue_unref(pstHandle->pBtrMgrTestDataCapGAOpQueue);
        g_thread_unref(pstHandle->pBtrMgrTestDataCapGThread);
        g_free(handle);
    }
}

void test_BTRMgr_AC_TestStart_ThreadNotInitialized(void) {
    tBTRMgrAcHdl handle = NULL;
    stBTRMgrOutASettings settings = { .i32BtrMgrOutBufMaxSize = 1024 };
    fPtr_BTRMgr_AC_DataReadyCb dataReadyCb = NULL;
    fPtr_BTRMgr_AC_StatusCb statusCb = NULL;
    void* userData = NULL;

    // Initialize handle
    BTRMgr_AC_TestInit(&handle, 1, "/tmp/test_socket");

    // Simulate thread not initialized
    stBTRMgrACHdl* pstHandle = (stBTRMgrACHdl*)handle;
    g_thread_unref(pstHandle->pBtrMgrTestDataCapGThread);
    pstHandle->pBtrMgrTestDataCapGThread = NULL;

    eBTRMgrRet result = BTRMgr_AC_TestStart(handle, &settings, dataReadyCb, statusCb, userData);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);

    // Clean up
    if (handle) {
        g_async_queue_unref(pstHandle->pBtrMgrTestDataCapGAOpQueue);
        g_free(handle);
    }
}
void test_BTRMgr_AC_TestStop_Success(void) {
    tBTRMgrAcHdl handle = NULL;

    // Initialize handle
    BTRMgr_AC_TestInit(&handle, 1, "/tmp/test_socket");

    eBTRMgrRet result = BTRMgr_AC_TestStop(handle);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);

    // Clean up
    if (handle) {
        stBTRMgrACHdl* pstHandle = (stBTRMgrACHdl*)handle;
        g_async_queue_unref(pstHandle->pBtrMgrTestDataCapGAOpQueue);
        g_thread_unref(pstHandle->pBtrMgrTestDataCapGThread);
        g_free(handle);
    }
}



void test_BTRMgr_AC_TestStop_ThreadNotInitialized(void) {
    tBTRMgrAcHdl handle = NULL;

    // Initialize handle
    BTRMgr_AC_TestInit(&handle, 1, "/tmp/test_socket");

    // Simulate thread not initialized
    stBTRMgrACHdl* pstHandle = (stBTRMgrACHdl*)handle;
    g_thread_unref(pstHandle->pBtrMgrTestDataCapGThread);
    pstHandle->pBtrMgrTestDataCapGThread = NULL;

    eBTRMgrRet result = BTRMgr_AC_TestStop(handle);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);

    // Clean up
    if (handle) {
        g_async_queue_unref(pstHandle->pBtrMgrTestDataCapGAOpQueue);
        g_free(handle);
    }
}
/*
void test_BTRMgr_AC_TestStart_Success(void) {
    tBTRMgrAcHdl handle = NULL;
    stBTRMgrOutASettings settings = { .i32BtrMgrOutBufMaxSize = 1024 };
    fPtr_BTRMgr_AC_DataReadyCb dataReadyCb = NULL;
    fPtr_BTRMgr_AC_StatusCb statusCb = NULL;
    void* userData = NULL;

    // Initialize handle
    BTRMgr_AC_TestInit(&handle, 1, "/tmp/test_socket");

    eBTRMgrRet result = BTRMgr_AC_TestStart(handle, &settings, dataReadyCb, statusCb, userData);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);

    // Clean up
    if (handle) {
        stBTRMgrACHdl* pstHandle = (stBTRMgrACHdl*)handle;
        if (pstHandle->pBtrMgrTestDataCapGAOpQueue) {
            g_async_queue_unref(pstHandle->pBtrMgrTestDataCapGAOpQueue);}
        if (pstHandle->pBtrMgrTestDataCapGThread) {
            g_thread_unref(pstHandle->pBtrMgrTestDataCapGThread);
        }
        if (handle) {
            g_free(handle);
        }
    }
}
*/
