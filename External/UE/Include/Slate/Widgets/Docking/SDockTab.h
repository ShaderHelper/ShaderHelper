// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Layout/Margin.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateColor.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "Styling/StyleDefaults.h"
#include "Animation/CurveSequence.h"
#include "Widgets/Layout/SBorder.h"
#include "Framework/Docking/TabManager.h"

class FActiveTimerHandle;
class SDockingArea;
class SDockingTabStack;
class SDockingTabWell;
class SImage;
class STextBlock;
class SToolTip;

/** How will this tab be used. */
enum ETabRole : uint8
{
	MajorTab,
	PanelTab,
	NomadTab,
	DocumentTab,
	NumRoles
};

	/** The cause of a tab activation */
enum ETabActivationCause : uint8
{
	UserClickedOnTab,
	SetDirectly
};

class FMenuBuilder;

/**
 * A tab widget that also holds on to some content that should be shown when this tab is selected.
 * Intended to be used in conjunction with SDockingTabStack.
 */
class SLATE_API SDockTab : public SBorder
{
	friend class FTabManager;
	friend class STabSidebar;
public:

	/** Invoked when a tab is closing */
	DECLARE_DELEGATE_OneParam(FOnTabClosedCallback, TSharedRef<SDockTab>);
	
	/** Invoked when a tab is activated */
	DECLARE_DELEGATE_TwoParams(FOnTabActivatedCallback, TSharedRef<SDockTab>, ETabActivationCause);

	/** Invoked when a tab is renamed */
	DECLARE_DELEGATE_OneParam(FOnTabRenamed, TSharedRef<SDockTab>);

	/** Invoked w`en this tab should save some information about its content. */
	DECLARE_DELEGATE(FOnPersistVisualState);

	/** Delegate called before a tab is closed.  Returning false will prevent the tab from closing */
	DECLARE_DELEGATE_RetVal( bool, FCanCloseTab );

	/** Invoked to add entries to the tab context menu */
	DECLARE_DELEGATE_OneParam(FExtendContextMenu, FMenuBuilder&);

	SLATE_BEGIN_ARGS(SDockTab)
		: _Content()
		, _TabWellContentLeft()
		, _TabWellContentRight()
		, _ContentPadding(0.f)
		, _TabRole(ETabRole::PanelTab)
		, _Label()
		, _LabelSuffix()
		, _OnTabClosed()
		, _OnTabActivated()
		, _OnTabRelocated()
		, _OnTabDraggedOverDockArea()
		, _ShouldAutosize(false)
		, _OnCanCloseTab()
		, _OnPersistVisualState()
		, _TabColorScale(FLinearColor::Transparent)
		, _ForegroundColor(FSlateColor::UseStyle())
		, _IconColor()
		{}

		SLATE_DEFAULT_SLOT( FArguments, Content )
		SLATE_NAMED_SLOT( FArguments, TabWellContentLeft )
		SLATE_NAMED_SLOT( FArguments, TabWellContentRight )
		SLATE_ATTRIBUTE( FMargin, ContentPadding )
		SLATE_ARGUMENT( ETabRole, TabRole )
		SLATE_ATTRIBUTE( FText, Label )
		SLATE_ATTRIBUTE(FText, LabelSuffix)
		UE_DEPRECATED(5.0, "Tab icons are now being managed by tab spawners and toolkits. In the rare case you need to set an icon manually, use SetTabIcon() instead")
		FArguments& Icon(const FSlateBrush* InIcon)
		{
			return Me();
		}

		SLATE_EVENT( FOnTabClosedCallback, OnTabClosed )
		SLATE_EVENT( FOnTabActivatedCallback, OnTabActivated )
		SLATE_EVENT( FSimpleDelegate, OnTabRelocated )
		SLATE_EVENT( FSimpleDelegate, OnTabDraggedOverDockArea )
		SLATE_ARGUMENT( bool, ShouldAutosize )
		SLATE_EVENT( FCanCloseTab, OnCanCloseTab )
		SLATE_EVENT( FOnPersistVisualState, OnPersistVisualState )
		SLATE_EVENT( FExtendContextMenu, OnExtendContextMenu )
		/** Invoked when a tab is closed from a drawer. This does not mean the tab or its contents is destroyed, just hidden. Use OnTabClosed for that */
		SLATE_EVENT( FSimpleDelegate, OnTabDrawerClosed)
		SLATE_ATTRIBUTE( FLinearColor, TabColorScale )
		SLATE_ATTRIBUTE( FSlateColor, ForegroundColor )
		SLATE_ATTRIBUTE( FLinearColor, IconColor)
	SLATE_END_ARGS()



	/** Construct the widget from the declaration. */
	void Construct( const FArguments& InArgs );

	// SWidget interface
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent  ) override;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual void OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) override;
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
	virtual FReply OnTouchStarted( const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent ) override;
	virtual FReply OnTouchEnded( const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent ) override;
	// End of SWidget interface

	// SBorder interface
	virtual void SetContent(TSharedRef<SWidget> InContent) override;
	// End of SBorder interface

	/** Content that appears in the TabWell to the left of the tabs */
	void SetLeftContent(TSharedRef<SWidget> InContent);
	/** Content that appears in the TabWell to the right of the tabs */
	void SetRightContent(TSharedRef<SWidget> InContent);
	/** Content that appears on the right side of the title bar in the window this stack is in */
	void SetTitleBarRightContent(TSharedRef<SWidget> InContent);

	/** @return True if this tab is currently focused */
	bool IsActive() const;

	/** @return True if this tab appears active; False otherwise */
	bool IsForeground() const;

	/** @return the Foreground color that this widget sets; unset options if the widget does not set a foreground color */
	virtual FSlateColor GetForegroundColor() const;

	/** Add any entries specific to this tab to the tab context menu */
	void ExtendContextMenu(FMenuBuilder& MenuBuilder);

	/** Is this an MajorTab? A tool panel tab? */
	ETabRole GetTabRole() const;

	/** Similar to GetTabRole() but returns the correct role for UI style and user input purposes */
	ETabRole GetVisualTabRole() const;

	/**
	 * What should the content area look like for this type of tab?
	 * Documents, Apps, and Tool Panels have different backgrounds.
	 */
	const FSlateBrush* GetContentAreaBrush() const;

	/** Depending on the tabs we put into the tab well, we want a different background brush. */
	const FSlateBrush* GetTabWellBrush() const;

	/** @return the content associated with this tab */
	TSharedRef<SWidget> GetContent();
	TSharedRef<SWidget> GetLeftContent();
	TSharedRef<SWidget> GetRightContent();
	TSharedRef<SWidget> GetTitleBarRightContent();

	/** Padding around the content when it is presented by the SDockingTabStack */
	FMargin GetContentPadding() const;

	/** Gets this tab's layout identifier */
	const FTabId GetLayoutIdentifier() const;

	/** Sets the tab's tab well parent, or resets it if nothing is passed in */
	void SetParent(TSharedPtr<SDockingTabWell> Parent = TSharedPtr<SDockingTabWell>());

	/** Gets the tab's tab well parent, or nothing, if it has none */
	TSharedPtr<SDockingTabWell> GetParent() const;

	/** Gets the dock tab stack this dockable tab resides within, if any */
	TSharedPtr<SDockingTabStack> GetParentDockTabStack() const;

	/** Gets the dock area that this resides in */
	TSharedPtr<class SDockingArea> GetDockArea() const;

	/** Get the window in which this tab's tabmanager has placed it */
	TSharedPtr<SWindow> GetParentWindow() const;

	/** The width that this tab will overlap with side-by-side tabs. */
	float GetOverlapWidth() const;

	/** The label on the tab */
	FText GetTabLabel() const;

	/** The label that appears on the tab. */
	void SetLabel( const TAttribute<FText>& InTabLabel );

	/** Get Label Suffix */
	FText GetTabLabelSuffix() const;

	/** Set Label Suffix.  A second text field at the end of the Label that takes precedence and isn't lost when space is restricted */
	void SetTabLabelSuffix(const TAttribute<FText>& InTabLabelSuffix);

	/** The tooltip text that appears on the tab. */
	void SetTabToolTipWidget(TSharedPtr<SToolTip> InTabToolTipWidget);

	/** Gets the tab icon */
	const FSlateBrush* GetTabIcon() const;

	/** Sets the tab icon */
	void SetTabIcon( const TAttribute<const FSlateBrush*> InTabIcon );

	/** Should this tab be sized based on its content. */
	bool ShouldAutosize() const;

	/** Set whether this tab should be sized based on its content. */
	void SetShouldAutosize(const bool bNewShouldAutosize);

	/** @return true if the tab can be closed */
	bool CanCloseTab() const;

	/** Requests that the tab be closed.  Tabs may prevent closing depending on their state */	
	bool RequestCloseTab();

	/** A chance for the tab's content to save any internal layout info */
	void PersistVisualState();

	/** 
	 * Pulls this tab out of it's parent tab stack and destroys it
	 * Note: This does not check if its safe to remove the tab.  Use RequestCloseTab to do this safely.
	 */
	void RemoveTabFromParent();

	/** Protected constructor; Widgets may only be constructed via a FArguments (i.e.: SNew(SDockTab) ) */
	SDockTab();

	/** 
	 * Make this tab active in its tabwell 
	 * @param	InActivationMethod	How this tab was activated.
	 */
	void ActivateInParent(ETabActivationCause InActivationCause);

	/** Set the tab manager that is controlling this tab */
	void SetTabManager( const TSharedPtr<FTabManager>& InTabManager );

	/**
	 * Set the custom code to execute for saving visual state in this tab.
	 * e.g. ContentBrowser saves the visible filters.OnExtendContextMenu
	 */
	void SetOnPersistVisualState( const FOnPersistVisualState& Handler );

	/** Set the handler to be invoked when the user requests that this tab be closed. */
	void SetCanCloseTab( const FCanCloseTab& InOnTabClosing );

	/** Set the handler that will be invoked when the tab is closed */
	void SetOnTabClosed( const FOnTabClosedCallback& InDelegate );

	/** Set the handler that will be invoked when the tab is activated */
	void SetOnTabActivated( const FOnTabActivatedCallback& InDelegate );

	/** Set the handler that will be invoked when the tab is relocated to a new tab well */
	void SetOnTabRelocated(const FSimpleDelegate InDelegate);

	/** Set the handler that will be invoked when the tab is dragged over dock area */
	void SetOnTabDraggedOverDockArea(const FSimpleDelegate InDelegate);

	/** Set the handler that will be invoked when the tab is renamed */
	void SetOnTabRenamed(const FOnTabRenamed& InDelegate);

	/** Set the handler that will be invoked when the tab is opened from a drawer */
	void SetOnTabDrawerOpened(const FSimpleDelegate InDelegate);

	/** Set the handler that will be invoked when the tab is closed from a drawer */
	void SetOnTabDrawerClosed(const FSimpleDelegate InDelegate);

	/** Set the handler for extending the tab context menu */
	void SetOnExtendContextMenu( const FExtendContextMenu& Handler );

	/** Get the tab manager currently managing this tab. Note that a user move the tab between Tab Managers, so this return value may change. */
	UE_DEPRECATED(5.0, "The tab manager is not guaranteed to exist, which will cause GetTabManager() to crash. Use GetTabManagerPtr() instead.")
	TSharedRef<FTabManager> GetTabManager() const;

	/** Get the tab manager currently managing this tab. Note that a user move the tab between Tab Managers, so this return value may change. */
	TSharedPtr<FTabManager> GetTabManagerPtr() const;

	/** Draws attention to the tab. */
	void DrawAttention();

	/** Provide a default tab label in case the spawner did not set one. */
	void ProvideDefaultLabel( const FText& InDefaultLabel );

	/** Play an animation showing this tab as opening */
	void PlaySpawnAnim();

	/** Flash the tab, used for drawing attention to it */
	void FlashTab();

	/** Used by the drag/drop operation to signal to this tab what it is dragging over. */
	void SetDraggedOverDockArea( const TSharedPtr<SDockingArea>& Area );

	/** 
	 * Check to see whether this tab has a sibling tab with the given tab ID
	 * 
	 * @param	SiblingTabId				The ID of the tab we want to find
	 * @param	TreatIndexNoneAsWildcard	Note that this variable only takes effect if SiblingTabId has an InstanceId of INDEX_NONE.
	 *										If true, we will consider this a "wildcard" search (matching any tab with the correct TabType, regardless 
	 *										of its InstanceId). If false, we will explicitly look for a tab with an InstanceId of INDEX_NONE
	 */
	bool HasSiblingTab(const FTabId& SiblingTabId, const bool TreatIndexNoneAsWildcard = true) const;

	/** Updates the 'last activated' time to the current time */
	void UpdateActivationTime();

	/** Returns the time this tab was last activated */
	double GetLastActivationTime()
	{
		return LastActivationTime;
	}

protected:
	/** Provide a default tab icon. */
	void ProvideDefaultIcon(const FSlateBrush* InDefaultIcon);

	/** @return the style currently applied to the dock tab */
	const FDockTabStyle& GetCurrentStyle() const;

	/** @return the image brush that best represents this tab's in its current state */
	const FSlateBrush* GetImageBrush() const;

	/** @return the padding for the tab widget */
	FMargin GetTabPadding() const;

	/** @return the image brush for the tab's color overlay */
	const FSlateBrush* GetColorOverlayImageBrush() const;

	/** @return The visibility of the active tab indicator */
	EVisibility GetActiveTabIndicatorVisibility() const;

	/** @return Returns a color to scale the background of this tab by */
	FSlateColor GetTabColor() const;

	/** @return Returns the color of this tab's icon */
	FSlateColor GetIconColor() const;

	/** @return the image brush for the tab's flasher overlay */
	const FSlateBrush* GetFlashOverlayImageBrush() const;

	/** @return Returns a color to flash the background of this tab with */
	FSlateColor GetFlashColor() const;

	/** Called when the close button is clicked on the tab. */
	FReply OnCloseButtonClicked();

	/** The close button tooltip showing the appropriate close command shortcut */
	FText GetCloseButtonToolTipText() const;

	/** Specify the TabId that was used to spawn this tab. */
	void SetLayoutIdentifier( const FTabId& TabId );

	/** @return if the close button should be visible. */
	EVisibility HandleIsCloseButtonVisible() const;

	/** @return the size the tab icon should be */
	TOptional<FVector2D> GetTabIconSize() const;

	/** @return the padding for the tab icon border */
	FMargin GetTabIconBorderPadding() const;

private:
	/** Activates the tab in its tab well */
	EActiveTimerReturnType TriggerActivateTab( double InCurrentTime, float InDeltaTime );

	EActiveTimerReturnType OnHandleUpdateStyle(double InCurrentTime, float InDeltaTime);

	void OnParentSet();

	void UpdateTabStyle();

	void OnTabDrawerOpened();
	void OnTabDrawerClosed();

	void NotifyTabRelocated();

	/** The handle to the active tab activation tick */
	TWeakPtr<FActiveTimerHandle> DragDropTimerHandle;
	TWeakPtr<FActiveTimerHandle> UpdateStyleTimerHandle;
protected:

	/** The tab manager that created this tab. */
	TWeakPtr<FTabManager> MyTabManager;

	/** The stuff to show when this tab is selected */
	TSharedRef<SWidget> Content;
	TSharedRef<SWidget> TabWellContentLeft;
	TSharedRef<SWidget> TabWellContentRight;
	TSharedRef<SWidget> TitleBarContentRight;
	
	/** The tab's layout identifier */
	FTabId LayoutIdentifier;

	/** Is this an MajorTab? A tool panel tab? */
	ETabRole TabRole;

	/** The tab's parent tab well. Null if it is a floating tab. */
	TWeakPtr<SDockingTabWell> ParentPtr;

	/** The label on the tab */
	TAttribute<FText> TabLabel;

	/** A second text field at the end of the Label that takes precedence and isn't lost when space is restricted */
	TAttribute<FText> TabLabelSuffix;

	/** The icon on the tab */
	TAttribute<const FSlateBrush*> TabIcon;
	
	/** Callback to call when this tab is destroyed */
	FOnTabClosedCallback OnTabClosed;

	/** Callback to call when this tab is activated */
	FOnTabActivatedCallback OnTabActivated;

	/** Delegate to execute to determine if we can close this tab */
	FCanCloseTab OnCanCloseTab;

	FSimpleDelegate OnTabDrawerClosedEvent;

	FSimpleDelegate OnTabDrawerOpenedEvent;

	FSimpleDelegate OnTabRelocated;

	FSimpleDelegate OnTabDraggedOverDockArea;

	FExtendContextMenu OnExtendContextMenu;

	/**
	 * Invoked during the Save Visual State pass; gives this tab a chance to save misc info about visual state.
	 * e.g. Content Browser might save the current filters, current folder, whether some panel is collapsed, etc.
	 */
	FOnPersistVisualState OnPersistVisualState;

	/** Invoked when the tab is renamed */
	FOnTabRenamed OnTabRenamed;

	/** The styles used to draw the tab in its various states */
	const FDockTabStyle* MajorTabStyle;
	const FDockTabStyle* GenericTabStyle;

	TAttribute<FMargin> ContentAreaPadding;

	/** Should this tab be auto-sized based on its content? */
	bool bShouldAutosize;

	/** Color of this tab */
	TAttribute<FLinearColor> TabColorScale;

	/** Color of this tab's icon */
	TAttribute<FLinearColor> IconColor;

	/** @return the scaling of the tab based on the opening/closing animation */
	FVector2D GetAnimatedScale() const;

	/** Animation that shows the tab opening up */
	FCurveSequence SpawnAnimCurve;

	/** Animation that causes the tab to flash */
	FCurveSequence FlashTabCurve;

	/** Get the desired color of tab. These change during flashing. */
	float GetFlashValue() const;

	/** The dock area this tab is currently being dragged over. Used in nomad tabs to change style */
	TSharedPtr<SDockingArea> DraggedOverDockingArea;

	/** Widget used to show the label on the tab */
	TSharedPtr<STextBlock> LabelWidget;
	TSharedPtr<STextBlock> LabelSuffix;

	/** Widget used to show the icon on the tab */
	TSharedPtr<SImage> IconWidget;
	
	/** Time this tab was last activated */
	double LastActivationTime;
};
