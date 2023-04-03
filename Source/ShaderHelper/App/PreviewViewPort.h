#pragma once
#include "GpuApi/GpuResource.h"
namespace SH
{
	class PreviewViewPort : public ISlateViewport
	{
	public:
		PreviewViewPort(int32 InSizeX, int32 InSizeY) : SizeX(InSizeX), SizeY(InSizeY) {}

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
