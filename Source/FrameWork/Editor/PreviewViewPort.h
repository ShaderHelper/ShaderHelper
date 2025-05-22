#pragma once
#include "GpuApi/GpuResource.h"
#include <Textures/SlateUpdatableTexture.h>

namespace FW
{
	DECLARE_MULTICAST_DELEGATE_OneParam(OnViewportResizeDelegate, const Vector2f&)
	DECLARE_MULTICAST_DELEGATE_TwoParams(OnMouseDownDelegate, const FGeometry&, const FPointerEvent&)
	DECLARE_MULTICAST_DELEGATE_TwoParams(OnMouseUpDelegate, const FGeometry&, const FPointerEvent&)
	DECLARE_MULTICAST_DELEGATE_TwoParams(OnMouseMoveDelegate, const FGeometry&, const FPointerEvent&)

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
		
		void SetAssociatedWidget(TWeakPtr<SWidget> InWidget) { AssociatedWidget = MoveTemp(InWidget); }

		void SetViewPortRenderTexture(GpuTexture* InGpuTex);
	//	void UpdateViewPortRenderTexture(GpuTexture* InGpuTex);
		void OnDrawViewport(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) override;
        
        void Clear() { ViewPortRT.Reset(); }

		Vector4f GetiMouse() const { return iMouse; }

		FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseDown.IsBound())
			{
				MouseDown.Broadcast(MyGeometry, MouseEvent);
			}
			iMouse.xy = (Vector2f)(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()) * MyGeometry.Scale);
			iMouse.zw = iMouse.xy;
			if(AssociatedWidget.IsValid())
			{
				return FReply::Handled().CaptureMouse(AssociatedWidget.Pin().ToSharedRef());
			}
			return FReply::Handled();
		}

		FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseUp.IsBound())
			{
				MouseUp.Broadcast(MyGeometry, MouseEvent);
			}
			if(iMouse.z > 0)
			{
				iMouse.z = -iMouse.z;
			}
			if(iMouse.w > 0)
			{
				iMouse.w = -iMouse.w;
			}
			if(AssociatedWidget.IsValid())
			{
				return FReply::Handled().ReleaseMouseCapture();
			}
			return FReply::Handled();
		}

		FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseMove.IsBound())
			{
				MouseMove.Broadcast(MyGeometry, MouseEvent);
			}
			if (iMouse.z > 0)
			{
				iMouse.xy = (Vector2f)(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()) * MyGeometry.Scale);
				if (iMouse.w > 0)
				{
					iMouse.w = -iMouse.w;
				}
			}
			return FReply::Handled();
		}

	public:
		OnViewportResizeDelegate ViewportResize;
		OnMouseDownDelegate MouseDown;
		OnMouseUpDelegate MouseUp;
		OnMouseMoveDelegate MouseMove;

	private:
		TWeakPtr<SWidget> AssociatedWidget;
		TSharedPtr<FSlateUpdatableTexture> ViewPortRT;
		int32 SizeX;
		int32 SizeY;
		Vector4f iMouse{};
	};
}
