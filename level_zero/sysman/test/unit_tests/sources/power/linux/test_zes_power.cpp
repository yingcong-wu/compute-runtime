/*
 * Copyright (C) 2020-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "level_zero/sysman/test/unit_tests/sources/power/linux/mock_sysfs_power.h"

namespace L0 {
namespace Sysman {
namespace ult {

constexpr uint64_t convertJouleToMicroJoule = 1000000u;
constexpr uint32_t powerHandleComponentCount = 1u;

ssize_t preadMockPower(int fd, void *buf, size_t count, off_t offset) {
    uint64_t *mockBuf = static_cast<uint64_t *>(buf);
    *mockBuf = setEnergyCounter;
    return count;
}

TEST_F(SysmanDevicePowerFixtureI915, GivenComponentCountZeroWhenEnumeratingPowerDomainsWhenhwmonInterfaceExistsThenValidCountIsReturnedAndVerifySysmanPowerGetCallSucceeds) {
    uint32_t count = 0;
    EXPECT_EQ(zesDeviceEnumPowerDomains(device->toHandle(), &count, nullptr), ZE_RESULT_SUCCESS);
    EXPECT_EQ(count, powerHandleComponentCount);
}

TEST_F(SysmanDevicePowerFixtureI915, GivenInvalidComponentCountWhenEnumeratingPowerDomainsWhenhwmonInterfaceExistsThenValidCountIsReturnedAndVerifySysmanPowerGetCallSucceeds) {
    uint32_t count = 0;
    EXPECT_EQ(zesDeviceEnumPowerDomains(device->toHandle(), &count, nullptr), ZE_RESULT_SUCCESS);
    EXPECT_EQ(count, powerHandleComponentCount);

    count = count + 1;
    EXPECT_EQ(zesDeviceEnumPowerDomains(device->toHandle(), &count, nullptr), ZE_RESULT_SUCCESS);
    EXPECT_EQ(count, powerHandleComponentCount);
}

TEST_F(SysmanDevicePowerFixtureI915, GivenComponentCountZeroWhenEnumeratingPowerDomainsWhenhwmonInterfaceExistsThenValidPowerHandlesIsReturned) {
    uint32_t count = 0;
    EXPECT_EQ(zesDeviceEnumPowerDomains(device->toHandle(), &count, nullptr), ZE_RESULT_SUCCESS);
    EXPECT_EQ(count, powerHandleComponentCount);

    std::vector<zes_pwr_handle_t> handles(count, nullptr);
    EXPECT_EQ(zesDeviceEnumPowerDomains(device->toHandle(), &count, handles.data()), ZE_RESULT_SUCCESS);
    for (auto handle : handles) {
        EXPECT_NE(handle, nullptr);
    }
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerPointerWhenGettingCardPowerDomainWhenhwmonInterfaceExistsAndThenCallSucceeds) {
    zes_pwr_handle_t phPower = {};
    EXPECT_EQ(zesDeviceGetCardPowerDomain(device->toHandle(), &phPower), ZE_RESULT_SUCCESS);
}

TEST_F(SysmanDevicePowerFixtureI915, GivenInvalidPowerPointerWhenGettingCardPowerDomainAndThenReturnsFailure) {
    EXPECT_EQ(zesDeviceGetCardPowerDomain(device->toHandle(), nullptr), ZE_RESULT_ERROR_INVALID_NULL_POINTER);
}

TEST_F(SysmanDevicePowerFixtureI915, GivenUninitializedPowerHandlesAndWhenGettingCardPowerDomainThenReturnsFailure) {
    for (const auto &handle : pSysmanDeviceImp->pPowerHandleContext->handleList) {
        delete handle;
    }
    pSysmanDeviceImp->pPowerHandleContext->handleList.clear();

    zes_pwr_handle_t phPower = {};
    EXPECT_EQ(zesDeviceGetCardPowerDomain(device->toHandle(), &phPower), ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleWhenGettingPowerPropertiesWhenhwmonInterfaceExistsThenCallSucceeds) {

    auto handles = getPowerHandles(powerHandleComponentCount);

    for (auto handle : handles) {
        zes_power_properties_t properties;
        EXPECT_EQ(ZE_RESULT_SUCCESS, zesPowerGetProperties(handle, &properties));
        EXPECT_FALSE(properties.onSubdevice);
        EXPECT_EQ(properties.subdeviceId, 0u);
        EXPECT_EQ(properties.canControl, true);
        EXPECT_EQ(properties.isEnergyThresholdSupported, false);
        EXPECT_EQ(properties.defaultLimit, static_cast<int32_t>(mockDefaultPowerLimitVal / milliFactor));
        EXPECT_EQ(properties.maxLimit, static_cast<int32_t>(mockMaxPowerLimitVal / milliFactor));
        EXPECT_EQ(properties.minLimit, static_cast<int32_t>(mockMinPowerLimitVal / milliFactor));
    }
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleWhenGettingPowerPropertiesAndSysfsReadFailsThenFailureIsReturned) {
    std::unique_ptr<PublicLinuxPowerImp> pLinuxPowerImp(new PublicLinuxPowerImp(pOsSysman, false, 0));
    pLinuxPowerImp->pSysfsAccess = pSysfsAccess;
    pLinuxPowerImp->pPmt = static_cast<MockPowerPmt *>(pLinuxSysmanImp->getPlatformMonitoringTechAccess(0));
    pLinuxPowerImp->isPowerModuleSupported();

    pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_ERROR_NOT_AVAILABLE);
    zes_power_properties_t properties{};
    EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, pLinuxPowerImp->getProperties(&properties));
    EXPECT_EQ(properties.defaultLimit, -1);
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleWhenGettingPowerPropertiesAndSustainedLimitReadFailsThenFailureIsReturned) {
    std::unique_ptr<PublicLinuxPowerImp> pLinuxPowerImp(new PublicLinuxPowerImp(pOsSysman, false, 0));
    pLinuxPowerImp->pSysfsAccess = pSysfsAccess;
    pLinuxPowerImp->pPmt = static_cast<MockPowerPmt *>(pLinuxSysmanImp->getPlatformMonitoringTechAccess(0));
    pLinuxPowerImp->isPowerModuleSupported();

    pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_SUCCESS);
    pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_ERROR_NOT_AVAILABLE);
    zes_power_properties_t properties{};
    EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, pLinuxPowerImp->getProperties(&properties));
    EXPECT_EQ(properties.minLimit, -1);
    EXPECT_EQ(properties.maxLimit, -1);
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleWhenGettingPowerPropertiesAndMinLimitReadFailsThenFailureIsReturned) {
    std::unique_ptr<PublicLinuxPowerImp> pLinuxPowerImp(new PublicLinuxPowerImp(pOsSysman, false, 0));
    pLinuxPowerImp->pSysfsAccess = pSysfsAccess;
    pLinuxPowerImp->pPmt = static_cast<MockPowerPmt *>(pLinuxSysmanImp->getPlatformMonitoringTechAccess(0));
    pLinuxPowerImp->isPowerModuleSupported();

    pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_SUCCESS);
    pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_SUCCESS);
    pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_ERROR_NOT_AVAILABLE);
    zes_power_properties_t properties{};
    EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, pLinuxPowerImp->getProperties(&properties));
    EXPECT_EQ(properties.minLimit, -1);
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleWhenGettingPowerPropertiesAndMaxLimitReadFailsThenFailureIsReturned) {
    std::unique_ptr<PublicLinuxPowerImp> pLinuxPowerImp(new PublicLinuxPowerImp(pOsSysman, false, 0));
    pLinuxPowerImp->pSysfsAccess = pSysfsAccess;
    pLinuxPowerImp->pPmt = static_cast<MockPowerPmt *>(pLinuxSysmanImp->getPlatformMonitoringTechAccess(0));
    pLinuxPowerImp->isPowerModuleSupported();

    pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_SUCCESS);
    pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_SUCCESS);
    pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_SUCCESS);
    pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_ERROR_NOT_AVAILABLE);
    zes_power_properties_t properties{};
    EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, pLinuxPowerImp->getProperties(&properties));
    EXPECT_EQ(properties.maxLimit, -1);
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleWhenGettingPowerEnergyCounterFailedWhenHwmonInterfaceExistThenValidErrorCodeReturned) {
    auto handles = getPowerHandles(powerHandleComponentCount);
    auto subDeviceCount = pLinuxSysmanImp->getSubDeviceCount();
    uint32_t subdeviceId = 0;
    do {
        auto pPmt = static_cast<MockPowerPmt *>(pLinuxSysmanImp->getPlatformMonitoringTechAccess(subdeviceId));
        pPmt->preadFunction = preadMockPower;
    } while (++subdeviceId < subDeviceCount);

    pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS);
    for (auto handle : handles) {
        ASSERT_NE(nullptr, handle);
        zes_power_energy_counter_t energyCounter = {};
        uint64_t expectedEnergyCounter = convertJouleToMicroJoule * (setEnergyCounter / 1048576);
        ASSERT_EQ(ZE_RESULT_SUCCESS, zesPowerGetEnergyCounter(handle, &energyCounter));
        EXPECT_EQ(energyCounter.energy, expectedEnergyCounter);
    }
}

TEST_F(SysmanDevicePowerFixtureI915, GivenSetPowerLimitsWhenGettingPowerLimitsWhenHwmonInterfaceExistThenLimitsSetEarlierAreRetrieved) {
    auto handles = getPowerHandles(powerHandleComponentCount);
    for (auto handle : handles) {
        ASSERT_NE(nullptr, handle);
        zes_power_sustained_limit_t sustainedSet = {};
        zes_power_sustained_limit_t sustainedGet = {};
        sustainedSet.enabled = 1;
        sustainedSet.interval = 10;
        sustainedSet.power = 300000;
        EXPECT_EQ(ZE_RESULT_SUCCESS, zesPowerSetLimits(handle, &sustainedSet, nullptr, nullptr));
        EXPECT_EQ(ZE_RESULT_SUCCESS, zesPowerGetLimits(handle, &sustainedGet, nullptr, nullptr));
        EXPECT_EQ(sustainedGet.power, sustainedSet.power);

        zes_power_burst_limit_t burstGet = {};
        EXPECT_EQ(ZE_RESULT_SUCCESS, zesPowerSetLimits(handle, nullptr, nullptr, nullptr));
        EXPECT_EQ(ZE_RESULT_SUCCESS, zesPowerGetLimits(handle, nullptr, &burstGet, nullptr));
        EXPECT_EQ(burstGet.enabled, false);
        EXPECT_EQ(burstGet.power, -1);

        zes_power_peak_limit_t peakSet = {};
        zes_power_peak_limit_t peakGet = {};
        peakSet.powerAC = 300000;
        EXPECT_EQ(ZE_RESULT_SUCCESS, zesPowerSetLimits(handle, nullptr, nullptr, &peakSet));
        EXPECT_EQ(ZE_RESULT_SUCCESS, zesPowerGetLimits(handle, nullptr, nullptr, &peakGet));
        EXPECT_EQ(peakGet.powerAC, peakSet.powerAC);
        EXPECT_EQ(peakGet.powerDC, -1);
    }
}

TEST_F(SysmanDevicePowerFixtureI915, GivenReadingSustainedPowerLimitNodeReturnErrorWhenSetOrGetPowerLimitsWhenHwmonInterfaceExistForSustainedPowerLimitEnabledThenProperErrorCodesReturned) {
    auto handles = getPowerHandles(powerHandleComponentCount);

    pSysfsAccess->mockWriteUnsignedResult.push_back(ZE_RESULT_ERROR_NOT_AVAILABLE);
    pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_ERROR_NOT_AVAILABLE);

    for (auto handle : handles) {
        ASSERT_NE(nullptr, handle);
        zes_power_sustained_limit_t sustainedSet = {};
        zes_power_sustained_limit_t sustainedGet = {};

        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, zesPowerSetLimits(handle, &sustainedSet, nullptr, nullptr));
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, zesPowerGetLimits(handle, &sustainedGet, nullptr, nullptr));
    }
}

TEST_F(SysmanDevicePowerFixtureI915, GivenReadingSustainedPowerNodeReturnErrorWhenGetPowerLimitsForSustainedPowerWhenHwmonInterfaceExistThenProperErrorCodesReturned) {
    auto handles = getPowerHandles(powerHandleComponentCount);

    pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_ERROR_NOT_AVAILABLE);
    for (auto handle : handles) {
        ASSERT_NE(nullptr, handle);
        zes_power_sustained_limit_t sustainedGet = {};
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, zesPowerGetLimits(handle, &sustainedGet, nullptr, nullptr));
    }
}

TEST_F(SysmanDevicePowerFixtureI915, GivenReadingpeakPowerNodeReturnErrorWhenGetPowerLimitsForpeakPowerWhenHwmonInterfaceExistThenProperErrorCodesReturned) {
    auto handles = getPowerHandles(powerHandleComponentCount);
    pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_ERROR_NOT_AVAILABLE);

    for (auto handle : handles) {
        ASSERT_NE(nullptr, handle);
        zes_power_peak_limit_t peakGet = {};
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, zesPowerGetLimits(handle, nullptr, nullptr, &peakGet));
    }
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleAndPermissionsThenFirstDisableSustainedPowerLimitAndThenEnableItAndCheckSuccesIsReturned) {
    auto handles = getPowerHandles(powerHandleComponentCount);
    ASSERT_NE(nullptr, handles[0]);
    zes_power_sustained_limit_t sustainedSet = {};
    zes_power_sustained_limit_t sustainedGet = {};
    sustainedSet.enabled = 0;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zesPowerSetLimits(handles[0], &sustainedSet, nullptr, nullptr));
    sustainedSet.power = 300000;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zesPowerSetLimits(handles[0], &sustainedSet, nullptr, nullptr));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zesPowerGetLimits(handles[0], &sustainedGet, nullptr, nullptr));
    EXPECT_EQ(sustainedGet.power, sustainedSet.power);
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleWhenGettingPowerLimitsThenUnsupportedFeatureErrorIsReturned) {
    pSysfsAccess->mockscanDirEntriesResult = ZE_RESULT_ERROR_NOT_AVAILABLE;
    for (const auto &handle : pSysmanDeviceImp->pPowerHandleContext->handleList) {
        delete handle;
    }
    pSysmanDeviceImp->pPowerHandleContext->handleList.clear();
    pSysmanDeviceImp->pPowerHandleContext->init(pLinuxSysmanImp->getSubDeviceCount());
    auto handles = getPowerHandles(powerHandleComponentCount);
    for (auto handle : handles) {
        ASSERT_NE(nullptr, handle);
        zes_power_sustained_limit_t sustained;
        zes_power_burst_limit_t burst;
        zes_power_peak_limit_t peak;
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, zesPowerGetLimits(handle, &sustained, &burst, &peak));
    }
}

TEST_F(SysmanDevicePowerFixtureI915, GivenwritingSustainedPowerNodeReturnErrorWhenSetPowerLimitsForSustainedPowerWhenHwmonInterfaceExistThenProperErrorCodesReturned) {
    auto handles = getPowerHandles(powerHandleComponentCount);
    ASSERT_NE(nullptr, handles[0]);

    pSysfsAccess->mockWriteUnsignedResult.push_back(ZE_RESULT_ERROR_NOT_AVAILABLE);
    zes_power_sustained_limit_t sustainedSet = {};
    sustainedSet.enabled = 1;
    sustainedSet.interval = 10;
    sustainedSet.power = 300000;
    EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, zesPowerSetLimits(handles[0], &sustainedSet, nullptr, nullptr));
}

TEST_F(SysmanDevicePowerFixtureI915, GivenwritingSustainedPowerIntervalNodeReturnErrorWhenSetPowerLimitsForSustainedPowerIntervalWhenHwmonInterfaceExistThenProperErrorCodesReturned) {
    auto handles = getPowerHandles(powerHandleComponentCount);
    ASSERT_NE(nullptr, handles[0]);

    pSysfsAccess->mockWriteUnsignedResult.push_back(ZE_RESULT_ERROR_NOT_AVAILABLE);
    zes_power_sustained_limit_t sustainedSet = {};
    sustainedSet.enabled = 1;
    sustainedSet.interval = 10;
    sustainedSet.power = 300000;
    EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, zesPowerSetLimits(handles[0], &sustainedSet, nullptr, nullptr));
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleWhenWritingToSustainedPowerEnableNodeWithoutPermissionsThenValidErrorIsReturned) {
    auto handles = getPowerHandles(powerHandleComponentCount);
    ASSERT_NE(nullptr, handles[0]);

    pSysfsAccess->mockWriteUnsignedResult.push_back(ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS);
    zes_power_sustained_limit_t sustainedSet = {};
    sustainedSet.enabled = 0;
    EXPECT_EQ(ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS, zesPowerSetLimits(handles[0], &sustainedSet, nullptr, nullptr));
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleWhenSettingPowerLimitsThenUnsupportedFeatureErrorIsReturned) {
    pSysfsAccess->mockscanDirEntriesResult = ZE_RESULT_ERROR_NOT_AVAILABLE;
    for (const auto &handle : pSysmanDeviceImp->pPowerHandleContext->handleList) {
        delete handle;
    }
    pSysmanDeviceImp->pPowerHandleContext->handleList.clear();
    pSysmanDeviceImp->pPowerHandleContext->init(pLinuxSysmanImp->getSubDeviceCount());
    auto handles = getPowerHandles(powerHandleComponentCount);
    for (auto handle : handles) {
        ASSERT_NE(nullptr, handle);
        zes_power_sustained_limit_t sustained;
        zes_power_burst_limit_t burst;
        zes_power_peak_limit_t peak;
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, zesPowerSetLimits(handle, &sustained, &burst, &peak));
    }
}

TEST_F(SysmanDevicePowerFixtureI915, GivenScanDirectoriesFailAndPmtIsNotNullPointerThenPowerModuleIsSupported) {

    pSysfsAccess->mockscanDirEntriesResult = ZE_RESULT_ERROR_NOT_AVAILABLE;
    auto subDeviceCount = pLinuxSysmanImp->getSubDeviceCount();
    pSysmanDeviceImp->pPowerHandleContext->init(subDeviceCount);
    ze_bool_t onSubdevice = (subDeviceCount == 0) ? false : true;
    uint32_t subdeviceId = 0;
    PublicLinuxPowerImp *pPowerImp = new PublicLinuxPowerImp(pOsSysman, onSubdevice, subdeviceId);
    EXPECT_TRUE(pPowerImp->isPowerModuleSupported());
    delete pPowerImp;
}

TEST_F(SysmanDevicePowerFixtureI915, GivenComponentCountZeroWhenEnumeratingPowerDomainsThenValidCountIsReturnedAndVerifySysmanPowerGetCallSucceeds) {

    pSysfsAccess->mockscanDirEntriesResult = ZE_RESULT_ERROR_NOT_AVAILABLE;

    for (const auto &handle : pSysmanDeviceImp->pPowerHandleContext->handleList) {
        delete handle;
    }
    pSysmanDeviceImp->pPowerHandleContext->handleList.clear();
    pSysmanDeviceImp->pPowerHandleContext->init(pLinuxSysmanImp->getSubDeviceCount());
    uint32_t count = 0;
    EXPECT_EQ(zesDeviceEnumPowerDomains(device->toHandle(), &count, nullptr), ZE_RESULT_SUCCESS);
    EXPECT_EQ(count, powerHandleComponentCount);
}

TEST_F(SysmanDevicePowerFixtureI915, GivenInvalidComponentCountWhenEnumeratingPowerDomainsThenValidCountIsReturnedAndVerifySysmanPowerGetCallSucceeds) {

    pSysfsAccess->mockscanDirEntriesResult = ZE_RESULT_ERROR_NOT_AVAILABLE;

    for (const auto &handle : pSysmanDeviceImp->pPowerHandleContext->handleList) {
        delete handle;
    }
    pSysmanDeviceImp->pPowerHandleContext->handleList.clear();
    pSysmanDeviceImp->pPowerHandleContext->init(pLinuxSysmanImp->getSubDeviceCount());
    uint32_t count = 0;
    EXPECT_EQ(zesDeviceEnumPowerDomains(device->toHandle(), &count, nullptr), ZE_RESULT_SUCCESS);
    EXPECT_EQ(count, powerHandleComponentCount);

    count = count + 1;
    EXPECT_EQ(zesDeviceEnumPowerDomains(device->toHandle(), &count, nullptr), ZE_RESULT_SUCCESS);
    EXPECT_EQ(count, powerHandleComponentCount);
}

TEST_F(SysmanDevicePowerFixtureI915, GivenComponentCountZeroWhenEnumeratingPowerDomainsThenValidPowerHandlesIsReturned) {

    pSysfsAccess->mockscanDirEntriesResult = ZE_RESULT_ERROR_NOT_AVAILABLE;

    for (const auto &handle : pSysmanDeviceImp->pPowerHandleContext->handleList) {
        delete handle;
    }
    pSysmanDeviceImp->pPowerHandleContext->handleList.clear();
    pSysmanDeviceImp->pPowerHandleContext->init(pLinuxSysmanImp->getSubDeviceCount());
    uint32_t count = 0;
    EXPECT_EQ(zesDeviceEnumPowerDomains(device->toHandle(), &count, nullptr), ZE_RESULT_SUCCESS);
    EXPECT_EQ(count, powerHandleComponentCount);

    std::vector<zes_pwr_handle_t> handles(count, nullptr);
    EXPECT_EQ(zesDeviceEnumPowerDomains(device->toHandle(), &count, handles.data()), ZE_RESULT_SUCCESS);
    for (auto handle : handles) {
        EXPECT_NE(handle, nullptr);
    }
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleWhenGettingPowerPropertiesThenCallSucceeds) {

    pSysfsAccess->mockscanDirEntriesResult = ZE_RESULT_ERROR_NOT_AVAILABLE;

    for (const auto &handle : pSysmanDeviceImp->pPowerHandleContext->handleList) {
        delete handle;
    }
    pSysmanDeviceImp->pPowerHandleContext->handleList.clear();
    pSysmanDeviceImp->pPowerHandleContext->init(pLinuxSysmanImp->getSubDeviceCount());
    auto handles = getPowerHandles(powerHandleComponentCount);

    for (auto handle : handles) {
        zes_power_properties_t properties;
        EXPECT_EQ(ZE_RESULT_SUCCESS, zesPowerGetProperties(handle, &properties));
        EXPECT_FALSE(properties.onSubdevice);
        EXPECT_EQ(properties.subdeviceId, 0u);
    }
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleWhenGettingPowerEnergyCounterThenValidPowerReadingsRetrieved) {

    pSysfsAccess->mockscanDirEntriesResult = ZE_RESULT_ERROR_NOT_AVAILABLE;

    for (const auto &handle : pSysmanDeviceImp->pPowerHandleContext->handleList) {
        delete handle;
    }
    pSysmanDeviceImp->pPowerHandleContext->handleList.clear();
    auto subDeviceCount = pLinuxSysmanImp->getSubDeviceCount();
    pSysmanDeviceImp->pPowerHandleContext->init(pLinuxSysmanImp->getSubDeviceCount());
    auto handles = getPowerHandles(powerHandleComponentCount);

    uint32_t subdeviceId = 0;
    do {
        auto pPmt = static_cast<MockPowerPmt *>(pLinuxSysmanImp->getPlatformMonitoringTechAccess(subdeviceId));
        pPmt->preadFunction = preadMockPower;
    } while (++subdeviceId < subDeviceCount);

    for (auto handle : handles) {
        zes_power_energy_counter_t energyCounter;
        uint64_t expectedEnergyCounter = convertJouleToMicroJoule * (setEnergyCounter / 1048576);
        ASSERT_EQ(ZE_RESULT_SUCCESS, zesPowerGetEnergyCounter(handle, &energyCounter));
        EXPECT_EQ(energyCounter.energy, expectedEnergyCounter);
    }
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleWhenGettingPowerEnergyCounterWhenEnergyHwmonFileReturnsErrorAndPmtFailsThenFailureIsReturned) {
    for (const auto &handle : pSysmanDeviceImp->pPowerHandleContext->handleList) {
        delete handle;
    }
    for (auto &subDeviceIdToPmtEntry : pLinuxSysmanImp->mapOfSubDeviceIdToPmtObject) {
        delete subDeviceIdToPmtEntry.second;
    }

    auto subDeviceCount = pLinuxSysmanImp->getSubDeviceCount();
    uint32_t subdeviceId = 0;
    pLinuxSysmanImp->mapOfSubDeviceIdToPmtObject.clear();
    do {
        pLinuxSysmanImp->mapOfSubDeviceIdToPmtObject.emplace(subdeviceId, nullptr);
    } while (++subdeviceId < subDeviceCount);

    pSysmanDeviceImp->pPowerHandleContext->handleList.clear();
    pSysmanDeviceImp->pPowerHandleContext->init(subDeviceCount);
    auto handles = getPowerHandles(powerHandleComponentCount);

    for (auto handle : handles) {
        pSysfsAccess->mockReadValUnsignedLongResult.push_back(ZE_RESULT_ERROR_NOT_AVAILABLE);
        ASSERT_NE(nullptr, handle);
        zes_power_energy_counter_t energyCounter = {};
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, zesPowerGetEnergyCounter(handle, &energyCounter));
    }
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleWhenGettingPowerEnergyThresholdThenUnsupportedFeatureErrorIsReturned) {

    pSysfsAccess->mockscanDirEntriesResult = ZE_RESULT_ERROR_NOT_AVAILABLE;

    for (const auto &handle : pSysmanDeviceImp->pPowerHandleContext->handleList) {
        delete handle;
    }
    pSysmanDeviceImp->pPowerHandleContext->handleList.clear();
    pSysmanDeviceImp->pPowerHandleContext->init(pLinuxSysmanImp->getSubDeviceCount());
    zes_energy_threshold_t threshold;
    auto handles = getPowerHandles(powerHandleComponentCount);
    for (auto handle : handles) {
        ASSERT_NE(nullptr, handle);
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, zesPowerGetEnergyThreshold(handle, &threshold));
    }
}

TEST_F(SysmanDevicePowerFixtureI915, GivenScanDirectoriesFailAndPmtIsNullWhenGettingCardPowerThenReturnsFailure) {

    pSysfsAccess->mockscanDirEntriesResult = ZE_RESULT_ERROR_NOT_AVAILABLE;

    for (const auto &handle : pSysmanDeviceImp->pPowerHandleContext->handleList) {
        delete handle;
    }
    pSysmanDeviceImp->pPowerHandleContext->handleList.clear();
    for (auto &pmtMapElement : pLinuxSysmanImp->mapOfSubDeviceIdToPmtObject) {
        if (pmtMapElement.second) {
            delete pmtMapElement.second;
            pmtMapElement.second = nullptr;
        }
    }
    pLinuxSysmanImp->mapOfSubDeviceIdToPmtObject.clear();
    pSysmanDeviceImp->pPowerHandleContext->init(pLinuxSysmanImp->getSubDeviceCount());

    zes_pwr_handle_t phPower = {};
    EXPECT_EQ(zesDeviceGetCardPowerDomain(device->toHandle(), &phPower), ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
}

TEST_F(SysmanDevicePowerFixtureI915, GivenValidPowerHandleWhenSettingPowerEnergyThresholdThenUnsupportedFeatureErrorIsReturned) {

    pSysfsAccess->mockscanDirEntriesResult = ZE_RESULT_ERROR_NOT_AVAILABLE;

    for (const auto &handle : pSysmanDeviceImp->pPowerHandleContext->handleList) {
        delete handle;
    }
    pSysmanDeviceImp->pPowerHandleContext->handleList.clear();
    pSysmanDeviceImp->pPowerHandleContext->init(pLinuxSysmanImp->getSubDeviceCount());
    double threshold = 0;
    auto handles = getPowerHandles(powerHandleComponentCount);
    for (auto handle : handles) {
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, zesPowerSetEnergyThreshold(handle, threshold));
    }
}

TEST_F(SysmanDevicePowerMultiDeviceFixture, GivenValidPowerHandleWhenGettingPowerEnergyCounterWhenEnergyHwmonFailsThenValidPowerReadingsRetrievedFromPmt) {

    pSysfsAccess->mockscanDirEntriesResult = ZE_RESULT_ERROR_NOT_AVAILABLE;

    for (const auto &handle : pSysmanDeviceImp->pPowerHandleContext->handleList) {
        delete handle;
    }
    pSysmanDeviceImp->pPowerHandleContext->handleList.clear();
    auto subDeviceCount = pLinuxSysmanImp->getSubDeviceCount();
    pSysmanDeviceImp->pPowerHandleContext->init(subDeviceCount);
    auto handles = getPowerHandles(powerHandleComponentCount);

    uint32_t subdeviceId = 0;
    do {
        auto pPmt = static_cast<MockPowerPmt *>(pLinuxSysmanImp->getPlatformMonitoringTechAccess(subdeviceId));
        pPmt->preadFunction = preadMockPower;
    } while (++subdeviceId < subDeviceCount);

    for (auto handle : handles) {
        ASSERT_NE(nullptr, handle);
        zes_power_energy_counter_t energyCounter;
        uint64_t expectedEnergyCounter = convertJouleToMicroJoule * (setEnergyCounter / 1048576);
        ASSERT_EQ(ZE_RESULT_SUCCESS, zesPowerGetEnergyCounter(handle, &energyCounter));
        EXPECT_EQ(energyCounter.energy, expectedEnergyCounter);
    }
}

TEST_F(SysmanDevicePowerFixtureXe, GivenKmdInterfaceWhenGettingSysFsFilenamesForPowerForXeVersionThenProperPathsAreReturned) {
    auto pSysmanKmdInterface = std::make_unique<SysmanKmdInterfaceXe>(pLinuxSysmanImp->getProductFamily());
    EXPECT_STREQ("energy1_input", pSysmanKmdInterface->getSysfsFilePath(SysfsName::sysfsNameEnergyCounterNode, 0, false).c_str());
    EXPECT_STREQ("power1_rated_max", pSysmanKmdInterface->getSysfsFilePath(SysfsName::sysfsNameDefaultPowerLimit, 0, false).c_str());
    EXPECT_STREQ("power1_max", pSysmanKmdInterface->getSysfsFilePath(SysfsName::sysfsNameSustainedPowerLimit, 0, false).c_str());
    EXPECT_STREQ("power1_max_interval", pSysmanKmdInterface->getSysfsFilePath(SysfsName::sysfsNameSustainedPowerLimitInterval, 0, false).c_str());
}

} // namespace ult
} // namespace Sysman
} // namespace L0
