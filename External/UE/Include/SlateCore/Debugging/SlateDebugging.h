// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "FastUpdate/WidgetUpdateFlags.h"
#include "Input/Reply.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Widgets/InvalidateWidgetReason.h"

#include "SlateDebugging.generated.h"

#ifndef WITH_SLATE_DEBUGGING
	#define WITH_SLATE_DEBUGGING !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#endif

#ifndef SLATE_CSV_TRACKER
	#define SLATE_CSV_TRACKER CSV_PROFILER
#endif

class SWindow;
class SWidget;
struct FGeometry;
class FPaintArgs;
class FSlateWindowElementList;
class FSlateDrawElement;
class FSlateRect;
class FWeakWidgetPath;
class FWidgetPath;
class FNavigationReply;
class FSlateInvalidationRoot;

UENUM()
enum class ESlateDebuggingInputEvent : uint8
{
	MouseMove = 0,
	MouseEnter,
	MouseLeave,
	PreviewMouseButtonDown,
	MouseButtonDown,
	MouseButtonUp,
	MouseButtonDoubleClick,
	MouseWheel,
	TouchStart,
	TouchEnd,
	TouchForceChanged,
	TouchFirstMove,
	TouchMoved,
	DragDetected,
	DragEnter,
	DragLeave,
	DragOver,
	DragDrop,
	DropMessage,
	PreviewKeyDown,
	KeyDown,
	KeyUp,
	KeyChar,
	AnalogInput,
	TouchGesture,
	MotionDetected,
	MAX,
};

UENUM()
enum class ESlateDebuggingStateChangeEvent : uint8
{
	MouseCaptureGained,
	MouseCaptureLost,
};

UENUM()
enum class ESlateDebuggingNavigationMethod : uint8
{
	Unknown,
	Explicit,
	CustomDelegateBound,
	CustomDelegateUnbound,
	NextOrPrevious,
	HitTestGrid
};

struct SLATECORE_API FSlateDebuggingInputEventArgs
{
public:
	FSlateDebuggingInputEventArgs(ESlateDebuggingInputEvent InInputEventType, const FInputEvent* InInputEvent, const FReply& InReply, const TSharedPtr<SWidget>& InHandlerWidget, const FString& InAdditionalContent);

	const ESlateDebuggingInputEvent InputEventType;
	const FInputEvent* InputEvent;
	const FReply& Reply;
	const TSharedPtr<SWidget>& HandlerWidget;
	const FString& AdditionalContent;

	FText ToText() const;
};

struct SLATECORE_API FSlateDebuggingCursorQueryEventArgs
{
public:
	FSlateDebuggingCursorQueryEventArgs(const TSharedPtr<const SWidget>& InWidgetOverridingCursor, const FCursorReply& InReply);

	const TSharedPtr<const SWidget>& WidgetOverridingCursor;
	const FCursorReply& Reply;

	FText ToText() const;
};

UENUM()
enum class ESlateDebuggingFocusEvent : uint8
{
	FocusChanging = 0,
	FocusLost,
	FocusReceived,
	MAX
};


#if WITH_SLATE_DEBUGGING


struct SLATECORE_API FSlateDebuggingFocusEventArgs
{
public:
	FSlateDebuggingFocusEventArgs(
		ESlateDebuggingFocusEvent InFocusEventType,
		const FFocusEvent& InFocusEvent,
		const FWeakWidgetPath& InOldFocusedWidgetPath,
		const TSharedPtr<SWidget>& InOldFocusedWidget,
		const FWidgetPath& InNewFocusedWidgetPath,
		const TSharedPtr<SWidget>& InNewFocusedWidget
	);

	ESlateDebuggingFocusEvent FocusEventType;
	const FFocusEvent& FocusEvent;
	const FWeakWidgetPath& OldFocusedWidgetPath;
	const TSharedPtr<SWidget>& OldFocusedWidget;
	const FWidgetPath& NewFocusedWidgetPath;
	const TSharedPtr<SWidget>& NewFocusedWidget;

	FText ToText() const;
};

struct SLATECORE_API FSlateDebuggingNavigationEventArgs
{
public:
	FSlateDebuggingNavigationEventArgs(
		const FNavigationEvent& InNavigationEvent,
		const FNavigationReply& InNavigationReply,
		const FWidgetPath& InNavigationSource,
		const TSharedPtr<SWidget>& InDestinationWidget,
		const ESlateDebuggingNavigationMethod InNavigationMethod
	);

	const FNavigationEvent& NavigationEvent;
	const FNavigationReply& NavigationReply;
	const FWidgetPath& NavigationSource;
	const TSharedPtr<SWidget>& DestinationWidget;
	const ESlateDebuggingNavigationMethod NavigationMethod;

	FText ToText() const;
};

struct SLATECORE_API FSlateDebuggingExecuteNavigationEventArgs
{
public:
};

struct SLATECORE_API FSlateDebuggingWarningEventArgs
{
public:
	FSlateDebuggingWarningEventArgs(
		const FText& InWarning,
		const TSharedPtr<SWidget>& InOptionalContextWidget
	);

	const FText& Warning;
	const TSharedPtr<SWidget>& OptionalContextWidget;

	FText ToText() const;
};

struct SLATECORE_API FSlateDebuggingMouseCaptureEventArgs
{
public:
	FSlateDebuggingMouseCaptureEventArgs(
		bool InCaptured,
		uint32 InUserIndex,
		uint32 InPointerIndex,
		const TSharedPtr<const SWidget>& InCapturingWidget
	);

	bool Captured;
	uint32 UserIndex;
	uint32 PointerIndex;
	const TSharedPtr<const SWidget>& CaptureWidget;

	FText ToText() const;
};

enum class ESlateDebuggingInvalidateRootReason
{
	None = 0,
	ChildOrder = 1 << 0,
	Root = 1 << 1,
	ScreenPosition = 1 << 2,
};

ENUM_CLASS_FLAGS(ESlateDebuggingInvalidateRootReason)

SLATECORE_API FString LexToString(ESlateDebuggingInvalidateRootReason Reason);
SLATECORE_API bool LexTryParseString(ESlateDebuggingInvalidateRootReason& OutMode, const TCHAR* InBuffer);
SLATECORE_API void LexFromString(ESlateDebuggingInvalidateRootReason& OutMode, const TCHAR* InBuffer);

struct SLATECORE_API FSlateDebuggingInvalidateArgs
{
	FSlateDebuggingInvalidateArgs(
		const SWidget* WidgetInvalidated,
		const SWidget* WidgetInvalidateInvestigator,
		EInvalidateWidgetReason InvalidateReason);

	FSlateDebuggingInvalidateArgs(
		const SWidget* WidgetInvalidated,
		const SWidget* WidgetInvalidateInvestigator,
		ESlateDebuggingInvalidateRootReason InvalidateReason);

	const SWidget* WidgetInvalidated;
	const SWidget* WidgetInvalidateInvestigator;
	EInvalidateWidgetReason InvalidateWidgetReason;
	ESlateDebuggingInvalidateRootReason InvalidateInvalidationRootReason;
};

struct SLATECORE_API FSlateDebuggingWidgetUpdatedEventArgs
{
public:
	FSlateDebuggingWidgetUpdatedEventArgs(
		const SWidget* Widget,
		EWidgetUpdateFlags UpdateFlags,
		bool bFromPaint
	);

	const SWidget* Widget;
	/** Flag that was set by an invalidation or on the widget directly. */
	EWidgetUpdateFlags UpdateFlags;
	/** The widget got painted as a side effect of another widget that got painted */
	bool bFromPaint;

	FText ToText() const;
};

/**
 * 
 */
class SLATECORE_API FSlateDebugging
{
public:
	/** Called when a widget begins painting. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FBeginWindow, const FSlateWindowElementList& /*ElementList*/);
	static FBeginWindow BeginWindow;

	/** Called when a window finishes painting. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FEndWindow, const FSlateWindowElementList& /*ElementList*/);
	static FEndWindow EndWindow;

	/** Called just before a widget paints. */
	DECLARE_MULTICAST_DELEGATE_SixParams(FBeginWidgetPaint, const SWidget* /*Widget*/, const FPaintArgs& /*Args*/, const FGeometry& /*AllottedGeometry*/, const FSlateRect& /*MyCullingRect*/, const FSlateWindowElementList& /*OutDrawElements*/, int32 /*LayerId*/);
	static FBeginWidgetPaint BeginWidgetPaint;

	/** Called after a widget finishes painting. */
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FEndWidgetPaint, const SWidget* /*Widget*/, const FSlateWindowElementList& /*OutDrawElements*/, int32 /*LayerId*/);
	static FEndWidgetPaint EndWidgetPaint;

	/** Called once the window is drawn (including PaintDeferred). It can be used to draw additional debug info on top of the window. */
	DECLARE_MULTICAST_DELEGATE_FourParams(FPaintDebugElements, const FPaintArgs& /*InArgs*/, const FGeometry& /*InAllottedGeometry*/, FSlateWindowElementList& /*InOutDrawElements*/, int32& /*InOutLayerId*/);
	static FPaintDebugElements PaintDebugElements;

	/**
	 * Called as soon as the element is added to the element list.
	 * NOTE: These elements are not valid until the widget finishes painting, or you can resolve them all after the window finishes painting.
	 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FDrawElement, const FSlateWindowElementList& /*ElementList*/, int32 /*ElementIndex*/);
	static FDrawElement ElementAdded;

public:
	/**  */
	DECLARE_MULTICAST_DELEGATE_OneParam(FWidgetWarningEvent, const FSlateDebuggingWarningEventArgs& /*EventArgs*/);
	static FWidgetWarningEvent Warning;
	static void BroadcastWarning(const FText& WarningText, const TSharedPtr<SWidget>& OptionalContextWidget);

public:
	/**  */
	DECLARE_MULTICAST_DELEGATE_OneParam(FWidgetInputEvent, const FSlateDebuggingInputEventArgs& /*EventArgs*/);
	static FWidgetInputEvent InputEvent;

	static void BroadcastInputEvent(ESlateDebuggingInputEvent InputEventType, const FInputEvent* InInputEvent, const FReply& InReply);
	static void BroadcastInputEvent(ESlateDebuggingInputEvent InputEventType, const FInputEvent* InInputEvent, const TSharedPtr<SWidget>& HandlerWidget);
	static void BroadcastInputEvent(ESlateDebuggingInputEvent InputEventType, const FInputEvent* InInputEvent, const FReply& InReply, const TSharedPtr<SWidget>& HandlerWidget);
	static void BroadcastInputEvent(ESlateDebuggingInputEvent InputEventType, const FInputEvent* InInputEvent, const FReply& InReply, const TSharedPtr<SWidget>& HandlerWidget, const FString& AdditionalContent);
	static void BroadcastInputEvent(ESlateDebuggingInputEvent InputEventType, const FInputEvent* InInputEvent, const FReply& InReply, const TSharedPtr<SWidget>& HandlerWidget, const FName& AdditionalContent);
	static void BroadcastInputEvent(ESlateDebuggingInputEvent InputEventType, const FInputEvent* InInputEvent, const FReply& InReply, const TSharedPtr<SWidget>& HandlerWidget, const TCHAR AdditionalContent);
	static void BroadcastNoReplyInputEvent(ESlateDebuggingInputEvent InputEventType, const FInputEvent* InInputEvent, const TSharedPtr<SWidget>& HandlerWidget);

public:
	/** */
	struct SLATECORE_API FScopeProcessInputEvent
	{
		ESlateDebuggingInputEvent InputEvent;
		FScopeProcessInputEvent(ESlateDebuggingInputEvent InputEvent, const FInputEvent& Event);
		~FScopeProcessInputEvent();
	};

	/** */
	struct SLATECORE_API FScopeRouteInputEvent
	{
		ESlateDebuggingInputEvent InputEvent;
		FScopeRouteInputEvent(ESlateDebuggingInputEvent InputEvent, const FName& RoutingType);
		~FScopeRouteInputEvent();
	};

	/** */
	static void BroadcastPreProcessInputEvent(ESlateDebuggingInputEvent InputEventType, const TCHAR* InputPrecessorName, bool bHandled);

	/** */
	struct IWidgetInputRoutingEvent
	{
		virtual void OnProcessInput(ESlateDebuggingInputEvent InputEventType, const FInputEvent& Event) = 0;
		virtual void OnPreProcessInput(ESlateDebuggingInputEvent InputEventType, const TCHAR* InputPrecessorName, bool bHandled) = 0;
		virtual void OnRouteInput(ESlateDebuggingInputEvent InputEventType, const FName& RoutedType) = 0;
		virtual void OnInputEvent(ESlateDebuggingInputEvent InputEventType, const FReply& InReply, const TSharedPtr<SWidget>& HandlerWidget) = 0;
		virtual void OnInputRouted(ESlateDebuggingInputEvent InputEventType) = 0;
		virtual void OnInputProcessed(ESlateDebuggingInputEvent InputEventType) = 0;
	};

	static void RegisterWidgetInputRoutingEvent(IWidgetInputRoutingEvent* Event);
	static void UnregisterWidgetInputRoutingEvent(IWidgetInputRoutingEvent* Event);

public:
	/**  */
	DECLARE_MULTICAST_DELEGATE_OneParam(FWidgetFocusEvent, const FSlateDebuggingFocusEventArgs& /*EventArgs*/);
	static FWidgetFocusEvent FocusEvent;

	static void BroadcastFocusChanging(const FFocusEvent& InFocusEvent, const FWeakWidgetPath& InOldFocusedWidgetPath, const TSharedPtr<SWidget>& InOldFocusedWidget, const FWidgetPath& InNewFocusedWidgetPath, const TSharedPtr<SWidget>& InNewFocusedWidget);
	static void BroadcastFocusLost(const FFocusEvent& InFocusEvent, const FWeakWidgetPath& InOldFocusedWidgetPath, const TSharedPtr<SWidget>& InOldFocusedWidget, const FWidgetPath& InNewFocusedWidgetPath, const TSharedPtr<SWidget>& InNewFocusedWidget);
	static void BroadcastFocusReceived(const FFocusEvent& InFocusEvent, const FWeakWidgetPath& InOldFocusedWidgetPath, const TSharedPtr<SWidget>& InOldFocusedWidget, const FWidgetPath& InNewFocusedWidgetPath, const TSharedPtr<SWidget>& InNewFocusedWidget);

public:
	/**  */
	DECLARE_MULTICAST_DELEGATE_OneParam(FWidgetAttemptNavigationEvent, const FSlateDebuggingNavigationEventArgs& /*EventArgs*/);
	static FWidgetAttemptNavigationEvent AttemptNavigationEvent;

	static void BroadcastAttemptNavigation(const FNavigationEvent& InNavigationEvent, const FNavigationReply& InNavigationReply, const FWidgetPath& InNavigationSource, const TSharedPtr<SWidget>& InDestinationWidget, ESlateDebuggingNavigationMethod InNavigationMethod);

	/**  */
	DECLARE_MULTICAST_DELEGATE_OneParam(FWidgetExecuteNavigationEvent, const FSlateDebuggingExecuteNavigationEventArgs& /*EventArgs*/);
	static FWidgetExecuteNavigationEvent ExecuteNavigationEvent;

	static void BroadcastExecuteNavigation();

public:
	/**  */
	DECLARE_MULTICAST_DELEGATE_OneParam(FWidgetMouseCaptureEvent, const FSlateDebuggingMouseCaptureEventArgs& /*EventArgs*/);
	static FWidgetMouseCaptureEvent MouseCaptureEvent;

	static void BroadcastMouseCapture(uint32 UserIndex, uint32 PointerIndex, TSharedPtr<const SWidget> InCapturingWidget);
	static void BroadcastMouseCaptureLost(uint32 UserIndex, uint32 PointerIndex, TSharedPtr<const SWidget> InWidgetLostCapture);

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FWidgetCursorQuery, const FSlateDebuggingCursorQueryEventArgs& /*EventArgs*/);
	static FWidgetCursorQuery CursorChangedEvent;

	static void BroadcastCursorQuery(TSharedPtr<const SWidget> InWidgetOverridingCursor, const FCursorReply& InReply);

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FWidgetInvalidate, const FSlateDebuggingInvalidateArgs& /*EventArgs*/);
	static FWidgetInvalidate WidgetInvalidateEvent;

	static void BroadcastWidgetInvalidate(const SWidget* WidgetInvalidated, const SWidget* WidgetInvalidateInvestigator, EInvalidateWidgetReason InvalidateReason);
	static void BroadcastInvalidationRootInvalidate(const SWidget* WidgetInvalidated, const SWidget* WidgetInvalidateInvestigator, ESlateDebuggingInvalidateRootReason InvalidateReason);

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FWidgetUpdatedEvent, const FSlateDebuggingWidgetUpdatedEventArgs& /*Args*/);
	static FWidgetUpdatedEvent WidgetUpdatedEvent;

	static void BroadcastWidgetUpdated(const SWidget* Invalidated, EWidgetUpdateFlags UpdateFlags);
	static void BroadcastWidgetUpdatedByPaint(const SWidget* Invalidated, EWidgetUpdateFlags UpdateFlags);

public:
	/**  */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FUICommandRun, const FName& /*CommandName*/, const FText& /*CommandLabel*/);
	static FUICommandRun CommandRun;

private:

	// This class is only for namespace use
	FSlateDebugging() = default;

	struct FLastCursorQuery
	{
		const SWidget* WidgetThatOverrideCursorLast_UnsafeToUseForAnythingButCompare = nullptr;
		TWeakPtr<SWidget> CursorWidget;
		EMouseCursor::Type MouseCursor;
	};
	static FLastCursorQuery LastCursorQuery;

	static TArray<IWidgetInputRoutingEvent*> RoutingEvents;
};

#endif