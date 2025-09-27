#pragma once
#include "GpuApi/GpuResource.h"
#include <Textures/SlateUpdatableTexture.h>

namespace FW
{
	DECLARE_MULTICAST_DELEGATE_OneParam(OnViewportResizeDelegate, const Vector2f&)
	DECLARE_MULTICAST_DELEGATE_OneParam(OnFocusLostDelegate, const FFocusEvent&)
	DECLARE_MULTICAST_DELEGATE_TwoParams(OnMouseDownDelegate, const FGeometry&, const FPointerEvent&)
	DECLARE_MULTICAST_DELEGATE_TwoParams(OnMouseUpDelegate, const FGeometry&, const FPointerEvent&)
	DECLARE_MULTICAST_DELEGATE_TwoParams(OnMouseMoveDelegate, const FGeometry&, const FPointerEvent&)
	DECLARE_MULTICAST_DELEGATE_TwoParams(OnKeyDownDelegate, const FGeometry&, const FKeyEvent&)
	DECLARE_MULTICAST_DELEGATE_TwoParams(OnKeyUpDelegate, const FGeometry&, const FKeyEvent&)

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
		TWeakPtr<SWidget> GetAssociatedWidget() const { return AssociatedWidget; }

		void SetViewPortRenderTexture(GpuTexture* InGpuTex);
	//	void UpdateViewPortRenderTexture(GpuTexture* InGpuTex);
		void OnDrawViewport(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) override;
        
        void Clear() { ViewPortRT.Reset(); }

		void OnFocusLost(const FFocusEvent& InFocusEvent) override 
		{
			if (FocusLostHandler.IsBound())
			{
				FocusLostHandler.Broadcast(InFocusEvent);
			}
		}

		FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
		{
			if (KeyDownHandler.IsBound())
			{
				KeyDownHandler.Broadcast(MyGeometry, InKeyEvent);
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}

		FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
		{
			if (KeyUpHandler.IsBound())
			{
				KeyUpHandler.Broadcast(MyGeometry, InKeyEvent);
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}

		FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			FReply Reply = FReply::Unhandled();
			if (MouseDownHandler.IsBound())
			{
				MouseDownHandler.Broadcast(MyGeometry, MouseEvent);
				Reply = FReply::Handled();
			}
			if(AssociatedWidget.IsValid())
			{
				Reply.CaptureMouse(AssociatedWidget.Pin().ToSharedRef());
			}
			return Reply;
		}

		FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			FReply Reply = FReply::Unhandled();
			if (MouseUpHandler.IsBound())
			{
				MouseUpHandler.Broadcast(MyGeometry, MouseEvent);
				Reply = FReply::Handled();
			}
			if(AssociatedWidget.IsValid())
			{
				Reply.ReleaseMouseCapture();
			}
			return Reply;
		}

		FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseMoveHandler.IsBound())
			{
				MouseMoveHandler.Broadcast(MyGeometry, MouseEvent);
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}

	public:
		OnViewportResizeDelegate ResizeHandler;
		OnFocusLostDelegate FocusLostHandler;
		OnKeyDownDelegate KeyDownHandler;
		OnKeyUpDelegate KeyUpHandler;
		OnMouseDownDelegate MouseDownHandler;
		OnMouseUpDelegate MouseUpHandler;
		OnMouseMoveDelegate MouseMoveHandler;

	private:
		TWeakPtr<SWidget> AssociatedWidget;
		TSharedPtr<FSlateUpdatableTexture> ViewPortRT;
		int32 SizeX;
		int32 SizeY;
	};
}
