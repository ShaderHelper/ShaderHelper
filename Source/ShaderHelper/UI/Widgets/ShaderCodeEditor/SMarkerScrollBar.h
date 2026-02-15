#pragma once
#include <Widgets/Layout/SScrollBar.h>

namespace SH
{
	struct FScrollBarMarker
	{
		int32 LineIndex = 0;
		FLinearColor Color = FLinearColor::Yellow;
	};

	/**
	 * SScrollBar subclass that can overlay colored marker blocks on its track.
	 * Used for showing occurrence-highlight / search-result positions
	 **/
	class SMarkerScrollBar : public SScrollBar
	{
	public:

		void Construct(const FArguments& InArgs);

		void SetMarkers(const TArray<FScrollBarMarker>& InMarkers);
		void SetMarkers(const TArray<int32>& InLineIndices, const FLinearColor& InColor);
		void ClearMarkers();
		void SetTotalLineCount(int32 InTotalLineCount);

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
			int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	private:
		TArray<FScrollBarMarker> Markers;
		int32 TotalLineCount = 1;
		static constexpr float MinMarkerHeight = 2.0f;
	};
}
