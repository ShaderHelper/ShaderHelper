#include "CommonHeader.h"
#include "SFragmentDebuggerViewport.h"
#include "Editor/ShaderHelperEditor.h"
#include "GpuApi/GpuResourceHelper.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "RenderResource/RenderPass/BlitPass.h"
#include "Renderer/RenderGraph.h"

#include <Widgets/SCanvas.h>
#include <Widgets/SViewport.h>
#include <Widgets/Input/SComboBox.h>
#include <Framework/Notifications/NotificationManager.h>
#include <Widgets/Notifications/SNotificationList.h>
#include <Math/Float16Color.h>

#include <stdexcept>

using namespace FW;

namespace SH
{
	namespace
	{
		constexpr float MinDebuggerZoom = 1.0f / 32.0f;
		constexpr float MaxDebuggerZoom = 128.0f;
		constexpr float DebuggerWheelZoomBase = 1.2f;

		bool IsDepthTexture(const GpuTexture* Texture)
		{
			return Texture && IsDepthFormat(Texture->GetFormat());
		}
	}

	void SFragmentDebuggerViewport::Construct(const FArguments& InArgs)
	{
		ViewPort = MakeShared<PreviewViewPort>();
		
		auto InfoTip = SNew(SBorder)
		.Padding(4)
		.BorderImage(FAppStyle::Get().GetBrush("ToolTip.Background"))
		.BorderBackgroundColor(FLinearColor{1,1,1,0.2f})
		.Visibility_Lambda([this]{
			return (IsPixelInBounds(PixelCoord) && GetSelectedDebuggerTexture()) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.Padding(0, 0, 0, 8)
			[
				SNew(STextBlock).ColorAndOpacity(FLinearColor::White).Text_Lambda([this]{
					return FText::FromString(FString::Printf(TEXT("Pixel Coord: %d %d"), PixelCoord.x, PixelCoord.y));
				})
			]
			+SVerticalBox::Slot()
			.Padding(0, 0, 0, 2)
			[
				SNew(STextBlock).ColorAndOpacity(FLinearColor::White).Text_Lambda([this]{
					GpuTexture* SelectedTex = GetSelectedOutputTexture();
					uint32 DataIndex = SelectedTex ? PixelCoord.x + PixelCoord.y * SelectedTex->GetWidth() : 0;
					return FText::FromString(FString::Printf(TEXT("R Channel: %f"), TexDatas.IsValidIndex(DataIndex) ? TexDatas[DataIndex].x : 0.0f));
				})
			]
			+SVerticalBox::Slot()
			.Padding(0, 0, 0, 2)
			[
				SNew(STextBlock).ColorAndOpacity(FLinearColor::White).Text_Lambda([this]{
					GpuTexture* SelectedTex = GetSelectedOutputTexture();
					uint32 DataIndex = SelectedTex ? PixelCoord.x + PixelCoord.y * SelectedTex->GetWidth() : 0;
					return FText::FromString(FString::Printf(TEXT("G Channel: %f"), TexDatas.IsValidIndex(DataIndex) ? TexDatas[DataIndex].y : 0.0f));
				})
			]
			+SVerticalBox::Slot()
			.Padding(0, 0, 0, 2)
			[
				SNew(STextBlock).ColorAndOpacity(FLinearColor::White).Text_Lambda([this]{
					GpuTexture* SelectedTex = GetSelectedOutputTexture();
					uint32 DataIndex = SelectedTex ? PixelCoord.x + PixelCoord.y * SelectedTex->GetWidth() : 0;
					return FText::FromString(FString::Printf(TEXT("B Channel: %f"), TexDatas.IsValidIndex(DataIndex) ? TexDatas[DataIndex].z : 0.0f));
				})
			]
			+SVerticalBox::Slot()
			.Padding(0, 0, 0, 2)
			[
				SNew(STextBlock).ColorAndOpacity(FLinearColor::White).Text_Lambda([this]{
					GpuTexture* SelectedTex = GetSelectedOutputTexture();
					uint32 DataIndex = SelectedTex ? PixelCoord.x + PixelCoord.y * SelectedTex->GetWidth() : 0;
					return FText::FromString(FString::Printf(TEXT("A Channel: %f"), TexDatas.IsValidIndex(DataIndex) ? TexDatas[DataIndex].w : 0.0f));
				})
			]
		];

		ChildSlot
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SNew(SCanvas)
				.Clipping(EWidgetClipping::ClipToBounds)
				+SCanvas::Slot()
				.Position_Lambda([this] {
					return FVector2D(-Offset.x, -Offset.y) / DpiScale;
				})
				.Size_Lambda([this] {
					GpuTexture* SelectedTex = GetSelectedOutputTexture();
					return SelectedTex ? FVector2D(SelectedTex->GetWidth(), SelectedTex->GetHeight()) * Zoom / DpiScale : FVector2D::ZeroVector;
				})
				[
					SNew(SViewport)
					.ViewportInterface(ViewPort)
				]
			]
			+SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.Padding(8.0f)
			[
				SNew(SBox)
				.WidthOverride(160.0f)
				.Visibility_Lambda([this] {
					return OutputOptions.Num() > 1 ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&OutputOptions)
					.OnSelectionChanged_Lambda([this](TSharedPtr<FString> InItem, ESelectInfo::Type) {
						if (!InItem)
						{
							return;
						}
						for (int32 Index = 0; Index < OutputOptions.Num(); ++Index)
						{
							if (OutputOptions[Index] == InItem)
							{
								SelectOutput(Index);
								break;
							}
						}
					})
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem) {
						return SNew(STextBlock).Text(FText::FromString(InItem ? *InItem : FString()));
					})
					[
						SNew(STextBlock)
						.Text_Lambda([this] {
							if (OutputOptions.IsValidIndex(TargetInfo.SelectedOutputIndex))
							{
								return FText::FromString(*OutputOptions[TargetInfo.SelectedOutputIndex]);
							}
							return FText::GetEmpty();
						})
					]
				]
			]
			+SOverlay::Slot()
			[
				SNew(SCanvas)
				+SCanvas::Slot()
				.Size_Lambda([InfoTip]{
					return InfoTip->GetDesiredSize();
				})
				.Position_Lambda([this, InfoTip]{
					FVector2D ViewportSize = GetCachedGeometry().GetLocalSize();
					FVector2D InfoTipSize = InfoTip->GetCachedGeometry().GetLocalSize();
					FVector2D MousePos = FVector2D{ MouseLoc.x, MouseLoc.y } / DpiScale;

					FVector2D TipOffset = FVector2D(8, 8);
					FVector2D Pos = MousePos + TipOffset;

					if (Pos.X + InfoTipSize.X > ViewportSize.X)
						Pos.X = MousePos.X - InfoTipSize.X - TipOffset.X;
					if (Pos.Y + InfoTipSize.Y > ViewportSize.Y)
						Pos.Y = MousePos.Y - InfoTipSize.Y - TipOffset.Y;

					return Pos;
				})
				[
					InfoTip
				]
			]
		];
	}

	Vector2i SFragmentDebuggerViewport::MouseToPixel(const Vector2f& InMouseLoc) const
	{
		GpuTexture* SelectedTex = GetSelectedOutputTexture();
		if (!SelectedTex || Zoom <= 0.0f)
		{
			return { -1 };
		}

		Vector2i Result;
		Result.x = FMath::FloorToInt((InMouseLoc.x + Offset.x) / Zoom);
		Result.y = FMath::FloorToInt((InMouseLoc.y + Offset.y) / Zoom);
		return IsPixelInBounds(Result) ? Result : Vector2i{ -1 };
	}

	bool SFragmentDebuggerViewport::IsPixelInBounds(const Vector2i& InPixelCoord) const
	{
		GpuTexture* SelectedTex = GetSelectedOutputTexture();
		return SelectedTex && InPixelCoord.x >= 0 && InPixelCoord.y >= 0
			&& InPixelCoord.x < int32(SelectedTex->GetWidth())
			&& InPixelCoord.y < int32(SelectedTex->GetHeight());
	}

	bool SFragmentDebuggerViewport::IsPixelDebuggable(const Vector2i& InPixelCoord) const
	{
		if (!IsPixelInBounds(InPixelCoord))
		{
			return false;
		}
		if (!TargetInfo.CoverageMask || CoverageDatas.IsEmpty())
		{
			return true;
		}
		GpuTexture* SelectedTex = GetSelectedOutputTexture();
		const uint32 DataIndex = InPixelCoord.x + InPixelCoord.y * SelectedTex->GetWidth();
		return CoverageDatas.IsValidIndex(DataIndex) && CoverageDatas[DataIndex] > 0;
	}

	void SFragmentDebuggerViewport::ClampOffset()
	{
		GpuTexture* SelectedTex = GetSelectedOutputTexture();
		if (!SelectedTex)
		{
			Offset = {};
			return;
		}

		const float ContentW = float(SelectedTex->GetWidth()) * Zoom;
		const float ContentH = float(SelectedTex->GetHeight()) * Zoom;
		const FVector2D ViewSize = GetCachedGeometry().GetLocalSize() * DpiScale;
		const float ViewW = float(FMath::Max(ViewSize.X, 1.0));
		const float ViewH = float(FMath::Max(ViewSize.Y, 1.0));
		Offset.x = FMath::Clamp(Offset.x, -ViewW, ContentW);
		Offset.y = FMath::Clamp(Offset.y, -ViewH, ContentH);
	}

	FReply SFragmentDebuggerViewport::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if(bFinalizePixel || IsValidating)
		{
			return FReply::Unhandled();
		}
		
		Vector2f CurMousePos = (Vector2f)(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()) * DpiScale);
		if(MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
		{
			Offset += (MouseLoc - CurMousePos);
			ClampOffset();
		}
		MouseLoc = CurMousePos;
		PixelCoord = MouseToPixel(MouseLoc);
		return FReply::Handled();
	}

	FReply SFragmentDebuggerViewport::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if(bFinalizePixel || IsValidating)
		{
			return FReply::Unhandled();
		}
		
		Vector2f CurMousePos = (Vector2f)(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()) * DpiScale);
		Vector2f ContentPosBefore = (CurMousePos + Offset) / Zoom;
		const float ZoomFactor = FMath::Pow(DebuggerWheelZoomBase, MouseEvent.GetWheelDelta());
		Zoom = FMath::Clamp(Zoom * ZoomFactor, MinDebuggerZoom, MaxDebuggerZoom);
		Offset = ContentPosBefore * Zoom - CurMousePos;
		ClampOffset();
		MouseLoc = CurMousePos;
		PixelCoord = MouseToPixel(MouseLoc);
		return FReply::Handled();
	}

	FReply SFragmentDebuggerViewport::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if(bFinalizePixel || IsValidating)
		{
			return FReply::Unhandled();
		}
		
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if(MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			ShEditor->EndDebugging();
			return FReply::Handled().ReleaseMouseLock();
		}
		else if(MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			PixelCoord = MouseToPixel(MouseLoc);
			if (!IsPixelDebuggable(PixelCoord))
			{
				return FReply::Handled();
			}
			bFinalizePixel = true;
			ShEditor->GetDebuggaleObject()->OnFinalizePixel(Vector2u(uint32(PixelCoord.x), uint32(PixelCoord.y)));
			return FReply::Handled().ReleaseMouseLock();
		}
		return FReply::Handled();
	}

	void SFragmentDebuggerViewport::Draw() const
	{
		if(bFinalizePixel || IsValidating)
		{
			return;
		}

		for (int32 Index = 0; Index < TargetInfo.Outputs.Num(); ++Index)
		{
			if (!TargetInfo.Outputs[Index].Tex || !DebuggerTextures.IsValidIndex(Index) || !DebuggerTextures[Index])
			{
				continue;
			}
			DrawOutput(TargetInfo.Outputs[Index].Tex.GetReference(), DebuggerTextures[Index].GetReference());
		}
	}

	void SFragmentDebuggerViewport::DrawOutput(GpuTexture* InputTex, GpuTexture* OutputTex) const
	{
		if (!InputTex || !OutputTex)
		{
			return;
		}

		RenderGraph RG;
		BlitPassInput BlitInput;
		BlitInput.InputView = InputTex->GetDefaultView();
		BlitInput.InputTexSampler = GpuResourceHelper::GetSampler({ .Filter = SamplerFilter::Point });
		BlitInput.OutputView = OutputTex->GetDefaultView();
		AddBlitPass(RG, MoveTemp(BlitInput));
		RG.Execute();
	}

	int32 SFragmentDebuggerViewport::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
	{
		Draw();
		int32 MaxLayer = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
		GpuTexture* SelectedTex = GetSelectedOutputTexture();
		if (!SelectedTex || !GetSelectedDebuggerTexture() || DpiScale <= 0.0f || Zoom <= 0.0f)
		{
			return MaxLayer;
		}

		const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
		const float PixelSize = Zoom / DpiScale;
		const FVector2D ContentPos = FVector2D(-Offset.x, -Offset.y) / DpiScale;
		const FVector2D ContentSize = FVector2D(SelectedTex->GetWidth(), SelectedTex->GetHeight()) * PixelSize;
		const double MinX = FMath::Max(0.0, ContentPos.X);
		const double MinY = FMath::Max(0.0, ContentPos.Y);
		const double MaxX = FMath::Min(ViewSize.X, ContentPos.X + ContentSize.X);
		const double MaxY = FMath::Min(ViewSize.Y, ContentPos.Y + ContentSize.Y);
		if (MaxX <= MinX || MaxY <= MinY)
		{
			return MaxLayer;
		}

		const int32 OverlayLayer = MaxLayer + 1;
		int32 MaxOverlayLayer = MaxLayer;
		const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();
		if (PixelSize >= 4.0f)
		{
			MaxOverlayLayer = FMath::Max(MaxOverlayLayer, OverlayLayer);
			const FLinearColor GridColor{0.5f, 0.5f, 0.5f, FMath::Clamp(PixelSize / 128.0f, 0.1f, 0.9f)};
			const int32 StartX = FMath::Max(0, FMath::FloorToInt(Offset.x / Zoom));
			const int32 EndX = FMath::Min(int32(SelectedTex->GetWidth()), FMath::CeilToInt((Offset.x + ViewSize.X * DpiScale) / Zoom));
			for (int32 X = StartX; X <= EndX; ++X)
			{
				const double LineX = ContentPos.X + X * PixelSize;
				if (LineX >= 0.0f && LineX <= ViewSize.X)
				{
					FSlateDrawElement::MakeLines(OutDrawElements, OverlayLayer, PaintGeometry, { FVector2D(LineX, MinY), FVector2D(LineX, MaxY) }, ESlateDrawEffect::None, GridColor, false, 1.0f);
				}
			}

			const int32 StartY = FMath::Max(0, FMath::FloorToInt(Offset.y / Zoom));
			const int32 EndY = FMath::Min(int32(SelectedTex->GetHeight()), FMath::CeilToInt((Offset.y + ViewSize.Y * DpiScale) / Zoom));
			for (int32 Y = StartY; Y <= EndY; ++Y)
			{
				const double LineY = ContentPos.Y + Y * PixelSize;
				if (LineY >= 0.0f && LineY <= ViewSize.Y)
				{
					FSlateDrawElement::MakeLines(OutDrawElements, OverlayLayer, PaintGeometry, { FVector2D(MinX, LineY), FVector2D(MaxX, LineY) }, ESlateDrawEffect::None, GridColor, false, 1.0f);
				}
			}
		}

		if (!CoverageOutlineRects.IsEmpty())
		{
			const int32 MaskLayer = MaxOverlayLayer + 1;
			const FSlateBrush* OutlineBrush = FAppCommonStyle::Get().GetBrush("WhiteBrush");
			const FLinearColor OutlineColor{0.1f, 1.0f, 0.22f, 0.82f};
			for (const CoverageOutlineRect& Rect : CoverageOutlineRects)
			{
				FVector2D RectPos = ContentPos + FVector2D(Rect.X, Rect.Y) * PixelSize;
				FVector2D RectSize = FVector2D(Rect.Width, Rect.Height) * PixelSize;
				if (Rect.Width == 1)
				{
					RectSize.X = FMath::Max(RectSize.X, 1.0f);
				}
				if (Rect.Height == 1)
				{
					RectSize.Y = FMath::Max(RectSize.Y, 1.0f);
				}
				if (RectPos.X >= ViewSize.X || RectPos.Y >= ViewSize.Y || RectPos.X + RectSize.X <= 0.0f || RectPos.Y + RectSize.Y <= 0.0f)
				{
					continue;
				}

				FSlateDrawElement::MakeBox(OutDrawElements, MaskLayer,
					AllottedGeometry.ToPaintGeometry(RectSize, FSlateLayoutTransform(RectPos)),
					OutlineBrush, ESlateDrawEffect::None, OutlineColor);
			}
			MaxOverlayLayer = MaskLayer;
		}

		if (IsPixelInBounds(PixelCoord))
		{
			const FLinearColor HighlightColor{0.678f, 0.847f, 0.902f, 0.9f};
			const FLinearColor CrossColor = IsPixelDebuggable(PixelCoord) ? HighlightColor : HighlightColor.CopyWithNewOpacity(0.4f);
			const FVector2D PixelPos = ContentPos + FVector2D(PixelCoord.x, PixelCoord.y) * PixelSize;
			const double Left = PixelPos.X;
			const double Top = PixelPos.Y;
			const double Right = PixelPos.X + PixelSize;
			const double Bottom = PixelPos.Y + PixelSize;
			const double CenterX = PixelPos.X + PixelSize * 0.5f;
			const double CenterY = PixelPos.Y + PixelSize * 0.5f;
			const int32 CrossLayer = MaxOverlayLayer + 1;
			const int32 BorderLayer = MaxOverlayLayer + 2;
			MaxOverlayLayer = BorderLayer;

			if (CenterX >= MinX && CenterX <= MaxX)
			{
				if (Top > MinY)
				{
					FSlateDrawElement::MakeLines(OutDrawElements, CrossLayer, PaintGeometry, { FVector2D(CenterX, MinY), FVector2D(CenterX, Top) }, ESlateDrawEffect::None, CrossColor, false, 1.0f);
				}
				if (Bottom < MaxY)
				{
					FSlateDrawElement::MakeLines(OutDrawElements, CrossLayer, PaintGeometry, { FVector2D(CenterX, Bottom), FVector2D(CenterX, MaxY) }, ESlateDrawEffect::None, CrossColor, false, 1.0f);
				}
			}
			if (CenterY >= MinY && CenterY <= MaxY)
			{
				if (Left > MinX)
				{
					FSlateDrawElement::MakeLines(OutDrawElements, CrossLayer, PaintGeometry, { FVector2D(MinX, CenterY), FVector2D(Left, CenterY) }, ESlateDrawEffect::None, CrossColor, false, 1.0f);
				}
				if (Right < MaxX)
				{
					FSlateDrawElement::MakeLines(OutDrawElements, CrossLayer, PaintGeometry, { FVector2D(Right, CenterY), FVector2D(MaxX, CenterY) }, ESlateDrawEffect::None, CrossColor, false, 1.0f);
				}
			}

			const double BorderMinX = FMath::Max(Left, MinX);
			const double BorderMinY = FMath::Max(Top, MinY);
			const double BorderMaxX = FMath::Min(Right, MaxX);
			const double BorderMaxY = FMath::Min(Bottom, MaxY);
			if (Left >= MinX && Left <= MaxX && BorderMinY <= BorderMaxY)
			{
				FSlateDrawElement::MakeLines(OutDrawElements, BorderLayer, PaintGeometry, { FVector2D(Left, BorderMinY), FVector2D(Left, BorderMaxY) }, ESlateDrawEffect::None, CrossColor, false, 1.0f);
			}
			if (Top >= MinY && Top <= MaxY && BorderMinX <= BorderMaxX)
			{
				FSlateDrawElement::MakeLines(OutDrawElements, BorderLayer, PaintGeometry, { FVector2D(BorderMinX, Top), FVector2D(BorderMaxX, Top) }, ESlateDrawEffect::None, CrossColor, false, 1.0f);
			}
			if (Right >= MinX && Right <= MaxX && BorderMinY <= BorderMaxY)
			{
				FSlateDrawElement::MakeLines(OutDrawElements, BorderLayer, PaintGeometry, { FVector2D(Right, BorderMinY), FVector2D(Right, BorderMaxY) }, ESlateDrawEffect::None, CrossColor, false, 1.0f);
			}
			if (Bottom >= MinY && Bottom <= MaxY && BorderMinX <= BorderMaxX)
			{
				FSlateDrawElement::MakeLines(OutDrawElements, BorderLayer, PaintGeometry, { FVector2D(BorderMinX, Bottom), FVector2D(BorderMaxX, Bottom) }, ESlateDrawEffect::None, CrossColor, false, 1.0f);
			}
		}

		return MaxOverlayLayer;
	}

	void SFragmentDebuggerViewport::BuildCoverageOutlineRects(uint32 Width, uint32 Height)
	{
		CoverageOutlineRects.Empty();
		if (CoverageDatas.IsEmpty())
		{
			return;
		}

		const int32 TextureWidth = int32(Width);
		const int32 TextureHeight = int32(Height);
		auto IsCovered = [this, TextureWidth, TextureHeight](int32 X, int32 Y) -> bool
		{
			return X >= 0 && Y >= 0 && X < TextureWidth && Y < TextureHeight && CoverageDatas[Y * TextureWidth + X] > 0;
		};
		auto AddRect = [this](int32 X, int32 Y, int32 Width, int32 Height)
		{
			CoverageOutlineRects.Add({ X, Y, Width, Height });
		};

		for (int32 Y = 0; Y < TextureHeight; ++Y)
		{
			int32 TopRunStart = -1;
			int32 BottomRunStart = -1;
			for (int32 X = 0; X <= TextureWidth; ++X)
			{
				const bool bTopEdge = X < TextureWidth && IsCovered(X, Y) && !IsCovered(X, Y - 1);
				const bool bBottomEdge = X < TextureWidth && IsCovered(X, Y) && !IsCovered(X, Y + 1);

				if (bTopEdge && TopRunStart < 0)
				{
					TopRunStart = X;
				}
				else if (!bTopEdge && TopRunStart >= 0)
				{
					AddRect(TopRunStart, Y - 1, X - TopRunStart, 1);
					TopRunStart = -1;
				}

				if (bBottomEdge && BottomRunStart < 0)
				{
					BottomRunStart = X;
				}
				else if (!bBottomEdge && BottomRunStart >= 0)
				{
					AddRect(BottomRunStart, Y + 1, X - BottomRunStart, 1);
					BottomRunStart = -1;
				}
			}
		}

		for (int32 X = 0; X < TextureWidth; ++X)
		{
			int32 LeftRunStart = -1;
			int32 RightRunStart = -1;
			for (int32 Y = 0; Y <= TextureHeight; ++Y)
			{
				const bool bLeftEdge = Y < TextureHeight && IsCovered(X, Y) && !IsCovered(X - 1, Y);
				const bool bRightEdge = Y < TextureHeight && IsCovered(X, Y) && !IsCovered(X + 1, Y);

				if (bLeftEdge && LeftRunStart < 0)
				{
					LeftRunStart = Y;
				}
				else if (!bLeftEdge && LeftRunStart >= 0)
				{
					AddRect(X - 1, LeftRunStart, 1, Y - LeftRunStart);
					LeftRunStart = -1;
				}

				if (bRightEdge && RightRunStart < 0)
				{
					RightRunStart = Y;
				}
				else if (!bRightEdge && RightRunStart >= 0)
				{
					AddRect(X + 1, RightRunStart, 1, Y - RightRunStart);
					RightRunStart = -1;
				}
			}
		}
	}

	void SFragmentDebuggerViewport::ReadCurrentTargetData()
	{
		TexDatas.Empty();
		CoverageDatas.Empty();
		CoverageOutlineRects.Empty();
		GpuTexture* SelectedTex = GetSelectedOutputTexture();
		if (!SelectedTex)
		{
			return;
		}

		const uint32 Width = SelectedTex->GetWidth();
		const uint32 Height = SelectedTex->GetHeight();
		const GpuFormat Format = SelectedTex->GetFormat();
		const uint32 PixelSize = GetFormatByteSize(Format);
		TexDatas.SetNumZeroed(Width * Height);

		uint32 PaddedRowPitch = 0;
		uint8* PaddedData = (uint8*)GGpuRhi->MapGpuTexture(SelectedTex, GpuResourceMapMode::Read_Only, PaddedRowPitch);
		if (PaddedData)
		{
			for (uint32 y = 0; y < Height; ++y)
			{
				const uint8* SrcRow = PaddedData + y * PaddedRowPitch;
				for (uint32 x = 0; x < Width; ++x)
				{
					const uint8* Pixel = SrcRow + x * PixelSize;
					if (Format == GpuFormat::B8G8R8A8_UNORM)
					{
						TexDatas[y * Width + x] = {Pixel[2] / 255.0f, Pixel[1] / 255.0f, Pixel[0] / 255.0f, Pixel[3] / 255.0f};
					}
					else if (Format == GpuFormat::R8G8B8A8_UNORM)
					{
						TexDatas[y * Width + x] = {Pixel[0] / 255.0f, Pixel[1] / 255.0f, Pixel[2] / 255.0f, Pixel[3] / 255.0f};
					}
					else if (Format == GpuFormat::R16_FLOAT)
					{
						FFloat16 Value;
						FMemory::Memcpy(&Value, Pixel, sizeof(Value));
						const float FloatValue = Value.GetFloat();
						TexDatas[y * Width + x] = { FloatValue, 0, 0, 1.0f };
					}
					else if (Format == GpuFormat::R16G16B16A16_FLOAT)
					{
						FFloat16Color Value;
						FMemory::Memcpy(&Value, Pixel, sizeof(Value));
						const FLinearColor Color = Value.GetFloats();
						TexDatas[y * Width + x] = { Color.R, Color.G, Color.B, Color.A };
					}
					else if (Format == GpuFormat::R32_FLOAT || Format == GpuFormat::D32_FLOAT)
					{
						float Value = *reinterpret_cast<const float*>(Pixel);
						TexDatas[y * Width + x] = { Value, 0, 0, 1.0f };
					}
					else if (Format == GpuFormat::R32G32_FLOAT)
					{
						TexDatas[y * Width + x] = { *((float*)Pixel), *((float*)Pixel + 1), 0.0f, 1.0f };
					}
					else if (Format == GpuFormat::R32G32B32A32_FLOAT)
					{
						TexDatas[y * Width + x] = { *((float*)Pixel), *((float*)Pixel + 1), *((float*)Pixel + 2), *((float*)Pixel + 3) };
					}
				}
			}
			GGpuRhi->UnMapGpuTexture(SelectedTex);
		}

		if (!TargetInfo.CoverageMask)
		{
			return;
		}

		CoverageDatas.SetNumZeroed(Width * Height);
		uint32 MaskPaddedRowPitch = 0;
		uint8* MaskPaddedData = (uint8*)GGpuRhi->MapGpuTexture(TargetInfo.CoverageMask, GpuResourceMapMode::Read_Only, MaskPaddedRowPitch);
		if (!MaskPaddedData)
		{
			return;
		}
		const uint32 MaskPixelSize = GetFormatByteSize(TargetInfo.CoverageMask->GetFormat());
		for (uint32 y = 0; y < Height; ++y)
		{
			const uint8* SrcRow = MaskPaddedData + y * MaskPaddedRowPitch;
			for (uint32 x = 0; x < Width; ++x)
			{
				const uint8* Pixel = SrcRow + x * MaskPixelSize;
				CoverageDatas[y * Width + x] = Pixel[0] > 0 ? 1 : 0;
			}
		}
		GGpuRhi->UnMapGpuTexture(TargetInfo.CoverageMask);
		BuildCoverageOutlineRects(Width, Height);
	}

	void SFragmentDebuggerViewport::SelectOutput(int32 OutputIndex)
	{
		if (!TargetInfo.Outputs.IsValidIndex(OutputIndex) || !TargetInfo.Outputs[OutputIndex].Tex
			|| !DebuggerTextures.IsValidIndex(OutputIndex) || !DebuggerTextures[OutputIndex])
		{
			return;
		}

		TargetInfo.SelectedOutputIndex = OutputIndex;
		TargetInfo.Tex = TargetInfo.Outputs[OutputIndex].Tex;
		DrawOutput(TargetInfo.Outputs[OutputIndex].Tex.GetReference(), DebuggerTextures[OutputIndex].GetReference());
		ViewPort->SetViewPortRenderTexture(DebuggerTextures[OutputIndex].GetReference());
		ReadCurrentTargetData();
		PixelCoord = MouseToPixel(MouseLoc);
		ClampOffset();
	}

	GpuTexture* SFragmentDebuggerViewport::GetSelectedOutputTexture() const
	{
		if (TargetInfo.Outputs.IsValidIndex(TargetInfo.SelectedOutputIndex))
		{
			return TargetInfo.Outputs[TargetInfo.SelectedOutputIndex].Tex.GetReference();
		}
		return TargetInfo.Tex.GetReference();
	}

	GpuTexture* SFragmentDebuggerViewport::GetSelectedDebuggerTexture() const
	{
		return DebuggerTextures.IsValidIndex(TargetInfo.SelectedOutputIndex) ? DebuggerTextures[TargetInfo.SelectedOutputIndex].GetReference() : nullptr;
	}

	void SFragmentDebuggerViewport::CreateDebuggerTextures()
	{
		DebuggerTextures.Empty();
		for (const DebugTargetOutput& Output : TargetInfo.Outputs)
		{
			if (!Output.Tex)
			{
				DebuggerTextures.Add(TRefCountPtr<GpuTexture>());
				continue;
			}

			DebuggerTextures.Add(GGpuRhi->CreateTexture({
				.Width = Output.Tex->GetWidth(),
				.Height = Output.Tex->GetHeight(),
				.Format = GpuFormat::B8G8R8A8_UNORM,
				.Usage =  GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared
			}));
		}
	}

	void SFragmentDebuggerViewport::SetDebugTarget(const DebugTargetInfo& InTarget, bool GlobalValidation)
	{
		bFinalizePixel = false;
		IsValidating = false;
		Zoom = 1.0f;
		Offset = {};
		TargetInfo = InTarget;
		TargetInfo.Normalize();
		OutputOptions.Empty();
		for (int32 Index = 0; Index < TargetInfo.Outputs.Num(); ++Index)
		{
			const FString& Name = TargetInfo.Outputs[Index].Name;
			OutputOptions.Add(MakeShared<FString>(Name.IsEmpty() ? FString::Printf(TEXT("RT%d"), Index) : Name));
		}
		CreateDebuggerTextures();
		if (!TargetInfo.Outputs.IsEmpty())
		{
			TargetInfo.SelectedOutputIndex = FMath::Clamp(TargetInfo.SelectedOutputIndex, 0, TargetInfo.Outputs.Num() - 1);
			SelectOutput(TargetInfo.SelectedOutputIndex);
			Draw();
		}
		else
		{
			DebuggerTextures.Empty();
			ViewPort->Clear();
		}

		TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(AsShared());
		DpiScale = (Window.IsValid() && Window->GetNativeWindow().IsValid()) ? Window->GetNativeWindow()->GetDPIScaleFactor() : 1.0f;

		if (GlobalValidation && GetSelectedOutputTexture())
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			auto Invocation = ShEditor->GetDebuggaleObject()->GetInvocationState(DebugItem::Fragment);
			if (std::holds_alternative<PixelState>(Invocation))
			{
				IsValidating = true;
				PixelCoord = { -1 };
				GApp->EnqueueBusyTask([=, this](TFunction<void()> Done) {
					FNotificationInfo Info(LOCALIZATION("StartValidatorTip"));
					Info.Image = FAppStyle::Get().GetBrush("NoBrush");
					Info.bFireAndForget = false;
					Info.FadeInDuration = 0.0f;
					Info.FadeOutDuration = 0.0f;
					auto Notification = FSlateNotificationManager::Get().AddNotification(Info);
					Notification->SetCompletionState(SNotificationItem::CS_Pending);
					Async(EAsyncExecution::Thread, [=, this]() {
						std::optional<Vector2u> ErrorCoord;
						try
						{
							ErrorCoord = ShEditor->ValidatePixel(Invocation);
						}
						catch (const std::runtime_error& e)
						{
							AsyncTask(ENamedThreads::GameThread, [=, this] {
								IsValidating = false;
								Notification->Fadeout();
								Done();
								FText FailureInfo = LOCALIZATION("DebugFailure");
								SH_LOG(LogDebugger, Error, TEXT("%s:\n\n%s"), *FailureInfo.ToString(), UTF8_TO_TCHAR(e.what()));
								MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, GApp->GetEditor()->GetMainWindow(), FailureInfo);
								ShEditor->EndDebugging();
							});
							return;
						}

						AsyncTask(ENamedThreads::GameThread, [=, this] {
							IsValidating = false;
							Notification->Fadeout();
							Done();
							if (ErrorCoord)
							{
								const Vector2u ErrorPixelCoord = ErrorCoord.value();
								PixelCoord = Vector2i(int32(ErrorPixelCoord.x), int32(ErrorPixelCoord.y));
								MouseLoc = Vector2f(float(PixelCoord.x), float(PixelCoord.y)) * Zoom - Offset;
								Draw();
								bFinalizePixel = true;
								ShEditor->GetDebuggaleObject()->OnFinalizePixel(Vector2u(uint32(PixelCoord.x), uint32(PixelCoord.y)));
							}
							else
							{
								MessageDialog::Open(MessageDialog::Ok, MessageDialog::Happy, GApp->GetEditor()->GetMainWindow(), LOCALIZATION("ValidationTip"));
								ShEditor->EndDebugging();
							}
						});
					});
				});

			}
		}
	}
}
