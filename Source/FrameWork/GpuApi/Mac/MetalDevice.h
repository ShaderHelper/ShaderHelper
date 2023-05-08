#pragma once
#include "MetalCommon.h"

namespace FRAMEWORK
{
    inline mtlpp::Device GDevice;
    inline mtlpp::CommandQueue GCommandQueue;
//    inline constexpr uint32 AllowableLag = 0;
//    inline dispatch_semaphore_t CpuSyncGpuSemaphore;
    
    inline mtlpp::CaptureScope GCaptureScope;

    extern void InitMetalCore();
}
