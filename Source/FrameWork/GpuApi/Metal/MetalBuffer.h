#pragma once
#include "GpuApi/GpuResource.h"
#include "MetalCommon.h"

namespace FW
{
    class MetalBuffer : public GpuBuffer
    {
    public:
        MetalBuffer(MTLBufferPtr InBuffer, const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState);
        
    public:
        MTL::Buffer* GetResource() const { return Buffer.get(); }
        void* GetContents() {
            void* Data = Buffer->contents();
            check(Data);
            return Data;
        }
        
    private:
        MTLBufferPtr Buffer;
    };

    TRefCountPtr<MetalBuffer> CreateMetalBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState = GpuResourceState::Unknown);
}
