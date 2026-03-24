#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
    inline MTL::Device* GDevice;
    inline MTL::CommandQueue* GCommandQueue;
    inline MTL::CounterSet* GTimestampCounterSet = nullptr;
    inline bool GSupportStageBoundaryCounter = false;

    extern void InitMetalCore();
}
