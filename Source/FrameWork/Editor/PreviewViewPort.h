#pragma once
#include "GpuApi/GpuResource.h"
#include <Textures/SlateUpdatableTexture.h>

namespace FW
{
	DECLARE_MULTICAST_DELEGATE_OneParam(OnViewportResizeDelegate, const Vector2f&)
	DECLARE_MULTICAST_DELEGATE_OneParam(OnMouseDownDelegate, const FPointerEvent&)
	DECLARE_MULTICAST_DELEGATE_OneParam(OnMouseUpDelegate, const FPointerEvent&)
	DECLARE_MULTICAST_DELEGATE_OneParam(OnMouseMoveDelegate, const FPointerEvent&)

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

		FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseDown.IsBound())
			{
				MouseDown.Broadcast(MouseEvent);
			}
			return FReply::Unhandled();
		}

		FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseUp.IsBound())
			{
				MouseUp.Broadcast(MouseEvent);
			}
			return FReply::Unhandled();
		}

		FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseMove.IsBound())
			{
				MouseMove.Broadcast(MouseEvent);
			}
			return FReply::Unhandled();
		}

	public:
		OnViewportResizeDelegate ViewportResize;
		OnMouseDownDelegate MouseDown;
		OnMouseUpDelegate MouseUp;
		OnMouseMoveDelegate MouseMove;

	private:
		TSharedPtr<FSlateUpdatableTexture> ViewPortRT;
		int32 SizeX;
		int32 SizeY;
	};
}
