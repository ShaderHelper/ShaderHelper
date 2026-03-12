#include "CommonHeader.h"
#include "SMarkerScrollBar.h"

namespace SH
{
	void SMarkerScrollBar::Construct(const SScrollBar::FArguments& InArgs)
	{
		SScrollBar::Construct(InArgs);
	}

	void SMarkerScrollBar::SetMarkers(const TArray<FScrollBarMarker>& InMarkers)
	{
		Markers = InMarkers;
	}

	void SMarkerScrollBar::SetMarkers(const TArray<int32>& InLineIndices, const FLinearColor& InColor)
	{
		Markers.Reset(InLineIndices.Num());
		for (int32 LineIdx : InLineIndices)
		{
			Markers.Add({ LineIdx, InColor });
		}
	}

	void SMarkerScrollBar::ClearMarkers()
	{
		Markers.Empty();
	}

	void SMarkerScrollBar::SetTotalLineCount(int32 InTotalLineCount)
	{
		TotalLineCount = InTotalLineCount;
	}

	int32 SMarkerScrollBar::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
	{
		LayerId = SScrollBar::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

		if (Markers.Num() == 0 || TotalLineCount <= 0)
		{
			return LayerId;
		}

		const float BarHeight = (float)AllottedGeometry.GetLocalSize().Y;
		const float BarWidth = (float)AllottedGeometry.GetLocalSize().X;

		if (BarHeight <= 0.0f || BarWidth <= 0.0f)
		{
			return LayerId;
		}

		// Each marker is drawn as a small rectangle at the proportional Y position.
		const float MarkerHeight = FMath::Max(MinMarkerHeight, BarHeight / TotalLineCount);
		// Leave a small horizontal margin so markers don't touch the edge
		const float HMargin = FMath::Max(1.0f, BarWidth * 0.15f);
		const float MarkerWidth = BarWidth - 2.0f * HMargin;

		if (MarkerWidth <= 0.0f)
		{
			return LayerId;
		}

		++LayerId;

		const bool bEnabled = bParentEnabled && IsEnabled();
		const ESlateDrawEffect DrawEffect = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

		for (const FScrollBarMarker& Marker : Markers)
		{
			const float NormalizedY = FMath::Clamp((float)Marker.LineIndex / (float)TotalLineCount, 0.0f, 1.0f);
			const float Y = NormalizedY * (BarHeight - MarkerHeight);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(
					FVector2D(MarkerWidth, MarkerHeight),
					FSlateLayoutTransform(FVector2D(HMargin, Y))
				),
				FAppStyle::Get().GetBrush("WhiteBrush"),
				DrawEffect,
				Marker.Color
			);
		}

		return LayerId;
	}
}
