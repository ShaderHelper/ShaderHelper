// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FastUpdate/SlateInvalidationWidgetSortOrder.h"
#include "Layout/SlateRect.h"
#include "Layout/ArrangedWidget.h"
#include "Layout/Clipping.h"
#include "Input/Events.h"
#include "Widgets/SWidget.h"

class FArrangedChildren;

class ICustomHitTestPath
{
public:
	virtual ~ICustomHitTestPath(){}

	virtual TArray<FWidgetAndPointer> GetBubblePathAndVirtualCursors( const FGeometry& InGeometry, FVector2D DesktopSpaceCoordinate, bool bIgnoreEnabledStatus ) const = 0;

	virtual void ArrangeCustomHitTestChildren( FArrangedChildren& ArrangedChildren ) const = 0;

	UE_DEPRECATED(5.0, "TranslateMouseCoordinateForCustomHitTestChild that returns a shared ptr is deprecated.")
	virtual TSharedPtr<struct FVirtualPointerPosition> TranslateMouseCoordinateForCustomHitTestChild( const TSharedRef<SWidget>& ChildWidget, const FGeometry& ViewportGeometry, const FVector2D& ScreenSpaceMouseCoordinate, const FVector2D& LastScreenSpaceMouseCoordinate ) const { return TSharedPtr<struct FVirtualPointerPosition>(); }

	virtual TOptional<FVirtualPointerPosition> TranslateMouseCoordinateForCustomHitTestChild(const SWidget& ChildWidget, const FGeometry& MyGeometry, const FVector2D ScreenSpaceMouseCoordinate, const FVector2D LastScreenSpaceMouseCoordinate) const = 0;
};

class SLATECORE_API FHittestGrid : public FNoncopyable
{
public:

	FHittestGrid();

	/**
	 * Given a Slate Units coordinate in virtual desktop space, perform a hittest
	 * and return the path along which the corresponding event would be bubbled.
	 */
	TArray<FWidgetAndPointer> GetBubblePath(FVector2D DesktopSpaceCoordinate, float CursorRadius, bool bIgnoreEnabledStatus, int32 UserIndex = INDEX_NONE);

	/**
	 * Set the position and size of the hittest area in desktop coordinates
	 *
	 * @param HittestPositionInDesktop	The position of this hit testing area in desktop coordinates.
	 * @param HittestDimensions			The dimensions of this hit testing area.
	 *
	 * @return      Returns true if a clear of the hittest grid was required. 
	 */
	bool SetHittestArea(const FVector2D& HittestPositionInDesktop, const FVector2D& HittestDimensions, const FVector2D& HitestOffsetInWindow = FVector2D::ZeroVector);

	/** Insert custom hit test data for a widget already in the grid */
	UE_DEPRECATED(5.0, "Deprecated. Use the InsertCustomHitTestPath with a pointer.")
	void InsertCustomHitTestPath(const TSharedRef<SWidget> InWidget, TSharedRef<ICustomHitTestPath> CustomHitTestPath);

	/** Insert custom hit test data for a widget already in the grid */
	void InsertCustomHitTestPath(const SWidget* InWidget, const TSharedRef<ICustomHitTestPath>& CustomHitTestPath);

	/** Sets the current slate user index that should be associated with any added widgets */
	void SetUserIndex(int32 UserIndex) { CurrentUserIndex = UserIndex; }

	/** Set the culling rect to be used by the parent grid (in case we are appended to another grid). */
	void SetCullingRect(const FSlateRect& InCullingRect) { CullingRect = InCullingRect; }

	/** Set the owner SWidget to be used by the parent grid (in case we are appended to another grid). */
	void SetOwner(const SWidget* InOwner) { check(Owner == nullptr || Owner == InOwner); Owner = InOwner; }

	/** Gets current slate user index that should be associated with any added widgets */
	int32 GetUserIndex() const { return CurrentUserIndex; }

	/**
	 * Finds the next focusable widget by searching through the hit test grid
	 *
	 * @param StartingWidget  This is the widget we are starting at, and navigating from.
	 * @param Direction       The direction we should search in.
	 * @param NavigationReply The Navigation Reply to specify a boundary rule for the search.
	 * @param RuleWidget      The Widget that is applying the boundary rule, used to get the bounds of the Rule.
	 */
	TSharedPtr<SWidget> FindNextFocusableWidget(const FArrangedWidget& StartingWidget, const EUINavigation Direction, const FNavigationReply& NavigationReply, const FArrangedWidget& RuleWidget, int32 UserIndex);

	FVector2D GetGridSize() const { return GridSize; }
	FVector2D GetGridOrigin() const { return GridOrigin; }
	FVector2D GetGridWindowOrigin() const { return GridWindowOrigin; }

	/** Clear the grid */
	void Clear();

	/** Add SWidget from the HitTest Grid */
	UE_DEPRECATED(4.27, "Deprecated. Use the AddWidget with the FSlateInvalidationWidgetSortOrder type parameters instead. Passing FSlateInvalidationWidgetSortOrder()")
	void AddWidget(const TSharedRef<SWidget>& InWidget, int32 InBatchPriorityGroup, int32 InLayerId, int32 InSecondarySort);

	/** Add SWidget from the HitTest Grid */
	UE_DEPRECATED(5.0, "Deprecated. Use the AddWidget with a pointer.")
	void AddWidget(const TSharedRef<SWidget>& InWidget, int32 InBatchPriorityGroup, int32 InLayerId, FSlateInvalidationWidgetSortOrder InSecondarySort);

	/** Add SWidget from the HitTest Grid */
	void AddWidget(const SWidget* InWidget, int32 InBatchPriorityGroup, int32 InLayerId, FSlateInvalidationWidgetSortOrder InSecondarySort);

	/** Remove SWidget from the HitTest Grid */
	UE_DEPRECATED(5.0, "Deprecated. Use the RemoveWidget with a pointer.")
	void RemoveWidget(const TSharedRef<SWidget>& InWidget);

	/** Remove SWidget from the HitTest Grid */
	void RemoveWidget(const SWidget* InWidget);

	/** Update the widget SecondarySort without removing it and readding it again. */
	UE_DEPRECATED(5.0, "Deprecated. Use the UpdateWidget with a pointer.")
	void UpdateWidget(const TSharedRef<SWidget>& InWidget, FSlateInvalidationWidgetSortOrder InSecondarySort);
	
	/** Update the widget SecondarySort without removing it and readding it again. */
	void UpdateWidget(const SWidget* InWidget, FSlateInvalidationWidgetSortOrder InSecondarySort);

	/** Append an already existing grid that occupy the same space. */
	UE_DEPRECATED(4.26, "Deprecated. Use the FHittestGrid::AddGrid method instead")
	void AppendGrid(FHittestGrid& OtherGrid) {}

	/**
	 * Add an already existing grid that occupy the same space.
	 * The grid needs to have an owner, not be this grid and occupy the same space as this grid.
	 */
	void AddGrid(const TSharedRef<const FHittestGrid>& OtherGrid);

	/** Remove a grid that was appended. */
	void RemoveGrid(const TSharedRef<const FHittestGrid>& OtherGrid);

	/** Remove a grid that was appended. */
	void RemoveGrid(const SWidget* OtherGridOwner);

	struct FDebuggingFindNextFocusableWidgetArgs
	{
		struct FWidgetResult
		{
			const TSharedPtr<const SWidget> Widget;
			const FText Result;
			FWidgetResult(const TSharedPtr<const SWidget>& InWidget, FText InResult)
				: Widget(InWidget), Result(InResult) {}
		};
		const FArrangedWidget StartingWidget;
		const EUINavigation Direction;
		const FNavigationReply NavigationReply;
		const FArrangedWidget RuleWidget;
		const int32 UserIndex;
		const TSharedPtr<const SWidget> Result;
		TArray<FWidgetResult> IntermediateResults;
	};

#if WITH_SLATE_DEBUGGING
	DECLARE_MULTICAST_DELEGATE_TwoParams(FDebuggingFindNextFocusableWidget, const FHittestGrid* /*HittestGrid*/, const FDebuggingFindNextFocusableWidgetArgs& /*Info*/);
	static FDebuggingFindNextFocusableWidget OnFindNextFocusableWidgetExecuted;

	void LogGrid() const;

	enum class EDisplayGridFlags
	{
		None = 0,
		HideDisabledWidgets = 1 << 0,					// Hide hit box for widgets that have IsEnabled false
		HideUnsupportedKeyboardFocusWidgets = 1 << 1,	// Hide hit box for widgets that have SupportsKeyboardFocus false
		UseFocusBrush = 1 << 2,
	};
	void DisplayGrid(int32 InLayer, const FGeometry& AllottedGeometry, FSlateWindowElementList& WindowElementList, EDisplayGridFlags DisplayFlags = EDisplayGridFlags::UseFocusBrush) const;

	struct FWidgetSortData
	{
		const TWeakPtr<SWidget> WeakWidget;
		int64 PrimarySort;
		FSlateInvalidationWidgetSortOrder SecondarySort;
	};
	TArray<FWidgetSortData> GetAllWidgetSortDatas() const;
#endif

private:
	/**
	 * Widget Data we maintain internally store along with the widget reference
	 */
	struct FWidgetData
	{
		FWidgetData(const TWeakPtr<SWidget>& InWidget, const FIntPoint& InUpperLeftCell, const FIntPoint& InLowerRightCell, int64 InPrimarySort, FSlateInvalidationWidgetSortOrder InSecondarySort, int32 InUserIndex)
			: WeakWidget(InWidget)
			, UpperLeftCell(InUpperLeftCell)
			, LowerRightCell(InLowerRightCell)
			, PrimarySort(InPrimarySort)
			, SecondarySort(InSecondarySort)
			, UserIndex(InUserIndex)
		{}
		TWeakPtr<SWidget> WeakWidget;
		TWeakPtr<ICustomHitTestPath> CustomPath;
		FIntPoint UpperLeftCell;
		FIntPoint LowerRightCell;
		int64 PrimarySort;
		FSlateInvalidationWidgetSortOrder SecondarySort;
		int32 UserIndex;

		TSharedPtr<SWidget> GetWidget() const { return WeakWidget.Pin(); }
	};

	struct FWidgetIndex
	{
		FWidgetIndex()
			: Grid(nullptr)
			, WidgetIndex(INDEX_NONE)
		{}
		FWidgetIndex(const FHittestGrid* InHittestGrid, int32 InIndex)
			: Grid(InHittestGrid)
			, WidgetIndex(InIndex)
		{}
		bool IsValid() const { return Grid != nullptr && Grid->WidgetArray.IsValidIndex(WidgetIndex); }
		const FWidgetData& GetWidgetData() const;
		const FSlateRect& GetCullingRect() const { return Grid->CullingRect; }
		const FHittestGrid* GetGrid() const { return Grid; }

	private:
		const FHittestGrid* Grid;
		int32 WidgetIndex;
	};

	struct FIndexAndDistance : FWidgetIndex
	{
		FIndexAndDistance()
			: FWidgetIndex()
			, DistanceSqToWidget(0)
		{}
		FIndexAndDistance(FWidgetIndex WidgetIndex, float InDistanceSq)
			: FWidgetIndex(WidgetIndex)
			, DistanceSqToWidget(InDistanceSq)
		{}
		float GetDistanceSqToWidget() const { return DistanceSqToWidget; }

	private:
		float DistanceSqToWidget;
	};

	struct FGridTestingParams;

	/**
	 * All the available space is partitioned into Cells.
	 * Each Cell contains a list of widgets that overlap the cell.
	 * The list is ordered from back to front.
	 */
	struct FCell
	{
	public:
		FCell() = default;

		void AddIndex(int32 WidgetIndex);
		void RemoveIndex(int32 WidgetIndex);

		const TArray<int32>& GetWidgetIndexes() const { return WidgetIndexes; }
		
	private:
		TArray<int32> WidgetIndexes;
	};

	struct FAppendedGridData
	{
		FAppendedGridData(const SWidget* InCachedOwner, const TWeakPtr<const FHittestGrid>& InGrid)
			 : CachedOwner(InCachedOwner), Grid(InGrid)
		{ }
		const SWidget* CachedOwner; // Cached owner of the grid
		TWeakPtr<const FHittestGrid> Grid;
	};

	//~ Helper functions
	bool IsValidCellCoord(const FIntPoint& CellCoord) const;
	bool IsValidCellCoord(const int32 XCoord, const int32 YCoord) const;
	void ClearInternal(int32 TotalCells);

	/** Return the Index and distance to a hit given the testing params */
	FIndexAndDistance GetHitIndexFromCellIndex(const FGridTestingParams& Params) const;

	/** @returns true if the child is a paint descendant of the provided Parent. */
	bool IsDescendantOf(const SWidget* Parent, const FWidgetData& ChildData) const;

	/** Utility function for searching for the next focusable widget. */
	template<typename TCompareFunc, typename TSourceSideFunc, typename TDestSideFunc>
	TSharedPtr<SWidget> FindFocusableWidget(const FSlateRect WidgetRect, const FSlateRect SweptRect, int32 AxisIndex, int32 Increment, const EUINavigation Direction, const FNavigationReply& NavigationReply, TCompareFunc CompareFunc, TSourceSideFunc SourceSideFunc, TDestSideFunc DestSideFunc, int32 UserIndex, TArray<FDebuggingFindNextFocusableWidgetArgs::FWidgetResult>* IntermediatedResultPtr) const;

	/** Constrains a float position into the grid coordinate. */
	FIntPoint GetCellCoordinate(FVector2D Position) const;

	/** Access a cell at coordinates X, Y. Coordinates are row and column indexes. */
	FORCEINLINE_DEBUGGABLE FCell& CellAt(const int32 X, const int32 Y)
	{
		checkfSlow((Y*NumCells.X + X) < Cells.Num(), TEXT("HitTestGrid CellAt() failed: X= %d Y= %d NumCells.X= %d NumCells.Y= %d Cells.Num()= %d"), X, Y, NumCells.X, NumCells.Y, Cells.Num());
		return Cells[Y*NumCells.X + X];
	}

	/** Access a cell at coordinates X, Y. Coordinates are row and column indexes. */
	FORCEINLINE_DEBUGGABLE const FCell& CellAt( const int32 X, const int32 Y ) const
	{
		checkfSlow((Y*NumCells.X + X) < Cells.Num(), TEXT("HitTestGrid CellAt() failed: X= %d Y= %d NumCells.X= %d NumCells.Y= %d Cells.Num()= %d"), X, Y, NumCells.X, NumCells.Y, Cells.Num());
		return Cells[Y*NumCells.X + X];
	}

	/** Is the other grid compatible with this grid. */
	bool CanBeAppended(const FHittestGrid* OtherGrid) const;

	/** Are both grid of the same size. */
	bool SameSize(const FHittestGrid* OtherGrid) const;

	using FCollapsedHittestGridArray = TArray<const FHittestGrid*, TInlineAllocator<16>>;
	/** Get all the hittest grid appended to this grid. */
	void GetCollapsedHittestGrid(FCollapsedHittestGridArray& OutResult) const;

	using FCollapsedWidgetsArray = TArray<FWidgetIndex, TInlineAllocator<100>>;
	/** Return the list of all the widget in that cell. */
	void GetCollapsedWidgets(FCollapsedWidgetsArray& Out, const int32 X, const int32 Y) const;

	/** Remove appended hittest grid that are not valid anymore. */
	void RemoveStaleAppendedHittestGrid();

private:
	/** Map of all the widgets currently in the hit test grid to their stable index. */
	TMap<const SWidget*, int32> WidgetMap;

	/** Stable indexed sparse array of all the widget data we track. */
	TSparseArray<FWidgetData> WidgetArray;

	/** The cells that make up the space partition. */
	TArray<FCell> Cells;

	/** The collapsed grid cached untiled it's dirtied. */
	TArray<FAppendedGridData> AppendedGridArray;

	/** A grid needs a owner to be appended. */
	const SWidget* Owner;

	/** Culling Rect used when the widget was painted. */
	FSlateRect CullingRect;

	/** The size of the grid in cells. */
	FIntPoint NumCells;

	/** Where the 0,0 of the upper-left-most cell corresponds to in desktop space. */
	FVector2D GridOrigin;

	/** Where the 0,0 of the upper-left-most cell corresponds to in window space. */
	FVector2D GridWindowOrigin;

	/** The Size of the current grid. */
	FVector2D GridSize;

	/** The current slate user index that should be associated with any added widgets */
	int32 CurrentUserIndex;
};

#if WITH_SLATE_DEBUGGING
ENUM_CLASS_FLAGS(FHittestGrid::EDisplayGridFlags);
#endif
