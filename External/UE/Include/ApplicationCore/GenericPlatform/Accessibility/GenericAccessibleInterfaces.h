// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Variant.h"
#include "Stats/Stats.h"
#include "Async/TaskGraphInterfaces.h"

/** Whether a widget should be included in accessibility, and if so, how its text should be retrieved. */
enum class EAccessibleBehavior : uint8
{
	/** Not accessible. */
	NotAccessible,
	/** Accessible, for the implementing library to decide what it means. Given all data about a particular widget, it should try to choose the most-relevant text automatically. */
	Auto,
	/** Accessible, and traverse all child widgets and concat their summary text together. */
	Summary,
	/** Accessible, and retrieve manually-assigned text from a TAttribute. */
	Custom,
	/** Accessible, and use the tooltip's accessible text. */
	ToolTip
};

APPLICATIONCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogAccessibility, Log, All);

#if WITH_ACCESSIBILITY

class FGenericWindow;
class IAccessibleWidget;

DECLARE_STATS_GROUP(TEXT("Accessibility"), STATGROUP_Accessibility, STATCAT_Advanced);

/** What kind of widget to tell the operating system this is. This may be translated to a different type depending on the platform. */
enum class EAccessibleWidgetType : uint8
{
	Unknown,
	Button,
	CheckBox,
	ComboBox,
	Hyperlink,
	Image,
	Layout,
	ScrollBar,
	Slider,
	Text,
	TextEdit,
	Window,
	List,
	ListItem
};

/** Events that can be raised from accessible widgets to report back to the platform */
enum class EAccessibleEvent : uint8
{
	/**
	 * A widget has become focused or unfocused.
	 * OldValue - The old focus state of the widget
	 * NewValue - The new focus state of the widget
	 */
	FocusChange,
	/**
	 * A widget has been clicked, checked, or otherwise activated.
	 * OldValue - N/A
	 * NewValue - N/A
	 */
	Activate,
	/**
	 * Warning: Partial implementation
	 * Notify the user that something has happened. The user is not guaranteed to get this message.
	 * OldValue - N/A
	 * NewValue - An FString of the message to read
	 */
	Notification,
	/**
	 * A widget's parent is about to be changed.
	 * OldValue - The AccessibleWidgetId of the old parent, or InvalidAccessibleWidgetId if there was none
	 * NewValue - The AccessibleWidgetId of the new parent, or InvalidAccessibleWidgetId if there was none
	 */
	ParentChanged,
	/**
	 * The widget was removed from the UI tree or deleted.
	 * OldValue - N/A
	 * NewValue - N/A
	 */
	WidgetRemoved
};

/**
 * An accessible window corresponds to a native OS window. Fake windows that are embedded
 * within other widgets that simply look and feel like windows are not IAccessibleWindows.
 */
class IAccessibleWindow
{
public:
	/** The different states a window can be in to describe its screen anchors */
	enum class EWindowDisplayState
	{
		Normal,
		Minimize,
		Maximize
	};

	/**
	 * Retrieve the native OS window backing this accessible window. This can be safely
	 * casted if you know what OS you're in (ie FWindowsWindows on Windows platform).
	 *
	 * @return The native window causing this accessible window to exist
	 */
	virtual TSharedPtr<FGenericWindow> GetNativeWindow() const = 0;

	/**
	 * Finds the deepest accessible widget in the hierarchy at the specified coordinates. The window
	 * may return a pointer to itself in the case where there are no accessible children at the position.
	 * This could return nullptr in the case where the coordinates are outside the window bounds.
	 *
	 * @param X The X coordinate in absolute screen space
	 * @param Y The Y coordinate in absolute screen space
	 * @return The deepest widget in the UI heirarchy at X,Y
	 */
	virtual TSharedPtr<IAccessibleWidget> GetChildAtPosition(int32 X, int32 Y) = 0;
	/**
	 * Retrieves the currently accessibilit focused widget
	 *
	 * @return The widget that has accessibility focus.
	 */
	virtual TSharedPtr<IAccessibleWidget> GetFocusedWidget() const = 0;
	/**
	 * Request that the window closes itself. This may not happen immediately.
	 */
	virtual void Close() = 0;

	/**
	 * Check if the window can be minimized or maximized.
	 *
	 * @param State Whether to check for minimize or maximize.
	 * @return True if the display state can be switched to, otherwise false.
	 */
	virtual bool SupportsDisplayState(EWindowDisplayState State) const = 0;
	/**
	 * Gets the current state minimize/maximize state of the window.
	 *
	 * @return The display state corresponding to how the window is displayed.
	 */
	virtual EWindowDisplayState GetDisplayState() const = 0;
	/**
	 * Sets a window to be minimized, maximized, or restored to normal.
	 *
	 * @param State What to change the window's display to.
	 */
	virtual void SetDisplayState(EWindowDisplayState State) = 0;
	/**
	 * Whether or not the window is modal.
	 *
	 * @return true if the window is modal, otherwise false.
	 */
	virtual bool IsModal() const = 0;
};

/**
 * A widget that can be triggered to fire an event, such as buttons or checkboxes.
 */
class IAccessibleActivatable
{
public:
	/** Trigger the widget */
	virtual void Activate() = 0;
	/**
	 * Check whether this widget can be toggled between various check states
	 *
	 * @return true if this widget supports being in different states
	 */
	virtual bool IsCheckable() const { return false; }
	/**
	 * If IsCheckable() is true, this gets the current state that the widget is in.
	 * //todo: return ECheckState
	 *
	 * @return true if the current state is considered "on" or "checked"
	 */
	virtual bool GetCheckedState() const { return false; }
};

/**
 * An accessible widget that stores an arbitrary value of any type capable of being serialized into a string.
 * Optional overrides add support for slider-like functionality.
 */
class IAccessibleProperty
{
public:
	/**
	 * Whether the widget is in read-only mode, which could be different than IsEnabled().
	 *
	 * @return true if the widget is in read-only mode.
	 */
	virtual bool IsReadOnly() const { return true; }
	/**
	 * Check if this text is storing password data, indicating that it may need special handling to presenting itself to the user.
	 *
	 * @return true if the text is storing a password or otherwise senstive data that should be hidden.
	 */
	virtual bool IsPassword() const { return false; }
	/**
	 * How much the value should increase/decrease when the user attempts to modify the value using UI controls.
	 * Note: This should always return a positive value. The caller is responsible for negating it when attempting to decrease.
	 *
	 * @return A number suggesting how much to modify GetValue() by when the user wants to increase/decrease the value.
	 */
	virtual float GetStepSize() const { return 0.0f; }
	/**
	 * The maximum allowed value for this property. This should only be used if GetStepSize is not 0.
	 *
	 * @return The maximum value that this property can be assigned when using step sizes.
	 */
	virtual float GetMaximum() const { return 0.0f; }
	/**
	 * The minimum allowed value for this property. This should only be used if GetStepSize is not 0.
	 *
	 * @return The minimum value that this property can be assigned when using step sizes.
	 */
	virtual float GetMinimum() const { return 0.0f; }
	/**
	 * The current value stored by the widget. Even if the underlying value is not a String, it should be serialized to one
	 * in order to match the return type.
	 *
	 * @return A string representing the serialized value stored in the widget.
	 */
	virtual FString GetValue() const = 0;
	/**
	 * The current value stored by the widget as a FVariant. Used on platforms where the type of the value is important for accessibility methods
	 * such as on Mac. .
	 * @return An FVariant that holds the widget's current value and type of value.
	 */
	virtual FVariant GetValueAsVariant() const { return FVariant(); }
	/*
	 * Set the value stored by the widget. While this function accepts a String, there is no way to know
	 * what the underlying data is stored as. The platform layer must retain some additional information
	 * about what kind of widget this is, and ensure it's being called with valid arguments.
	 *
	 * @param Value The new value to assign to the widget, which may need to be converted before assigning to a variable.
	 */
	virtual void SetValue(const FString& Value) {}
};

/**
 * A widget that contains text, with the potential ability to select sections, read specific words/paragraphs, etc.
 * Note: This class is currently incomplete.
 */
class IAccessibleText
{
public:
	/**
	 * Get the full text contained in this widget, even if some if it is clipped.
	 *
	 * @return All the text held by this widget.
	 */
	virtual const FString& GetText() const = 0;
};

/**
* A widget that represents a table such as list views, tile views or tree views 
* Data about the items that are selected in the table or if selection is supported can be queried.
*/
class IAccessibleTable
{
public: 
	/**
	 * Get all of the elements that are selected in the table. 
	 *
	 * @return All the items selected in this table. Returns an empty array if nothing is selected. 
	 */
	virtual TArray<TSharedPtr<IAccessibleWidget>> GetSelectedItems() const { return TArray < TSharedPtr<IAccessibleWidget>>(); }

	/**
	 * Check if the table can select more than one element at a time. 
	 *
	 * @return True if the table can support selection for more than one element at a time. Else false. 
	 */
	virtual bool CanSupportMultiSelection() const { return false; }

	/**
	 * Checks if the table must have an element selected at all times.
	 *
	 * @return True if an element of the table must be selected at all times, else false
	 */
	virtual bool IsSelectionRequired() const { return false; }
};

/**
* A widget that is an element in an accessible table.
* These widgets can be interacted with by selecting them or querying them for the owning table. 
* NOTE: All accessible table rows must have an accessible table as an owner 
*/
class IAccessibleTableRow
{
public:
	/** Selects this table row in the owning table. */
	virtual void Select() {}

	/** Adds this table row to the list of selected items in the owning table */
	virtual void AddToSelection() {}

	/** Removes this table row from the list of selected items in the owning table */
	virtual void RemoveFromSelection() {}

	/**
	 * Checks if this table row is currently selected in the owning table. 
	 *
	 * @return True if this table row is currently selected in the owning table. Else false 
	 */
	virtual bool IsSelected() const { return false; }

	/**
	 * Get the accessible table that owns this accessible table row. 
	 * NOTE: This should always return a valid pointer in concrete classes. An accessible table row must ALWAYS have an accessible table as an owner. 
	 *
	 * @return The owning table 
	 */
	virtual TSharedPtr<IAccessibleWidget> GetOwningTable() const { return nullptr; }
};

typedef int32 AccessibleWidgetId; 

/**
 * Provides the core set of accessible data that is necessary in order for widget traversal and TTS to be implemented.
 * In order to support functionality beyond this, subclasses must implement the other accessible interfaces and
 * then override the As*() functions.
 */
class IAccessibleWidget : public TSharedFromThis<IAccessibleWidget>
{
public:
	IAccessibleWidget() {}
	virtual ~IAccessibleWidget() {}

	static const AccessibleWidgetId InvalidAccessibleWidgetId = -1;

	/**
	 * Get an application-unique identifier for this widget. If the widget is destroyed,
	 * a different widget is allowed to re-use that ID.
	 *
	 * @return A unique ID that specifically refers to this widget.
	 */
	virtual AccessibleWidgetId GetId() const = 0;
	/**
	 * Whether or not the underlying widget backing this interface still exists
	 *
	 * @return true if functions can be called on this interface and should return valid results
	 */
	virtual bool IsValid() const = 0;

	/**
	 * Returns the window at the top of this widget's hierarchy. This function may return itself for accessible windows,
	 * and could return nullptr in cases where the widget is not currently part of a hierarchy.
	 *
	 * @return The root window in this widget's widget tree, or nullptr if there is no window.
	 */
	virtual TSharedPtr<IAccessibleWidget> GetWindow() const = 0;
	/**
	 * Retrieving the bounding rect in absolute coordinates for this widget. On some platforms this may be used for hit testing.
	 *
	 * @return The bounds of the widget.
	 */
	virtual FBox2D GetBounds() const = 0;
	/**
	 * Get the accessible parent of this widget. This may be nullptr if this widget is a window, or if the widget is
	 * currently disconnected from the UI tree.
	 *
	 * @return The accessible parent widget of this widget.
	 */
	virtual TSharedPtr<IAccessibleWidget> GetParent() = 0;
	/**
	 * Retrieves the widget after this one in the parent's list of children. This should return nullptr for the last widget.
	 *
	 * @return The next widget on the same level of the UI hierarchy.
	 */
	virtual TSharedPtr<IAccessibleWidget> GetNextSibling() = 0;
	/**
	 * Retrieves the widget before this one in the parent's list of children. This should return nullptr for the first widget.
	 *
	 * @return The previous widget on the same level of the UI hierarchy.
	 */
	virtual TSharedPtr<IAccessibleWidget> GetPreviousSibling() = 0;
	/**
	 * Retrieves the accessible child widget at a certain index. This should return nullptr if Index < 0 or Index >= GetNumberOfChildren().
	 *
	 * @param The index of the child widget to get
	 * @return The accessible child widget at the specified index.
	 */
	virtual TSharedPtr<IAccessibleWidget> GetChildAt(int32 Index) = 0;
	/**
	 * How many accessible children this widget has.
	 *
	 * @return The number of accessible children that exist for this widget.
	 */
	virtual int32 GetNumberOfChildren() = 0;
	/**
	 * What type of accessible widget the underlying widget should be treated as. A widget may be capable of presenting itself
	 * as multiple different types of widgets, but only one can be reported back to the platform.
	 *
	 * @return Which type of widget the platform layer should treat this as.
	 */
	virtual EAccessibleWidgetType GetWidgetType() const = 0;

	/**
	 * The name of the underlying class that this accessible widget represents.
	 *
	 * @return The class name of the underlying widget.
	 */
	virtual FString GetClassName() const = 0;
	/**
	 * The name of the widget to report to the platform layer. For screen readers, this is often the text that will be spoken.
	 *
	 * @return Ideally, a human-readable name that represents what the widget does.
	 */
	virtual FString GetWidgetName() const = 0;
	/**
	 * Additional information a user may need in order to effectively interact or use the widget, such as a tooltip.
	 *
	 * @return A more-detailed description of what the widget is or how its used.
	 */
	virtual FString GetHelpText() const = 0;

	/**
	 * Whether the widget is enabled and can be interacted with.
	 *
	 * @return true if the widget is enabled.
	 */
	virtual bool IsEnabled() const = 0;
	/**
	 * Whether the widget is being rendered on screen or not.
	 *
	 * @return true if the widget is hidden off screen, collapsed, or something similar.
	 */
	virtual bool IsHidden() const = 0;
	/**
	 * Whether the widget supports accessibility focus or not.
	 *
	 * @return true if the widget can receive accessibility focus.
	 */
	virtual bool SupportsFocus() const = 0;
	/**
	 * Whether the widget has accessibility focus or not.
	 *
	 * @return true if the widget currently has accessibility focus.
	 */
	virtual bool HasFocus() const = 0;
	/** Assign keyboard/gamepad focus to this widget, if it supports it. If not, focus should not be affected. 
	* Note: For widgets that support accessibility focus but not keyboard/gamepad focus, this should NOT repreplace the widget with accessibility focus. 
	*/
	virtual void SetFocus() = 0;

	/**
	 * Attempt to cast this to an IAccessibleWindow
	 *
	 * @return 'this' as an IAccessibleWindow if possible, otherwise nullptr
	 */
	virtual IAccessibleWindow* AsWindow() { return nullptr; }
	/**
	 * Attempt to cast this to an IAccessibleActivatable
	 *
	 * @return 'this' as an IAccessibleActivatable if possible, otherwise nullptr
	 */
	virtual IAccessibleActivatable* AsActivatable() { return nullptr; }
	/**
	 * Attempt to cast this to an IAccessibleProperty
	 *
	 * @return 'this' as an IAccessibleProperty if possible, otherwise nullptr
	 */
	virtual IAccessibleProperty* AsProperty() { return nullptr; }
	/**
	 * Attempt to cast this to an IAccessibleText
	 *
	 * @return 'this' as an IAccessibleText if possible, otherwise nullptr
	 */
	virtual IAccessibleText* AsText() { return nullptr; }

	/**
	 * Attempt to cast this to an IAccessibleTable
	 *
	 * @return 'this' as an IAccessibleTable if possible, otherwise nullptr
	 */
	virtual IAccessibleTable* AsTable() { return nullptr; }

	/**
	 * Attempt to cast this to an IAccessibleTableRow
	 *
	 * @return 'this' as an IAccessibleTableRow if possible, otherwise nullptr
	 */
	virtual IAccessibleTableRow* AsTableRow() { return nullptr; }
};

/**
 * Platform and application-agnostic messaging system for accessible events. The message handler
 * lives in GenericApplication and any subclass that wishes to support accessibility should subclass
 * this and use GenericAppliation::SetAccessibleMessageHandler to enable functionality.
 *
 * GetAccessibleWindow() is tne entry point to all accessible widgets. Once the window is retrieved, it
 * can be queried for children in various ways. RaiseEvent() allows messages to bubble back up to the
 * native OS through anything bound to the AccessibleEventDelegate.
 *
 * Callers can use ApplicationIsAccessible() to see if accessibility is supported or not. Alternatively,
 * calling GetAccessibleWindow and seeing if the result is valid should provide the same information.
 */
class FGenericAccessibleMessageHandler
{
public:
	/** A widget raised an event to pass to the native OS implementation. */
	DECLARE_DELEGATE_FourParams(
		FAccessibleEvent,
		/** The accessible widget that generated the event */
		TSharedRef<IAccessibleWidget>,
		/** The type of event generated */
		EAccessibleEvent,
		/** If this was a property changed event, the 'before' value */
		FVariant,
		/** If this was a property changed event, the 'after' value. This may also be set for other events such as Notification. */
		FVariant);

	FGenericAccessibleMessageHandler() : bApplicationIsAccessible(false), bIsActive(false) {}

	virtual ~FGenericAccessibleMessageHandler()
	{
		UnbindAccessibleEventDelegate();
	}

	/**
	 * Subclasses should return true to indicate that they support accessibility.
	 *
	 * @return true if the application intends to return valid accessible widgets when queried.
	 */
	bool ApplicationIsAccessible() const;

	/**
	 * Checks if accessibility is enabled in the application. Usually this happens when screen-reading software is turned on.
	 * Note: On some platforms, there is no way to deactivate this after enabling it. 
	 *
	 * @return The last value SetActive() was called with.
	 */
	bool IsActive() const { return bIsActive; }

	/**
	 * Notify the application to start or stop processing accessible messages from the platform layer.
	 *
	 * @param bActive Whether to enable to disable the message handler.
	 */
	void SetActive(bool bActive);

	/**
	 * Creates or retrieves an accessible object for a native OS window.
	 * todo: Behavior for non-native windows (virtual or others) is currently undefined.
	 *
	 * @param InWindow The native window to find the accessible window for
	 * @return The accessible object corresponding to the supplied native window
	 */
	virtual TSharedPtr<IAccessibleWidget> GetAccessibleWindow(const TSharedRef<FGenericWindow>& InWindow) const { return nullptr; }

	/**
	 * Creates or retrieves the identifier for an accessible object for a native OS window.
	 * todo: Behavior for non-native windows (virtual or others) is currently undefined.
	 *
	 * @param InWindow The native window to find the accessible window for
	 * @return The identifier for the accessible window created
	 */
	virtual AccessibleWidgetId GetAccessibleWindowId(const TSharedRef<FGenericWindow>& InWindow) const { return IAccessibleWidget::InvalidAccessibleWidgetId; }

	/**
	 * Retrieves an accessible widget that matches the given identifier.
	 *
	 * @param Id The identifier for the widget to get.
	 * @return The widget that matches this identifier, or nullptr if the widget does not exist.
	 */
	virtual TSharedPtr<IAccessibleWidget> GetAccessibleWidgetFromId(AccessibleWidgetId Id) const { return nullptr; }

	/**
	 * Push an event from an accessible widget back to the platform layer.
	 *
	 * @param Widget The widget raising the event
	 * @param Event The type of event being raised
	 * @param OldValue See EAccessibleEvent documentation for more details.
	 * @param NewValue See EAccessibleEvent documentation for more details.
	 */
	void RaiseEvent(TSharedRef<IAccessibleWidget> Widget, EAccessibleEvent Event, FVariant OldValue = FVariant(), FVariant NewValue = FVariant())
	{
		AccessibleEventDelegate.ExecuteIfBound(Widget, Event, OldValue, NewValue);
	}

	/**
	 * Assign a function to be called whenever an accessible event is raised.
	 *
	 * @param Delegate The delegate to execute when an event is raised.
	 */
	void SetAccessibleEventDelegate(const FAccessibleEvent& Delegate)
	{
		AccessibleEventDelegate = Delegate;
	}

	/**
 * Unbinds the delegate called during accessible events if it is bound. 
 */
	void UnbindAccessibleEventDelegate()
	{
		AccessibleEventDelegate.Unbind();
	}

	/**
	 * Request a function to be run in a particular thread. 
	 * Primarily used to service accessibility requests from the OS thread about accessible Slate widget data in the game thread. 
	 *
	 * @param Function The function to execute in the game thread.
	 * @bWaitForCompletion If true, the function will block until the requested Function completes executing on the requested Thread
	 * @param Thread The thread to run the Function in.
	 */
	virtual void RunInThread(const TFunction<void()>& Function, bool bWaitForCompletion = true, ENamedThreads::Type Thread = ENamedThreads::GameThread) { }

	/**
	 * Request a string to be announced to the user.
	 * Note: Currently only supported on Win10, Mac, iOS. If requests are made too close together,
	 * earlier announcement requests will get stomped by later announcements on certain platforms.
	 */
	virtual void MakeAccessibleAnnouncement(const FString& AnnouncementString) { }
protected:
	/** Triggered when bIsActive changes from false to true. */
	virtual void OnActivate() {}
	/** Triggered when bIsActive changes from true to false. */
	virtual void OnDeactivate() {}

	/** Subclasses should override this to indicate that they support accessibility. */
	bool bApplicationIsAccessible;

private:
	/** Whether or not accessibility is currently enabled in the application */
	bool bIsActive;
	/** Delegate for the platform layer to listen to widget events */
	FAccessibleEvent AccessibleEventDelegate;
};

#endif
