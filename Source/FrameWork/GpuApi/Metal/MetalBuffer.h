#pragma once
#include "GpuApi/GpuResource.h"
#include "MetalCommon.h"

namespace FW
{
    class MetalBuffer : public GpuBuffer
    {
    public:
        MetalBuffer(MTLBufferPtr InBuffer, GpuBufferUsage Usage)
            : GpuBuffer(Usage)
            , Buffer(MoveTemp(InBuffer))
        {}
        
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

    TRefCountPtr<MetalBuffer> CreateMetalBuffer(uint64 BufferSize, GpuBufferUsage Usage);
}
