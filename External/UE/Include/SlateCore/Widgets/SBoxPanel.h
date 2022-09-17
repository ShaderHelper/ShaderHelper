// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Layout/Margin.h"
#include "Layout/Visibility.h"
#include "Types/SlateStructs.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SlotBase.h"
#include "Widgets/SWidget.h"
#include "Layout/BasicLayoutWidgetSlot.h"
#include "Layout/Children.h"
#include "Widgets/SPanel.h"
#include "Layout/ArrangedChildren.h"
#include "Input/DragAndDrop.h"
#include "Rendering/DrawElements.h"

class FArrangedChildren;

/**
 * A BoxPanel contains one child and describes how that child should be arranged on the screen.
 */
class SLATECORE_API SBoxPanel : public SPanel
{
	SLATE_DECLARE_WIDGET(SBoxPanel, SPanel)

protected:
	/**
	 * A BoxPanel contains one BoxPanel child and describes how that
	 * child should be arranged on the screen.
	 */
	template<typename SlotType>
	class TSlot : public TBasicLayoutWidgetSlot<SlotType>
	{
	public:
		SLATE_SLOT_BEGIN_ARGS(TSlot, TBasicLayoutWidgetSlot<SlotType>)
			SLATE_ARGUMENT(TOptional<FSizeParam>, SizeParam)
			TAttribute<float> _MaxSize;
		SLATE_SLOT_END_ARGS()

	protected:
		/** Default values for a slot. */
		TSlot()
			: TBasicLayoutWidgetSlot<SlotType>(HAlign_Fill, VAlign_Fill)
			, SizeRule(FSizeParam::SizeRule_Stretch)
			, SizeValue(*this, 1.f)
			, MaxSize(*this, 0.0f)
		{ }

	public:
		void Construct(const FChildren& SlotOwner, FSlotArguments&& InArgs)
		{
			TBasicLayoutWidgetSlot<SlotType>::Construct(SlotOwner, MoveTemp(InArgs));
			if (InArgs._MaxSize.IsSet())
			{
				SetMaxSize(MoveTemp(InArgs._MaxSize));
			}
			if (InArgs._SizeParam.IsSet())
			{
				SetSizeParam(MoveTemp(InArgs._SizeParam.GetValue()));
			}
		}

		static void RegisterAttributes(FSlateWidgetSlotAttributeInitializer& AttributeInitializer)
		{
			TBasicLayoutWidgetSlot<SlotType>::RegisterAttributes(AttributeInitializer);
			SLATE_ADD_SLOT_ATTRIBUTE_DEFINITION_WITH_NAME(TSlot<SlotType>, AttributeInitializer, "Slot.MaxSize", MaxSize, EInvalidateWidgetReason::Layout);
			SLATE_ADD_SLOT_ATTRIBUTE_DEFINITION_WITH_NAME(TSlot<SlotType>, AttributeInitializer, "Slot.SizeValue", SizeValue, EInvalidateWidgetReason::Layout)
				.UpdatePrerequisite("Slot.MaxSize");
		}

		/** Get the space rule this slot should occupy along panel's direction. */
		FSizeParam::ESizeRule GetSizeRule() const
		{
			return SizeRule;
		}

		/** Get the space rule value this slot should occupy along panel's direction. */
		float GetSizeValue() const
		{
			return SizeValue.Get();
		}

		/** Get the max size the slot can be.*/
		float GetMaxSize() const
		{
			return MaxSize.Get();
		}

	public:
		/** Set the size Param of the slot, It could be a FStretch or a FAuto. */
		void SetSizeParam(FSizeParam InSizeParam)
		{
			SizeRule = InSizeParam.SizeRule;
			SizeValue.Assign(*this, MoveTemp(InSizeParam.Value));
		}

		/** The widget's DesiredSize will be used as the space required. */
		void SetSizeToAuto()
		{
			SetSizeParam(FAuto());
		}

		/** The available space will be distributed proportionately. */
		void SetSizeToStretch(TAttribute<float> StretchCoefficient)
		{
			SetSizeParam(FStretch(MoveTemp(StretchCoefficient)));
		}

		/** Set the max size in SlateUnit this slot can be. */
		void SetMaxSize(TAttribute<float> InMaxSize)
		{
			MaxSize.Assign(*this, MoveTemp(InMaxSize));
		}

	private:
		/**
		 * How much space this slot should occupy along panel's direction.
		 * When SizeRule is SizeRule_Auto, the widget's DesiredSize will be used as the space required.
		 * When SizeRule is SizeRule_Stretch, the available space will be distributed proportionately between
		 * peer Widgets depending on the Value property. Available space is space remaining after all the
		 * peers' SizeRule_Auto requirements have been satisfied.
		 */

		/** The sizing rule to use. */
		FSizeParam::ESizeRule SizeRule;

		/** The actual value this size parameter stores. */
		typename TBasicLayoutWidgetSlot<SlotType>::template TSlateSlotAttribute<float> SizeValue;

		/** The max size that this slot can be (0 if no max) */
		typename TBasicLayoutWidgetSlot<SlotType>::template TSlateSlotAttribute<float> MaxSize;
	};

public:
	class FSlot : public TSlot<FSlot>
	{
	};

protected:
	template<typename SlotType>
	struct FScopedWidgetSlotArguments : public SlotType::FSlotArguments
	{
	public:
		FScopedWidgetSlotArguments(TUniquePtr<SlotType> InSlot, TPanelChildren<FSlot>& InChildren, int32 InIndex)
			: SlotType::FSlotArguments(MoveTemp(InSlot))
			, Children(InChildren)
			, Index(InIndex)
		{
		}

		FScopedWidgetSlotArguments() = delete;
		FScopedWidgetSlotArguments(const FScopedWidgetSlotArguments&) = delete;
		FScopedWidgetSlotArguments& operator=(const FScopedWidgetSlotArguments&) = delete;
		FScopedWidgetSlotArguments(FScopedWidgetSlotArguments&&) = default;
		FScopedWidgetSlotArguments& operator=(FScopedWidgetSlotArguments&&) = default;

		virtual ~FScopedWidgetSlotArguments()
		{
			if (this->GetSlot())	// Is nullptr when the FScopedWidgetSlotArguments is moved-constructed.
			{
				SBoxPanel::FSlot::FSlotArguments* SelfAsBaseSlot = static_cast<SBoxPanel::FSlot::FSlotArguments*>(static_cast<FSlotBase::FSlotArguments*>(this));
				if (Index == INDEX_NONE)
				{
					Children.AddSlot(MoveTemp(*SelfAsBaseSlot));
				}
				else
				{
					Children.InsertSlot(MoveTemp(*SelfAsBaseSlot), Index);
				}
			}
		}

	private:
		TPanelChildren<FSlot>& Children;
		int32 Index;
	};

public:
	/** Removes a slot from this box panel which contains the specified SWidget
	 *
	 * @param SlotWidget The widget to match when searching through the slots
	 * @returns The index in the children array where the slot was removed and -1 if no slot was found matching the widget
	 */
	int32 RemoveSlot( const TSharedRef<SWidget>& SlotWidget );

	/** Removes all children from the box. */
	void ClearChildren();

	/** @return the number of slots. */
	int32 NumSlots() const { return Children.Num(); }

	/** @return if it's a valid index slot index. */
	bool IsValidSlotIndex(int32 Index) const { return Children.IsValidIndex(Index); }

public:
	//~ Begin SWidget overrides.
	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
	virtual FChildren* GetChildren() override;
protected:
	virtual FVector2D ComputeDesiredSize(float) const override;
	//~ End SWidget overrides.

	/**
	 * A Box Panel's orientation cannot be changed once it is constructed..
	 *
	 * @param InOrientation   The orientation of the Box Panel
	 */
	SBoxPanel( EOrientation InOrientation );

	/** The Box Panel's children. */
	TPanelChildren<FSlot> Children;

	/** The Box Panel's orientation; determined at construct time. */
	const EOrientation Orientation;
};


/** A Horizontal Box Panel. See SBoxPanel for more info. */
class SLATECORE_API SHorizontalBox : public SBoxPanel
{
	SLATE_DECLARE_WIDGET(SHorizontalBox, SBoxPanel)
public:
	class FSlot : public SBoxPanel::TSlot<FSlot>
	{
	public:
		SLATE_SLOT_BEGIN_ARGS(FSlot, SBoxPanel::TSlot<FSlot>)
			/** The widget's DesiredSize will be used as the space required. */
			FSlotArguments& AutoWidth()
			{
				_SizeParam = FAuto();
				return Me();
			}
			/** The available space will be distributed proportionately. */
			FSlotArguments& FillWidth(TAttribute<float> InStretchCoefficient)
			{
				_SizeParam = FStretch(MoveTemp(InStretchCoefficient));
				return Me();
			}
			/** Set the max size in SlateUnit this slot can be. */
			FSlotArguments& MaxWidth(TAttribute<float> InMaxWidth)
			{
				_MaxSize = MoveTemp(InMaxWidth);
				return Me();
			}
		SLATE_SLOT_END_ARGS()

		/** The widget's DesiredSize will be used as the space required. */
		void SetAutoWidth()
		{
			SetSizeToAuto();
		}

		/** The available space will be distributed proportionately. */
		void SetFillWidth(TAttribute<float> InStretchCoefficient)
		{
			SetSizeToStretch(MoveTemp(InStretchCoefficient));
		}

		/** Set the max size in SlateUnit this slot can be. */
		void SetMaxWidth(TAttribute<float> InMaxWidth)
		{
			SetMaxSize(MoveTemp(InMaxWidth));
		}

		void Construct(const FChildren& SlotOwner, FSlotArguments&& InArgs)
		{
			SBoxPanel::TSlot<FSlot>::Construct(SlotOwner, MoveTemp(InArgs));
		}

		UE_DEPRECATED(5.0, "Chained AutoWidth is deprecated. Use the FSlotArgument or SetAutoWidth")
		FSlot& AutoWidth() { SetAutoWidth(); return *this; }

		UE_DEPRECATED(5.0, "Chained FillWidth is deprecated. Use the FSlotArgument or SetFillWidth")
		FSlot& FillWidth(TAttribute<float> InStretchCoefficient) { SetFillWidth(InStretchCoefficient); return *this; }

		UE_DEPRECATED(5.0, "Chained MaxWidth is deprecated. Use the FSlotArgument or SetMaxWidth")
		FSlot& MaxWidth(TAttribute<float> InMaxWidth) { SetMaxWidth(InMaxWidth); return *this; }
	};

	static FSlot::FSlotArguments Slot()
	{
		return FSlot::FSlotArguments(MakeUnique<FSlot>());
	}

	SLATE_BEGIN_ARGS( SHorizontalBox )
	{
		_Visibility = EVisibility::SelfHitTestInvisible;
	}
		SLATE_SLOT_ARGUMENT(SHorizontalBox::FSlot, Slots)
	SLATE_END_ARGS()

	using FScopedWidgetSlotArguments = SBoxPanel::FScopedWidgetSlotArguments<SHorizontalBox::FSlot>;
	FScopedWidgetSlotArguments AddSlot()
	{
		return InsertSlot(INDEX_NONE);
	}

	FScopedWidgetSlotArguments InsertSlot(int32 Index = INDEX_NONE)
	{
		return FScopedWidgetSlotArguments(MakeUnique<FSlot>(), this->Children, Index);
	}

	FSlot& GetSlot(int32 SlotIndex);
	const FSlot& GetSlot(int32 SlotIndex) const;

	FORCENOINLINE SHorizontalBox()
		: SBoxPanel( Orient_Horizontal )
	{
		SetCanTick(false);
		bCanSupportFocus = false;
	}

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );
};

/** A Vertical Box Panel. See SBoxPanel for more info. */
class SLATECORE_API SVerticalBox : public SBoxPanel
{
	SLATE_DECLARE_WIDGET(SVerticalBox, SBoxPanel)
public:
	class FSlot : public SBoxPanel::TSlot<FSlot>
	{
	public:
		SLATE_SLOT_BEGIN_ARGS(FSlot, SBoxPanel::TSlot<FSlot>)
			/** The widget's DesiredSize will be used as the space required. */
			FSlotArguments& AutoHeight()
			{
				_SizeParam = FAuto();
				return Me();
			}
			/** The available space will be distributed proportionately. */
			FSlotArguments& FillHeight(TAttribute<float> InStretchCoefficient)
			{
				_SizeParam = FStretch(MoveTemp(InStretchCoefficient));
				return Me();
			}
			/** Set the max size in SlateUnit this slot can be. */
			FSlotArguments& MaxHeight(TAttribute<float> InMaxHeight)
			{
				_MaxSize = MoveTemp(InMaxHeight);
				return Me();
			}
		SLATE_SLOT_END_ARGS()

		/** The widget's DesiredSize will be used as the space required. */
		void SetAutoHeight()
		{
			SetSizeToAuto();
		}

		/** The available space will be distributed proportionately. */
		void SetFillHeight(TAttribute<float> InStretchCoefficient)
		{
			SetSizeToStretch(MoveTemp(InStretchCoefficient));
		}

		/** Set the max size in SlateUnit this slot can be. */
		void SetMaxHeight(TAttribute<float> InMaxHeight)
		{
			SetMaxSize(MoveTemp(InMaxHeight));
		}

		void Construct(const FChildren& SlotOwner, FSlotArguments&& InArgs)
		{
			SBoxPanel::TSlot<FSlot>::Construct(SlotOwner, MoveTemp(InArgs));
		}

		UE_DEPRECATED(5.0, "Chained AutoHeight is deprecated. Use the FSlotArgument or SetAutoHeight")
		FSlot& AutoHeight() { SetAutoHeight(); return *this; }

		UE_DEPRECATED(5.0, "Chained FillWidth is deprecated. Use the FSlotArgument or SetFillWidth")
		FSlot& FillHeight(TAttribute<float> InStretchCoefficient) { SetFillHeight(InStretchCoefficient); return *this; }

		UE_DEPRECATED(5.0, "Chained MaxWidth is deprecated. Use the FSlotArgument or SetMaxHeight")
		FSlot& MaxHeight(TAttribute<float> InMaxHeight) { SetMaxHeight(InMaxHeight); return *this; }
	};

	static FSlot::FSlotArguments Slot()
	{
		return FSlot::FSlotArguments(MakeUnique<FSlot>());
	}


	SLATE_BEGIN_ARGS( SVerticalBox )
	{
		_Visibility = EVisibility::SelfHitTestInvisible;
	}
		SLATE_SLOT_ARGUMENT(SVerticalBox::FSlot, Slots)
	SLATE_END_ARGS()

	using FScopedWidgetSlotArguments = SBoxPanel::FScopedWidgetSlotArguments<SVerticalBox::FSlot>;
	FScopedWidgetSlotArguments AddSlot()
	{
		return InsertSlot(INDEX_NONE);
	}

	FScopedWidgetSlotArguments InsertSlot(int32 Index = INDEX_NONE)
	{
		return FScopedWidgetSlotArguments(MakeUnique<FSlot>(), this->Children, Index);
	}

	FSlot& GetSlot(int32 SlotIndex);
	const FSlot& GetSlot(int32 SlotIndex) const;

	FORCENOINLINE SVerticalBox()
		: SBoxPanel( Orient_Vertical )
	{
		SetCanTick(false);
		bCanSupportFocus = false;
	}

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );
};

class FDragAndDropVerticalBoxOp : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FDragAndDropVerticalBoxOp, FDragDropOperation)

	/** Data this item represent */
	int32 SlotIndexBeingDragged;
	SVerticalBox::FSlot* SlotBeingDragged;

	virtual ~FDragAndDropVerticalBoxOp()
	{}
};

/** A Vertical Box Panel. See SBoxPanel for more info. */
class SLATECORE_API SDragAndDropVerticalBox : public SVerticalBox
{
public:
	/**
	* Where we are going to drop relative to the target item.
	*/
	enum class EItemDropZone
	{
		AboveItem,
		BelowItem
	};

	DECLARE_DELEGATE_RetVal_FourParams(FReply, FOnDragAndDropVerticalBoxDragDetected, const FGeometry&, const FPointerEvent&, int32, SVerticalBox::FSlot*)
	DECLARE_DELEGATE_OneParam(FOnDragAndDropVerticalBoxDragEnter, FDragDropEvent const&);
	DECLARE_DELEGATE_OneParam(FOnDragAndDropVerticalBoxDragLeave, FDragDropEvent const&);
	DECLARE_DELEGATE_RetVal_OneParam(FReply, FOnDragAndDropVerticalBoxDrop, FDragDropEvent const&);

	/** Delegate signature for querying whether this FDragDropEvent will be handled by the drop target of type ItemType. */
	DECLARE_DELEGATE_RetVal_ThreeParams(TOptional<EItemDropZone>, FOnCanAcceptDrop, const FDragDropEvent&, SDragAndDropVerticalBox::EItemDropZone, SVerticalBox::FSlot*);
	/** Delegate signature for handling the drop of FDragDropEvent onto target of type ItemType */
	DECLARE_DELEGATE_RetVal_FourParams(FReply, FOnAcceptDrop, const FDragDropEvent&, SDragAndDropVerticalBox::EItemDropZone, int32, SVerticalBox::FSlot*);

	SLATE_BEGIN_ARGS(SDragAndDropVerticalBox)
		{}

		// High Level DragAndDrop

		/**
		* Handle this event to determine whether a drag and drop operation can be executed on top of the target row widget.
		* Most commonly, this is used for previewing re-ordering and re-organization operations in lists or trees.
		* e.g. A user is dragging one item into a different spot in the list or tree.
		*      This delegate will be called to figure out if we should give visual feedback on whether an item will
		*      successfully drop into the list.
		*/
		SLATE_EVENT(FOnCanAcceptDrop, OnCanAcceptDrop)

		/**
		* Perform a drop operation onto the target row widget
		* Most commonly used for executing a re-ordering and re-organization operations in lists or trees.
		* e.g. A user was dragging one item into a different spot in the list; they just dropped it.
		*      This is our chance to handle the drop by reordering items and calling for a list refresh.
		*/
		SLATE_EVENT(FOnAcceptDrop, OnAcceptDrop)

		// Low level DragAndDrop
		SLATE_EVENT(FOnDragAndDropVerticalBoxDragDetected, OnDragDetected)
		SLATE_EVENT(FOnDragAndDropVerticalBoxDragEnter, OnDragEnter)
		SLATE_EVENT(FOnDragAndDropVerticalBoxDragLeave, OnDragLeave)
		SLATE_EVENT(FOnDragAndDropVerticalBoxDrop, OnDrop)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Set the Drop indicators */
	SDragAndDropVerticalBox& SetDropIndicator_Above(const FSlateBrush& InValue) { DropIndicator_Above = InValue; return *this; }
	SDragAndDropVerticalBox& SetDropIndicator_Below(const FSlateBrush& InValue) { DropIndicator_Below = InValue; return *this; }

	/** Drag detection and handling */
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnDragEnter(FGeometry const& MyGeometry, FDragDropEvent const& DragDropEvent) override;
	virtual void OnDragLeave(FDragDropEvent const& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	/** @see SDragAndDropVerticalBox's OnCanAcceptDrop event */
	FOnCanAcceptDrop OnCanAcceptDrop;

	/** @see SDragAndDropVerticalBox's OnAcceptDrop event */
	FOnAcceptDrop OnAcceptDrop;

	/** Are we currently dragging/dropping over this item? */
	TOptional<EItemDropZone> ItemDropZone;

	/** Delegate triggered when a user starts to drag a slot item */
	FOnDragAndDropVerticalBoxDragDetected OnDragDetected_Handler;

	/** Delegate triggered when a user's drag enters the bounds of this slot item */
	FOnDragAndDropVerticalBoxDragEnter OnDragEnter_Handler;

	/** Delegate triggered when a user's drag leaves the bounds of this slot item */
	FOnDragAndDropVerticalBoxDragLeave OnDragLeave_Handler;

	/** Delegate triggered when a user's drag is dropped in the bounds of this slot item */
	FOnDragAndDropVerticalBoxDrop OnDrop_Handler;

	/** Brush used to provide feedback that a user can drop below the hovered row. */
	FSlateBrush DropIndicator_Above;
	FSlateBrush DropIndicator_Below;

	/** This is required for the paint to access which item we're hovering */
	FVector2D CurrentDragOperationScreenSpaceLocation;
	int32 CurrentDragOverSlotIndex;

	/** @return the zone (above, below) based on where the user is hovering over */
	EItemDropZone ZoneFromPointerPosition(FVector2D LocalPointerPos, const FGeometry& CurrentGeometry, const FGeometry& StartGeometry) const;
};
