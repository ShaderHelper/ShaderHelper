#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"
#include "MetalBuffer.h"

namespace FW
{
    
    class MetalSampler : public GpuSampler
    {
    public:
        MetalSampler(MTLSamplerStatePtr InSampler, const GpuSamplerDesc& InDesc);
        
        MTL::SamplerState* GetResource() const {
            return Sampler.get();
        }
        
    private:
        MTLSamplerStatePtr Sampler;
    };

	class MetalTexture : public GpuTexture
	{
    public:
        MetalTexture(MTLTexturePtr InTex, GpuTextureDesc InDesc, CVPixelBufferRef InSharedHandle = nullptr);
        
        ~MetalTexture()
        {
            CVPixelBufferRelease(SharedHandle);
        }
        
    public:
        MTL::Texture* GetResource() const {
            return Tex.get();
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
