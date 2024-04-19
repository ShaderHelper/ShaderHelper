#include "CommonHeader.h"
#include "MetalDevice.h"
#include "GpuApi/GpuFeature.h"

namespace FRAMEWORK
{
    void InitMetalCore()
    {
        GDevice = mtlpp::Device::CreateSystemDefaultDevice();
		GpuFeature::Support16bitType = true;

        GCommandQueue = GDevice.NewCommandQueue();
//        CpuSyncGpuSemaphore = dispatch_semaphore_create(AllowableLag);
        
        GCaptureScope = mtlpp::CaptureManager::SharedCaptureManager().NewCaptureScopeWithDevice(GDevice);
        GCaptureScope.SetLabel(@"Default Capture Scope");
        mtlpp::CaptureManager::SharedCaptureManager().SetDefaultCaptureScope(GCaptureScope);
    }
}
