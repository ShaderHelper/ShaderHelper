#include "CommonHeader.h"
#include "MetalBuffer.h"
#include "MetalDevice.h"
#include "MetalGpuRhiBackend.h"

namespace FW
{
    MetalBuffer::MetalBuffer(MTLBufferPtr InBuffer, GpuBufferUsage Usage)
    : GpuBuffer(Usage)
    , Buffer(MoveTemp(InBuffer))
    {
        GDeferredReleaseOneFrame.Add(this);
    }

    TRefCountPtr<MetalBuffer> CreateMetalBuffer(uint64 BufferSize, GpuBufferUsage Usage)
    {
        MTL::Buffer* Buffer = nullptr;
        if(EnumHasAnyFlags(Usage, GpuBufferUsage::Static))
        {
            Buffer = GDevice->newBuffer(BufferSize, MTL::ResourceStorageModePrivate);
        }
        else
        {
            Buffer = GDevice->newBuffer(BufferSize, MTL::ResourceStorageModeShared);
        }
        return new MetalBuffer(NS::TransferPtr(Buffer), Usage);
    }
}
