#ifndef UNIT_TEST
#define UNIT_TEST
#endif
#include "unity.h" // The testing framework
/* System Headers */
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef BUILD_RDKTV
#define BUILD_RDKTV
#endif

/* Ext lib Headers */
#include "cjson/cJSON.h"
#define JSON_PATH_UNIT_TEST "unitTest/support/jsonUnderTest.json"
#define BUFFER_LENGTH 256

#include "btrMgr_persistIface.h"
#include "mock_btrMgr_logger.h"
#include "mock_btrMgr_Types.h"



TEST_FILE("btrMgr_persistIface.c")

tBTRMgrPIHdl testBTRMgrPiHdl;
void test_BTRMgr_PI_Init_MemoryAllocationFailed(void) {
    char limitCheck[64];
    memset(limitCheck, '\0', BTRMGR_NAME_LEN_MAX);
    eBTRMgrRet result = BTRMgr_PI_Init(&testBTRMgrPiHdl);

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    
    //compare result array
    TEST_ASSERT_EQUAL_CHAR_ARRAY(limitCheck, ((BTRMGR_PersistentData_t*)testBTRMgrPiHdl)->limitBeaconDetection,BTRMGR_NAME_LEN_MAX);

}

void test_BTRMgr_PI_DeInit_Success(void) {
    // Allocate memory and initialize the PI handle
    tBTRMgrPIHdl hBTRMgrPiHdl = malloc(sizeof(BTRMGR_PersistentData_t));
    TEST_ASSERT_NOT_NULL(hBTRMgrPiHdl);

    // Call the function being tested
    eBTRMgrRet result = BTRMgr_PI_DeInit(hBTRMgrPiHdl);

    // Check if the function returns the expected value
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}

void test_BTRMgr_PI_DeInit_Failure(void) {
    // Call the function with a NULL handle
    tBTRMgrPIHdl hBTRMgrPiHdl = NULL;
    eBTRMgrRet result = BTRMgr_PI_DeInit(hBTRMgrPiHdl);

    // Check if the function returns the expected value
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);

}

void test_BTRMgr_PI_GetLEBeaconLimitingStatus_EmptyFile(void) {
    // Create a JSON string to simulate the behavior

    // Assuming a valid BTRMGR_Beacon_PersistentData_t structure
    char* jsonName = "unitTest/support/get_Limit_Beacon_Empty.json";
    FILE *file = fopen(jsonName,"w");   
     if (file != NULL) {
        fclose(file);
    }
    
    BTRMGR_Beacon_PersistentData_t persistentData;
    rename(jsonName,JSON_PATH_UNIT_TEST );
    eBTRMgrRet result = BTRMgr_PI_GetLEBeaconLimitingStatus(&persistentData);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
    rename(JSON_PATH_UNIT_TEST, jsonName);
}
 /*
void test_BTRMgr_PI_GetLEBeaconLimitingStatus_Success(void) {
    // Create a JSON string to simulate the behavior

    // Assuming a valid BTRMGR_Beacon_PersistentData_t structure
    char* jsonName = "unitTest/support/get_Limit_Beacon_Good.json";
    BTRMGR_Beacon_PersistentData_t persistentData;
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
    if (system(command) != 0) {
        printf("File copy failed1.\n");
    }
    eBTRMgrRet result = BTRMgr_PI_GetLEBeaconLimitingStatus(&persistentData);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL_STRING("true", persistentData.limitBeaconDetection);
}*/
/*
void test_BTRMgr_PI_GetLEBeaconLimitingStatus_CorruptedJSON(void) {
    // Create a JSON string to simulate the behavior

    // Assuming a valid BTRMGR_Beacon_PersistentData_t structure
    char* jsonName = "unitTest/support/get_Limit_Beacon_CorruptedFile.json";
    BTRMGR_Beacon_PersistentData_t persistentData;
    rename(jsonName,JSON_PATH_UNIT_TEST );
    eBTRMgrRet result = BTRMgr_PI_GetLEBeaconLimitingStatus(&persistentData);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
  rename(JSON_PATH_UNIT_TEST, jsonName);
}

void test_BTRMgr_PI_GetLEBeaconLimitingStatus_CorruptedJSON_Null(void) {
    // Create a JSON string to simulate the behavior

    // Assuming a valid BTRMGR_Beacon_PersistentData_t structure
    char* jsonName = "unitTest/support/get_Limit_Beacon_CorruptedFile_Null.json";
    BTRMGR_Beacon_PersistentData_t persistentData;
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
    if (system(command) != 0) {
        printf("File copy failed.%d\n",__LINE__);
    }
    eBTRMgrRet result = BTRMgr_PI_GetLEBeaconLimitingStatus(&persistentData);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}

void test_BTRMgr_PI_GetLEBeaconLimitingStatus_MissingJSONItem(void) {
    // Create a JSON string to simulate the behavior

    // Assuming a valid BTRMGR_Beacon_PersistentData_t structure
    char* jsonName = "unitTest/support/get_Limit_Beacon_MissingJson.json";
    BTRMGR_Beacon_PersistentData_t persistentData;
  rename(jsonName,JSON_PATH_UNIT_TEST );
   eBTRMgrRet result = BTRMgr_PI_GetLEBeaconLimitingStatus(&persistentData);

    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
    rename(JSON_PATH_UNIT_TEST, jsonName);
}
*/
void test_BTRMgr_PI_SetLEBeaconLimitingStatus_EmptyFile(void) {
    // Create a JSON string to simulate the behavior

    // Assuming a valid BTRMGR_Beacon_PersistentData_t structure
    char* jsonName = "unitTest/support/get_Limit_Beacon_Empty.json";
    FILE *file = fopen(jsonName,"w");   
     if (file != NULL) {
        fclose(file);
    }
    
    BTRMGR_Beacon_PersistentData_t persistentData;
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
    if (system(command) != 0) {
        printf("File copy failed5.\n");
    }
    eBTRMgrRet result = BTRMgr_PI_SetLEBeaconLimitingStatus(&persistentData);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}

void test_BTRMgr_PI_SetLEBeaconLimitingStatus_CorruptedJSON(void) {
    // Create a JSON string to simulate the behavior

    // Assuming a valid BTRMGR_Beacon_PersistentData_t structure
    char* jsonName = "unitTest/support/get_Limit_Beacon_CorruptedFile.json";
    BTRMGR_Beacon_PersistentData_t persistentData;
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
    if (system(command) != 0) {
        printf("File copy failed6.\n");
    }
    eBTRMgrRet result = BTRMgr_PI_SetLEBeaconLimitingStatus(&persistentData);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}
/*
void test_BTRMgr_PI_SetLEBeaconLimitingStatus_Success(void) {
    // Create a JSON string to simulate the behavior

    // Assuming a valid BTRMGR_Beacon_PersistentData_t structure
    char* jsonName = "unitTest/support/set_Limit_Beacon_Good.json";
    BTRMGR_Beacon_PersistentData_t persistentData;
   strcpy( persistentData.limitBeaconDetection,"true");
     char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
    if (system(command) != 0) {
        printf("File copy failed. line  %d\n ", __LINE__ );
    }
    eBTRMgrRet result = BTRMgr_PI_SetLEBeaconLimitingStatus(&persistentData);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
    TEST_ASSERT_EQUAL_STRING("true", persistentData.limitBeaconDetection);
}

void test_BTRMgr_PI_SetLEBeaconLimitingStatus_MissingJSONItem(void) {
    // Create a JSON string to simulate the behavior
    remove(JSON_PATH_UNIT_TEST);
    // Assuming a valid BTRMGR_Beacon_PersistentData_t structure
    BTRMGR_Beacon_PersistentData_t persistentData;
    eBTRMgrRet result = BTRMgr_PI_SetLEBeaconLimitingStatus(&persistentData);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}

void test_BTRMgr_PI_SetConnectionStatus_fileReadFail(void)
{
    char* jsonName = "unitTest/support/get_Limit_Beacon_Empty.json";
    BTRMGR_Beacon_PersistentData_t persistentData;
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
    if (system(command) != 0) {
        printf("File copy failed9.\n");
    }
    eBTRMgrRet result = BTRMgr_PI_SetConnectionStatus(NULL, NULL, 0);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}
void test_BTRMgr_PI_SetConnectionStatus_bothProfileAndDeviceMatch(void)
{

    char* jsonName = "unitTest/support/set_Connection_status_both_profile_and_device_match.json";
    BTRMGR_Beacon_PersistentData_t persistentData;
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
    if (system(command) != 0) {
        printf("File copy failed %d.\n", __LINE__);
    }
    eBTRMgrRet result = BTRMgr_PI_SetConnectionStatus(NULL, "0x110b", 0x110b);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);

}

void test_BTRMgr_PI_SetConnectionStatus_profileMatchesButDeviceDoesNot(void)
{
 
      eBTRMgrRet result = BTRMgr_PI_SetConnectionStatus(NULL, "ProfileStr", 0);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}
void test_BTRMgr_PI_SetConnectionStatus_missingProfiles(void)
{
    // Set up Mocks here similar to the above example
    // Use cJSON_GetObjectItem_ExpectAndReturn to expect and return NULL for "Profiles"
    // ...
    char* jsonName = "unitTest/support/set_Connection_Status_Missing_Profiles.json";
    BTRMGR_Beacon_PersistentData_t persistentData;
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
    if (system(command) != 0) {
        printf("File copy faile12.\n");
    }
    eBTRMgrRet result = BTRMgr_PI_SetConnectionStatus(NULL, NULL, 0);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
   
}
void test_BTRMgr_PI_SetConnectionStatus_corruptedJSON(void)
{
    char* jsonName = "unitTest/support/set_Connection_Status_Corrupted_Json.json";
    BTRMGR_Beacon_PersistentData_t persistentData;
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
    if (system(command) != 0) {
        printf("File copy faile13.\n");
    }
    eBTRMgrRet result = BTRMgr_PI_SetConnectionStatus(NULL, NULL, 0);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}*/
void test_BTRMgr_PI_GetVolume_when_FileIsEmpty(void)
{
    char* actualPath = "unitTest/support/file";
   char command[256];
    snprintf(command, sizeof(command), "cp %s %s", actualPath, JSON_PATH_UNIT_TEST);
    if (system(command) != 0) {
        printf("File copy faile14.\n");
    }
    BTRMGR_Volume_PersistentData_t pd = { 0 };
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_PI_GetVolume(&pd, "prof", 1));
}
void test_BTRMgr_PI_GetVolume_when_NoVolumeData(void)
{
    char* actualPath = "unitTest/support/noVolume_data.json";
       char command[256];
    snprintf(command, sizeof(command), "cp %s %s", actualPath, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy faile15.\n");
        }
    BTRMGR_Volume_PersistentData_t pd = { 0 };
    
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_PI_GetVolume(&pd, "0x110b", 163411682490176));
}
void test_BTRMgr_PI_GetVolume_when_ProfileIdNotFound(void)
{
    char* actualPath = "unitTest/support/profile_id_abscent.json";
   char command[256];
snprintf(command, sizeof(command), "cp %s %s", actualPath, JSON_PATH_UNIT_TEST);
    if (system(command) != 0) {
        printf("File copy faile352.\n");
    }
    BTRMGR_Volume_PersistentData_t pd = { 0 };

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_PI_GetVolume(&pd, "1234", 1));
   // todo improve function to cover failure scenario where is no profile ID(add the else case in main code)
}
void test_BTRMgr_PI_GetVolume_when_AllParametersAreCorrect(void)
{
    char* actualPath = "unitTest/support/correct_param.json";
        char command[256];
    snprintf(command, sizeof(command), "cp %s %s", actualPath, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy faile17.\n");
        }
    BTRMGR_Volume_PersistentData_t pd = { 0 };

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_PI_GetVolume(&pd, "0x110b", 163411682490176));
    TEST_ASSERT_EQUAL(90, pd.Volume);  //If Volume is 90 in the JSON file
}
void test_BTRMgr_PI_SetVolume_when_FileIsEmpty(void)
{
    char* actualPath = "unitTest/support/set_file.json";
        char command[256];
    snprintf(command, sizeof(command), "cp %s %s", actualPath, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line 383.\n");
        }
    BTRMGR_Volume_PersistentData_t pd = { 0 };
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_PI_SetVolume(&pd, "prof", 1));
}
void test_BTRMgr_PI_SetVolume_when_AllParametersAreCorrect(void)
{
    char* actualPath = "unitTest/support/volume_tv.json";
   rename(actualPath, BTRMGR_PERSISTENT_DATA_PATH);
   /*
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", actualPath, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed20.\n");
        }
    */
       
    BTRMGR_Volume_PersistentData_t pd = { 0 };
    pd.Volume = 90;
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_PI_SetVolume(&pd, "0x110b", 163411682490176));
    rename(BTRMGR_PERSISTENT_DATA_PATH, actualPath);  
}
void test_BTRMgr_PI_SetVolume_when_ProfileIdNotFound(void)
{
      eBTRMgrRet result = BTRMgr_PI_SetVolume(NULL, NULL, 0);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}
// Invalid JSON content in File
void test_BTRMgr_PI_SetVolume_when_InvalidJsonContent(void) {
    char* actualPath = "unitTest/support/SetVolume_when_InvalidJsonContent.json";
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", actualPath, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed451.\n");
        }
    BTRMGR_Volume_PersistentData_t pd = { 0 };

    pd.Volume = 20;
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_PI_SetVolume(&pd, "0x110b", 163411682490176));
    rename(BTRMGR_PERSISTENT_DATA_PATH, actualPath);
}

void test_BTRMgr_PI_GetMute_fileReadFail(void)
{
    char* actualPath = "unitTest/support/set_volume_corruptedfile.json";
    char command[256];
  
    eBTRMgrRet result = BTRMgr_PI_GetMute(NULL);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
    
}
void test_BTRMgr_PI_GetMute_Missing_mute(void){
    // Create a JSON string to simulate the behavior

    // Assuming a valid BTRMGR_Beacon_PersistentData_t structure
    char* jsonName = "unitTest/support/Get_mute_missing_mute.json";
    BTRMGR_Beacon_PersistentData_t persistentData;
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line 481.\n");
        }
    BTRMGR_Mute_PersistentData_t pd = { 0 };
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_PI_GetMute(NULL));
}
void test_BTRMgr_PI_GetMute_corruptedJSON(void){
    // Create a JSON string to simulate the behavior

    // Assuming a valid BTRMGR_Beacon_PersistentData_t structure
    char* jsonName = "unitTest/support/Get_mute_CorruptedFile.json";
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line 497.\n");
        }
    BTRMGR_Beacon_PersistentData_t persistentData;
    eBTRMgrRet result =BTRMgr_PI_GetMute(NULL);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}

void test_BTRMgr_PI_GetMute_allDetailsPresent(void)
{
    char* actualPath = "unitTest/support/set_allDetailsPresent.json";
   // rename(actualPath,JSON_PATH_UNIT_TEST);
   char command[256];
    snprintf(command, sizeof(command), "cp %s %s", actualPath, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line 513.\n");
        }
    BTRMGR_Mute_PersistentData_t pd ;
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_PI_GetMute(&pd));
    TEST_ASSERT_EQUAL_STRING("false", pd.Mute);  
}
/*
void test_BTRMgr_PI_SetMute_fileReadFail(void)
{
    char* actualPath = "unitTest/support/file";
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", actualPath, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line 548.\n");
        }
    eBTRMgrRet result = BTRMgr_PI_SetMute(NULL);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result); 
}
void test_BTRMgr_PI_SetMute_Missing_mute(void){
    // Create a JSON string to simulate the behavior

    // Assuming a valid BTRMGR_Beacon_PersistentData_t structure
    char* jsonName = "unitTest/support/Set_mute_missing_mute.json";
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line 552.\n");
        }
    BTRMGR_Mute_PersistentData_t persistentData = {"OFF"};
    eBTRMgrRet result = BTRMgr_PI_SetMute(&persistentData);
}
void test_BTRMgr_PI_SetMute_corruptedJSON(void){
    // Create a JSON string to simulate the behavior

    // Assuming a valid BTRMGR_Beacon_PersistentData_t structure
    char* jsonName = "unitTest/support/Set_mute_CorruptedFile.json";
    BTRMGR_Beacon_PersistentData_t persistentData;
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line 568.\n");
        }
    eBTRMgrRet result =BTRMgr_PI_SetMute(NULL);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}*/
void test_BTRMgr_PI_SetMute_sameMuteValue(void)
{
    // Set up Mocks to simulate "Mute" being present with same value
    // ...
    char* jsonName = "unitTest/support/set_Mute_Off.json";
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line 582.\n");
        }
    BTRMGR_Mute_PersistentData_t persistentData = {"OFF"};
    eBTRMgrRet result = BTRMgr_PI_SetMute(&persistentData);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
}

void test_BTRMgr_PI_SetMute_changedMuteValue(void)
{
    // Set up Mocks to simulate "Mute" being present with different value
    // ...
     char* jsonName = "unitTest/support/set_Mute_On.json";
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line 599.\n");
        }
    BTRMGR_Mute_PersistentData_t persistentData = {"On"};
    eBTRMgrRet result = BTRMgr_PI_SetMute(&persistentData);
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, result);
} 
void test_BTRMgr_PI_GetAllProfiles_when_InvalidJsonContent(void) {
    BTRMGR_PersistentData_t persistentData;
    tBTRMgrPIHdl                hBTRMgrPiHdl;
    char* jsonName ="unitTest/support/GetAllProfiles_when_InvalidJsonContent.json";
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line 618.\n");
        }
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_PI_GetAllProfiles(hBTRMgrPiHdl, &persistentData));
}
void test_BTRMgr_PI_GetAllProfiles_when_AdapterIdNotFoundInJsonContent(void) {
    BTRMGR_PersistentData_t persistentData;
    tBTRMgrPIHdl                hBTRMgrPiHdl;
     char* jsonName ="unitTest/support/AdapterIdNotFoundInJsonContent.json";
   
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line 632.\n");
        }  
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_PI_GetAllProfiles(hBTRMgrPiHdl, &persistentData));
}
void test_BTRMgr_PI_GetAllProfiles_when_ProfilesNotFoundInJsonContent(void) {
    BTRMGR_PersistentData_t persistentData;
     tBTRMgrPIHdl                hBTRMgrPiHdl;
     char* jsonName ="unitTest/support/ProfilesNotFoundInJsonContent.json";
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line %d.\n", __LINE__);
        }
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_PI_GetAllProfiles(hBTRMgrPiHdl, &persistentData));
}

/*
void test_BTRMgr_PI_GetAllProfiles_when_Successful(void) {
    BTRMGR_PersistentData_t persistentData;
    tBTRMgrPIHdl                hBTRMgrPiHdl;
    char* jsonName ="unitTest/support/When_Success.json";
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line %d.\n", __LINE__);
        }
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_PI_GetAllProfiles(hBTRMgrPiHdl, &persistentData));
}*/


void test_BTRMgr_PI_GetAllProfiles_NullHandle(void)
{
    eBTRMgrRet result = BTRMgr_PI_GetAllProfiles(NULL, NULL);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}
void test_BTRMgr_PI_GetAllProfiles_SecondNullHandle(void)
{

     BTRMGR_PersistentData_t persistentData;
    tBTRMgrPIHdl                hBTRMgrPiHdl;
     char* jsonName ="unitTest/support/Second_Null.json";
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line %d.\n", __LINE__);
        }
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_PI_GetAllProfiles(hBTRMgrPiHdl, NULL));
}

void test_BTRMgr_PI_GetAllProfiles_fileReadFail(void)
{
    BTRMGR_PersistentData_t persistentData;
    tBTRMgrPIHdl                hBTRMgrPiHdl;
    remove(JSON_PATH_UNIT_TEST);
    eBTRMgrRet result = BTRMgr_PI_GetAllProfiles(hBTRMgrPiHdl, &persistentData );
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}
BTRMGR_Profile_t profileData = {
    .adapterId = "C0:E7:BF:07:47:1B",
    .profileId = "0x110b",
    .deviceId = 163411682490176,
#ifdef RDKTV_PERSIST_VOLUME
    .Volume = 50,
#endif
    .isConnect = 1
};

void test_BTRMgr_PI_AddProfile_Should_ReturnFailure_When_BothHandlesAreNull(void) {
    eBTRMgrRet result = BTRMgr_PI_AddProfile(NULL, NULL);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
}
void test_BTRMgr_PI_AddProfile_when_JsonFileFetchFails(void) {

   remove(JSON_PATH_UNIT_TEST);
    tBTRMgrPIHdl                hBTRMgrPiHdl=0x01;
      BTRMGR_Profile_t persistentData = {
        .profileId ="0x110b",
        .adapterId ="C0:E7:BF:07:47:1B",
        .deviceId = 163411682490179,
        .isConnect = 0
    };
    eBTRMgrRet result = BTRMgr_PI_AddProfile(hBTRMgrPiHdl,&persistentData);
    TEST_ASSERT_EQUAL(eBTRMgrFailure, result);
    //Add profile doesn't check return of get all profiles, need to do for chase when there is no json input.
}


void test_BTRMgr_PI_AddProfile_DeviceIdIsDuplicate(void) {
    tBTRMgrPIHdl                hBTRMgrPiHdl=0x01;
    
    BTRMGR_Profile_t profileToEdit = {
        .profileId ="0x110b",
        .adapterId ="C0:E7:BF:07:47:1B",
        .deviceId = 163411682490179,
        .isConnect = 0
    };
     char* jsonName ="unitTest/support/Duplicate.json";
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line %d.\n", __LINE__);
        }
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_PI_AddProfile(hBTRMgrPiHdl, &profileToEdit));

    FILE *f = fopen(JSON_PATH_UNIT_TEST, "r");
    char data[1024] = {0};
    if (f!=NULL)
    {
     // Assuming file content is within BUFFER_LENGTH
     printf("entering if at line %d\n", __LINE__);
    fread(data, 1, 1023, f);
    fclose(f);
    }
    printf("exiting if at line %d\n", __LINE__);
    cJSON *root = cJSON_Parse(data);
    cJSON *adapterIdJson = cJSON_GetObjectItem(root, "AdapterId");
    TEST_ASSERT_NOT_NULL(adapterIdJson);
    TEST_ASSERT_EQUAL_STRING("C0:E7:BF:07:47:1B", adapterIdJson->valuestring);
    cJSON *profilesJson = cJSON_GetObjectItem(root, "Profiles");
    TEST_ASSERT_NOT_NULL(profilesJson);
    cJSON *profileJson = cJSON_GetArrayItem(profilesJson, 0);
    TEST_ASSERT_NOT_NULL(profileJson);
    cJSON *profileIdJson = cJSON_GetObjectItem(profileJson, "ProfileId");
    TEST_ASSERT_NOT_NULL(profileIdJson);
    TEST_ASSERT_EQUAL_STRING("0x110b", profileIdJson->valuestring);
    cJSON *deviceSJson = cJSON_GetObjectItem(profileJson, "Devices");
    TEST_ASSERT_NOT_NULL(deviceSJson);
    cJSON *deviceJson = cJSON_GetArrayItem(deviceSJson, 0);
    TEST_ASSERT_NOT_NULL(deviceSJson);
    cJSON *deviceIdJson = cJSON_GetObjectItem(deviceJson, "DeviceId");
    TEST_ASSERT_NOT_NULL(deviceIdJson);
    TEST_ASSERT_EQUAL_STRING("163411682490179", deviceIdJson->valuestring);
    cJSON *connectStatus = cJSON_GetObjectItem(deviceJson, "ConnectionStatus");
    TEST_ASSERT_EQUAL(0, deviceIdJson->valueint );

     cJSON_Delete(root);
}


void test_BTRMgr_PI_AddProfile_NewDeviceId(void) {
    tBTRMgrPIHdl                hBTRMgrPiHdl=0x01;
    char* jsonName ="unitTest/support/Duplicate.json";
    BTRMGR_PersistentData_t persistentData;
    char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
    if (system(command) != 0) {
        printf("File copy failed Line %d.\n", __LINE__);
    }
    BTRMGR_Profile_t profileToEdit = {
        .profileId ="0x110b",
        .adapterId ="C0:E7:BF:07:47:1B",
        .deviceId = 163411682490180,
        .isConnect = 0
    };
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_PI_AddProfile(hBTRMgrPiHdl, &profileToEdit));
    FILE *f = fopen(JSON_PATH_UNIT_TEST, "r");
    char data[1024] = {0};
    if (f!=NULL)
    {
        // Assuming file content is within BUFFER_LENGTH
        printf("entering if at line %d\n", __LINE__);
        fread(data, 1, 1023, f);
        fclose(f);
    }
    cJSON *root = cJSON_Parse(data);

    // Verify the data
    cJSON *adapterIdJson = cJSON_GetObjectItem(root, "AdapterId");
    TEST_ASSERT_NOT_NULL(adapterIdJson);
    TEST_ASSERT_EQUAL_STRING("C0:E7:BF:07:47:1B", adapterIdJson->valuestring);

    cJSON *profilesJson = cJSON_GetObjectItem(root, "Profiles");
    TEST_ASSERT_NOT_NULL(profilesJson);
    cJSON *profileJson = cJSON_GetArrayItem(profilesJson, 0);
    TEST_ASSERT_NOT_NULL(profileJson);
    cJSON *profileIdJson = cJSON_GetObjectItem(profileJson, "ProfileId");
    TEST_ASSERT_NOT_NULL(profileIdJson);
    TEST_ASSERT_EQUAL_STRING("0x110b", profileIdJson->valuestring);
    cJSON *deviceSJson = cJSON_GetObjectItem(profileJson, "Devices");
    TEST_ASSERT_NOT_NULL(deviceSJson);
    cJSON *deviceJson = cJSON_GetArrayItem(deviceSJson, 1);
    TEST_ASSERT_NOT_NULL(deviceSJson);
    cJSON *deviceIdJson = cJSON_GetObjectItem(deviceJson, "DeviceId");
    TEST_ASSERT_NOT_NULL(deviceIdJson);
    TEST_ASSERT_EQUAL_STRING("163411682490180", deviceIdJson->valuestring);

     cJSON_Delete(root);
}

void test_BTRMgr_PI_AddProfile_ProfileId(void) {
    tBTRMgrPIHdl                hBTRMgrPiHdl=0x01;
    char* jsonName ="unitTest/support/Duplicate.json";
    BTRMGR_PersistentData_t persistentData;
     char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line %d.\n", __LINE__);
        }
    BTRMGR_Profile_t profileToEdit = {
        .profileId ="0x110d",
        .adapterId ="C0:E7:BF:07:47:1B",
        .deviceId = 163411682490179,
        .isConnect = 0
    };
    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_PI_AddProfile(hBTRMgrPiHdl, &profileToEdit));

    FILE *f = fopen(JSON_PATH_UNIT_TEST, "r");
    char data[1024] = {0};
    if (f!=NULL)
    {
        // Assuming file content is within BUFFER_LENGTH
        printf("entering if at line %d\n", __LINE__);
        fread(data, 1, 1023, f);
        fclose(f);
    }
    cJSON *root = cJSON_Parse(data);

    // Verify the data
    cJSON *adapterIdJson = cJSON_GetObjectItem(root, "AdapterId");
    TEST_ASSERT_NOT_NULL(adapterIdJson);
    TEST_ASSERT_EQUAL_STRING("C0:E7:BF:07:47:1B", adapterIdJson->valuestring);

    cJSON *profilesJson = cJSON_GetObjectItem(root, "Profiles");
    TEST_ASSERT_NOT_NULL(profilesJson);
    cJSON *profileJson = cJSON_GetArrayItem(profilesJson, 1);
    TEST_ASSERT_NOT_NULL(profileJson);
    cJSON *profileIdJson = cJSON_GetObjectItem(profileJson, "ProfileId");
    TEST_ASSERT_NOT_NULL(profileIdJson);
    TEST_ASSERT_EQUAL_STRING("0x110d", profileIdJson->valuestring);
     cJSON_Delete(root);
}

void test_BTRMgr_PI_SetAllProfiles_when_ProfileHasNoDevices(void) {
    // Create persistent data with valid profile having 0 devices
    BTRMGR_PersistentData_t persistentData = {0};
    tBTRMgrPIHdl                hBTRMgrPiHdl=0x01;
    strcpy(persistentData.adapterId, "C0:E7:BF:07:47:1B");
    persistentData.numOfProfiles = 1;
    strcpy(persistentData.profileList[0].profileId, "0x110b");

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_PI_SetAllProfiles(hBTRMgrPiHdl, &persistentData));
    // We assume the JSON file is not updated with this profile as it has 0 devices.
}

/*
void test_BTRMgr_PI_SetAllProfiles_when_DeviceVolumeOutOfRange(void) {
    // Create persistent data with valid profile having 1 device where volume is out of range
    BTRMGR_PersistentData_t persistentData = {0};
    strcpy(persistentData.adapterId, "C0:E7:BF:07:47:1B");
    tBTRMgrPIHdl                hBTRMgrPiHdl=0x01;
    persistentData.numOfProfiles = 1;
    strcpy(persistentData.profileList[0].profileId, "0x110b");
    persistentData.profileList[0].numOfDevices = 1;
#ifdef RDKTV_PERSIST_VOLUME
    persistentData.profileList[0].deviceList[0].Volume = 256; // out of range
#endif

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_PI_SetAllProfiles(hBTRMgrPiHdl, &persistentData));
    // Verify the JSON file is not updated with the volume of 256. You can manually check the file or use a library to parse JSON and validate.
}

void test_BTRMgr_PI_SetAllProfiles_when_Successful(void) {

    BTRMGR_PersistentData_t persistentData = {0};
    tBTRMgrPIHdl                hBTRMgrPiHdl=0x01;// random PiHdl 
    strcpy(persistentData.adapterId, "C0:E7:BF:07:47:1B");
    persistentData.numOfProfiles = 1;
    strcpy(persistentData.profileList[0].profileId, "0x110b");
    persistentData.profileList[0].numOfDevices = 1;
    persistentData.profileList[0].deviceList[0].deviceId = 163411682490179;
    persistentData.profileList[0].deviceList[0].isConnected = 1;
#ifdef RDKTV_PERSIST_VOLUME
    persistentData.profileList[0].deviceList[0].Volume = 25; // valid volume
#endif

    TEST_ASSERT_EQUAL(eBTRMgrSuccess, BTRMgr_PI_SetAllProfiles(hBTRMgrPiHdl, &persistentData));

    // Verify the JSON file after updates to check if changes take effect.
    FILE *f = fopen(BTRMGR_PERSISTENT_DATA_PATH, "r");
    char data[BUFFER_LENGTH] = {0}; // Assuming file content is within BUFFER_LENGTH
    fread(data, 1, BUFFER_LENGTH - 1, f);
    fclose(f);

    cJSON *root = cJSON_Parse(data);

    // Verify the data
    cJSON *adapterIdJson = cJSON_GetObjectItem(root, "AdapterId");
    TEST_ASSERT_NOT_NULL(adapterIdJson);
    TEST_ASSERT_EQUAL_STRING("C0:E7:BF:07:47:1B", adapterIdJson->valuestring);

    cJSON *profilesJson = cJSON_GetObjectItem(root, "Profiles");
    TEST_ASSERT_NOT_NULL(profilesJson);

    size_t profilesCount = cJSON_GetArraySize(profilesJson);
    TEST_ASSERT_EQUAL_INT(1, profilesCount);

    cJSON *profileJson = cJSON_GetArrayItem(profilesJson, 0);
    TEST_ASSERT_NOT_NULL(profileJson);

    cJSON *profileIdJson = cJSON_GetObjectItem(profileJson, "ProfileId");
    TEST_ASSERT_NOT_NULL(profileIdJson);
    TEST_ASSERT_EQUAL_STRING("0x110b", profileIdJson->valuestring);

    // Continue with other fields

    cJSON_Delete(root);
}
*/
void test_BTRMgr_PI_RemoveProfile_when_ParametersAreNull(void) {
    BTRMGR_Profile_t profileData;
    // assuming you initialize the profile data
     tBTRMgrPIHdl                hBTRMgrPiHdl;
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_PI_RemoveProfile(NULL, &profileData));
}
void test_BTRMgr_PI_RemoveProfile_when_Parameters2AreNull(void) {
    BTRMGR_Profile_t profileData;
    // assuming you initialize the profile data
     tBTRMgrPIHdl                hBTRMgrPiHdl;
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_PI_RemoveProfile(hBTRMgrPiHdl, NULL));
}
void test_BTRMgr_PI_RemoveProfile_when_ProfileNotFound(void) {
    BTRMGR_Profile_t profileData;
    // assuming you initialize the profileData
    tBTRMgrPIHdl                hBTRMgrPiHdl;
     char* jsonName ="unitTest/support/noSuchProfileFile.json";
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_PI_RemoveProfile(hBTRMgrPiHdl, &profileData));
}

void test_BTRMgr_PI_RemoveProfile_when_JsonFileIsEmpty(void) {
    BTRMGR_Profile_t profileData;
    // assuming you initialize the profileData
    tBTRMgrPIHdl                hBTRMgrPiHdl;
     char* jsonName ="unitTest/support/JsonFileIsEmpty.json";
   char command[256];
    snprintf(command, sizeof(command), "cp %s %s", jsonName, JSON_PATH_UNIT_TEST);
        if (system(command) != 0) {
            printf("File copy failed Line 788.\n");
        }
    TEST_ASSERT_EQUAL(eBTRMgrFailure, BTRMgr_PI_RemoveProfile(hBTRMgrPiHdl, &profileData));
}
