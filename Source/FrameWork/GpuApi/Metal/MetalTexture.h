#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"
#include "MetalBuffer.h"

namespace FRAMEWORK
{
    
    class MetalSampler : public GpuSampler
    {
    public:
        MetalSampler(mtlpp::SamplerState InSampler)
            : Sampler(MoveTemp(InSampler))
        {}
        
        id<MTLSamplerState> GetResource() const {
            return Sampler.GetPtr();
        }
        
    private:
        mtlpp::SamplerState Sampler;
    };

	class MetalTexture : public GpuTexture
	{
    public:
        MetalTexture(mtlpp::Texture InTex, GpuTextureDesc InDesc, CVPixelBufferRef InSharedHandle = nullptr)
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
            return Tex.GetPtr();
        }
        
        CVPixelBufferRef GetSharedHandle() const {
            check(SharedHandle);
            return SharedHandle;
        }
        
    public:
        TRefCountPtr<MetalBuffer> UploadBuffer;
        TRefCountPtr<MetalBuffer> ReadBackBuffer;
        
    private:
        mtlpp::Texture Tex;
        CVPixelBufferRef SharedHandle;
	};

    TRefCountPtr<MetalTexture> CreateMetalTexture2D(const GpuTextureDesc& InTexDesc);
    TRefCountPtr<MetalTexture> CreateSharedMetalTexture(const GpuTextureDesc& InTexDesc);

    TRefCountPtr<MetalSampler> CreateMetalSampler(const GpuSamplerDesc& InSamplerDesc);
}
