// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateScope.h"
#include "Layout/WidgetPath.h"
#include "GestureDetector.h"

class SWidget;
class SWindow;
class ICursor;
class FDragDropOperation;
class FMenuStack;
class FNavigationConfig;

/** Handle to a virtual user of slate. */
class SLATE_API FSlateVirtualUserHandle
{
public:
	virtual ~FSlateVirtualUserHandle();

	FORCEINLINE int32 GetUserIndex() const { return UserIndex; }
	FORCEINLINE int32 GetVirtualUserIndex() const { return VirtualUserIndex; }

SLATE_SCOPE:
	FSlateVirtualUserHandle(int32 InUserIndex, int32 InVirtualUserIndex);

private:
	/** The index the user was assigned. */
	int32 UserIndex;

	/** The virtual index the user was assigned. */
	int32 VirtualUserIndex;
};

/**
 * Slate's representation of an individual input-providing user.  
 * As new input sources are connected, new SlateUsers are created.
 */
class SLATE_API FSlateUser : public TSharedFromThis<FSlateUser>
{
public:
	virtual ~FSlateUser();

	FORCEINLINE int32 GetUserIndex() const { return UserIndex; }
	FORCEINLINE bool IsVirtualUser() const { return !Cursor.IsValid(); }

	TSharedPtr<SWidget> GetFocusedWidget() const;
	bool ShouldShowFocus(TSharedPtr<const SWidget> Widget) const;
	bool SetFocus(const TSharedRef<SWidget>& WidgetToFocus, EFocusCause ReasonFocusIsChanging = EFocusCause::SetDirectly);
	void ClearFocus(EFocusCause ReasonFocusIsChanging = EFocusCause::SetDirectly);

	/** Returns the cause for which the provided widget was focused, or nothing if the given widget is not the current focus target. */
	TOptional<EFocusCause> HasFocus(TSharedPtr<const SWidget> Widget) const;

	/** Returns true if the given widget is in the focus path, but is not the focused widget itself. */
	bool HasFocusedDescendants(TSharedRef<const SWidget> Widget) const;

	/** Returns true if the given widget is anywhere in the focus path, including the focused widget itself. */
	bool IsWidgetInFocusPath(TSharedPtr<const SWidget> Widget) const;
	
	bool HasAnyCapture() const;
	bool HasCursorCapture() const;
	bool HasCapture(uint32 PointerIndex) const;
	
	bool DoesWidgetHaveAnyCapture(TSharedPtr<const SWidget> Widget) const;
	bool DoesWidgetHaveCursorCapture(TSharedPtr<const SWidget> Widget) const;
	bool DoesWidgetHaveCapture(TSharedPtr<const SWidget> Widget, uint32 PointerIndex) const;
	
	bool SetCursorCaptor(TSharedRef<const SWidget> Widget, const FWidgetPath& EventPath);
	bool SetPointerCaptor(uint32 PointerIndex, TSharedRef<const SWidget> Widget, const FWidgetPath& EventPath);
	
	void ReleaseAllCapture();
	void ReleaseCursorCapture();
	void ReleaseCapture(uint32 PointerIndex);

	TArray<FWidgetPath> GetCaptorPaths();
	FWidgetPath GetCursorCaptorPath(FWeakWidgetPath::EInterruptedPathHandling::Type InterruptedPathHandling = FWeakWidgetPath::EInterruptedPathHandling::Truncate, const FPointerEvent* PointerEvent = nullptr);
	FWidgetPath GetCaptorPath(uint32 PointerIndex, FWeakWidgetPath::EInterruptedPathHandling::Type InterruptedPathHandling = FWeakWidgetPath::EInterruptedPathHandling::Truncate, const FPointerEvent* PointerEvent = nullptr);

	FWeakWidgetPath GetWeakCursorCapturePath() const;
	FWeakWidgetPath GetWeakCapturePath(uint32 PointerIndex) const;

	TArray<TSharedRef<SWidget>> GetCaptorWidgets() const;
	TSharedPtr<SWidget> GetCursorCaptor() const;
	TSharedPtr<SWidget> GetPointerCaptor(uint32 PointerIndex) const;

	void SetCursorVisibility(bool bDrawCursor);

	void SetCursorPosition(int32 PosX, int32 PosY);
	void SetCursorPosition(const FVector2D& NewCursorPos);
	void SetPointerPosition(uint32 PointerIndex, int32 PosX, int32 PosY);
	void SetPointerPosition(uint32 PointerIndex, const FVector2D& NewPointerPos);
	
	FVector2D GetCursorPosition() const;
	FVector2D GetPreviousCursorPosition() const;
	
	FVector2D GetPointerPosition(uint32 PointerIndex) const;
	FVector2D GetPreviousPointerPosition(uint32 PointerIndex) const;

	bool IsWidgetUnderCursor(TSharedPtr<const SWidget> Widget) const;
	bool IsWidgetUnderPointer(TSharedPtr<const SWidget> Widget, uint32 PointerIndex) const;
	bool IsWidgetUnderAnyPointer(TSharedPtr<const SWidget> Widget) const;

	bool IsWidgetDirectlyUnderCursor(TSharedPtr<const SWidget> Widget) const;
	bool IsWidgetDirectlyUnderPointer(TSharedPtr<const SWidget> Widget, uint32 PointerIndex) const;
	bool IsWidgetDirectlyUnderAnyPointer(TSharedPtr<const SWidget> Widget) const;

	FWeakWidgetPath GetLastWidgetsUnderCursor() const;
	FWeakWidgetPath GetLastWidgetsUnderPointer(uint32 PointerIndex) const;
	const TMap<uint32, FWeakWidgetPath>& GetWidgetsUnderPointerLastEventByIndex() const { return WidgetsUnderPointerLastEventByIndex; }

	TSharedPtr<FDragDropOperation> GetDragDropContent() const { return DragDropContent; }
	bool IsDragDropping() const;
	bool IsDragDroppingAffected(const FPointerEvent& InPointerEvent) const;
	void CancelDragDrop();
	
	void ShowTooltip(const TSharedRef<IToolTip>& InTooltip, const FVector2D& InSpawnLocation);
	void CloseTooltip();

	const FGestureDetector& GetGestureDetector() const { return GestureDetector; }

	void SetUserNavigationConfig(TSharedPtr<FNavigationConfig> InNavigationConfig);
	TSharedPtr<FNavigationConfig> GetUserNavigationConfig() const { return UserNavigationConfig; }

SLATE_SCOPE:
	static TSharedRef<FSlateUser> Create(int32 InUserIndex, TSharedPtr<ICursor> InCursor);
	
	FORCEINLINE bool HasValidFocusPath() const { return WeakFocusPath.IsValid(); }
	FORCEINLINE const FWeakWidgetPath& GetWeakFocusPath() const { return WeakFocusPath; }
	
	FORCEINLINE TSharedRef<FWidgetPath> GetFocusPath() const
	{
		if (!StrongFocusPath.IsValid())
		{
			StrongFocusPath = WeakFocusPath.ToWidgetPathRef();
		}
		return StrongFocusPath.ToSharedRef();
	}

	void DrawWindowlessDragDropContent(const TSharedRef<SWindow>& WindowToDraw, FSlateWindowElementList& WindowElementList, int32& MaxLayerId);
	void DrawCursor(const TSharedRef<SWindow>& WindowToDraw, FSlateWindowElementList& WindowElementList, int32& MaxLayerId);

	void QueueSyntheticCursorMove();
	bool SynthesizeCursorMoveIfNeeded();

	TSharedPtr<ICursor> GetCursor() const { return Cursor; }
	void LockCursor(const TSharedRef<SWidget>& Widget);
	void UnlockCursor();
	void UpdateCursor();
	void ProcessCursorReply(const FCursorReply& CursorReply);
	void RequestCursorQuery() { bQueryCursorRequested = true; }
	void QueryCursor();
	void OverrideCursor(const TSharedPtr<ICursor> InCursor) { Cursor = InCursor; }
	
	void SetFocusPath(const FWidgetPath& NewFocusPath, EFocusCause InFocusCause, bool bInShowFocus);
	
	void FinishFrame();
	void NotifyWindowDestroyed(TSharedRef<SWindow> DestroyedWindow);

	bool IsTouchPointerActive(int32 TouchPointerIndex) const;

	void NotifyTouchStarted(const FPointerEvent& TouchEvent);
	void NotifyPointerMoveBegin(const FPointerEvent& PointerEvent);
	void NotifyPointerMoveComplete(const FPointerEvent& PointerEvent, const FWidgetPath& WidgetsUnderPointer);
	void NotifyPointerReleased(const FPointerEvent& PointerEvent, const FWidgetPath& WidgetsUnderCursor, TSharedPtr<FDragDropOperation> DroppedContent, bool bWasHandled);
	void UpdatePointerPosition(const FPointerEvent& PointerEvent);

	void StartDragDetection(const FWidgetPath& PathToWidget, int32 PointerIndex, FKey DragButton, FVector2D StartLocation);
	FWidgetPath DetectDrag(const FPointerEvent& PointerEvent, float DragTriggerDistance);
	bool IsDetectingDrag(uint32 PointerIndex) const;
	void ResetDragDetection();

	void SetDragDropContent(TSharedRef<FDragDropOperation> InDragDropContent);
	void ResetDragDropContent();

	int32 GetFocusVersion() const { return FocusVersion; }
	void IncrementFocusVersion() { FocusVersion++; }

	void UpdateTooltip(const FMenuStack& MenuStack, bool bCanSpawnNewTooltip);
	void ResetTooltipWindow();
	bool IsWindowHousingInteractiveTooltip(const TSharedRef<const SWindow>& WindowToTest) const;

	FGestureDetector& GetGestureDetector() { return GestureDetector; }

#if PLATFORM_MAC
	// Unclear why from existing code, but Mac seems to need to cache & restore all mouse captor paths when activating the top level window
	const TMap<uint32, FWeakWidgetPath> GetCaptorPathsByIndex() const { return PointerCaptorPathsByIndex; }
	void RestoreCaptorPathsByIndex(const TMap<uint32, FWeakWidgetPath>& InPointerCaptorPathsByIndex) { PointerCaptorPathsByIndex = InPointerCaptorPathsByIndex; }
#endif

private:
	FSlateUser(int32 InUserIndex, TSharedPtr<ICursor> InCursor);
	void UpdatePointerPosition(uint32 PointerIndex, const FVector2D& Position);
	void LockCursorInternal(const FWidgetPath& WidgetPath);
	TSharedRef<SWindow> GetOrCreateTooltipWindow();

	/** The index the user was assigned. */
	int32 UserIndex = INDEX_NONE;

	/** The cursor this user is in control of. Guaranteed to be valid for all real users, absence implies this is a virtual user. */
	TSharedPtr<ICursor> Cursor;

	/** SlateUsers can optionally be individually assigned a navigation config to use. This overrides the global nav config that lives on FSlateApplication when valid. */
	TSharedPtr<FNavigationConfig> UserNavigationConfig;

	/** Whether this user is currently drawing the cursor onscreen each frame */
	bool bCanDrawCursor = true;

	/** The OS or actions taken by the user may require we refresh the current state of the cursor. */
	bool bQueryCursorRequested = false;

	/**
	 * Whenever something happens that can affect Slate layout, we need to process a mouse move even if the mouse didn't move at all.
	 * @see SynthesizeCursorMoveIfNeeded
	 */
	int32 NumPendingSyntheticCursorMoves = 0;

	/** The cursor widget and window to render that cursor for the current software cursor.*/
	TWeakPtr<SWindow> CursorWindowPtr;
	TWeakPtr<SWidget> CursorWidgetPtr;

	/** A weak path to the widget currently focused by a user, if any. */
	FWeakWidgetPath WeakFocusPath;

	/** A strong widget path to the focused widget, if any. This is cleared after the end of pumping messages. */
	mutable TSharedPtr<FWidgetPath> StrongFocusPath;

	/** Reason a widget was focused by a user, if any. */
	EFocusCause FocusCause = EFocusCause::Cleared;

	/** If we should show this focus */
	bool bShowFocus = false;

	/**
	 * The FocusVersion is used to know if the focus state is modified for a user while processing focus
	 * events, that way upon returning from focus calls, we know if we should abandon the remainder of the event.
	 */
	int32 FocusVersion = 0;

	/** Current position of all pointers controlled by this user */
	TMap<uint32, FVector2D> PointerPositionsByIndex;
	TMap<uint32, FVector2D> PreviousPointerPositionsByIndex;

	/** Weak paths to widgets that are currently capturing a particular pointer */
	TMap<uint32, FWeakWidgetPath> PointerCaptorPathsByIndex;

	struct FDragDetectionState
	{
		FDragDetectionState(const FWidgetPath& PathToWidget, int32 PointerIndex, FKey DragButton, const FVector2D& StartLocation)
			: DetectDragForWidget(PathToWidget)
			, DragStartLocation(StartLocation)
			, TriggerButton(DragButton)
			, PointerIndex(PointerIndex)
		{
		}

		/** If not null, a widget has requested that we detect a drag being triggered in this widget and send an OnDragDetected() event*/
		FWeakWidgetPath DetectDragForWidget;

		FVector2D DragStartLocation = FVector2D::ZeroVector;
		FKey TriggerButton = EKeys::Invalid;
		int32 PointerIndex = INDEX_NONE;
	};

	/** Current drag status for pointers currently executing a drag/drop operation */
	TMap<uint32, FDragDetectionState> DragStatesByPointerIndex;

	/** When not null, the content of the current drag drop operation. */
	TSharedPtr<FDragDropOperation> DragDropContent;

	/** The window the drag drop content is over. */
	TWeakPtr<SWindow> DragDropWindowPtr;

	/** Weak paths to the last widget each pointer was under last time an event was processed */
	TMap<uint32, FWeakWidgetPath> WidgetsUnderPointerLastEventByIndex;

	/** Path to widget that currently holds the cursor lock; invalid path if no cursor lock. */
	FWeakWidgetPath LockingWidgetPath;

	/** Desktop Space Rect that bounds the cursor. */
	FSlateRect LastComputedLockBounds;

	/** Window that we'll re-use for spawned tool tips */
	TWeakPtr<SWindow> TooltipWindowPtr;

	enum class ETooltipOffsetDirection : uint8
	{
		Undetermined,
		Down,
		Right
	};
	struct FActiveTooltipInfo
	{
		void Reset();

		/** The actual tooltip object running the show here (not necessarily the same object as the tooltip SWidget, but can be) */
		TWeakPtr<IToolTip> Tooltip;

		/** The widget that is taking care of visualizing this tooltip (can be null) */
		TWeakPtr<SWidget> TooltipVisualizer;

		/** The widget that sourced the currently active tooltip */
		TWeakPtr<SWidget> SourceWidget;

		/** Desired position of the tooltip in screen space, updated whenever the mouse moves */
		FVector2D DesiredLocation = FVector2D::ZeroVector;

		/** The time at which the tooltip was summoned */
		double SummonTime = 0.0;

		/** 
		 * Direction in which the tooltip widget is being offset from the source widget  
		 * Cached to prevent the direction of offset from changing as the user moves the mouse cursor (otherwise the tooltip teleports around)
		 */
		ETooltipOffsetDirection OffsetDirection = ETooltipOffsetDirection::Undetermined;
	};
	FActiveTooltipInfo ActiveTooltipInfo;

	FGestureDetector GestureDetector;
};