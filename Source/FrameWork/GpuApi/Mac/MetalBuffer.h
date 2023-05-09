#pragma once
#include "GpuApi/GpuResource.h"
#include "MetalCommon.h"

namespace FRAMEWORK
{
    class MetalBuffer : public GpuBuffer
    {
    public:
        MetalBuffer(mtlpp::Buffer InBuffer) : Buffer(MoveTemp(InBuffer)) {}
        
    public:
        id<MTLBuffer> GetResource() const { return Buffer.GetPtr(); }
        
    private:
        mtlpp::Buffer Buffer;
    };

    TRefCountPtr<MetalBuffer> CreateMetalBuffer(uint64 BufferSize);
}
