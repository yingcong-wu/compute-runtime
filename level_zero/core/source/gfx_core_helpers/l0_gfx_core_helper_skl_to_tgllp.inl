/*
 * Copyright (C) 2021-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "level_zero/core/source/gfx_core_helpers/l0_gfx_core_helper.h"

namespace L0 {

template <typename Family>
bool L0GfxCoreHelperHw<Family>::platformSupportsCmdListHeapSharing() const {
    return true;
}

template <typename Family>
bool L0GfxCoreHelperHw<Family>::platformSupportsStateComputeModeTracking() const {
    return false;
}

template <typename Family>
bool L0GfxCoreHelperHw<Family>::platformSupportsFrontEndTracking() const {
    return false;
}

template <typename Family>
bool L0GfxCoreHelperHw<Family>::platformSupportsPipelineSelectTracking() const {
    return false;
}

template <typename Family>
bool L0GfxCoreHelperHw<Family>::platformSupportsStateBaseAddressTracking() const {
    return false;
}

template <typename Family>
uint32_t L0GfxCoreHelperHw<Family>::getEventMaxKernelCount(const NEO::HardwareInfo &hwInfo) const {
    return 1;
}

template <typename Family>
uint32_t L0GfxCoreHelperHw<Family>::getEventBaseMaxPacketCount(const NEO::RootDeviceEnvironment &rootDeviceEnvironment) const {
    return 1u;
}

template <typename Family>
bool L0GfxCoreHelperHw<Family>::platformSupportsRayTracing() const {
    return false;
}

template <typename Family>
bool L0GfxCoreHelperHw<Family>::isZebinAllowed(const NEO::Debugger *debugger) const {
    return !debugger;
}

template <typename Family>
NEO::HeapAddressModel L0GfxCoreHelperHw<Family>::getPlatformHeapAddressModel() const {
    return NEO::HeapAddressModel::privateHeaps;
}

template <typename Family>
ze_rtas_format_exp_t L0GfxCoreHelperHw<Family>::getSupportedRTASFormat() const {
    return ZE_RTAS_FORMAT_EXP_INVALID;
}

template <typename Family>
bool L0GfxCoreHelperHw<Family>::platformSupportsPrimaryBatchBufferCmdList() const {
    return true;
}

template <typename Family>
bool L0GfxCoreHelperHw<Family>::platformSupportsImmediateComputeFlushTask() const {
    return false;
}

} // namespace L0
