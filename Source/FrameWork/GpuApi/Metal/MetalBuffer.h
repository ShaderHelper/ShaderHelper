#pragma once
#include "GpuApi/GpuResource.h"
#include "MetalCommon.h"

namespace FRAMEWORK
{
    class MetalBuffer : public GpuBuffer
    {
    public:
        MetalBuffer(MTLBufferPtr InBuffer, GpuBufferUsage Usage)
            : GpuBuffer(Usage)
            , Buffer(MoveTemp(InBuffer))
        {}
        
    public:
        id<MTLBuffer> GetResource() const { return (id<MTLBuffer>)Buffer.get(); }
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
