#include "CommonHeader.h"
#include "MetalDevice.h"
#include "GpuApi/GpuFeature.h"

namespace FRAMEWORK
{
    void InitMetalCore()
    {
        GDevice = MTL::CreateSystemDefaultDevice();
		GpuFeature::Support16bitType = true;

        GCommandQueue = GDevice->newCommandQueue();
    }
}
