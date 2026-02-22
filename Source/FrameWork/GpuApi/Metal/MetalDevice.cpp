#include "CommonHeader.h"
#include "MetalDevice.h"
#include "GpuApi/GpuFeature.h"

namespace FW
{
    void InitMetalCore()
    {
        GDevice = MTL::CreateSystemDefaultDevice();
		GpuFeature::Support16bitType = true;
//		if(@available(macOS 13.3, *))
//		{
//			GDevice->setShouldMaximizeConcurrentCompilation(true);
//		}
        GCommandQueue = GDevice->newCommandQueue();
    }
}
