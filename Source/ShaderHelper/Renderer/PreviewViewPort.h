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

		void SetViewPortRenderTexture(TRefCountPtr<GpuTexture> InGpuTex);
	private:
		TSharedPtr<FSlateShaderResource> ViewPortRT;
		int32 SizeX;
		int32 SizeY;
	};
}
