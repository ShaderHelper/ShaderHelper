#pragma once
#include "MetalCommon.h"

namespace FRAMEWORK
{
    inline TUniquePtr<mtlpp::Device> GDevice;
    inline constexpr uint32 AllowableLag = 0;
    inline dispatch_semaphore_t CpuSyncGpuSemaphore;

    extern void InitMetalCore();
}
