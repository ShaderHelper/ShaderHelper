#pragma once
#include "GpuApi/GpuResource.h"
namespace SH
{
	class PreViewPort : public ISlateViewport
	{
	public:
		PreViewPort() : SizeX(0), SizeY(0) {}
	public:
		FIntPoint GetSize() const override
		{
			return { SizeX,SizeY };
		}

		FSlateShaderResource* GetViewportRenderTargetTexture() const override
		{
			return ViewPortRT.Get();
		}

		void SetViewPortRenderTexture(TRefCountPtr<GpuTextureResource> InGpuTex);
	private:
		TSharedPtr<FSlateShaderResource> ViewPortRT;
		uint32 SizeX;
		uint32 SizeY;
	};
}
