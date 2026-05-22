#pragma once
#include "Editor/PreviewViewPort.h"
#include "Debugger/DebuggableObject.h"

namespace SH
{
	class SFragmentDebuggerViewport : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFragmentDebuggerViewport){}
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs);
		
		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
		
		void SetDebugTarget(const DebugTargetInfo& InTarget, bool GlobalValidation);
		void Draw() const;
		
		FW::Vector2i GetPixelCoord() const { return PixelCoord; }
		bool FinalizedPixel() const { return bFinalizePixel; }
		
	private:
		struct CoverageOutlineRect
		{
			int32 X;
			int32 Y;
			int32 Width;
			int32 Height;
		};

		void SelectOutput(int32 OutputIndex);
		void CreateDebuggerTextures();
		void DrawOutput(FW::GpuTexture* InputTex, FW::GpuTexture* OutputTex) const;
		FW::GpuTexture* GetSelectedOutputTexture() const;
		FW::GpuTexture* GetSelectedDebuggerTexture() const;
		void ReadCurrentTargetData();
		void BuildCoverageOutlineRects(uint32 Width, uint32 Height);
		void ClampOffset();
		bool IsPixelInBounds(const FW::Vector2i& InPixelCoord) const;
		bool IsPixelDebuggable(const FW::Vector2i& InPixelCoord) const;
		FW::Vector2i MouseToPixel(const FW::Vector2f& InMouseLoc) const;

	private:
		DebugTargetInfo TargetInfo;
		TArray<TSharedPtr<FString>> OutputOptions;
		TArray<FW::Vector4f> TexDatas;
		TArray<uint8> CoverageDatas;
		TArray<CoverageOutlineRect> CoverageOutlineRects;
		TArray<TRefCountPtr<FW::GpuTexture>> DebuggerTextures;
		TSharedPtr<FW::PreviewViewPort> ViewPort;
		FW::Vector2f Offset{};
		float Zoom = 1.0f;

		FW::Vector2f MouseLoc{};
		float DpiScale = 1.0f;
		FW::Vector2i PixelCoord{-1};
		
		bool bFinalizePixel{};
		bool IsValidating{};
	};
}
