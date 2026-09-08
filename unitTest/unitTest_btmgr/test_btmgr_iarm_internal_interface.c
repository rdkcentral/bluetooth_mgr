#ifndef UNIT_TEST
#define UNIT_TEST
#endif
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "mock_btmgr.h"
#include "mock_btrMgr_logger.h"
#include "btmgr_iarm_interface.h"
#include "btrMgr_IarmInternalIfce.h"
#include "mock_libIBus.h"
TEST_FILE("btmgr_iarm_internal_interface.c") 


extern bool gIsBTRMGR_Internal_Inited;  /* extern access to the variable if it's defined in a C file */

void test_btrMgr_GetNumberOfAdapters_WhenNotInitialized(void) {
    unsigned char number_adapters = 0;

    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetNumberOfAdapters((void*)&number_adapters));
}
void test_btrMgr_GetNumberOfAdapters_WhenArgIsNull(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetNumberOfAdapters(NULL));
}
void test_btrMgr_GetNumberOfAdapters_WhenGetNumAdaptersFails(void) {
    unsigned char number_adapters = 0;

    gIsBTRMGR_Internal_Inited = true;

    /* Configure the mock */
    BTRMGR_GetNumberOfAdapters_IgnoreAndReturn(BTRMGR_RESULT_GENERIC_FAILURE);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetNumberOfAdapters((void*)&number_adapters));
}
void test_btrMgr_GetNumberOfAdapters_Success(void) {
    unsigned char number_adapters = 0;

    gIsBTRMGR_Internal_Inited = true;

    /* Configure the mock */
   BTRMGR_GetNumberOfAdapters_ExpectAndReturn(&number_adapters, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetNumberOfAdapters((void*)&number_adapters));
}

void test_btrMgr_SetAdapterName_WhenNotInitialized(void) {
    BTRMGR_IARMAdapterName_t adapterName;

    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_SetAdapterName((void*)&adapterName));
}

void test_btrMgr_SetAdapterName_WhenArgIsNull(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_SetAdapterName(NULL));
}
void test_btrMgr_SetAdapterName_WhenSetAdapterNameFails(void) {
    BTRMGR_IARMAdapterName_t adapterName;

    gIsBTRMGR_Internal_Inited = true;

    /* Configure the mock */
    BTRMGR_SetAdapterName_IgnoreAndReturn(BTRMGR_RESULT_GENERIC_FAILURE);
    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_SetAdapterName((void*)&adapterName));
}
void test_btrMgr_SetAdapterName_Success(void) {
    BTRMGR_IARMAdapterName_t adapterName;

    gIsBTRMGR_Internal_Inited = true;

    /* Configure the mock */
    BTRMGR_SetAdapterName_ExpectAndReturn(adapterName.m_adapterIndex, adapterName.m_name, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_SetAdapterName((void*)&adapterName));
}

void test_btrMgr_GetAdapterName_WhenNotInitialized(void) {
    BTRMGR_IARMAdapterName_t adapterName;

    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetAdapterName((void*)&adapterName));
}

void test_btrMgr_GetAdapterName_WhenArgIsNull(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetAdapterName(NULL));
}

void test_btrMgr_GetAdapterName_WhenCannotGetAdapterName(void) {
    BTRMGR_IARMAdapterName_t adapterName = {0};
    strcpy(adapterName.m_name, "adapter 1");
    adapterName.m_adapterIndex = 0;

    gIsBTRMGR_Internal_Inited = true;

    /* Configure the mock */
     BTRMGR_GetAdapterName_ExpectAndReturn(adapterName.m_adapterIndex, adapterName.m_name, BTRMGR_RESULT_GENERIC_FAILURE);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetAdapterName((void*)&adapterName));
}
void test_btrMgr_GetAdapterName_Success(void) {
    BTRMGR_IARMAdapterName_t adapterName = {0};
    strcpy(adapterName.m_name, "adapter 1");
    adapterName.m_adapterIndex = 0;

    gIsBTRMGR_Internal_Inited = true;

    /* Configure the mock */
    BTRMGR_GetAdapterName_ExpectAndReturn(adapterName.m_adapterIndex, adapterName.m_name, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetAdapterName((void*)&adapterName));
}
void test_btrMgr_SetAdapterPowerStatus_WhenNotInitialized_ExpectInvalidState(void) {
    BTRMGR_IARMAdapterPower_t powerStatus;

    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_SetAdapterPowerStatus((void*)&powerStatus));
}

void test_btrMgr_SetAdapterPowerStatus_WhenArgIsNull_ExpectInvalidParam(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_SetAdapterPowerStatus(NULL));
}
void test_btrMgr_SetAdapterPowerStatus_WhenSetAdapterPowerStatusFails_ExpectIpcCoreFail(void) {
    BTRMGR_IARMAdapterPower_t powerStatus = {.m_adapterIndex = 0, .m_powerStatus = 1};

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetAdapterPowerStatus_ExpectAndReturn(powerStatus.m_adapterIndex, powerStatus.m_powerStatus, BTRMGR_RESULT_GENERIC_FAILURE);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_SetAdapterPowerStatus((void*)&powerStatus));
}

void test_btrMgr_SetAdapterPowerStatus_Success(void) {
    BTRMGR_IARMAdapterPower_t powerStatus = {.m_adapterIndex = 0, .m_powerStatus = 1};

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetAdapterPowerStatus_ExpectAndReturn(powerStatus.m_adapterIndex, powerStatus.m_powerStatus, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_SetAdapterPowerStatus((void*)&powerStatus));
}

void test_btrMgr_GetAdapterPowerStatus_WhenNotInitialized_ExpectInvalidState(void) {
    BTRMGR_IARMAdapterPower_t powerStatus;

    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetAdapterPowerStatus((void*)&powerStatus));
}

void test_btrMgr_GetAdapterPowerStatus_WhenArgIsNull_ExpectInvalidParam(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetAdapterPowerStatus(NULL));
}

void test_btrMgr_GetAdapterPowerStatus_WhenGetAdapterPowerStatusFails_ExpectIpcCoreFail(void) {
    BTRMGR_IARMAdapterPower_t powerStatus = {.m_adapterIndex = 0, .m_powerStatus = 1};

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetAdapterPowerStatus_ExpectAndReturn(powerStatus.m_adapterIndex, &powerStatus.m_powerStatus, BTRMGR_RESULT_GENERIC_FAILURE);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetAdapterPowerStatus((void*)&powerStatus));
}

void test_btrMgr_GetAdapterPowerStatus_Success(void) {
    BTRMGR_IARMAdapterPower_t powerStatus = {.m_adapterIndex = 0, .m_powerStatus = 1};

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetAdapterPowerStatus_ExpectAndReturn(powerStatus.m_adapterIndex, &powerStatus.m_powerStatus, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetAdapterPowerStatus((void*)&powerStatus));
}

void test_btrMgr_SetAdapterDiscoverable_WhenNotInitialized(void) {
    BTRMGR_IARMAdapterDiscoverable_t discoverable;

    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_SetAdapterDiscoverable((void*)&discoverable));
}

void test_btrMgr_SetAdapterDiscoverable_WhenArgIsNull(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_SetAdapterDiscoverable(NULL));
}

void test_btrMgr_SetAdapterDiscoverable_WhenSetAdapterDiscoverableFails(void) {
    BTRMGR_IARMAdapterDiscoverable_t discoverable = {.m_adapterIndex = 0, .m_isDiscoverable = 1, .m_timeout = 10};

    gIsBTRMGR_Internal_Inited = true;

    /* Configure the mock */
    BTRMGR_SetAdapterDiscoverable_ExpectAndReturn(discoverable.m_adapterIndex, discoverable.m_isDiscoverable, discoverable.m_timeout, BTRMGR_RESULT_GENERIC_FAILURE);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_SetAdapterDiscoverable((void*)&discoverable));
}

void test_btrMgr_SetAdapterDiscoverable_Success(void) {
    BTRMGR_IARMAdapterDiscoverable_t discoverable = {.m_adapterIndex = 0, .m_isDiscoverable = 1, .m_timeout = 10};

    gIsBTRMGR_Internal_Inited = true;

    /* Configure the mock */
    BTRMGR_SetAdapterDiscoverable_ExpectAndReturn(discoverable.m_adapterIndex, discoverable.m_isDiscoverable, discoverable.m_timeout, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_SetAdapterDiscoverable((void*)&discoverable));
}


void test_btrMgr_ChangeDeviceDiscoveryStatus_WhenNotInitialized(void) {
    BTRMGR_IARMAdapterDiscover_t adapterDiscover;

    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_ChangeDeviceDiscoveryStatus((void*)&adapterDiscover));
}

void test_btrMgr_ChangeDeviceDiscoveryStatus_WhenArgIsNull(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_ChangeDeviceDiscoveryStatus(NULL));
}

void test_btrMgr_ChangeDeviceDiscoveryStatus_WhenChangeDiscoveryStatusFails(void) {
    BTRMGR_IARMAdapterDiscover_t adapterDiscover = {.m_adapterIndex = 0, .m_setDiscovery = 1, .m_enBTRMgrDevOpT = BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT};

    gIsBTRMGR_Internal_Inited = true;

    /* Configure the mock */
    BTRMGR_StartDeviceDiscovery_ExpectAndReturn(adapterDiscover.m_adapterIndex, adapterDiscover.m_enBTRMgrDevOpT, BTRMGR_RESULT_GENERIC_FAILURE);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_ChangeDeviceDiscoveryStatus((void*)&adapterDiscover));
}

void test_btrMgr_ChangeDeviceDiscoveryStatus_Success(void) {
    BTRMGR_IARMAdapterDiscover_t adapterDiscover = {.m_adapterIndex = 0, .m_setDiscovery = 1, .m_enBTRMgrDevOpT = BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT};

    gIsBTRMGR_Internal_Inited = true;

    /* Configure the mock */
    BTRMGR_StartDeviceDiscovery_ExpectAndReturn(adapterDiscover.m_adapterIndex, adapterDiscover.m_enBTRMgrDevOpT, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_ChangeDeviceDiscoveryStatus((void*)&adapterDiscover));
}

void test_btrMgr_GetDeviceDiscoveryStatus_WhenNotInitialized(void) {
    BTRMGR_IARMDiscoveryStatus_t discoveryStatus;

    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetDeviceDiscoveryStatus((void*)&discoveryStatus));
}

void test_btrMgr_GetDeviceDiscoveryStatus_WhenArgIsNull(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetDeviceDiscoveryStatus(NULL));
}

void test_btrMgr_GetDeviceDiscoveryStatus_WhenGetDiscoveryStatusFails(void) {
    BTRMGR_IARMDiscoveryStatus_t discoveryStatus = {.m_adapterIndex = 0};

    gIsBTRMGR_Internal_Inited = true;

    /* Configure the mock */
    BTRMGR_GetDiscoveryStatus_ExpectAndReturn(discoveryStatus.m_adapterIndex, &discoveryStatus.m_discoveryInProgress, &discoveryStatus.m_discoveryType, BTRMGR_RESULT_GENERIC_FAILURE);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetDeviceDiscoveryStatus((void*)&discoveryStatus));
}

void test_btrMgr_GetDeviceDiscoveryStatus_Success(void) {
    BTRMGR_IARMDiscoveryStatus_t discoveryStatus = {.m_adapterIndex = 0};

    gIsBTRMGR_Internal_Inited = true;

    /* Configure the mock */
    BTRMGR_GetDiscoveryStatus_ExpectAndReturn(discoveryStatus.m_adapterIndex, &discoveryStatus.m_discoveryInProgress, &discoveryStatus.m_discoveryType, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetDeviceDiscoveryStatus((void*)&discoveryStatus));
}

void test_btrMgr_GetDiscoveredDevices_not_inited(void)
{
    gIsBTRMGR_Internal_Inited = false;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetDiscoveredDevices(NULL));
}

void test_btrMgr_GetDiscoveredDevices_null_param(void)
{
    gIsBTRMGR_Internal_Inited = true;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetDiscoveredDevices(NULL));
}

void test_btrMgr_GetDiscoveredDevices_discovery_success(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMDiscoveredDevices_t devices;
    BTRMGR_GetDiscoveredDevices_ExpectAndReturn(devices.m_adapterIndex, &devices.m_devices, BTRMGR_RESULT_SUCCESS);
    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetDiscoveredDevices(&devices));
}

void test_btrMgr_GetDiscoveredDevices_discovery_failure(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMDiscoveredDevices_t devices;
    BTRMGR_GetDiscoveredDevices_ExpectAndReturn(devices.m_adapterIndex, &devices.m_devices, BTRMGR_RESULT_GENERIC_FAILURE);
    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetDiscoveredDevices(&devices));
}

void test_btrMgr_PairDevice_not_inited(void)
{
    gIsBTRMGR_Internal_Inited = false;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_PairDevice(NULL));
}

void test_btrMgr_PairDevice_null_param(void)
{
    gIsBTRMGR_Internal_Inited = true;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_PairDevice(NULL));
}

void test_btrMgr_PairDevice_pairing_success(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMPairDevice_t device;

    BTRMGR_PairDevice_ExpectAndReturn(device.m_adapterIndex, device.m_deviceHandle, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_PairDevice(&device));
}

void test_btrMgr_PairDevice_pairing_failure(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMPairDevice_t device;

    BTRMGR_PairDevice_ExpectAndReturn(device.m_adapterIndex, device.m_deviceHandle, BTRMGR_RESULT_GENERIC_FAILURE);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_PairDevice(&device)); // function currently does not fail on pairing failure, it always returns success
}

void test_btrMgr_UnpairDevice_not_inited(void)
{
    gIsBTRMGR_Internal_Inited = false;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_UnpairDevice(NULL));
}

void test_btrMgr_UnpairDevice_null_param(void)
{
    gIsBTRMGR_Internal_Inited = true;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_UnpairDevice(NULL));
}

void test_btrMgr_UnpairDevice_unpairing_success(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMPairDevice_t device;

    BTRMGR_UnpairDevice_ExpectAndReturn(device.m_adapterIndex, device.m_deviceHandle, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_UnpairDevice(&device));
}

void test_btrMgr_UnpairDevice_unpairing_failure(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMPairDevice_t device;

    BTRMGR_UnpairDevice_ExpectAndReturn(device.m_adapterIndex, device.m_deviceHandle, BTRMGR_RESULT_GENERIC_FAILURE);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_UnpairDevice(&device)); // function currently does not fail on unpairing failure, it always returns success
}

void test_btrMgr_GetPairedDevices_not_inited(void)
{
    gIsBTRMGR_Internal_Inited = false;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetPairedDevices(NULL));
}

void test_btrMgr_GetPairedDevices_null_param(void)
{
    gIsBTRMGR_Internal_Inited = true;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetPairedDevices(NULL));
}

void test_btrMgr_GetPairedDevices_success(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMPairedDevices_t devices;
    
    BTRMGR_GetPairedDevices_ExpectAndReturn(devices.m_adapterIndex, &devices.m_devices, BTRMGR_RESULT_SUCCESS);
    
    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetPairedDevices(&devices));
}

void test_btrMgr_GetPairedDevices_failure(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMPairedDevices_t devices;
    
    BTRMGR_GetPairedDevices_ExpectAndReturn(devices.m_adapterIndex, &devices.m_devices, BTRMGR_RESULT_GENERIC_FAILURE);
    
    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetPairedDevices(&devices));
}

void test_btrMgr_ConnectToDevice_not_inited(void)
{
    gIsBTRMGR_Internal_Inited = false;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_ConnectToDevice(NULL));
}

void test_btrMgr_ConnectToDevice_null_param(void)
{
    gIsBTRMGR_Internal_Inited = true;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_ConnectToDevice(NULL));
}

void test_btrMgr_ConnectToDevice_success(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMConnectDevice_t device;
    
    BTRMGR_ConnectToDevice_ExpectAndReturn(device.m_adapterIndex, device.m_deviceHandle, device.m_connectAs, BTRMGR_RESULT_SUCCESS);
    
    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_ConnectToDevice(&device));
}

void test_btrMgr_ConnectToDevice_failure(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMConnectDevice_t device;
    
    BTRMGR_ConnectToDevice_ExpectAndReturn(device.m_adapterIndex, device.m_deviceHandle, device.m_connectAs, BTRMGR_RESULT_GENERIC_FAILURE);
    
    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_ConnectToDevice(&device));
}

void test_btrMgr_DisconnectFromDevice_not_inited(void)
{
    gIsBTRMGR_Internal_Inited = false;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_DisconnectFromDevice(NULL));
}

void test_btrMgr_DisconnectFromDevice_null_param(void)
{
    gIsBTRMGR_Internal_Inited = true;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_DisconnectFromDevice(NULL));
}

void test_btrMgr_DisconnectFromDevice_success(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMConnectDevice_t device;

    BTRMGR_DisconnectFromDevice_ExpectAndReturn(device.m_adapterIndex, device.m_deviceHandle, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_DisconnectFromDevice(&device));
}

void test_btrMgr_DisconnectFromDevice_failure(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMConnectDevice_t device;

    BTRMGR_DisconnectFromDevice_ExpectAndReturn(device.m_adapterIndex, device.m_deviceHandle, BTRMGR_RESULT_GENERIC_FAILURE);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_DisconnectFromDevice(&device));
}

void test_btrMgr_GetConnectedDevices_not_inited(void)
{
    gIsBTRMGR_Internal_Inited = false;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetConnectedDevices(NULL));
}

void test_btrMgr_GetConnectedDevices_null_param(void)
{
    gIsBTRMGR_Internal_Inited = true;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetConnectedDevices(NULL));
}

void test_btrMgr_GetConnectedDevices_success(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMConnectedDevices_t devices;

    BTRMGR_GetConnectedDevices_ExpectAndReturn(devices.m_adapterIndex, &devices.m_devices, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetConnectedDevices(&devices));
}

void test_btrMgr_GetConnectedDevices_failure(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMConnectedDevices_t devices;

    BTRMGR_GetConnectedDevices_ExpectAndReturn(devices.m_adapterIndex, &devices.m_devices, BTRMGR_RESULT_GENERIC_FAILURE);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetConnectedDevices(&devices));
}

void test_btrMgr_GetDeviceProperties_not_inited(void)
{
    gIsBTRMGR_Internal_Inited = false;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetDeviceProperties(NULL));
}

void test_btrMgr_GetDeviceProperties_null_param(void)
{
    gIsBTRMGR_Internal_Inited = true;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetDeviceProperties(NULL));
}

void test_btrMgr_GetDeviceProperties_success(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMDDeviceProperty_t deviceProperty;

    BTRMGR_GetDeviceProperties_ExpectAndReturn(deviceProperty.m_adapterIndex, deviceProperty.m_deviceHandle, &deviceProperty.m_deviceProperty, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetDeviceProperties(&deviceProperty));
}

void test_btrMgr_GetDeviceProperties_failure(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMDDeviceProperty_t deviceProperty;

    BTRMGR_GetDeviceProperties_ExpectAndReturn(deviceProperty.m_adapterIndex, deviceProperty.m_deviceHandle, &deviceProperty.m_deviceProperty, BTRMGR_RESULT_GENERIC_FAILURE);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetDeviceProperties(&deviceProperty));
}

void test_btrMgr_StartAudioStreamingOut_not_inited(void)
{
    gIsBTRMGR_Internal_Inited = false;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_StartAudioStreamingOut(NULL));
}


void test_btrMgr_StartAudioStreamingOut_null_param(void)
{
    gIsBTRMGR_Internal_Inited = true;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_StartAudioStreamingOut(NULL));
}

void test_btrMgr_StartAudioStreamingOut_success(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMStreaming_t startStream;

    BTRMGR_StartAudioStreamingOut_ExpectAndReturn(startStream.m_adapterIndex, startStream.m_deviceHandle, startStream.m_audioPref, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_StartAudioStreamingOut(&startStream));
}

void test_btrMgr_StartAudioStreamingOut_failure(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMStreaming_t startStream;

    BTRMGR_StartAudioStreamingOut_ExpectAndReturn(startStream.m_adapterIndex, startStream.m_deviceHandle, startStream.m_audioPref, BTRMGR_RESULT_GENERIC_FAILURE);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_StartAudioStreamingOut(&startStream));
}

void test_btrMgr_StopAudioStreamingOut_not_inited(void)
{
    gIsBTRMGR_Internal_Inited = false;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_StopAudioStreamingOut(NULL));
}

void test_btrMgr_StopAudioStreamingOut_null_param(void)
{
    gIsBTRMGR_Internal_Inited = true;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_StopAudioStreamingOut(NULL));
}

void test_btrMgr_StopAudioStreamingOut_success(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMStreaming_t stopStream;

    BTRMGR_StopAudioStreamingOut_ExpectAndReturn(stopStream.m_adapterIndex, stopStream.m_deviceHandle, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_StopAudioStreamingOut(&stopStream));
}

void test_btrMgr_StopAudioStreamingOut_failure(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMStreaming_t stopStream;

    BTRMGR_StopAudioStreamingOut_ExpectAndReturn(stopStream.m_adapterIndex, stopStream.m_deviceHandle, BTRMGR_RESULT_GENERIC_FAILURE);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_StopAudioStreamingOut(&stopStream));
}


void test_btrMgr_IsAudioStreamingOut_not_inited(void)
{
    gIsBTRMGR_Internal_Inited = false;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_IsAudioStreamingOut(NULL));
}

void test_btrMgr_IsAudioStreamingOut_null_param(void)
{
    gIsBTRMGR_Internal_Inited = true;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_IsAudioStreamingOut(NULL));
}

void test_btrMgr_IsAudioStreamingOut_success(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMStreamingStatus_t streamStatus;

    BTRMGR_IsAudioStreamingOut_ExpectAndReturn(streamStatus.m_adapterIndex, &streamStatus.m_streamingStatus, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_IsAudioStreamingOut(&streamStatus));
}

void test_btrMgr_IsAudioStreamingOut_failure(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMStreamingStatus_t streamStatus;

    BTRMGR_IsAudioStreamingOut_ExpectAndReturn(streamStatus.m_adapterIndex, &streamStatus.m_streamingStatus, BTRMGR_RESULT_GENERIC_FAILURE);
    
    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_IsAudioStreamingOut(&streamStatus));
}

void test_btrMgr_SetAudioStreamOutType_not_inited(void)
{
    gIsBTRMGR_Internal_Inited = false;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_SetAudioStreamOutType(NULL));
}

void test_btrMgr_SetAudioStreamOutType_null_param(void)
{
    gIsBTRMGR_Internal_Inited = true;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_SetAudioStreamOutType(NULL));
}

void test_btrMgr_SetAudioStreamOutType_success(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMStreamingType_t streamOutType;

    BTRMGR_SetAudioStreamingOutType_ExpectAndReturn(streamOutType.m_adapterIndex, streamOutType.m_audioOutType, BTRMGR_RESULT_SUCCESS);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_SetAudioStreamOutType(&streamOutType));
}

void test_btrMgr_SetAudioStreamOutType_failure(void)
{
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IARMStreamingType_t streamOutType;

    BTRMGR_SetAudioStreamingOutType_ExpectAndReturn(streamOutType.m_adapterIndex, streamOutType.m_audioOutType, BTRMGR_RESULT_GENERIC_FAILURE);
    
    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_SetAudioStreamOutType(&streamOutType));
}
extern bool gIsBTRMGR_Internal_Inited;


void test_btrMgr_StartAudioStreamingIn_not_inited(void) {
    BTRMGR_IARMStreaming_t startStream;

    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_StartAudioStreamingIn(&startStream));
}

void test_btrMgr_StartAudioStreamingIn_failure(void) {
    BTRMGR_IARMStreaming_t startStream;
    BTRMGR_Result_t mockBtmStartResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_StartAudioStreamingIn_IgnoreAndReturn(mockBtmStartResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_StartAudioStreamingIn(&startStream));
}

void test_btrMgr_StartAudioStreamingIn_success(void) {
    BTRMGR_IARMStreaming_t startStream;
    BTRMGR_Result_t mockBtmStartResult = BTRMGR_RESULT_SUCCESS;
    startStream.m_audioPref= BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_StartAudioStreamingIn_ExpectAndReturn(startStream.m_adapterIndex, startStream.m_deviceHandle, startStream.m_audioPref, mockBtmStartResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_StartAudioStreamingIn(&startStream));
}


void test_btrMgr_StopAudioStreamingIn_not_inited(void) {
    BTRMGR_IARMStreaming_t stopStream;

    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_StopAudioStreamingIn(&stopStream));
}

void test_btrMgr_StopAudioStreamingIn_failure(void) {
    BTRMGR_IARMStreaming_t stopStream;
    BTRMGR_Result_t mockBtmStopResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_StopAudioStreamingIn_IgnoreAndReturn(mockBtmStopResult);
    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_StopAudioStreamingIn(&stopStream));
}

void test_btrMgr_StopAudioStreamingIn_success(void) {
    BTRMGR_IARMStreaming_t stopStream;
    stopStream.m_audioPref=BTRMGR_DEVICE_OP_TYPE_AUDIO_INPUT;
    BTRMGR_Result_t mockBtmStopResult = BTRMGR_RESULT_SUCCESS;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_StopAudioStreamingIn_ExpectAndReturn(stopStream.m_adapterIndex, stopStream.m_deviceHandle, mockBtmStopResult);
    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_StopAudioStreamingIn(&stopStream));
}

void test_btrMgr_isAudioStreamingIn_not_inited(void) {
    BTRMGR_IARMStreamingStatus_t streamStatus;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_IsAudioStreamingIn(&streamStatus));
}

void test_btrMgr_isAudioStreamingIn_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_IsAudioStreamingIn(NULL));
}

void test_btrMgr_isAudioStreamingIn_failure(void) {
    BTRMGR_IARMStreamingStatus_t streamStatus;
    BTRMGR_Result_t mockBtmIsStreamingResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IsAudioStreamingIn_IgnoreAndReturn(mockBtmIsStreamingResult);
    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_IsAudioStreamingIn(&streamStatus));
}

void test_btrMgr_isAudioStreamingIn_success(void) {
    BTRMGR_IARMStreamingStatus_t streamStatus;
    BTRMGR_Result_t mockBtmIsStreamingResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IsAudioStreamingIn_ExpectAndReturn(streamStatus.m_adapterIndex, &streamStatus.m_streamingStatus, mockBtmIsStreamingResult);
 //Test Failure
    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_IsAudioStreamingIn(&streamStatus));
}

void test_btrMgr_setEventResponse_not_inited(void) {
    BTRMGR_IARMEventResp_t iArmEvtResp;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_SetEventResponse(&iArmEvtResp));
}

void test_btrMgr_setEventResponse_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_SetEventResponse(NULL));
}

void test_btrMgr_setEventResponse_failure(void) {
    BTRMGR_IARMEventResp_t iArmEvtResp;
    BTRMGR_Result_t mockBtmSetEvtRespResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetEventResponse_IgnoreAndReturn(mockBtmSetEvtRespResult);
    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_SetEventResponse(&iArmEvtResp));
}

void test_btrMgr_setEventResponse_success(void) {
    BTRMGR_IARMEventResp_t iArmEvtResp;
    iArmEvtResp.m_stBTRMgrEvtRsp.m_eventType= BTRMGR_EVENT_DEVICE_DISCOVERY_UPDATE;
    BTRMGR_Result_t mockBtmSetEvtRespResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetEventResponse_ExpectAndReturn(iArmEvtResp.m_adapterIndex, &iArmEvtResp.m_stBTRMgrEvtRsp, mockBtmSetEvtRespResult);
 //Test Failure
    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_SetEventResponse(&iArmEvtResp));
}
void test_btrMgr_mediaControl_not_inited(void) {
    BTRMGR_IARMMediaProperty_t mediaControl;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_MediaControl(&mediaControl));
}

void test_btrMgr_mediaControl_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_MediaControl(NULL));
}

void test_btrMgr_mediaControl_failure(void) {
    BTRMGR_IARMMediaProperty_t mediaControl;
    BTRMGR_Result_t mockBtmMediaControlResult = BTRMGR_RESULT_GENERIC_FAILURE;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_MediaControl_IgnoreAndReturn( mockBtmMediaControlResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_MediaControl(&mediaControl));
}

void test_btrMgr_mediaControl_success(void) {
    BTRMGR_IARMMediaProperty_t mediaControl;
    mediaControl.m_mediaControlCmd=BTRMGR_MEDIA_CTRL_PLAY;
    BTRMGR_Result_t mockBtmMediaControlResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_MediaControl_ExpectAndReturn(mediaControl.m_adapterIndex, mediaControl.m_deviceHandle, mediaControl.m_mediaControlCmd, mockBtmMediaControlResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_MediaControl(&mediaControl));
}

void test_btrMgr_getMediaCurrentPosition_not_inited(void) {
    BTRMGR_IARMMediaProperty_t mediaPos;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetMediaCurrentPosition(&mediaPos));
}

void test_btrMgr_getMediaCurrentPosition_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetMediaCurrentPosition(NULL));
}

void test_btrMgr_getMediaCurrentPosition_failure(void) {
    BTRMGR_IARMMediaProperty_t mediaPos;
    BTRMGR_Result_t mockBtmMediaPosResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetMediaCurrentPosition_IgnoreAndReturn(mockBtmMediaPosResult);
    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetMediaCurrentPosition(&mediaPos));
}

void test_btrMgr_getMediaCurrentPosition_success(void) {
    BTRMGR_IARMMediaProperty_t mediaPos;
    BTRMGR_Result_t mockBtmMediaPosResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetMediaCurrentPosition_ExpectAndReturn(mediaPos.m_adapterIndex, mediaPos.m_deviceHandle, &mediaPos.m_mediaPositionInfo, mockBtmMediaPosResult);
    //Test Failure 8
    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetMediaCurrentPosition(&mediaPos));
}

void test_btrMgr_getDeviceVolumeMute_not_inited(void) {
    BTRMGR_IARMDeviceVolumeMute_t deviceVolumeMute;
    gIsBTRMGR_Internal_Inited = false;
    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetDeviceVolumeMute(&deviceVolumeMute));
}

void test_btrMgr_getDeviceVolumeMute_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetDeviceVolumeMute(NULL));
}

void test_btrMgr_getDeviceVolumeMute_failure(void) {
    BTRMGR_IARMDeviceVolumeMute_t deviceVolumeMute;
    BTRMGR_Result_t mockBtmGetDeviceVolumeMuteResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetDeviceVolumeMute_IgnoreAndReturn(mockBtmGetDeviceVolumeMuteResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetDeviceVolumeMute(&deviceVolumeMute));
}

void test_btrMgr_getDeviceVolumeMute_success(void) {
    BTRMGR_IARMDeviceVolumeMute_t deviceVolumeMute;
    BTRMGR_Result_t mockBtmGetDeviceVolumeMuteResult = BTRMGR_RESULT_SUCCESS;
    deviceVolumeMute.m_deviceOpType= BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT ;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetDeviceVolumeMute_ExpectAndReturn(deviceVolumeMute.m_adapterIndex, deviceVolumeMute.m_deviceHandle, deviceVolumeMute.m_deviceOpType, &deviceVolumeMute.m_volume, &deviceVolumeMute.m_mute, mockBtmGetDeviceVolumeMuteResult);
    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetDeviceVolumeMute(&deviceVolumeMute)); 
}

void test_btrMgr_setDeviceVolumeMute_not_inited(void) {
    BTRMGR_IARMDeviceVolumeMute_t deviceVolumeMute;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_SetDeviceVolumeMute(&deviceVolumeMute));
}
void test_btrMgr_setDeviceVolumeMute_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_SetDeviceVolumeMute(NULL));
}

void test_btrMgr_setDeviceVolumeMute_failure(void) {
    BTRMGR_IARMDeviceVolumeMute_t deviceVolumeMute;
    BTRMGR_Result_t mockBtmSetDeviceVolumeMuteResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetDeviceVolumeMute_IgnoreAndReturn(mockBtmSetDeviceVolumeMuteResult);
    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_SetDeviceVolumeMute(&deviceVolumeMute));
}


void test_btrMgr_setDeviceVolumeMute_success(void) {
    BTRMGR_IARMDeviceVolumeMute_t deviceVolumeMute;
    BTRMGR_Result_t mockBtmSetDeviceVolumeMuteResult = BTRMGR_RESULT_SUCCESS;
    deviceVolumeMute.m_deviceOpType = BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT; 
    deviceVolumeMute.m_volume =50;
    deviceVolumeMute.m_mute=true;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetDeviceVolumeMute_ExpectAndReturn(deviceVolumeMute.m_adapterIndex, deviceVolumeMute.m_deviceHandle, deviceVolumeMute.m_deviceOpType , deviceVolumeMute.m_volume, deviceVolumeMute.m_mute, mockBtmSetDeviceVolumeMuteResult);
//Test Case Failure
    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_SetDeviceVolumeMute(&deviceVolumeMute));
}


void test_btrMgr_setDeviceDelay_not_inited(void) {
    BTRMGR_IARMDeviceDelay_t deviceDelay;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_SetDeviceDelay(&deviceDelay));
}

void test_btrMgr_setDeviceDelay_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_SetDeviceDelay(NULL));
}

void test_btrMgr_setDeviceDelay_failure(void) {
    BTRMGR_IARMDeviceDelay_t deviceDelay;
    BTRMGR_Result_t mockBtmSetDeviceDelayResult = BTRMGR_RESULT_GENERIC_FAILURE;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetDeviceDelay_IgnoreAndReturn(mockBtmSetDeviceDelayResult);
    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_SetDeviceDelay(&deviceDelay));
}

void test_btrMgr_setDeviceDelay_success(void) {
    BTRMGR_IARMDeviceDelay_t deviceDelay;
    BTRMGR_Result_t mockBtmSetDeviceDelayResult = BTRMGR_RESULT_SUCCESS;
    deviceDelay.m_delay=50;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetDeviceDelay_ExpectAndReturn(deviceDelay.m_adapterIndex, deviceDelay.m_deviceHandle, deviceDelay.m_deviceOpType, deviceDelay.m_delay, mockBtmSetDeviceDelayResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_SetDeviceDelay(&deviceDelay));
}


void test_btrMgr_getDeviceDelay_not_inited(void) {
    BTRMGR_IARMDeviceDelay_t deviceDelay;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetDeviceDelay(&deviceDelay));
}

void test_btrMgr_getDeviceDelay_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetDeviceDelay(NULL));
}

void test_btrMgr_getDeviceDelay_failure(void) {
    BTRMGR_IARMDeviceDelay_t deviceDelay;
    BTRMGR_Result_t mockBtmGetDeviceDelayResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetDeviceDelay_IgnoreAndReturn(mockBtmGetDeviceDelayResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetDeviceDelay(&deviceDelay));
}

void test_btrMgr_getDeviceDelay_success(void) {
    BTRMGR_IARMDeviceDelay_t deviceDelay;
    BTRMGR_Result_t mockBtmGetDeviceDelayResult = BTRMGR_RESULT_SUCCESS;
    deviceDelay.m_deviceOpType = BTRMGR_DEVICE_OP_TYPE_AUDIO_OUTPUT;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetDeviceDelay_ExpectAndReturn(deviceDelay.m_adapterIndex, deviceDelay.m_deviceHandle, deviceDelay.m_deviceOpType, &deviceDelay.m_delay, &deviceDelay.m_msinqueue , mockBtmGetDeviceDelayResult);
//TEST CASE FAILURE
    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetDeviceDelay(&deviceDelay));
}

void test_btrMgr_getMediaTrackInfo_not_inited(void) {
    BTRMGR_IARMMediaProperty_t mediaTrackInfo;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetMediaTrackInfo(&mediaTrackInfo));
}

void test_btrMgr_getMediaTrackInfo_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetMediaTrackInfo(NULL));
}

void test_btrMgr_getMediaTrackInfo_failure(void) {
    BTRMGR_IARMMediaProperty_t mediaTrackInfo;
    BTRMGR_Result_t mockBtmGetMediaTrackInfoResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetMediaTrackInfo_IgnoreAndReturn(mockBtmGetMediaTrackInfoResult);
  
    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetMediaTrackInfo(&mediaTrackInfo));
}

void test_btrMgr_getMediaTrackInfo_success(void) {
    BTRMGR_IARMMediaProperty_t mediaTrackInfo;
    BTRMGR_Result_t mockBtmGetMediaTrackInfoResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetMediaTrackInfo_ExpectAndReturn(mediaTrackInfo.m_adapterIndex, mediaTrackInfo.m_deviceHandle, &mediaTrackInfo.m_mediaTrackInfo, mockBtmGetMediaTrackInfoResult);
    //Test case Failure
    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetMediaTrackInfo(&mediaTrackInfo));
}

void test_btrMgr_getMediaElementTrackInfo_not_inited(void) {
    BTRMGR_IARMMediaProperty_t mediaElementTrackInfo;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetMediaElementTrackInfo(&mediaElementTrackInfo));
}
void test_btrMgr_getMediaElementTrackInfo_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetMediaElementTrackInfo(NULL));
}
void test_btrMgr_getMediaElementTrackInfo_failure(void) {
    BTRMGR_IARMMediaProperty_t mediaElementTrackInfo;
    BTRMGR_Result_t mockBtmGetMediaElementTrackInfoResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetMediaElementTrackInfo_IgnoreAndReturn(mockBtmGetMediaElementTrackInfoResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetMediaElementTrackInfo(&mediaElementTrackInfo));
}

void test_btrMgr_getMediaElementTrackInfo_success(void) {
    BTRMGR_IARMMediaProperty_t mediaElementTrackInfo;
    BTRMGR_Result_t mockBtmGetMediaElementTrackInfoResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetMediaElementTrackInfo_ExpectAndReturn(mediaElementTrackInfo.m_adapterIndex, mediaElementTrackInfo.m_deviceHandle, mediaElementTrackInfo.m_mediaElementHandle, &mediaElementTrackInfo.m_mediaTrackInfo, mockBtmGetMediaElementTrackInfoResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetMediaElementTrackInfo(&mediaElementTrackInfo));
}
void test_btrMgr_setMediaElementActive_not_inited(void) {
    BTRMGR_IARMMediaElementListInfo_t mediaElementList;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_SetMediaElementActive(&mediaElementList));
}

void test_btrMgr_setMediaElementActive_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_SetMediaElementActive(NULL));
}

void test_btrMgr_setMediaElementActive_failure(void) {
    BTRMGR_IARMMediaElementListInfo_t mediaElementList;
    BTRMGR_Result_t mockBtmSetMediaElementActiveResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetMediaElementActive_IgnoreAndReturn(mockBtmSetMediaElementActiveResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_SetMediaElementActive(&mediaElementList));
}

void test_btrMgr_setMediaElementActive_success(void) {
    BTRMGR_IARMMediaElementListInfo_t mediaElementList;
    BTRMGR_Result_t mockBtmSetMediaElementActiveResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetMediaElementActive_ExpectAndReturn(mediaElementList.m_adapterIndex, mediaElementList.m_deviceHandle, mediaElementList.m_mediaElementHandle, mediaElementList.m_mediaElementType, mockBtmSetMediaElementActiveResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_SetMediaElementActive(&mediaElementList));
}
void test_btrMgr_getMediaElementList_not_inited(void) {
    BTRMGR_IARMMediaElementListInfo_t mediaElementList;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetMediaElementList(&mediaElementList));
}

void test_btrMgr_getMediaElementList_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetMediaElementList(NULL));
}

void test_btrMgr_getMediaElementList_failure(void) {
    BTRMGR_IARMMediaElementListInfo_t mediaElementList;
    BTRMGR_Result_t mockBtmGetMediaElementListResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetMediaElementList_IgnoreAndReturn(mockBtmGetMediaElementListResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetMediaElementList(&mediaElementList));
}

void test_btrMgr_getMediaElementList_success(void) {
    BTRMGR_IARMMediaElementListInfo_t mediaElementList;
    BTRMGR_Result_t mockBtmGetMediaElementListResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetMediaElementList_ExpectAndReturn(mediaElementList.m_adapterIndex, mediaElementList.m_deviceHandle, mediaElementList.m_mediaElementHandle, mediaElementList.m_mediaElementStartIdx, mediaElementList.m_mediaElementEndIdx, mediaElementList.m_mediaElementListDepth, mediaElementList.m_mediaElementType, &mediaElementList.m_mediaTrackListInfo, mockBtmGetMediaElementListResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetMediaElementList(&mediaElementList));
}
void test_btrMgr_selectMediaElement_not_inited(void) {
    BTRMGR_IARMMediaElementListInfo_t mediaElementList;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_SelectMediaElement(&mediaElementList));
}

void test_btrMgr_selectMediaElement_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_SelectMediaElement(NULL));
}

void test_btrMgr_selectMediaElement_failure(void) {
    BTRMGR_IARMMediaElementListInfo_t mediaElementList;
    BTRMGR_Result_t mockBtmSelectMediaElementResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SelectMediaElement_IgnoreAndReturn(mockBtmSelectMediaElementResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_SelectMediaElement(&mediaElementList));
}

void test_btrMgr_selectMediaElement_success(void) {
    BTRMGR_IARMMediaElementListInfo_t mediaElementList;
    BTRMGR_Result_t mockBtmSelectMediaElementResult = BTRMGR_RESULT_SUCCESS;
    mediaElementList.m_mediaElementType= BTRMGR_MEDIA_ELEMENT_TYPE_UNKNOWN;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SelectMediaElement_ExpectAndReturn(mediaElementList.m_adapterIndex, mediaElementList.m_deviceHandle, mediaElementList.m_mediaElementHandle, mediaElementList.m_mediaElementType, mockBtmSelectMediaElementResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_SelectMediaElement(&mediaElementList));
}

void test_btrMgr_getLeProperty_not_inited(void) {
    BTRMGR_IARMLeProperty_t leProperty;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetLeProperty(&leProperty));
}

void test_btrMgr_getLeProperty_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetLeProperty(NULL));
}

void test_btrMgr_getLeProperty_failure(void) {
    BTRMGR_IARMLeProperty_t leProperty;
    leProperty.m_enLeProperty = BTRMGR_LE_PROP_UUID;
    BTRMGR_Result_t mockBtmGetLePropertyResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetLeProperty_IgnoreAndReturn(mockBtmGetLePropertyResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetLeProperty(&leProperty));
}

void test_btrMgr_getLeProperty_success(void) {
    BTRMGR_IARMLeProperty_t leProperty;
    BTRMGR_Result_t mockBtmGetLePropertyResult = BTRMGR_RESULT_SUCCESS;
    leProperty.m_enLeProperty=BTRMGR_LE_PROP_UUID;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetLeProperty_ExpectAndReturn(leProperty.m_adapterIndex, leProperty.m_deviceHandle, leProperty.m_propUuid, leProperty.m_enLeProperty , leProperty.m_value, mockBtmGetLePropertyResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetLeProperty(&leProperty));
}

void test_btrMgr_performLeOp_not_inited(void) {
    BTRMGR_IARMLeOp_t leOp;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_PerformLeOp(&leOp));
}

void test_btrMgr_performLeOp_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_PerformLeOp(NULL));
}

void test_btrMgr_performLeOp_failure(void) {
    BTRMGR_IARMLeOp_t leOp;
    BTRMGR_Result_t mockBtmPerformLeOpResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_PerformLeOp_IgnoreAndReturn(mockBtmPerformLeOpResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_PerformLeOp(&leOp));
}

void test_btrMgr_performLeOp_success(void) {
    BTRMGR_IARMLeOp_t leOp;
    leOp.m_leOpType=BTRMGR_LE_OP_WRITE_VALUE;
    BTRMGR_Result_t mockBtmPerformLeOpResult = BTRMGR_RESULT_SUCCESS;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_PerformLeOp_ExpectAndReturn(leOp.m_adapterIndex, leOp.m_deviceHandle, leOp.m_uuid, leOp.m_leOpType , leOp.m_opArg,leOp.m_opRes, mockBtmPerformLeOpResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_PerformLeOp(&leOp));
}

void test_btrMgr_getLimitBeaconDetection_not_inited(void) {
    BTRMGR_IARMBeaconDetection_t beaconDetection;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_GetLimitBeaconDetection(&beaconDetection));
}

void test_btrMgr_getLimitBeaconDetection_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_GetLimitBeaconDetection(NULL));
}
void test_btrMgr_getLimitBeaconDetection_failure(void) {
    BTRMGR_IARMBeaconDetection_t beaconDetection;
    BTRMGR_Result_t mockBtmGetLimitBeaconDetectionResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetLimitBeaconDetection_IgnoreAndReturn(mockBtmGetLimitBeaconDetectionResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_GetLimitBeaconDetection(&beaconDetection));
}

void test_btrMgr_getLimitBeaconDetection_success(void) {
    BTRMGR_IARMBeaconDetection_t beaconDetection;
    BTRMGR_Result_t mockBtmGetLimitBeaconDetectionResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_GetLimitBeaconDetection_ExpectAndReturn(beaconDetection.m_adapterIndex, &beaconDetection.m_limitBeaconDetection, mockBtmGetLimitBeaconDetectionResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_GetLimitBeaconDetection(&beaconDetection));
}

void test_btrMgr_setLimitBeaconDetection_not_inited(void) {
    BTRMGR_IARMBeaconDetection_t beaconDetection;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_SetLimitBeaconDetection(&beaconDetection));
}

void test_btrMgr_setLimitBeaconDetection_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_SetLimitBeaconDetection(NULL));
}

void test_btrMgr_setLimitBeaconDetection_failure(void) {
    BTRMGR_IARMBeaconDetection_t beaconDetection;
    BTRMGR_Result_t mockBtmSetLimitBeaconDetectionResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetLimitBeaconDetection_IgnoreAndReturn(mockBtmSetLimitBeaconDetectionResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_SetLimitBeaconDetection(&beaconDetection));
}

void test_btrMgr_setLimitBeaconDetection_success(void) {
    BTRMGR_IARMBeaconDetection_t beaconDetection;
    BTRMGR_Result_t mockBtmSetLimitBeaconDetectionResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetLimitBeaconDetection_ExpectAndReturn(beaconDetection.m_adapterIndex, beaconDetection.m_limitBeaconDetection, mockBtmSetLimitBeaconDetectionResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_SetLimitBeaconDetection(&beaconDetection));
}

void test_btrMgr_LeStartAdvertisement_not_inited(void) {
    BTRMGR_IARMAdvtInfo_t lpstAdvtInfo;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_LeStartAdvertisement(&lpstAdvtInfo));
}

void test_btrMgr_LeStartAdvertisement_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_LeStartAdvertisement(NULL));
}

void test_btrMgr_LeStartAdvertisement_failure(void) {
    BTRMGR_IARMAdvtInfo_t lpstAdvtInfo;
    BTRMGR_Result_t mockBtmStartAdvtResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_LE_StartAdvertisement_IgnoreAndReturn(mockBtmStartAdvtResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_LeStartAdvertisement(&lpstAdvtInfo));
}

void test_btrMgr_LeStartAdvertisement_success(void) {
    BTRMGR_IARMAdvtInfo_t lpstAdvtInfo;
    BTRMGR_Result_t mockBtmStartAdvtResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_LE_StartAdvertisement_ExpectAndReturn(lpstAdvtInfo.m_adapterIndex, &lpstAdvtInfo.m_CustAdvt, mockBtmStartAdvtResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_LeStartAdvertisement(&lpstAdvtInfo));
}

void test_btrMgr_LeStopAdvertisement_not_inited(void) {
    unsigned char adapterIndex = 0;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_LeStopAdvertisement(&adapterIndex));
}

void test_btrMgr_LeStopAdvertisement_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_LeStopAdvertisement(NULL));
}

void test_btrMgr_LeStopAdvertisement_failure(void) {
    unsigned char adapterIndex = 0;
    BTRMGR_Result_t mockBtmStopAdvtResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_LE_StopAdvertisement_ExpectAndReturn(0, mockBtmStopAdvtResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_LeStopAdvertisement(&adapterIndex));
}

void test_btrMgr_LeStopAdvertisement_success(void) {
    unsigned char adapterIndex = 0;
    BTRMGR_Result_t mockBtmStopAdvtResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_LE_StopAdvertisement_ExpectAndReturn(0, mockBtmStopAdvtResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_LeStopAdvertisement(&adapterIndex));
}

void test_btrMgr_LeGetPropertyValue_not_inited(void) {
    BTRMGR_IARMGATTValue_t lpstGattInfo;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_LeGetPropertyValue(&lpstGattInfo));
}

void test_btrMgr_LeGetPropertyValue_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_LeGetPropertyValue(NULL));
}

void test_btrMgr_LeGetPropertyValue_failure(void) {
    BTRMGR_IARMGATTValue_t lpstGattInfo;
    BTRMGR_Result_t mockBtmGetPropValueResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_LE_GetPropertyValue_IgnoreAndReturn(mockBtmGetPropValueResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_LeGetPropertyValue(&lpstGattInfo));
}

void test_btrMgr_LeGetPropertyValue_success(void) {
    BTRMGR_IARMGATTValue_t lpstGattInfo;
    BTRMGR_Result_t mockBtmGetPropValueResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_LE_GetPropertyValue_ExpectAndReturn(lpstGattInfo.m_adapterIndex, lpstGattInfo.m_UUID, lpstGattInfo.m_Value, lpstGattInfo.aElement, mockBtmGetPropValueResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_LeGetPropertyValue(&lpstGattInfo));
}

void test_btrMgr_LeSetServiceInfo_not_inited(void) {
    BTRMGR_IARMGATTServiceInfo_t lpstGattServiceInfo;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_LeSetServiceInfo(&lpstGattServiceInfo));
}

void test_btrMgr_LeSetServiceInfo_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_LeSetServiceInfo(NULL));
}

void test_btrMgr_LeSetServiceInfo_failure(void) {
    BTRMGR_IARMGATTServiceInfo_t lpstGattServiceInfo;
    BTRMGR_Result_t mockBtmSetServiceInfoResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_LE_SetServiceInfo_IgnoreAndReturn(mockBtmSetServiceInfoResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_LeSetServiceInfo(&lpstGattServiceInfo));
}

void test_btrMgr_LeSetServiceInfo_success(void) {
    BTRMGR_IARMGATTServiceInfo_t lpstGattServiceInfo;
    BTRMGR_Result_t mockBtmSetServiceInfoResult = BTRMGR_RESULT_SUCCESS;
   // lpstGattServiceInfo.m_ServiceType = BTRMGR_LE_SERVICE_TYPE_PRIMARY;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_LE_SetServiceInfo_ExpectAndReturn(lpstGattServiceInfo.m_adapterIndex, lpstGattServiceInfo.m_UUID, lpstGattServiceInfo.m_ServiceType, mockBtmSetServiceInfoResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_LeSetServiceInfo(&lpstGattServiceInfo));
}


void test_btrMgr_LeSetGattInfo_not_inited(void) {
    BTRMGR_IARMGATTInfo_t lpstGattInfo;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_LeSetGattInfo(&lpstGattInfo));
}

void test_btrMgr_LeSetGattInfo_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_LeSetGattInfo(NULL));
}

void test_btrMgr_LeSetGattInfo_failure(void) {
    BTRMGR_IARMGATTInfo_t lpstGattInfo;
    BTRMGR_Result_t mockBtmSetGattInfoResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_LE_SetGattInfo_IgnoreAndReturn(mockBtmSetGattInfoResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_LeSetGattInfo(&lpstGattInfo));
}

void test_btrMgr_LeSetGattInfo_success(void) {
    BTRMGR_IARMGATTInfo_t lpstGattInfo;
    BTRMGR_Result_t mockBtmSetGattInfoResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_LE_SetGattInfo_ExpectAndReturn(lpstGattInfo.m_adapterIndex, lpstGattInfo.m_ParentUUID, lpstGattInfo.m_UUID, lpstGattInfo.m_Flags, lpstGattInfo.m_Value, lpstGattInfo.m_Element, mockBtmSetGattInfoResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_LeSetGattInfo(&lpstGattInfo));
}

void test_btrMgr_LeSetGattPropertyValue_not_inited(void) {
    BTRMGR_IARMGATTValue_t lpstGattInfo;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_LeSetGattPropertyValue(&lpstGattInfo));
}

void test_btrMgr_LeSetGattPropertyValue_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_LeSetGattPropertyValue(NULL));
}

void test_btrMgr_LeSetGattPropertyValue_failure(void) {
    BTRMGR_IARMGATTValue_t lpstGattInfo;
    BTRMGR_Result_t mockBtmSetGattValueResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_LE_SetGattPropertyValue_IgnoreAndReturn(mockBtmSetGattValueResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_LeSetGattPropertyValue(&lpstGattInfo));
}

void test_btrMgr_LeSetGattPropertyValue_success(void) {
    BTRMGR_IARMGATTValue_t lpstGattInfo;
    BTRMGR_Result_t mockBtmSetGattValueResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_LE_SetGattPropertyValue_ExpectAndReturn(lpstGattInfo.m_adapterIndex, lpstGattInfo.m_UUID, lpstGattInfo.m_Value, lpstGattInfo.aElement, mockBtmSetGattValueResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_LeSetGattPropertyValue(&lpstGattInfo));
}

void test_btrMgr_setAudioInServiceState_not_inited(void) {
    BTRMGR_IARMAudioInServiceState_t audioInSerivceState;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_SetAudioInServiceState(&audioInSerivceState));
}

void test_btrMgr_setAudioInServiceState_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_SetAudioInServiceState(NULL));
}

void test_btrMgr_setAudioInServiceState_failure(void) {
    BTRMGR_IARMAudioInServiceState_t audioInSerivceState;
    BTRMGR_Result_t mockBtmSetInServiceStateResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetAudioInServiceState_IgnoreAndReturn(mockBtmSetInServiceStateResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_SetAudioInServiceState(&audioInSerivceState));
}

void test_btrMgr_setAudioInServiceState_success(void) {
    BTRMGR_IARMAudioInServiceState_t audioInSerivceState;
    BTRMGR_Result_t mockBtmSetInServiceStateResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetAudioInServiceState_ExpectAndReturn(audioInSerivceState.m_adapterIndex, audioInSerivceState.m_serviceState, mockBtmSetInServiceStateResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_SetAudioInServiceState(&audioInSerivceState));
}

void test_btrMgr_setHidGamePadServiceState_not_inited(void) {
    BTRMGR_IARMHidGamePadServiceState_t hidGamePadSerivceState;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_SetHidGamePadServiceState(&hidGamePadSerivceState));
}

void test_btrMgr_setHidGamePadServiceState_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_SetHidGamePadServiceState(NULL));
}

void test_btrMgr_setHidGamePadServiceState_failure(void) {
    BTRMGR_IARMHidGamePadServiceState_t hidGamePadSerivceState;
    BTRMGR_Result_t mockBtmSetGamePadServiceStateResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetHidGamePadServiceState_IgnoreAndReturn(mockBtmSetGamePadServiceStateResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_SetHidGamePadServiceState(&hidGamePadSerivceState));
}

void test_btrMgr_setHidGamePadServiceState_success(void) {
    BTRMGR_IARMHidGamePadServiceState_t hidGamePadSerivceState;
    BTRMGR_Result_t mockBtmSetGamePadServiceStateResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SetHidGamePadServiceState_ExpectAndReturn(hidGamePadSerivceState.m_adapterIndex, hidGamePadSerivceState.m_serviceState, mockBtmSetGamePadServiceStateResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_SetHidGamePadServiceState(&hidGamePadSerivceState));
}
void test_btrMgr_sysDiagInfo_not_inited(void) {
    BTRMGR_IARMDiagInfo_t lDiagInfo;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_SysDiagInfo(&lDiagInfo));
}

void test_btrMgr_sysDiagInfo_failure(void) {
    BTRMGR_IARMDiagInfo_t lDiagInfo;
    lDiagInfo.m_OpType = BTRMGR_LE_OP_UNKNOWN;
    BTRMGR_Result_t mockBtmSysDiagInfoResult = BTRMGR_RESULT_GENERIC_FAILURE;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SysDiagInfo_IgnoreAndReturn(mockBtmSysDiagInfoResult);
    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_SysDiagInfo(&lDiagInfo));
}
void test_btrMgr_sysDiagInfo_success(void) {
    BTRMGR_IARMDiagInfo_t lDiagInfo;
    BTRMGR_Result_t mockBtmSysDiagInfoResult = BTRMGR_RESULT_SUCCESS;
    lDiagInfo.m_OpType = BTRMGR_LE_OP_UNKNOWN;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_SysDiagInfo_IgnoreAndReturn(mockBtmSysDiagInfoResult);
    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_SysDiagInfo(&lDiagInfo));
}

void test_btrMgr_connectToWifi_not_inited(void) {
    BTRMGR_IARMWifiConnectInfo_t lWifiInfo;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_ConnectToWifi(&lWifiInfo));
}

void test_btrMgr_connectToWifi_failure(void) {
    BTRMGR_IARMWifiConnectInfo_t lWifiInfo;
    BTRMGR_Result_t mockBtmConnectToWifiResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_ConnectToWifi_IgnoreAndReturn(mockBtmConnectToWifiResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_ConnectToWifi(&lWifiInfo));
}

void test_btrMgr_connectToWifi_success(void) {
    BTRMGR_IARMWifiConnectInfo_t lWifiInfo;
    BTRMGR_Result_t mockBtmConnectToWifiResult = BTRMGR_RESULT_SUCCESS;
    //lWifiInfo.m_SecMode=BTRMGR_WIFI_SECURITY_MODE_UNKNOWN;
    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_ConnectToWifi_ExpectAndReturn(lWifiInfo.m_adapterIndex, lWifiInfo.m_SSID, lWifiInfo.m_Password, lWifiInfo.m_SecMode , mockBtmConnectToWifiResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_ConnectToWifi(&lWifiInfo));
}
extern bool gIsBTRMGR_Internal_Inited;

void test_btrMgr_IsAdapterDiscoverable_not_inited(void) {
    BTRMGR_IARMAdapterDiscoverable_t adapterDiscoverable;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_IsAdapterDiscoverable(&adapterDiscoverable));
}
void test_btrMgr_IsAdapterDiscoverable_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_IsAdapterDiscoverable(NULL));
}

void test_btrMgr_IsAdapterDiscoverable_failure(void) {
    BTRMGR_IARMAdapterDiscoverable_t adapterDiscoverable;
    BTRMGR_Result_t mockBtmIsAdapterDiscoverableResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IsAdapterDiscoverable_ExpectAndReturn(adapterDiscoverable.m_adapterIndex, &adapterDiscoverable.m_isDiscoverable, mockBtmIsAdapterDiscoverableResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_IsAdapterDiscoverable(&adapterDiscoverable));
}
void test_btrMgr_IsAdapterDiscoverable_success(void) {
    BTRMGR_IARMAdapterDiscoverable_t adapterDiscoverable;
    BTRMGR_Result_t mockBtmIsAdapterDiscoverableResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_IsAdapterDiscoverable_ExpectAndReturn(adapterDiscoverable.m_adapterIndex, &adapterDiscoverable.m_isDiscoverable, mockBtmIsAdapterDiscoverableResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_IsAdapterDiscoverable(&adapterDiscoverable));
}
extern bool gIsBTRMGR_Internal_Inited;

void test_btrMgr_ResetAdapter_not_inited(void) {
    unsigned char adapterIndex = 0;
    gIsBTRMGR_Internal_Inited = false;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_STATE, btrMgr_ResetAdapter(&adapterIndex));
}
void test_btrMgr_ResetAdapter_null_param(void) {
    gIsBTRMGR_Internal_Inited = true;

    TEST_ASSERT_EQUAL(IARM_RESULT_INVALID_PARAM, btrMgr_ResetAdapter(NULL));
}

void test_btrMgr_ResetAdapter_failure(void) {
    unsigned char adapterIndex = 0;
    BTRMGR_Result_t mockBtmResetAdapterResult = BTRMGR_RESULT_GENERIC_FAILURE;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_ResetAdapter_ExpectAndReturn(adapterIndex, mockBtmResetAdapterResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_IPCCORE_FAIL, btrMgr_ResetAdapter(&adapterIndex));
}
void test_btrMgr_ResetAdapter_success(void) {
    unsigned char adapterIndex = 0;
    BTRMGR_Result_t mockBtmResetAdapterResult = BTRMGR_RESULT_SUCCESS;

    gIsBTRMGR_Internal_Inited = true;
    BTRMGR_ResetAdapter_ExpectAndReturn(adapterIndex, mockBtmResetAdapterResult);

    TEST_ASSERT_EQUAL(IARM_RESULT_SUCCESS, btrMgr_ResetAdapter(&adapterIndex));
}
