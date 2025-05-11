#include "CommonHeader.h"
#include "MetalBuffer.h"
#include "MetalDevice.h"
#include "MetalGpuRhiBackend.h"

namespace FW
{
    MetalBuffer::MetalBuffer(MTLBufferPtr InBuffer, const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState)
    : GpuBuffer(InBufferDesc, InResourceState)
    , Buffer(MoveTemp(InBuffer))
    {
        GDeferredReleaseOneFrame.Add(this);
    }

    TRefCountPtr<MetalBuffer> CreateMetalBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState)
    {
        MTL::Buffer* Buffer = nullptr;
        if(EnumHasAnyFlags(InBufferDesc.Usage, GpuBufferUsage::StaticMask))
        {
            Buffer = GDevice->newBuffer(InBufferDesc.BufferSize, MTL::ResourceStorageModePrivate);
        }
        else
        {
            Buffer = GDevice->newBuffer(InBufferDesc.BufferSize, MTL::ResourceStorageModeShared);
        }
        return new MetalBuffer(NS::TransferPtr(Buffer), InBufferDesc, InResourceState);
    }
}
