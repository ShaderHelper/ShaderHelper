#pragma once
#include "MetalCommon.h"

namespace FW
{
    inline MTL::Device* GDevice;
    inline MTL::CommandQueue* GCommandQueue;

    extern void InitMetalCore();
}
