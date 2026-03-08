#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"
#include "MetalBuffer.h"
#include "MetalGpuRhiBackend.h"

namespace FW
{
	class MetalTextureView : public GpuTextureView
	{
	public:
		MetalTextureView(GpuTextureViewDesc InDesc, MTLTexturePtr InTexView)
			: GpuTextureView(MoveTemp(InDesc))
			, TexView(MoveTemp(InTexView))
		{
			GMtlDeferredReleaseManager.AddResource(this);
		}

		MTL::Texture* GetResource() const { return TexView.get(); }

	private:
		MTLTexturePtr TexView;
	};
    
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
        MetalTexture(MTLTexturePtr InTex, GpuTextureDesc InDesc, GpuResourceState InitState, CVPixelBufferRef InSharedHandle = nullptr);
        
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

	public:
		MetalTextureView* GetMtlDefaultView() const { return static_cast<MetalTextureView*>(DefaultView.GetReference()); }
        
    private:
        MTLTexturePtr Tex;
        CVPixelBufferRef SharedHandle;
	};

    TRefCountPtr<MetalTexture> CreateMetalTexture2D(const GpuTextureDesc& InTexDesc, GpuResourceState InitState);
    TRefCountPtr<MetalTexture> CreateSharedMetalTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState);

    TRefCountPtr<MetalSampler> CreateMetalSampler(const GpuSamplerDesc& InSamplerDesc);
}
