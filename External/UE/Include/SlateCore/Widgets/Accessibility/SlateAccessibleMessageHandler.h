// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_ACCESSIBILITY

#include "GenericPlatform/Accessibility/GenericAccessibleInterfaces.h"
#include "Misc/Variant.h"

class FSlateAccessibleWidget;
class SWidget;
class SWindow;

// Used to radically increase the responsiveness of accessibility navigation for debugging pruposes. The implementation is too slow to be feasible for shipping builds. 
#define ACCESSIBILITY_DEBUG_RESPONSIVENESS 0 && !UE_BUILD_SHIPPING

/**
 * Message handling system for Slate Accessibility API, dealing with both receiving events and pushing them back to the platform layer.
 * The message handler is also responsible for processing the Slate widget tree and queuing/processing accessibility requests for widget data from the platform.
 */
class SLATECORE_API FSlateAccessibleMessageHandler : public FGenericAccessibleMessageHandler
{
public:
	FSlateAccessibleMessageHandler();

	// FGenericAccessibleMessageHandler
	virtual void OnActivate() override;
	virtual void OnDeactivate() override;
	virtual TSharedPtr<IAccessibleWidget> GetAccessibleWindow(const TSharedRef<FGenericWindow>& InWindow) const override;
	virtual AccessibleWidgetId GetAccessibleWindowId(const TSharedRef<FGenericWindow>& InWindow) const override;
	virtual TSharedPtr<IAccessibleWidget> GetAccessibleWidgetFromId(AccessibleWidgetId Id) const override;
	virtual void RunInThread(const TFunction<void()>& InFunction, bool bWaitForCompletion = true, ENamedThreads::Type InThread = ENamedThreads::GameThread) override;
	virtual void MakeAccessibleAnnouncement(const FString& AnnouncementString) override;
	//~

	/**
	 * Callback for SWidget destructor. Removes the corresponding accessible widget for the Slate widget.
	 *
	 * @param Widget The widget that is being deleted.
	 */
	void OnWidgetRemoved(SWidget* Widget);

	/**
	 * Callback for a Slate widget indicating that a property change occurred. This may also be used by certain events
	 * such as Notification which don't have an 'OldValue'. Only NewValue should be set for those types of events.
	 *
	 * @param Widget The widget raising the event.
	 * @param Event The type of event being raised.
	 * @param OldValue The value of the property being changed before the change occurred.
	 * @param NewValue The value of the property being changed after the change occurred, or any miscellaneous data for the event.
	 */
	void OnWidgetEventRaised(TSharedRef<SWidget> Widget, EAccessibleEvent Event, FVariant OldValue = FVariant(), FVariant NewValue = FVariant());

	/**
	 * Refresh the accessible widget tree next available tick. This should be called any time the Slate tree changes.
	 */
	void MarkDirty() { bDirty = true; }

	/**
	* Processes all the queued tasks in the AccessibleTaskQueue
	* Should only be called from FSlateApplication::TickPlatform() 
	*/
	void ProcessAccessibleTasks();

	/**
	 * Process any pending Slate widgets and update the accessible widget tree.
	 */
	void Tick();

	/**
	* Returns the Widget that currently has accessibility focus in the application.
	* accessibility focusable widgets are a superset of keyboard/gamepad focusable widgetes.
	* i.e A widget can support accessibility focus but NOT keyboard/gamepad focus.
	* Can return nullptr if no widget has accessibility focus. Up to caller to do validity check.
	* @return  The widget that currently has accessible focus.
	*/
	TSharedPtr<FSlateAccessibleWidget> GetAccessibilityFocusedWidget() const;

	/**
	* Sets the currently accessibility focused widget in the application.
	* If you're trying to clear the accessibility focus, use ClearAccessibilityFocus() 
	* @param NewAccessibleFocusedWidget The widget that  now has accessible focus
	*/
	void SetAccessibilityFocusedWidget(const TSharedRef<FSlateAccessibleWidget>& NewAccessibilityFocusedWidget);

	/** Reset the accessibility focused widget to nullptr*/
	void ClearAccessibilityFocus();
private:
	/**
	*  A helper class that wraps an accessibility task and the event to be triggered when the task finishes executing. 
	*/
	class FSlateAccessibleTask
	{
	public:
		/** If completion event is nullptr, The task is treated as asynchronous */
		FSlateAccessibleTask(const TFunction<void()>& InTask, FEvent* InCompletionEvent = nullptr)
			: Task(InTask)
			, CompletionEvent(InCompletionEvent)
		{
			check(Task);
		}

		/** Executes the task and triggers the completion event if available */
		void DoTask()
		{
			// We should always have a valid task 
			check(Task);
			Task();
			if (CompletionEvent)
			{
				CompletionEvent->Trigger();
			}
		}
	private:
		/** A lambda function that requests accessibility data from Slate in the Game Thread */
		TFunction<void()> Task;

		/** The event to be triggered when the task has completed execution. Can be null if the task should be executed asynchronously */
		FEvent* CompletionEvent;
	};

	/** Queues an FSlateAccessibleTask to be processed in the Game Thread */
	void EnqueueAccessibleTask(const FSlateAccessibleTask& InAccessibleTask);

	struct FWidgetAndParent {
		FWidgetAndParent(TWeakPtr<SWidget> InWidget, TSharedRef<FSlateAccessibleWidget> InParent)
			: Widget(InWidget), Parent(InParent)
		{
		}

		TWeakPtr<SWidget> Widget;
		TSharedRef<FSlateAccessibleWidget> Parent;
	};

	/** A list of widgets waiting to be processed in order to keep the accessible widget tree up to date */
	TArray<FWidgetAndParent> ToProcess;

	/** 
	* A queue of FSlateAccessibleTasks to be processed in the Game Thread 
	* This queue is only meant to store tasks, NOT process them . 
	* We opt for a TArray as a TQueue has slower performance in this use case 
	* @see ProcessAccessibleTasks() 
	*/
	TArray<FSlateAccessibleTask> AccessibleTaskStorageQueue;
	/**
	* The queue used to process all queued accessible tasks 
	* The contents of AccessibleTaskStorageQueue is  swapped inot this queue as an optimization to avoid holding on to the lock 
	* @see ProcessAccessibleTasks() 
	*/
	TArray<FSlateAccessibleTask> AccessibleTaskProcessingQueue;

	/** Critical section used for synchronization of the AccessibleTaskQueue */
	FCriticalSection QueueCriticalSection;

#if ACCESSIBILITY_DEBUG_RESPONSIVENESS
	FEvent* EnqueueEvent;
#endif 

	/** 
	* The widget tht currently has accessibility focus 
	* @see GetAccessibilityFocusedWidget, SetAccessibilityFocusedWidget
	*/
	TWeakPtr<FSlateAccessibleWidget> AccessibilityFocusedWidget;

	/** If true, Tick() will begin the update process to the accessible widget tree. Use MarkDirty() to set. */
	bool bDirty;
};

#endif
