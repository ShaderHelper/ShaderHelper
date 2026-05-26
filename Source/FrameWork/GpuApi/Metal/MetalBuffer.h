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
        MTL::Texture* GetTypedView() const { return TypedView.get(); }
        void SetTypedView(MTLTexturePtr InView) { TypedView = MoveTemp(InView); }
        void* GetContents() {
            void* Data = Buffer->contents();
            check(Data);
            return Data;
        }

    public:
        TRefCountPtr<MetalBuffer> ReadBackBuffer;

    private:
        MTLBufferPtr Buffer;
        MTLTexturePtr TypedView;
    };

    TRefCountPtr<MetalBuffer> CreateMetalBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState = GpuResourceState::Unknown);
}
