// Copyright Epic Games, Inc. All Rights Reserved
 
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Framework/SlateDelegates.h"
#include "Widgets/Views/STableViewBase.h"
#include "Styling/SlateTypes.h"
#include "Framework/Views/TableViewTypeTraits.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Layout/Overscroll.h"
#include "Widgets/Views/SListView.h"

/** Info needed by a (relatively) small fraction of the tree items; some of them may not be visible. */
struct FSparseItemInfo
{
	/**
	 * Construct a new FTreeItem.
	 *
	 * @param InItemToVisualize   The DateItem pointer being wrapped by this FTreeItem
	 * @param InHasChildren       Does this item have children? True if yes.
	 */
	FSparseItemInfo( bool InIsExpanded, bool InHasExpandedChildren )
	: bIsExpanded( InIsExpanded )
	, bHasExpandedChildren( InHasExpandedChildren )
	{	
	}

	/** Is this tree item expanded? */
	bool bIsExpanded;

	/** Does this tree item have any expanded children? */
	bool bHasExpandedChildren;
};


/** Info needed by every visible item in the tree */
struct FItemInfo
{
	FItemInfo()
	{		
	}

	FItemInfo(TBitArray<> InNeedsVerticalWire, bool InHasChildren, bool InIsLastChild, int32 InParentIndex )
	: NeedsVerticalWire(InNeedsVerticalWire)
	, bHasChildren( InHasChildren )
	, bIsLastChild( InIsLastChild )
	, ParentIndex( InParentIndex )
	{
	}

	/**
	 * Flags for whether we need a wire drawn for this level of the tree.
	 *
	 * NeedsVerticalWrite.Num() is the nesting level within the tree. e.g. 0 is root-level, 1 is children of root, etc.
	 */
	TBitArray<> NeedsVerticalWire;

	int32 GetNestingLevel() const { return NeedsVerticalWire.Num()-1; }

	/** Does this tree item have children? */
	uint32 bHasChildren : 1;

	/** Is this the last child of its parent? If so, it gets a special kind of wire/connector. */
	uint32 bIsLastChild : 1;

	/** Index into the linearized tree of the parent for this item, if any, otherwise INDEX_NONE. */
	int32 ParentIndex;
};


/**
 * This assumes you are familiar with SListView; see SListView.
 *
 * TreeView setup is virtually identical to that of ListView.
 * Additionally, TreeView introduces a new delegate: OnGetChildren().
 * OnGetChildren() takes some DataItem being observed by the tree
 * and returns that item's children. Like ListView, TreeView operates
 * exclusively with pointers to DataItems.
 * 
 */
template<typename ItemType>
class STreeView : public SListView< ItemType >
{
public:

	using NullableItemType  = typename TListTypeTraits< ItemType >::NullableType;
	using MapKeyFuncs       = typename TListTypeTraits< ItemType >::MapKeyFuncs;
	using MapKeyFuncsSparse = typename TListTypeTraits< ItemType >::MapKeyFuncsSparse;

	using TSparseItemMap    = TMap< ItemType, FSparseItemInfo, FDefaultSetAllocator, MapKeyFuncsSparse >;
	using TItemSet          = TSet< ItemType, typename TListTypeTraits< ItemType >::SetKeyFuncs >;

	using FOnGetChildren            = typename TSlateDelegates< ItemType >::FOnGetChildren;
	using FOnGenerateRow            = typename TSlateDelegates< ItemType >::FOnGenerateRow;
	using FOnSetExpansionRecursive  = typename TSlateDelegates< ItemType >::FOnSetExpansionRecursive;
	using FOnItemScrolledIntoView   = typename TSlateDelegates< ItemType >::FOnItemScrolledIntoView;
	using FOnSelectionChanged       = typename TSlateDelegates< NullableItemType >::FOnSelectionChanged;
	using FIsSelectableOrNavigable	= typename TSlateDelegates< ItemType >::FIsSelectableOrNavigable;
	using FOnMouseButtonClick       = typename TSlateDelegates< ItemType >::FOnMouseButtonClick;
	using FOnMouseButtonDoubleClick = typename TSlateDelegates< ItemType >::FOnMouseButtonDoubleClick;
	using FOnExpansionChanged       = typename TSlateDelegates< ItemType >::FOnExpansionChanged;

	using FOnItemToString_Debug     = typename TSlateDelegates< ItemType >::FOnItemToString_Debug; 

	using FOnWidgetToBeRemoved      = typename SListView< ItemType >::FOnWidgetToBeRemoved;

public:
	
	SLATE_BEGIN_ARGS( STreeView<ItemType> )
		: _TreeViewStyle(&FAppStyle::Get().GetWidgetStyle<FTableViewStyle>("TreeView"))
		, _OnGenerateRow()
		, _OnGetChildren()
		, _OnSetExpansionRecursive()
		, _TreeItemsSource( static_cast< const TArray<ItemType>* >(nullptr) ) //@todo Slate Syntax: Initializing from nullptr without a cast
		, _ItemHeight(16)
		, _OnContextMenuOpening()
		, _OnMouseButtonClick()
		, _OnMouseButtonDoubleClick()
		, _OnSelectionChanged()
		, _OnExpansionChanged()
		, _OnIsSelectableOrNavigable()
		, _SelectionMode(ESelectionMode::Multi)
		, _ClearSelectionOnClick(true)
		, _ExternalScrollbar()
		, _EnableAnimatedScrolling(false)
		, _ScrollbarDragFocusCause(EFocusCause::Mouse)
		, _ConsumeMouseWheel( EConsumeMouseWheel::WhenScrollingPossible )
		, _AllowOverscroll(EAllowOverscroll::Yes)
		, _ScrollBarStyle(&FAppStyle::Get().GetWidgetStyle<FScrollBarStyle>("ScrollBar"))
		, _WheelScrollMultiplier(GetGlobalScrollAmount())
		, _OnItemToString_Debug()
		, _OnEnteredBadState()
		, _HandleGamepadEvents(true)
		, _HandleDirectionalNavigation(true)
		, _AllowInvisibleItemSelection(false)
		, _HighlightParentNodesForSelection(false)
		, _ReturnFocusToSelection()
		{
			this->_Clipping = EWidgetClipping::ClipToBounds;
		}

		SLATE_STYLE_ARGUMENT( FTableViewStyle, TreeViewStyle )

		SLATE_EVENT( FOnGenerateRow, OnGenerateRow )

		SLATE_EVENT( FOnWidgetToBeRemoved, OnRowReleased )

		SLATE_EVENT( FOnTableViewScrolled, OnTreeViewScrolled )

		SLATE_EVENT( FOnItemScrolledIntoView, OnItemScrolledIntoView )

		SLATE_EVENT( FOnGetChildren, OnGetChildren )

		SLATE_EVENT( FOnSetExpansionRecursive, OnSetExpansionRecursive )

		SLATE_ARGUMENT( const TArray<ItemType>* , TreeItemsSource )

		SLATE_ATTRIBUTE( float, ItemHeight )

		SLATE_EVENT( FOnContextMenuOpening, OnContextMenuOpening )

		SLATE_EVENT(FOnMouseButtonClick, OnMouseButtonClick)

		SLATE_EVENT( FOnMouseButtonDoubleClick, OnMouseButtonDoubleClick )

		SLATE_EVENT( FOnSelectionChanged, OnSelectionChanged )

		SLATE_EVENT( FOnExpansionChanged, OnExpansionChanged )

		SLATE_EVENT(FIsSelectableOrNavigable, OnIsSelectableOrNavigable)

		SLATE_ATTRIBUTE( ESelectionMode::Type, SelectionMode )

		SLATE_ARGUMENT( TSharedPtr<SHeaderRow>, HeaderRow )

		SLATE_ARGUMENT ( bool, ClearSelectionOnClick )

		SLATE_ARGUMENT( TSharedPtr<SScrollBar>, ExternalScrollbar )

		SLATE_ARGUMENT( bool, EnableAnimatedScrolling)

		SLATE_ARGUMENT( TOptional<double>, FixedLineScrollOffset )

		SLATE_ARGUMENT( EFocusCause, ScrollbarDragFocusCause )

		SLATE_ARGUMENT( EConsumeMouseWheel, ConsumeMouseWheel );
		
		SLATE_ARGUMENT( EAllowOverscroll, AllowOverscroll );
		
		SLATE_STYLE_ARGUMENT( FScrollBarStyle, ScrollBarStyle );

		SLATE_ARGUMENT( float, WheelScrollMultiplier );

		/** Assign this to get more diagnostics from the list view. */
		SLATE_EVENT(FOnItemToString_Debug, OnItemToString_Debug)

		SLATE_EVENT(FOnTableViewBadState, OnEnteredBadState);

		SLATE_ARGUMENT(bool, HandleGamepadEvents);

		SLATE_ARGUMENT(bool, HandleDirectionalNavigation);

		SLATE_ARGUMENT(bool, AllowInvisibleItemSelection);

		SLATE_ARGUMENT(bool, HighlightParentNodesForSelection);

		SLATE_ARGUMENT(bool, ReturnFocusToSelection)

	SLATE_END_ARGS()

		
	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget.
	 */
	void Construct( const FArguments& InArgs )
	{
		this->Clipping = InArgs._Clipping;

		this->OnGenerateRow = InArgs._OnGenerateRow;
		this->OnRowReleased = InArgs._OnRowReleased;
		this->OnItemScrolledIntoView = InArgs._OnItemScrolledIntoView;
		this->OnGetChildren = InArgs._OnGetChildren;
		this->OnSetExpansionRecursive = InArgs._OnSetExpansionRecursive;
		this->TreeItemsSource = InArgs._TreeItemsSource;

		this->OnContextMenuOpening = InArgs._OnContextMenuOpening;
		this->OnClick = InArgs._OnMouseButtonClick;
		this->OnDoubleClick = InArgs._OnMouseButtonDoubleClick;
		this->OnSelectionChanged = InArgs._OnSelectionChanged;
		this->OnExpansionChanged = InArgs._OnExpansionChanged;
		this->OnIsSelectableOrNavigable = InArgs._OnIsSelectableOrNavigable;
		this->SelectionMode = InArgs._SelectionMode;

		this->bClearSelectionOnClick = InArgs._ClearSelectionOnClick;
		this->ConsumeMouseWheel = InArgs._ConsumeMouseWheel;
		this->AllowOverscroll = InArgs._AllowOverscroll;

		this->WheelScrollMultiplier = InArgs._WheelScrollMultiplier;

		this->bEnableAnimatedScrolling = InArgs._EnableAnimatedScrolling;
		this->FixedLineScrollOffset = InArgs._FixedLineScrollOffset;

		this->OnItemToString_Debug = InArgs._OnItemToString_Debug.IsBound()
			? InArgs._OnItemToString_Debug
			: SListView< ItemType >::GetDefaultDebugDelegate();
		this->OnEnteredBadState = InArgs._OnEnteredBadState;

		this->bHandleGamepadEvents = InArgs._HandleGamepadEvents;
		this->bHandleDirectionalNavigation = InArgs._HandleDirectionalNavigation;
		this->bAllowInvisibleItemSelection = InArgs._AllowInvisibleItemSelection;
		this->bHighlightParentNodesForSelection = InArgs._HighlightParentNodesForSelection;

		this->bReturnFocusToSelection = InArgs._ReturnFocusToSelection;

		this->SetStyle(InArgs._TreeViewStyle);

		// Check for any parameters that the coder forgot to specify.
		FString ErrorString;
		{
			if ( !this->OnGenerateRow.IsBound() )
			{
				ErrorString += TEXT("Please specify an OnGenerateRow. \n");
			}

			if ( this->TreeItemsSource == nullptr )
			{
				ErrorString += TEXT("Please specify a TreeItemsSource. \n");
			}

			if ( !this->OnGetChildren.IsBound() )
			{
				ErrorString += TEXT("Please specify an OnGetChildren. \n");
			}
		}

		if (ErrorString.Len() > 0)
		{
			// Let the coder know what they forgot
			this->ChildSlot
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(ErrorString))
			];
		}
		else
		{
			// Make the TableView
			this->ConstructChildren( 0, InArgs._ItemHeight, EListItemAlignment::LeftAligned, InArgs._HeaderRow, InArgs._ExternalScrollbar, Orient_Vertical, InArgs._OnTreeViewScrolled, InArgs._ScrollBarStyle );
			if (this->ScrollBar.IsValid())
			{
				this->ScrollBar->SetDragFocusCause(InArgs._ScrollbarDragFocusCause);
			}
			this->AddMetadata(MakeShared<TTableViewMetadata<ItemType>>(this->SharedThis(this)));
		}
	}

	/** Default constructor. */
	STreeView()
		: SListView< ItemType >( ETableViewMode::Tree )
		, TreeItemsSource( nullptr )
		, bTreeItemsAreDirty( true )
	{
		this->ItemsSource = &LinearizedItems;
	}

public:

	// SWidget overrides

	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) override
	{
		// Check for selection/expansion toggling keys (Left, Right)
		// SelectorItem represents the keyboard selection. If it isn't valid then we don't know what to expand.
		// Don't respond to key-presses containing "Alt" as a modifier
		if ( TListTypeTraits<ItemType>::IsPtrValid(this->SelectorItem) && !InKeyEvent.IsAltDown() )
		{
			if ( InKeyEvent.GetKey() == EKeys::Left )
			{
				if( TListTypeTraits<ItemType>::IsPtrValid(this->SelectorItem) )
				{
					ItemType RangeSelectionEndItem = TListTypeTraits<ItemType>::NullableItemTypeConvertToItemType( this->SelectorItem );
					int32 SelectionIndex = this->LinearizedItems.Find( RangeSelectionEndItem );

					if ( Private_DoesItemHaveChildren(SelectionIndex) && Private_IsItemExpanded( RangeSelectionEndItem ) )
					{
						// Collapse the selected item
						Private_SetItemExpansion(RangeSelectionEndItem, false);
					}
					else
					{
						// Select the parent, who should be a previous item in the list whose nesting level is less than the selected one
						int32 SelectedNestingDepth = Private_GetNestingDepth(SelectionIndex);
						for (SelectionIndex--; SelectionIndex >= 0; --SelectionIndex)
						{
							if ( Private_GetNestingDepth(SelectionIndex) < SelectedNestingDepth )
							{
								// Found the parent
								this->NavigationSelect(this->LinearizedItems[SelectionIndex], InKeyEvent);
								break;
							}
						}
					}
				}

				return FReply::Handled();
			}
			else if ( InKeyEvent.GetKey() == EKeys::Right )
			{
				if( TListTypeTraits<ItemType>::IsPtrValid(this->SelectorItem) )
				{
					ItemType RangeSelectionEndItem = TListTypeTraits<ItemType>::NullableItemTypeConvertToItemType( this->SelectorItem );
					int32 SelectionIndex = this->LinearizedItems.Find( RangeSelectionEndItem );

					// Right only applies to items with children
					if ( Private_DoesItemHaveChildren(SelectionIndex) )
					{
						if ( !Private_IsItemExpanded(RangeSelectionEndItem) )
						{
							// Expand the selected item
							Private_SetItemExpansion(RangeSelectionEndItem, true);
						}
						else
						{
							// Select the first child, who should be the next item in the list						
							// Make sure we aren't the last item on the list
							if ( SelectionIndex < this->LinearizedItems.Num() - 1 )
							{
								this->NavigationSelect(this->LinearizedItems[SelectionIndex + 1], InKeyEvent);
							}
						}
					}
				}

				return FReply::Handled();
			}
		}

		return SListView<ItemType>::OnKeyDown(MyGeometry, InKeyEvent);
	}
	
private:

	// Tree View adds the ability to expand/collapse items.
	// All the selection functionality is inherited from ListView.

	virtual bool Private_IsItemExpanded( const ItemType& TheItem ) const override
	{
		const FSparseItemInfo* ItemInfo = SparseItemInfos.Find(TheItem);
		return ItemInfo != nullptr && ItemInfo->bIsExpanded;
	}

	virtual void Private_SetItemExpansion( ItemType TheItem, bool bShouldBeExpanded ) override
	{
		const FSparseItemInfo* const SparseItemInfo = SparseItemInfos.Find(TheItem);
		bool bWasExpanded = false;

		if(SparseItemInfo)
		{
			bWasExpanded = SparseItemInfo->bIsExpanded;
			SparseItemInfos.Add(TheItem, FSparseItemInfo(bShouldBeExpanded, SparseItemInfo->bHasExpandedChildren));
		}
		else if(bShouldBeExpanded)
		{
			SparseItemInfos.Add(TheItem, FSparseItemInfo(bShouldBeExpanded, false));
		}
		
		if(bWasExpanded != bShouldBeExpanded)
		{
			OnExpansionChanged.ExecuteIfBound(TheItem, bShouldBeExpanded);

			// We must rebuild the linearized version of the tree because
			// either some children became visible or some got removed.
			RequestTreeRefresh();
		}
	}

	virtual void Private_OnExpanderArrowShiftClicked( ItemType TheItem, bool bShouldBeExpanded ) override
	{
		if(OnSetExpansionRecursive.IsBound())
		{
			OnSetExpansionRecursive.Execute(TheItem, bShouldBeExpanded);

			// We must rebuild the linearized version of the tree because
			// either some children became visible or some got removed.
			RequestTreeRefresh();
		}
	}

	virtual bool Private_DoesItemHaveChildren( int32 ItemIndexInList ) const override
	{
		bool bHasChildren = false;
		if (DenseItemInfos.IsValidIndex(ItemIndexInList))
		{
			bHasChildren = DenseItemInfos[ItemIndexInList].bHasChildren;
		}
		return bHasChildren;
	}

	virtual int32 Private_GetNestingDepth( int32 ItemIndexInList ) const override
	{
		int32 NestingLevel = 0;
		if (DenseItemInfos.IsValidIndex(ItemIndexInList))
		{
			NestingLevel = DenseItemInfos[ItemIndexInList].GetNestingLevel();
		}
		return NestingLevel;
	}

	virtual const TBitArray<>& Private_GetWiresNeededByDepth(int32 ItemIndexInList) const override
	{
		return (DenseItemInfos.IsValidIndex(ItemIndexInList))
			? DenseItemInfos[ItemIndexInList].NeedsVerticalWire
			: TableViewHelpers::GetEmptyBitArray();
	}

	virtual bool Private_IsLastChild(int32 ItemIndexInList) const override
	{
		return (DenseItemInfos.IsValidIndex(ItemIndexInList))
			? DenseItemInfos[ItemIndexInList].bIsLastChild
			: false;
	}

	/**
	 * This clears the highlight state from all nodes and then traverses the parents of each selected node to add it to the highlighted set.
	 */
	virtual void Private_UpdateParentHighlights()
	{
		this->Private_ClearHighlightedItems();
		
		for( typename TItemSet::TConstIterator SelectedItemIt(this->SelectedItems); SelectedItemIt; ++SelectedItemIt )
		{
			// Sometimes selection events can come through before the Linearized List is built, so the item may not exist yet.
			int32 ItemIndex = LinearizedItems.Find(*SelectedItemIt);
			if (ItemIndex == INDEX_NONE)
			{
				continue;
			}

			if(DenseItemInfos.IsValidIndex(ItemIndex))
			{
				const FItemInfo& ItemInfo = DenseItemInfos[ItemIndex];
				int32 ParentIndex = ItemInfo.ParentIndex;
				while (ParentIndex != INDEX_NONE)
				{
					const ItemType& ParentItem = this->LinearizedItems[ParentIndex];
					this->Private_SetItemHighlighted(ParentItem, true);

					const FItemInfo& ParentItemInfo = DenseItemInfos[ParentIndex];
					ParentIndex = ParentItemInfo.ParentIndex;
				}
			}
		}
	}

public:
	virtual void Private_SignalSelectionChanged(ESelectInfo::Type SelectInfo) override
	{
		SListView< ItemType >::Private_SignalSelectionChanged(SelectInfo);
		
		if (bHighlightParentNodesForSelection)
		{
			this->Private_UpdateParentHighlights();
		}
	}



	/**
	 * See SWidget::Tick()
	 *
	 * @param  AllottedGeometry The space allotted for this widget.
	 * @param  InCurrentTime  Current absolute real time.
	 * @param  InDeltaTime  Real time passed since last tick.
	 */
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override
	{
		if ( bTreeItemsAreDirty )
		{
			// Check that ItemsPanel was made; we never make it if the user failed to specify all the parameters necessary to make the tree work.
			if ( this->ItemsPanel.IsValid() )
			{
				// We are about to repopulate linearized items; the ListView that TreeView is built on top of will also need to refresh.
				bTreeItemsAreDirty = false;

				if ( OnGetChildren.IsBound() && TreeItemsSource != nullptr )								
				{
					// We make copies of the old expansion and selection sets so that we can remove
					// any items that are no longer seen by the tree.
					TItemSet TempSelectedItemsMap;
					TSparseItemMap TempSparseItemInfo;
					TArray<FItemInfo> TempDenseItemInfos;
					
					// Rebuild the linearized view of the tree data.
					LinearizedItems.Empty();
					PopulateLinearizedItems( *TreeItemsSource, LinearizedItems, TempDenseItemInfos, TBitArray<>(), TempSelectedItemsMap, TempSparseItemInfo, true, INDEX_NONE );

					if( !bAllowInvisibleItemSelection &&
						(this->SelectedItems.Num() != TempSelectedItemsMap.Num() ||
						this->SelectedItems.Difference(TempSelectedItemsMap).Num() > 0 || 
						TempSelectedItemsMap.Difference(this->SelectedItems).Num() > 0 ))
					{
						this->SelectedItems = TempSelectedItemsMap;

						if ( !TListTypeTraits<ItemType>::IsPtrValid( this->RangeSelectionStart ) ||
								!this->SelectedItems.Contains( TListTypeTraits<ItemType>::NullableItemTypeConvertToItemType( this->RangeSelectionStart ) ))
						{
								TListTypeTraits< ItemType >::ResetPtr( this->RangeSelectionStart );
								TListTypeTraits< ItemType >::ResetPtr( this->SelectorItem );
						}	
						else if ( !TListTypeTraits<ItemType>::IsPtrValid( this->SelectorItem ) || 
									!this->SelectedItems.Contains( TListTypeTraits<ItemType>::NullableItemTypeConvertToItemType( this->SelectorItem ) ) )
						{
							this->SelectorItem = this->RangeSelectionStart;
						}

						this->Private_SignalSelectionChanged(ESelectInfo::Direct);
					}
						
					// these should come after Private_SignalSelectionChanged(); because through a
					// series of calls, Private_SignalSelectionChanged() could end up in a child
					// that indexes into either of these arrays (the index wouldn't be updated yet,
					// and could be invalid)
					SparseItemInfos = MoveTemp(TempSparseItemInfo);
					DenseItemInfos  = MoveTemp(TempDenseItemInfos);

					// Once the selection changed events have gone through we can update the parent highlight statuses, which are based on your current selection.
					if (bHighlightParentNodesForSelection)
					{
						this->Private_UpdateParentHighlights();
					}
				}
			}
		}
			
		// Tick the TreeView superclass so that it can refresh.
		// This may be due to TreeView requesting a refresh or because new items became visible due to resizing or scrolling.
		SListView< ItemType >::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	}
		
	/** 
	 * Given: an array of items (ItemsSource) each of which potentially has a child.
	 * Task: populate the LinearizedItems array with a flattened version of the visible data items.
	 *       In the process, remove any items that are not visible while maintaining any collapsed
	 *       items that may have expanded children.
	 *
	 * @param ItemsSource          An array of data items each of which may have 0 or more children.
	 * @param LinearizedItems      Array to populate with items based on expanded/collapsed state.
	 * @param NewDenseItemInfos    Array representing how nested each item in the Linearized items is, and whether it has children.
	 * @param TreeLevel            The current level of indentation.
	 * @param OutNewSelectedItems  Selected items minus any items that are no longer observed by the list.
	 * @param NewSparseItemInfo    Expanded items and items that have expanded children minus any items that are no longer observed by the list.
	 * @param bAddingItems         Are we adding encountered items to the linearized items list or just testing them for existence.
	 * @param ParentIndex		   The index in the resulting linearized item list of the parent node for the currently processed level.
	 *
	 * @return true if we encountered expanded children; false otherwise.
	 */
	bool PopulateLinearizedItems(
		const TArray<ItemType>& InItemsSource,
		TArray< ItemType >& InLinearizedItems,
		TArray< FItemInfo >& NewDenseItemInfos,
		TBitArray<> NeedsParentWire,
		TItemSet& OutNewSelectedItems,
		TSparseItemMap& NewSparseItemInfo,
		bool bAddingItems,
		int32 ParentIndex)
	{

		NeedsParentWire.Add(false);
		const int32 NestingDepthIndex = NeedsParentWire.Num()-1;

		bool bSawExpandedItems = false;
		for ( int32 ItemIndex = 0; ItemIndex < InItemsSource.Num(); ++ItemIndex )
		{
			const ItemType& CurItem = InItemsSource[ItemIndex];

			// Find this items children.
			TArray<ItemType> ChildItems;
			OnGetChildren.Execute(InItemsSource[ItemIndex], ChildItems );

			const bool bHasChildren = ChildItems.Num() > 0;

			// Child items will need a parent wire at this depth if the item we are inserting now is
			// not the last item in its immediate parent's list.
			const bool bIsLastChild = (ItemIndex == InItemsSource.Num() - 1);			
			NeedsParentWire[NestingDepthIndex] = !bIsLastChild;
			
			// Is this item expanded, does it have expanded children?
			const FSparseItemInfo* CurItemInfo = SparseItemInfos.Find( CurItem );
			const bool bIsExpanded = (CurItemInfo == nullptr) ? false : CurItemInfo->bIsExpanded;
			bool bHasExpandedChildren = (CurItemInfo == nullptr) ? false : CurItemInfo->bHasExpandedChildren;
								
			// Add this item to the linearized list and update the selection set.
			if (bAddingItems)
			{
				InLinearizedItems.Add( CurItem );

				NewDenseItemInfos.Add( FItemInfo(NeedsParentWire, bHasChildren, bIsLastChild, ParentIndex) );

				const bool bIsSelected = this->IsItemSelected( CurItem );
				if (bIsSelected)
				{
					OutNewSelectedItems.Add( CurItem );
				}
			}

			if ( bIsExpanded || bHasExpandedChildren )
			{ 
				// If this item is expanded, we should process all of its children at the next indentation level.
				// If it is collapsed, process its children but do not add them to the linearized list.
				const bool bAddChildItems = bAddingItems && bIsExpanded;
				bHasExpandedChildren = PopulateLinearizedItems( ChildItems, InLinearizedItems, NewDenseItemInfos, NeedsParentWire, OutNewSelectedItems, NewSparseItemInfo, bAddChildItems, InLinearizedItems.Num() - 1);
			}

			if ( bIsExpanded || bHasExpandedChildren )
			{
				// Update the item info for this tree item.
				NewSparseItemInfo.Add( CurItem, FSparseItemInfo( bIsExpanded, bHasExpandedChildren) );
			}


			// If we encountered any expanded nodes along the way, the parent has expanded children.
			bSawExpandedItems = bSawExpandedItems || bIsExpanded || bHasExpandedChildren;				
		}

		return bSawExpandedItems;
	}
		
	/**
	 * Given a TreeItem, create a Widget to represent it in the tree view.
	 *
	 * @param InItem   Item to visualize.
	 * @return A widget that represents this item.
	 */
	virtual TSharedRef<ITableRow> GenerateNewWidget( ItemType InItem ) override
	{
		if ( this->OnGenerateRow.IsBound() )
		{
			return this->OnGenerateRow.Execute( InItem, this->SharedThis(this) );
		}
		else
		{
			TSharedRef< STreeView<ItemType> > This = StaticCastSharedRef< STreeView<ItemType> >(this->AsShared());

			// The programmer did not provide an OnGenerateRow() handler; let them know.
			TSharedRef< STableRow<ItemType> > NewTreeItemWidget = 
				SNew( STableRow<ItemType>, This )
				.Content()
				[
					SNew(STextBlock) .Text( NSLOCTEXT("STreeView", "BrokenSetupMessage", "OnGenerateWidget() not assigned.") )
				];

			return NewTreeItemWidget;
		}
	}
		
	/** Queue up a regeneration of the linearized items on the next tick. */
	virtual void RequestListRefresh() override
	{
		bTreeItemsAreDirty = true;
		SListView<ItemType>::RequestListRefresh();
	}

	void RequestTreeRefresh()
	{
		RequestListRefresh();
	}

	virtual void RebuildList() override
	{
		LinearizedItems.Empty();
		SListView<ItemType>::RebuildList();
	}

	void SetStyle(const FTableViewStyle* InStyle)
	{
		Style = InStyle;
		STableViewBase::SetBackgroundBrush( Style != nullptr ? &Style->BackgroundBrush : FStyleDefaults::GetNoBrush() );
	}

	/**
	 * Set whether some data item is expanded or not.
	 * 
	 * @param InItem         The item whose expansion state to control.
	 * @param InExpandItem   If true the item should be expanded; otherwise collapsed.
	 */
	void SetItemExpansion( const ItemType& InItem, bool InShouldExpandItem )
	{
		Private_SetItemExpansion(InItem, InShouldExpandItem);
	}

	/** Collapse all the items in the tree and expand InItem */
	void SetSingleExpandedItem( const ItemType& InItem )
	{
		// Will we have to do any work?
		const bool bItemAlreadyLoneExpanded = (this->SparseItemInfos.Num() == 1) && this->IsItemExpanded(InItem);

		if (!bItemAlreadyLoneExpanded)
		{
			this->SparseItemInfos.Empty();
			Private_SetItemExpansion(InItem, true);
		}
	}
		
	/** 
	 * @param InItem   The data item whose expansion state to query.
	 *
	 * @return true if the item is expanded; false otherwise.
	 */
	bool IsItemExpanded( const ItemType& InItem ) const
	{
		return Private_IsItemExpanded( InItem );
	}

		
	/**
	 * Set the TreeItemsSource. The Tree will generate widgets to represent these items.
	 *
	 * @param InItemsSource  A pointer to the array of items that should be observed by this TreeView.
	 */
	void SetTreeItemsSource( const TArray<ItemType>* InItemsSource)
	{
		TreeItemsSource = InItemsSource;
		RequestTreeRefresh();
	}

	/**
	 * Generates a set of items that are currently expanded.
	 *
	 * @param ExpandedItems	The generated set of expanded items.
	 */
	void GetExpandedItems( TItemSet& ExpandedItems ) const
	{
		for( typename TSparseItemMap::TConstIterator InfoIterator(SparseItemInfos); InfoIterator; ++InfoIterator )
		{
			if ( InfoIterator.Value().bIsExpanded )
			{
				ExpandedItems.Add( InfoIterator.Key() );
			}
		}			
	}

	/** Clears the entire set of expanded items. */
	void ClearExpandedItems()
	{
		SparseItemInfos.Empty();
		RequestTreeRefresh();
	}

protected:
	
	/** The delegate that is invoked whenever we need to gather an item's children. */
	FOnGetChildren OnGetChildren;

	/** The delegate that is invoked to recursively expand/collapse a tree items children. */
	FOnSetExpansionRecursive OnSetExpansionRecursive;

	/** A pointer to the items being observed by the tree view. */
	const TArray<ItemType>* TreeItemsSource;		
		
	/** Info needed by a small fraction of tree items; some of these are not visible to the user. */
	TSparseItemMap SparseItemInfos;

	/** Info needed by every item in the linearized version of the tree. */
	TArray<FItemInfo> DenseItemInfos;

	/**
	 * A linearized version of the items being observed by the tree view.
	 * Note that we inherit from a ListView, which we point at this linearized version of the tree.
	 */
	TArray< ItemType > LinearizedItems;

	/** The delegate that is invoked whenever an item in the tree is expanded or collapsed. */
	FOnExpansionChanged OnExpansionChanged;

	/** Style resource for the tree */
	const FTableViewStyle* Style;

private:		

	/** true when the LinearizedItems need to be regenerated. */
	bool bTreeItemsAreDirty = false;

	/** true if we allow invisible items to stay selected. */
	bool bAllowInvisibleItemSelection = false;

	/** true if we should highlight all parents for each of the currently selected items */
	bool bHighlightParentNodesForSelection = false;
};
