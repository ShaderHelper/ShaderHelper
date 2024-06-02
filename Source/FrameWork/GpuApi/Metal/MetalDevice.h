#pragma once
#include "MetalCommon.h"

namespace FRAMEWORK
{
    inline MTL::Device* GDevice;
    inline MTL::CommandQueue* GCommandQueue;

    extern void InitMetalCore();
}
