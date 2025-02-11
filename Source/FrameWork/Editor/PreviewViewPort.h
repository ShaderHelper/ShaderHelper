#pragma once
#include "GpuApi/GpuResource.h"
#include <Textures/SlateUpdatableTexture.h>

namespace FW
{
	DECLARE_MULTICAST_DELEGATE_OneParam(OnViewportResizeDelegate, const Vector2f&)

	class FRAMEWORK_API PreviewViewPort : public ISlateViewport
	{
	public:
		PreviewViewPort() : SizeX(1), SizeY(1) {}

	public:
		FIntPoint GetSize() const override
		{
			return { SizeX,SizeY };
		}

		FSlateShaderResource* GetViewportRenderTargetTexture() const override
		{
            if(!ViewPortRT.IsValid()) {
                return nullptr;
                
            }
			return ViewPortRT->GetSlateResource();;
		}

		bool RequiresVsync() const override { return false; }

		void SetViewPortRenderTexture(GpuTexture* InGpuTex);
	//	void UpdateViewPortRenderTexture(GpuTexture* InGpuTex);
		void OnDrawViewport(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) override;
        
        void Clear() { ViewPortRT.Reset(); }
	public:
		OnViewportResizeDelegate OnViewportResize;
		
	private:
		TSharedPtr<FSlateUpdatableTexture> ViewPortRT;
		int32 SizeX;
		int32 SizeY;
	};
}
