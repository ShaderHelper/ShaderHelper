#include "CommonHeader.h"
#include "MetalBuffer.h"
#include "MetalDevice.h"

namespace FRAMEWORK
{
    TRefCountPtr<MetalBuffer> CreateMetalBuffer(uint64 BufferSize)
    {
        mtlpp::Buffer Buffer = GDevice.NewBuffer(BufferSize, mtlpp::ResourceOptions::StorageModeShared);
        return new MetalBuffer(MoveTemp(Buffer));
    }
}
