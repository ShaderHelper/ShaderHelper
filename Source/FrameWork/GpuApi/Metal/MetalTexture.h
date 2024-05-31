#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"
#include "MetalBuffer.h"

namespace FRAMEWORK
{
    
    class MetalSampler : public GpuSampler
    {
    public:
        MetalSampler(MTLSamplerStatePtr InSampler)
            : Sampler(MoveTemp(InSampler))
        {}
        
        id<MTLSamplerState> GetResource() const {
            return (id<MTLSamplerState>)Sampler.get();
        }
        
    private:
        MTLSamplerStatePtr Sampler;
    };

	class MetalTexture : public GpuTexture
	{
    public:
        MetalTexture(MTLTexturePtr InTex, GpuTextureDesc InDesc, CVPixelBufferRef InSharedHandle = nullptr)
            : GpuTexture(MoveTemp(InDesc))
            , Tex(MoveTemp(InTex))
            , SharedHandle(InSharedHandle)
        {}
        ~MetalTexture()
        {
            CVPixelBufferRelease(SharedHandle);
        }
        
    public:
        id<MTLTexture> GetResource() const {
            return (id<MTLTexture>)Tex.get();
        }
        
        CVPixelBufferRef GetSharedHandle() const {
            check(SharedHandle);
            return SharedHandle;
        }
        
    public:
        TRefCountPtr<MetalBuffer> UploadBuffer;
        TRefCountPtr<MetalBuffer> ReadBackBuffer;
        
    private:
        MTLTexturePtr Tex;
        CVPixelBufferRef SharedHandle;
	};

    TRefCountPtr<MetalTexture> CreateMetalTexture2D(const GpuTextureDesc& InTexDesc);
    TRefCountPtr<MetalTexture> CreateSharedMetalTexture(const GpuTextureDesc& InTexDesc);

    TRefCountPtr<MetalSampler> CreateMetalSampler(const GpuSamplerDesc& InSamplerDesc);
}
