#include "CommonHeader.h"
#include "MetalBuffer.h"
#include "MetalDevice.h"

namespace FRAMEWORK
{
    TRefCountPtr<MetalBuffer> CreateMetalBuffer(uint64 BufferSize, GpuBufferUsage Usage)
    {
        mtlpp::Buffer Buffer;
        if(EnumHasAnyFlags(Usage, GpuBufferUsage::Static))
        {
            Buffer = GDevice.NewBuffer(BufferSize, mtlpp::ResourceOptions::StorageModePrivate);
        }
        else
        {
            Buffer = GDevice.NewBuffer(BufferSize, mtlpp::ResourceOptions::StorageModeShared);
        }
        return new MetalBuffer(MoveTemp(Buffer), Usage);
    }
}
