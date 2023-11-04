#pragma once
#include "GpuApi/GpuResource.h"
#include "MetalCommon.h"

namespace FRAMEWORK
{
    class MetalBuffer : public GpuBuffer
    {
    public:
        MetalBuffer(mtlpp::Buffer InBuffer, GpuBufferUsage Usage)
            : GpuBuffer(Usage)
            , Buffer(MoveTemp(InBuffer))
        {}
        
    public:
        id<MTLBuffer> GetResource() const { return Buffer.GetPtr(); }
        void* GetContents() {
            void* Data = Buffer.GetContents();
            check(Data);
            return Data;
        }
        
    private:
        mtlpp::Buffer Buffer;
    };

    TRefCountPtr<MetalBuffer> CreateMetalBuffer(uint64 BufferSize, GpuBufferUsage Usage);
}
