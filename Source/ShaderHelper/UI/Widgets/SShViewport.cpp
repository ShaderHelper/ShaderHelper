#include "CommonHeader.h"
#include "SShViewport.h"
#include "Editor/PreviewViewPort.h"

using namespace FW;

namespace SH
{
	void SShViewport::Construct(const FArguments& InArgs)
	{
		SViewport::Construct(InArgs);
	}

	void SShViewport::ResetSpliter()
	{
		SpliterFraction = {}; 
		OnSpliterChanged.Reset();
		bIsDraggingSplitter = false;
		DragOffset = 0.0f;
	}

	FCursorReply SShViewport::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
	{
		FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(CursorEvent.GetScreenSpacePosition());
		double SplitterX = MyGeometry.GetLocalSize().X * SpliterFraction.Get();
		bool bNearSplitter = FMath::Abs(LocalMousePos.X - SplitterX) <= SplitterHitTestWidth;
		return bNearSplitter ? FCursorReply::Cursor(EMouseCursor::ResizeLeftRight) : FCursorReply::Unhandled();
	}

	FReply SShViewport::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && SpliterFraction.IsSet())
		{
			FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			double SplitterX = MyGeometry.GetLocalSize().X * SpliterFraction.Get();

			if (FMath::Abs(LocalMousePos.X - SplitterX) <= SplitterHitTestWidth)
			{
				bIsDraggingSplitter = true;
				DragOffset = LocalMousePos.X - SplitterX;
				return FReply::Handled().CaptureMouse(AsShared());
			}
		}

		return SViewport::OnMouseButtonDown(MyGeometry, MouseEvent);
	}

	FReply SShViewport::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDraggingSplitter)
		{
			bIsDraggingSplitter = false;
			return FReply::Handled().ReleaseMouseCapture();
		}

		return SViewport::OnMouseButtonUp(MyGeometry, MouseEvent);
	}

	FReply SShViewport::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (SpliterFraction.IsSet() && OnSpliterChanged)
		{
			FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			if (bIsDraggingSplitter)
			{
				double NewFraction = (LocalMousePos.X - DragOffset) / MyGeometry.GetLocalSize().X;
				NewFraction = FMath::Clamp(NewFraction, 0.0, 1.0);
				if (NewFraction != SpliterFraction.Get())
				{
					OnSpliterChanged((float)NewFraction);
				}
				return FReply::Handled();
			}
		}
		return SViewport::OnMouseMove(MyGeometry, MouseEvent);
	}

	void SShViewport::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		if (auto VP = StaticCastSharedPtr<PreviewViewPort>(GetViewportInterface().Pin()))
		{
			if (VP->DragEnterHandler.IsBound())
				VP->DragEnterHandler.Execute(MyGeometry, DragDropEvent);
		}
	}

	void SShViewport::OnDragLeave(const FDragDropEvent& DragDropEvent)
	{
		if (auto VP = StaticCastSharedPtr<PreviewViewPort>(GetViewportInterface().Pin()))
		{
			if (VP->DragLeaveHandler.IsBound())
				VP->DragLeaveHandler.Execute(DragDropEvent);
		}
	}

	FReply SShViewport::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		if (auto VP = StaticCastSharedPtr<PreviewViewPort>(GetViewportInterface().Pin()))
		{
			if (VP->DropHandler.IsBound())
				return VP->DropHandler.Execute(MyGeometry, DragDropEvent);
		}
		return FReply::Unhandled();
	}

	int32 SShViewport::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
	{
		LayerId = SViewport::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
		if (SpliterFraction.IsSet())
		{
			FVector2D P0 = { AllottedGeometry.GetLocalSize().X * SpliterFraction.Get(), 0};
			FVector2D P1 = { AllottedGeometry.GetLocalSize().X * SpliterFraction.Get(), AllottedGeometry.GetLocalSize().Y };
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				{ P0, P1 },
				ESlateDrawEffect::None,
				FLinearColor::White,
				true,
				1.5f
			);
		}
		return LayerId;
	}
}
