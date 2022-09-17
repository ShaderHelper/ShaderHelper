// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/WidgetSlotWithAttributeSupport.h"
#include "Layout/FlowDirection.h"
#include "Layout/Margin.h"
#include "Misc/Optional.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

#include <type_traits>

PRAGMA_DISABLE_DEPRECATION_WARNINGS
/** Mixin to add the alignment functionality to a base slot. */
template <typename MixedIntoType>
class TAlignmentWidgetSlotMixin
{
public:
	TAlignmentWidgetSlotMixin()
		: HAlignment(HAlign_Fill)
		, VAlignment(VAlign_Fill)
	{}

	TAlignmentWidgetSlotMixin(const EHorizontalAlignment InHAlign, const EVerticalAlignment InVAlign)
		: HAlignment(InHAlign)
		, VAlignment(InVAlign)
	{}
	
public:
	struct FSlotArgumentsMixin
	{
	private:
		friend TAlignmentWidgetSlotMixin;

	public:
		typename MixedIntoType::FSlotArguments& HAlign(EHorizontalAlignment InHAlignment)
		{
			_HAlignment = InHAlignment;
			return static_cast<typename MixedIntoType::FSlotArguments&>(*this);
		}

		typename MixedIntoType::FSlotArguments& VAlign(EVerticalAlignment InVAlignment)
		{
			_VAlignment = InVAlignment;
			return static_cast<typename MixedIntoType::FSlotArguments&>(*this);
		}

	private:
		TOptional<EHorizontalAlignment> _HAlignment;
		TOptional<EVerticalAlignment> _VAlignment;
	};

protected:
	void ConstructMixin(const FChildren& SlotOwner, FSlotArgumentsMixin&& InArgs)
	{
		HAlignment = InArgs._HAlignment.Get(HAlignment);
		VAlignment = InArgs._VAlignment.Get(VAlignment);
	}

public:
	UE_DEPRECATED(5.0, "HAlign is now deprecated. Use the FSlotArgument or the SetHorizontalAlignment function.")
	MixedIntoType& HAlign(EHorizontalAlignment InHAlignment)
	{
		HAlignment = InHAlignment;
		return *(static_cast<MixedIntoType*>(this));
	}

	UE_DEPRECATED(5.0, "VAlign is now deprecated. Use the FSlotArgument or the SetVerticalAlignment function.")
	MixedIntoType& VAlign(EVerticalAlignment InVAlignment)
	{
		VAlignment = InVAlignment;
		return *(static_cast<MixedIntoType*>(this));
	}

public:
	void SetHorizontalAlignment(EHorizontalAlignment Alignment)
	{
		if (HAlignment != Alignment)
		{
			HAlignment = Alignment;
			static_cast<MixedIntoType*>(this)->Invalidate(EInvalidateWidgetReason::Layout);
		}
	}

	EHorizontalAlignment GetHorizontalAlignment() const
	{
		return HAlignment;
	}

	void SetVerticalAlignment(EVerticalAlignment Alignment)
	{
		if (VAlignment != Alignment)
		{
			VAlignment = Alignment;
			static_cast<MixedIntoType*>(this)->Invalidate(EInvalidateWidgetReason::Layout);
		}
	}

	EVerticalAlignment GetVerticalAlignment() const
	{
		return VAlignment;
	}

public:
	/** Horizontal positioning of child within the allocated slot */
	UE_DEPRECATED(5.0, "Direct access to HAlignment is now deprecated. Use the getter.")
	EHorizontalAlignment HAlignment;
	/** Vertical positioning of child within the allocated slot */
	UE_DEPRECATED(5.0, "Direct access to VAlignment is now deprecated. Use the getter.")
	EVerticalAlignment VAlignment;
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS


PRAGMA_DISABLE_DEPRECATION_WARNINGS
/** Mixin to add the alignment functionality to a base slot that is also a single children. */
template <typename MixedIntoType>
class TAlignmentSingleWidgetSlotMixin
{
public:
	template<typename WidgetType, typename V = typename std::enable_if<std::is_base_of<SWidget, WidgetType>::value>::type>
	TAlignmentSingleWidgetSlotMixin(WidgetType& InParent)
		: HAlignment(HAlign_Fill)
		, VAlignment(VAlign_Fill)
	{}

	template<typename WidgetType, typename V = typename std::enable_if<std::is_base_of<SWidget, WidgetType>::value>::type>
	TAlignmentSingleWidgetSlotMixin(WidgetType& InParent, const EHorizontalAlignment InHAlign, const EVerticalAlignment InVAlign)
		: HAlignment(InHAlign)
		, VAlignment(InVAlign)
	{}

public:
	struct FSlotArgumentsMixin
	{
	private:
		friend TAlignmentSingleWidgetSlotMixin;

	public:
		typename MixedIntoType::FSlotArguments& HAlign(EHorizontalAlignment InHAlignment)
		{
			_HAlignment = InHAlignment;
			return static_cast<typename MixedIntoType::FSlotArguments&>(*this);
		}

		typename MixedIntoType::FSlotArguments& VAlign(EVerticalAlignment InVAlignment)
		{
			_VAlignment = InVAlignment;
			return static_cast<typename MixedIntoType::FSlotArguments&>(*this);
		}

	private:
		TOptional<EHorizontalAlignment> _HAlignment;
		TOptional<EVerticalAlignment> _VAlignment;
	};

protected:
	void ConstructMixin(FSlotArgumentsMixin&& InArgs)
	{
		HAlignment = InArgs._HAlignment.Get(HAlignment);
		VAlignment = InArgs._VAlignment.Get(VAlignment);
	}

public:
	// HAlign will be deprecated soon. Use SetVerticalAlignment or construct a new slot with FSlotArguments
	MixedIntoType& HAlign(EHorizontalAlignment InHAlignment)
	{
		HAlignment = InHAlignment;
		return *(static_cast<MixedIntoType*>(this));
	}

	// VAlign will be deprecated soon. Use SetVerticalAlignment or construct a new slot with FSlotArguments
	MixedIntoType& VAlign(EVerticalAlignment InVAlignment)
	{
		VAlignment = InVAlignment;
		return *(static_cast<MixedIntoType*>(this));
	}

public:
	void SetHorizontalAlignment(EHorizontalAlignment Alignment)
	{
		if (HAlignment != Alignment)
		{
			HAlignment = Alignment;
			static_cast<MixedIntoType*>(this)->Invalidate(EInvalidateWidgetReason::Layout);
		}
	}

	EHorizontalAlignment GetHorizontalAlignment() const
	{
		return HAlignment;
	}

	void SetVerticalAlignment(EVerticalAlignment Alignment)
	{
		if (VAlignment != Alignment)
		{
			VAlignment = Alignment;
			static_cast<MixedIntoType*>(this)->Invalidate(EInvalidateWidgetReason::Layout);
		}
	}

	EVerticalAlignment GetVerticalAlignment() const
	{
		return VAlignment;
	}

public:
	/** Horizontal positioning of child within the allocated slot */
	UE_DEPRECATED(5.0, "Direct access to HAlignment is now deprecated. Use the getter.")
	EHorizontalAlignment HAlignment;
	/** Vertical positioning of child within the allocated slot */
	UE_DEPRECATED(5.0, "Direct access to VAlignment is now deprecated. Use the getter.")
	EVerticalAlignment VAlignment;
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS


PRAGMA_DISABLE_DEPRECATION_WARNINGS
/** Mixin to add the padding functionality to a base slot. */
template <typename MixedIntoType>
class TPaddingWidgetSlotMixin
{
public:
	TPaddingWidgetSlotMixin()
		: SlotPaddingAttribute(*static_cast<MixedIntoType*>(this))
	{}
	TPaddingWidgetSlotMixin(const FMargin& Margin)
		: SlotPaddingAttribute(*static_cast<MixedIntoType*>(this), Margin)
	{}

public:
	struct FSlotArgumentsMixin
	{
	private:
		friend TPaddingWidgetSlotMixin;
		TAttribute<FMargin> _Padding;

	public:
		typename MixedIntoType::FSlotArguments& Padding(TAttribute<FMargin> InPadding)
		{
			_Padding = MoveTemp(InPadding);
			return static_cast<typename MixedIntoType::FSlotArguments&>(*this);
		}

		typename MixedIntoType::FSlotArguments& Padding(float Uniform)
		{
			_Padding = FMargin(Uniform);
			return static_cast<typename MixedIntoType::FSlotArguments&>(*this);
		}

		typename MixedIntoType::FSlotArguments& Padding(float Horizontal, float Vertical)
		{
			_Padding = FMargin(Horizontal, Vertical);
			return static_cast<typename MixedIntoType::FSlotArguments&>(*this);
		}

		typename MixedIntoType::FSlotArguments& Padding(float Left, float Top, float Right, float Bottom)
		{
			_Padding = FMargin(Left, Top, Right, Bottom);
			return static_cast<typename MixedIntoType::FSlotArguments&>(*this);
		}
	};

protected:
	void ConstructMixin(const FChildren& SlotOwner, FSlotArgumentsMixin&& InArgs)
	{
		if (InArgs._Padding.IsSet())
		{
			SlotPaddingAttribute.Assign(*static_cast<MixedIntoType*>(this), MoveTemp(InArgs._Padding));
		}
	}

	static void RegisterAttributesMixin(FSlateWidgetSlotAttributeInitializer& AttributeInitializer)
	{
		SLATE_ADD_SLOT_ATTRIBUTE_DEFINITION_WITH_NAME(MixedIntoType, AttributeInitializer, "Slot.Padding", SlotPaddingAttribute, EInvalidateWidgetReason::Layout);
	}

public:
	UE_DEPRECATED(5.0, "Padding is now deprecated. Use the FSlotArgument or the SetPadding function.")
	MixedIntoType& Padding(TAttribute<FMargin> InPadding)
	{
		SetPadding(MoveTemp(InPadding));
		return *(static_cast<MixedIntoType*>(this));
	}

	UE_DEPRECATED(5.0, "Padding is now deprecated. Use the FSlotArgument or the SetPadding function.")
	MixedIntoType& Padding(float Uniform)
	{
		SetPadding(FMargin(Uniform));
		return *(static_cast<MixedIntoType*>(this));
	}

	UE_DEPRECATED(5.0, "Padding is now deprecated. Use the FSlotArgument or the SetPadding function.")
	MixedIntoType& Padding(float Horizontal, float Vertical)
	{
		SetPadding(FMargin(Horizontal, Vertical));
		return *(static_cast<MixedIntoType*>(this));
	}

	UE_DEPRECATED(5.0, "Padding is now deprecated. Use the FSlotArgument or the SetPadding function.")
	MixedIntoType& Padding(float Left, float Top, float Right, float Bottom)
	{
		SetPadding(FMargin(Left, Top, Right, Bottom));
		return *(static_cast<MixedIntoType*>(this));
	}

public:
	void SetPadding(TAttribute<FMargin> InPadding)
	{
		SlotPaddingAttribute.Assign(static_cast<MixedIntoType&>(*this), MoveTemp(InPadding));
	}

	FMargin GetPadding() const
	{
		return SlotPaddingAttribute.Get();
	}

public:
	UE_DEPRECATED(5.0, "Direct access to SlotPadding is now deprecated. Use the setter or getter.")
	TSlateDeprecatedTAttribute<FMargin> SlotPadding;

private:
	using SlotPaddingCompareType = TSlateAttributeComparePredicate<>;
	using SlotPaddingType = ::SlateAttributePrivate::TSlateContainedAttribute<FMargin, ::SlateAttributePrivate::FSlateAttributeNoInvalidationReason, SlotPaddingCompareType>;
	SlotPaddingType SlotPaddingAttribute;
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS



PRAGMA_DISABLE_DEPRECATION_WARNINGS
/** Mixin to add the padding functionality to a base slot that is also a single children. */
template <typename MixedIntoType, EInvalidateWidgetReason InPaddingInvalidationReason = EInvalidateWidgetReason::Layout>
class TPaddingSingleWidgetSlotMixin
{
public:
	template<typename WidgetType, typename V = typename std::enable_if<std::is_base_of<SWidget, WidgetType>::value>::type>
	TPaddingSingleWidgetSlotMixin(WidgetType& InParent)
		: SlotPaddingAttribute(InParent)
	{}

	template<typename WidgetType, typename V = typename std::enable_if<std::is_base_of<SWidget, WidgetType>::value>::type>
	TPaddingSingleWidgetSlotMixin(WidgetType& InParent, const FMargin & Margin)
		: SlotPaddingAttribute(InParent, Margin)
	{}

public:
	struct FSlotArgumentsMixin
	{
	private:
		friend TPaddingSingleWidgetSlotMixin;
		TAttribute<FMargin> _Padding;

	public:
		typename MixedIntoType::FSlotArguments& Padding(TAttribute<FMargin> InPadding)
		{
			_Padding = MoveTemp(InPadding);
			return static_cast<typename MixedIntoType::FSlotArguments&>(*this);
		}

		typename MixedIntoType::FSlotArguments& Padding(float Uniform)
		{
			_Padding = FMargin(Uniform);
			return static_cast<typename MixedIntoType::FSlotArguments&>(*this);
		}

		typename MixedIntoType::FSlotArguments& Padding(float Horizontal, float Vertical)
		{
			_Padding = FMargin(Horizontal, Vertical);
			return static_cast<typename MixedIntoType::FSlotArguments&>(*this);
		}

		typename MixedIntoType::FSlotArguments& Padding(float Left, float Top, float Right, float Bottom)
		{
			_Padding = FMargin(Left, Top, Right, Bottom);
			return static_cast<typename MixedIntoType::FSlotArguments&>(*this);
		}
	};

protected:
	void ConstructMixin(FSlotArgumentsMixin&& InArgs)
	{
		if (InArgs._Padding.IsSet())
		{
			SWidget* OwnerWidget = static_cast<MixedIntoType*>(this)->GetOwnerWidget();
			check(OwnerWidget);
			SlotPaddingAttribute.Assign(*OwnerWidget, MoveTemp(InArgs._Padding));
		}
	}

public:
	// Padding will be deprecated soon. Use SetPadding or construct a new slot with FSlotArguments
	MixedIntoType& Padding(TAttribute<FMargin> InPadding)
	{
		SetPadding(MoveTemp(InPadding));
		return *(static_cast<MixedIntoType*>(this));
	}

	// Padding will be deprecated soon. Use SetPadding or construct a new slot with FSlotArguments
	MixedIntoType& Padding(float Uniform)
	{
		SetPadding(FMargin(Uniform));
		return *(static_cast<MixedIntoType*>(this));
	}

	// Padding will be deprecated soon. Use SetPadding or construct a new slot with FSlotArguments
	MixedIntoType& Padding(float Horizontal, float Vertical)
	{
		SetPadding(FMargin(Horizontal, Vertical));
		return *(static_cast<MixedIntoType*>(this));
	}

	// Padding will be deprecated soon. Use SetPadding or construct a new slot with FSlotArguments
	MixedIntoType& Padding(float Left, float Top, float Right, float Bottom)
	{
		SetPadding(FMargin(Left, Top, Right, Bottom));
		return *(static_cast<MixedIntoType*>(this));
	}

#if WITH_EDITORONLY_DATA
	UE_DEPRECATED(5.0, "Direct access to SlotPadding is now deprecated. Use the setter or getter.")
	TSlateDeprecatedTAttribute<FMargin> SlotPadding;
#endif

public:
	void SetPadding(TAttribute<FMargin> InPadding)
	{
		SWidget* OwnerWidget = static_cast<MixedIntoType*>(this)->GetOwnerWidget();
		check(OwnerWidget);
		SlotPaddingAttribute.Assign(*OwnerWidget, MoveTemp(InPadding));
	}

	FMargin GetPadding() const
	{
		return SlotPaddingAttribute.Get();
	}

public:
	using SlotPaddingInvalidationType = typename std::conditional<InPaddingInvalidationReason == EInvalidateWidgetReason::None, ::SlateAttributePrivate::FSlateAttributeNoInvalidationReason, TSlateAttributeInvalidationReason<InPaddingInvalidationReason>>::type;
	using SlotPaddingAttributeType = SlateAttributePrivate::TSlateMemberAttribute<FMargin, SlotPaddingInvalidationType, TSlateAttributeComparePredicate<>>;
	using SlotPaddingAttributeRefType = SlateAttributePrivate::TSlateMemberAttributeRef<SlotPaddingAttributeType>;

	template<typename WidgetType, typename V = typename std::enable_if<std::is_base_of<SWidget, WidgetType>::value>::type>
	SlotPaddingAttributeRefType GetSlotPaddingAttribute() const
	{
		WidgetType* Widget = static_cast<WidgetType*>(static_cast<MixedIntoType*>(this)->GetOwnerWidget());
		check(Widget);
		return SlotPaddingAttributeRefType(Widget->template SharedThis<WidgetType>(Widget), SlotPaddingAttribute);
	}

protected:
	SlotPaddingAttributeType SlotPaddingAttribute;
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS


/** A templated basic slot that can be used by layout. */
template <typename SlotType>
class TBasicLayoutWidgetSlot : public TWidgetSlotWithAttributeSupport<SlotType>
	, public TPaddingWidgetSlotMixin<SlotType>
	, public TAlignmentWidgetSlotMixin<SlotType>
{
public:
	TBasicLayoutWidgetSlot()
		: TWidgetSlotWithAttributeSupport<SlotType>()
		, TPaddingWidgetSlotMixin<SlotType>()
		, TAlignmentWidgetSlotMixin<SlotType>()
	{}

	TBasicLayoutWidgetSlot(FChildren& InOwner)
		: TWidgetSlotWithAttributeSupport<SlotType>(InOwner)
		, TPaddingWidgetSlotMixin<SlotType>()
		, TAlignmentWidgetSlotMixin<SlotType>()
	{}

	TBasicLayoutWidgetSlot(const EHorizontalAlignment InHAlign, const EVerticalAlignment InVAlign)
		: TWidgetSlotWithAttributeSupport<SlotType>()
		, TPaddingWidgetSlotMixin<SlotType>()
		, TAlignmentWidgetSlotMixin<SlotType>(InHAlign, InVAlign)
	{}

	TBasicLayoutWidgetSlot(FChildren& InOwner, const EHorizontalAlignment InHAlign, const EVerticalAlignment InVAlign)
		: TWidgetSlotWithAttributeSupport<SlotType>(InOwner)
		, TPaddingWidgetSlotMixin<SlotType>()
		, TAlignmentWidgetSlotMixin<SlotType>(InHAlign, InVAlign)
	{}

public:
	SLATE_SLOT_BEGIN_ARGS_TwoMixins(TBasicLayoutWidgetSlot, TSlotBase<SlotType>, TPaddingWidgetSlotMixin<SlotType>, TAlignmentWidgetSlotMixin<SlotType>)
	SLATE_SLOT_END_ARGS()

	void Construct(const FChildren& SlotOwner, FSlotArguments&& InArgs)
	{
		TWidgetSlotWithAttributeSupport<SlotType>::Construct(SlotOwner, MoveTemp(InArgs));
		TPaddingWidgetSlotMixin<SlotType>::ConstructMixin(SlotOwner, MoveTemp(InArgs));
		TAlignmentWidgetSlotMixin<SlotType>::ConstructMixin(SlotOwner, MoveTemp(InArgs));
	}

	static void RegisterAttributes(FSlateWidgetSlotAttributeInitializer& AttributeInitializer)
	{
		TWidgetSlotWithAttributeSupport<SlotType>::RegisterAttributes(AttributeInitializer);
		TPaddingWidgetSlotMixin<SlotType>::RegisterAttributesMixin(AttributeInitializer);
	}
};


/** The basic slot that can be used by layout. */
class FBasicLayoutWidgetSlot : public TBasicLayoutWidgetSlot<FBasicLayoutWidgetSlot>
{
public:
	using TBasicLayoutWidgetSlot<FBasicLayoutWidgetSlot>::TBasicLayoutWidgetSlot;
};
