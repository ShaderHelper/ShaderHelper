#include "CommonHeader.h"
#include "MetalDevice.h"

namespace FRAMEWORK
{
    void InitMetalCore()
    {
        GDevice.Reset(new mtlpp::Device{ mtlpp::Device::CreateSystemDefaultDevice() });
        CpuSyncGpuSemaphore = dispatch_semaphore_create(AllowableLag);
    }
}
