#pragma once
#include "GpuApi/GpuResource.h"
#include <Textures/SlateUpdatableTexture.h>

namespace SH
{
	DECLARE_MULTICAST_DELEGATE(OnViewportResizeDelegate)

	class PreviewViewPort : public ISlateViewport
	{
	public:
		PreviewViewPort() : SizeX(0), SizeY(0) {}

	public:
		FIntPoint GetSize() const override
		{
			return { SizeX,SizeY };
		}

		FSlateShaderResource* GetViewportRenderTargetTexture() const override
		{
			return ViewPortRT->GetSlateResource();;
		}

		bool RequiresVsync() const override { return false; }

		void SetViewPortRenderTexture(GpuTexture* InGpuTex);
		void UpdateViewPortRenderTexture(GpuTexture* InGpuTex);
		void OnDrawViewport(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) override;
	public:
		OnViewportResizeDelegate OnViewportResize;
		
	private:
		TSharedPtr<FSlateUpdatableTexture> ViewPortRT;
		int32 SizeX;
		int32 SizeY;
	};
}
