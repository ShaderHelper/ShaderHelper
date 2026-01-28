#pragma once
#include "Editor/PreviewViewPort.h"
#include "Debugger/DebuggableObject.h"

namespace SH
{
	class SDebuggerViewport : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDebuggerViewport){}
		SLATE_END_ARGS()
		void Construct( const FArguments& InArgs );
		
		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
		
		void SetDebugTarget(const DebugTargetInfo& InTarget, bool GlobalValidation);
		void Draw() const;
		
		FW::Vector2i GetPixelCoord() const {
			return PixelCoord;
		}
		bool FinalizedPixel() const { return bFinalizePixel; }
		
	private:
		TArray<FW::Vector4f> TexDatas;
		TRefCountPtr<FW::GpuTexture> RawTex;
		TRefCountPtr<FW::GpuTexture> DebuggerTex;
		TSharedPtr<FW::PreviewViewPort> ViewPort;
		FW::Vector2f Offset{};
		uint32 Zoom = 1;

		FW::Vector2f MouseLoc{};
		float DpiScale{};
		FW::Vector2i PixelCoord{-1};
		
		bool bFinalizePixel{};
		bool IsValidating{};
	};
}
