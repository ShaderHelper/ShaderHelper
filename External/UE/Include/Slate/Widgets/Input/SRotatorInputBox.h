// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"
#include "Fonts/SlateFontInfo.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/CoreStyle.h"
#include "Framework/SlateDelegates.h"
#include "Widgets/Input/NumericTypeInterface.h"
#include "Widgets/Input/SNumericEntryBox.h"

class FArrangedChildren;

/**
 * FRotator Slate control
 */
template<typename NumericType>
class SNumericRotatorInputBox : public SCompoundWidget
{
public:
	/** Notification for float value change */
	DECLARE_DELEGATE_OneParam(FOnNumericValueChanged, NumericType);

	/** Notification for float value committed */
	DECLARE_DELEGATE_TwoParams(FOnNumericValueCommitted, NumericType, ETextCommit::Type);
	
	SLATE_BEGIN_ARGS( SNumericRotatorInputBox<NumericType> )
		: _bColorAxisLabels(false)
		, _Font(FAppStyle::Get().GetFontStyle("NormalFont"))
		, _AllowSpin(true)
		, _DisplayToggle( false )
		, _TogglePitchChecked( ECheckBoxState::Checked )
		, _ToggleYawChecked( ECheckBoxState::Checked )
		, _ToggleRollChecked( ECheckBoxState::Checked )
		, _TogglePadding( FMargin( 1.f,0.f,1.f,0.f ) )
		{}

		/** Roll component of the rotator */
		SLATE_ATTRIBUTE( TOptional<NumericType>, Roll )

		/** Pitch component of the rotator */
		SLATE_ATTRIBUTE( TOptional<NumericType>, Pitch )

		/** Yaw component of the rotator */
		SLATE_ATTRIBUTE( TOptional<NumericType>, Yaw )

		/** Should the axis labels be colored */
		SLATE_ARGUMENT( bool, bColorAxisLabels )		

		UE_DEPRECATED(5.0, "AllowResponsiveLayout unused as it is no longer necessary.")
		FArguments& AllowResponsiveLayout(bool bAllow)
		{
			return TSlateBaseNamedArgs<SNumericRotatorInputBox<NumericType>>::Me();
		}

		/** Font to use for the text in this box */
		SLATE_ATTRIBUTE( FSlateFontInfo, Font )

		/** Whether or not values can be spun or if they should be typed in */
		SLATE_ARGUMENT( bool, AllowSpin )

		/** Called when the pitch value is changed */
		SLATE_EVENT( FOnNumericValueChanged, OnPitchChanged )

		/** Called when the yaw value is changed */
		SLATE_EVENT( FOnNumericValueChanged, OnYawChanged )

		/** Called when the roll value is changed */
		SLATE_EVENT( FOnNumericValueChanged, OnRollChanged )

		/** Called when the pitch value is committed */
		SLATE_EVENT( FOnNumericValueCommitted, OnPitchCommitted )

		/** Called when the yaw value is committed */
		SLATE_EVENT( FOnNumericValueCommitted, OnYawCommitted )

		/** Called when the roll value is committed */
		SLATE_EVENT( FOnNumericValueCommitted, OnRollCommitted )

		/** Called when the slider begins to move on any axis */
		SLATE_EVENT( FSimpleDelegate, OnBeginSliderMovement )

		/** Called when the slider for any axis is released */
		SLATE_EVENT( FOnNumericValueChanged, OnEndSliderMovement )

		/** Provide custom type functionality for the rotator */
		SLATE_ARGUMENT( TSharedPtr< INumericTypeInterface<NumericType> >, TypeInterface )

		/** Whether or not to include a toggle checkbox to the left of the widget */
		SLATE_ARGUMENT( bool, DisplayToggle )
			
		/** The value of the toggle Pitch checkbox */
		SLATE_ATTRIBUTE( ECheckBoxState, TogglePitchChecked )

		/** The value of the toggle Yaw checkbox */
		SLATE_ATTRIBUTE( ECheckBoxState, ToggleYawChecked )

		/** The value of the toggle Roll checkbox */
		SLATE_ATTRIBUTE( ECheckBoxState, ToggleRollChecked )

		/** Called whenever the toggle Pitch changes state */
		SLATE_EVENT( FOnCheckStateChanged, OnTogglePitchChanged )

		/** Called whenever the toggle Yaw changes state */
		SLATE_EVENT( FOnCheckStateChanged, OnToggleYawChanged )

		/** Called whenever the toggle Roll changes state */
		SLATE_EVENT( FOnCheckStateChanged, OnToggleRollChanged )

		/** Padding around the toggle checkbox */
		SLATE_ARGUMENT( FMargin, TogglePadding )

	SLATE_END_ARGS()

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs )
	{
		this->ChildSlot
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			[
				SNew(SNumericEntryBox<NumericType>)
				.AllowSpin(InArgs._AllowSpin)
				.MinSliderValue(0.0f)
				.MaxSliderValue(359.999f)
				.LabelPadding(FMargin(3.0f))
				.LabelLocation(SNumericEntryBox<NumericType>::ELabelLocation::Inside)
				.Label()
				[
					InArgs._bColorAxisLabels ? SNumericEntryBox<NumericType>::BuildNarrowColorLabel(SNumericEntryBox<NumericType>::RedLabelBackgroundColor) : SNullWidget::NullWidget
				]
				.Font( InArgs._Font )
				.Value( InArgs._Roll )
				.OnValueChanged( InArgs._OnRollChanged )
				.OnValueCommitted( InArgs._OnRollCommitted )
				.OnBeginSliderMovement( InArgs._OnBeginSliderMovement )
				.OnEndSliderMovement( InArgs._OnEndSliderMovement )
				.UndeterminedString( NSLOCTEXT("SRotatorInputBox", "MultipleValues", "Multiple Values") )
				.ToolTipText_Lambda([RollAttr = InArgs._Roll]
				{
					const TOptional<NumericType>& RollValue = RollAttr.Get();
					return RollValue.IsSet() 
						? FText::Format(NSLOCTEXT("SRotatorInputBox", "Roll_ToolTip", "X(Roll): {0}"), RollValue.GetValue())
						: NSLOCTEXT("SRotatorInputBox", "MultipleValues", "Multiple Values");
				})
				.TypeInterface(InArgs._TypeInterface)
				.DisplayToggle(InArgs._DisplayToggle)
				.ToggleChecked(InArgs._ToggleRollChecked)
				.OnToggleChanged(InArgs._OnToggleRollChanged)
				.TogglePadding(InArgs._TogglePadding)
			]
			+SHorizontalBox::Slot()
			[
				SNew(SNumericEntryBox<NumericType>)
				.AllowSpin(InArgs._AllowSpin)
				.MinSliderValue(0.0f)
				.MaxSliderValue(359.999f)
				.LabelPadding(FMargin(3.0f))
				.LabelLocation(SNumericEntryBox<NumericType>::ELabelLocation::Inside)
				.Label()
				[
					InArgs._bColorAxisLabels ? SNumericEntryBox<NumericType>::BuildNarrowColorLabel(SNumericEntryBox<NumericType>::GreenLabelBackgroundColor) : SNullWidget::NullWidget
				]
				.Font( InArgs._Font )
				.Value( InArgs._Pitch )
				.OnValueChanged( InArgs._OnPitchChanged )
				.OnValueCommitted( InArgs._OnPitchCommitted )
				.OnBeginSliderMovement( InArgs._OnBeginSliderMovement )
				.OnEndSliderMovement( InArgs._OnEndSliderMovement )
				.UndeterminedString( NSLOCTEXT("SRotatorInputBox", "MultipleValues", "Multiple Values") )
				.ToolTipText_Lambda([PitchAttr = InArgs._Pitch]
				{
					const TOptional<NumericType>& PitchValue = PitchAttr.Get();
					return PitchValue.IsSet()
						? FText::Format(NSLOCTEXT("SRotatorInputBox", "Pitch_ToolTip", "Y(Pitch): {0}"), PitchValue.GetValue())
						: NSLOCTEXT("SRotatorInputBox", "MultipleValues", "Multiple Values");
				})
				.TypeInterface(InArgs._TypeInterface)
				.DisplayToggle(InArgs._DisplayToggle)
				.ToggleChecked(InArgs._TogglePitchChecked)
				.OnToggleChanged(InArgs._OnTogglePitchChanged)
				.TogglePadding(InArgs._TogglePadding)
			]
			+SHorizontalBox::Slot()
			[
				SNew(SNumericEntryBox<NumericType>)
				.AllowSpin(InArgs._AllowSpin)
				.MinSliderValue(0.0f)
				.MaxSliderValue(359.999f)
				.LabelPadding(FMargin(3.0f))
				.LabelLocation(SNumericEntryBox<NumericType>::ELabelLocation::Inside)
				.Label()
				[
					InArgs._bColorAxisLabels ? SNumericEntryBox<NumericType>::BuildNarrowColorLabel(SNumericEntryBox<NumericType>::BlueLabelBackgroundColor) : SNullWidget::NullWidget
				]
				.Font( InArgs._Font )
				.Value( InArgs._Yaw )
				.OnValueChanged( InArgs._OnYawChanged )
				.OnValueCommitted( InArgs._OnYawCommitted )
				.OnBeginSliderMovement( InArgs._OnBeginSliderMovement )
				.OnEndSliderMovement( InArgs._OnEndSliderMovement )
				.UndeterminedString( NSLOCTEXT("SRotatorInputBox", "MultipleValues", "Multiple Values") )
				.ToolTipText_Lambda([YawAttr = InArgs._Yaw]
				{
					const TOptional<NumericType>& YawValue = YawAttr.Get();
					return YawValue.IsSet()
						? FText::Format(NSLOCTEXT("SRotatorInputBox", "Yaw_ToolTip", "Z(Yaw): {0}"), YawValue.GetValue())
						: NSLOCTEXT("SRotatorInputBox", "MultipleValues", "Multiple Values");
				})
				.TypeInterface(InArgs._TypeInterface)
				.DisplayToggle(InArgs._DisplayToggle)
				.ToggleChecked(InArgs._ToggleYawChecked)
				.OnToggleChanged(InArgs._OnToggleYawChanged)
				.TogglePadding(InArgs._TogglePadding)
			]
		];

	}	
};

/**
 * For backward compatibility
 */
using SRotatorInputBox = SNumericRotatorInputBox<float>;
