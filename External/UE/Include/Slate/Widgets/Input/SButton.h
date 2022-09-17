// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateColor.h"
#include "Layout/Margin.h"
#include "Sound/SlateSound.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "Framework/SlateDelegates.h"
#include "Styling/SlateWidgetStyleAsset.h"
#include "Widgets/Layout/SBorder.h"

class FPaintArgs;
class FSlateWindowElementList;
enum class ETextFlowDirection : uint8;
enum class ETextShapingMethod : uint8;

/**
 * Slate's Buttons are clickable Widgets that can contain arbitrary widgets as its Content().
 */
class SLATE_API SButton : public SBorder
{
	SLATE_DECLARE_WIDGET(SButton, SBorder)

#if WITH_ACCESSIBILITY
	// Allow the accessible button to "click" this button
	friend class FSlateAccessibleButton;
#endif
public:

	SLATE_BEGIN_ARGS( SButton )
		: _Content()
		, _ButtonStyle( &FCoreStyle::Get().GetWidgetStyle< FButtonStyle >( "Button" ) )
		, _TextStyle( &FCoreStyle::Get().GetWidgetStyle< FTextBlockStyle >("ButtonText") )
		, _HAlign( HAlign_Fill )
		, _VAlign( VAlign_Fill )
		, _ContentPadding(FMargin(4.0, 2.0))
		, _Text()
		, _ClickMethod( EButtonClickMethod::DownAndUp )
		, _TouchMethod( EButtonTouchMethod::DownAndUp )
		, _PressMethod( EButtonPressMethod::DownAndUp )
		, _DesiredSizeScale( FVector2D(1,1) )
		, _ContentScale( FVector2D(1,1) )
		, _ButtonColorAndOpacity(FLinearColor::White)
		, _ForegroundColor(FSlateColor::UseStyle())
		, _IsFocusable( true )
		{
		}

		/** Slot for this button's content (optional) */
		SLATE_DEFAULT_SLOT( FArguments, Content )

		/** The visual style of the button */
		SLATE_STYLE_ARGUMENT( FButtonStyle, ButtonStyle )

		/** The text style of the button */
		SLATE_STYLE_ARGUMENT( FTextBlockStyle, TextStyle )

		/** Horizontal alignment */
		SLATE_ARGUMENT( EHorizontalAlignment, HAlign )

		/** Vertical alignment */
		SLATE_ARGUMENT( EVerticalAlignment, VAlign )
	
		/** Spacing between button's border and the content. */
		SLATE_ATTRIBUTE( FMargin, ContentPadding )
	
		/** The text to display in this button, if no custom content is specified */
		SLATE_ATTRIBUTE( FText, Text )
	
		/** Called when the button is clicked */
		SLATE_EVENT( FOnClicked, OnClicked )

		/** Called when the button is pressed */
		SLATE_EVENT( FSimpleDelegate, OnPressed )

		/** Called when the button is released */
		SLATE_EVENT( FSimpleDelegate, OnReleased )

		SLATE_EVENT( FSimpleDelegate, OnHovered )

		SLATE_EVENT( FSimpleDelegate, OnUnhovered )

		/** Sets the rules to use for determining whether the button was clicked.  This is an advanced setting and generally should be left as the default. */
		SLATE_ARGUMENT( EButtonClickMethod::Type, ClickMethod )

		/** How should the button be clicked with touch events? */
		SLATE_ARGUMENT( EButtonTouchMethod::Type, TouchMethod )

		/** How should the button be clicked with keyboard/controller button events? */
		SLATE_ARGUMENT( EButtonPressMethod::Type, PressMethod )

		SLATE_ATTRIBUTE( FVector2D, DesiredSizeScale )

		SLATE_ATTRIBUTE( FVector2D, ContentScale )

		SLATE_ATTRIBUTE( FSlateColor, ButtonColorAndOpacity )

		SLATE_ATTRIBUTE( FSlateColor, ForegroundColor )

		/** Sometimes a button should only be mouse-clickable and never keyboard focusable. */
		SLATE_ARGUMENT( bool, IsFocusable )

		/** The sound to play when the button is pressed */
		SLATE_ARGUMENT( TOptional<FSlateSound>, PressedSoundOverride )

		/** The sound to play when the button is hovered */
		SLATE_ARGUMENT( TOptional<FSlateSound>, HoveredSoundOverride )

		/** Which text shaping method should we use? (unset to use the default returned by GetDefaultTextShapingMethod) */
		SLATE_ARGUMENT( TOptional<ETextShapingMethod>, TextShapingMethod )
		
		/** Which text flow direction should we use? (unset to use the default returned by GetDefaultTextFlowDirection) */
		SLATE_ARGUMENT( TOptional<ETextFlowDirection>, TextFlowDirection )

	SLATE_END_ARGS()

protected:
		SButton();

public:
	/**
	 * @return An image that represents this button's border
	 */
	UE_DEPRECATED(5.0, "GetBorder is deprecated. Use SetBorderImage or GetBorderImage")
	virtual const FSlateBrush* GetBorder() const
	{
		return GetBorderImage();
	}

	/** @return the Foreground color that this widget sets; unset options if the widget does not set a foreground color */
	virtual FSlateColor GetForegroundColor() const final
	{
		return Super::GetForegroundColor();
	}

	/** @return the Foreground color that this widget sets when this widget or any of its ancestors are disabled; unset options if the widget does not set a foreground color */
	virtual FSlateColor GetDisabledForegroundColor() const final;

	/**
	 * Returns true if this button is currently pressed
	 *
	 * @return	True if pressed, otherwise false
	 * @note IsPressed used to be virtual. Use SetAppearPressed to assign an attribute if you need to override the default behavior.
	 */
	bool IsPressed() const
	{
		return bIsPressed || AppearPressedAttribute.Get();
	}

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );

	/** See ContentPadding attribute */
	void SetContentPadding(TAttribute<FMargin> InContentPadding);

	/** See HoveredSound attribute */
	void SetHoveredSound(TOptional<FSlateSound> InHoveredSound);

	/** See PressedSound attribute */
	void SetPressedSound(TOptional<FSlateSound> InPressedSound);

	/** See OnClicked event */
	void SetOnClicked(FOnClicked InOnClicked);

	/** Set OnHovered event */
	void SetOnHovered(FSimpleDelegate InOnHovered);

	/** Set OnUnhovered event */
	void SetOnUnhovered(FSimpleDelegate InOnUnhovered);

	/** See ButtonStyle attribute */
	void SetButtonStyle(const FButtonStyle* ButtonStyle);

	void SetClickMethod(EButtonClickMethod::Type InClickMethod);
	void SetTouchMethod(EButtonTouchMethod::Type InTouchMethod);
	void SetPressMethod(EButtonPressMethod::Type InPressMethod);

#if !UE_BUILD_SHIPPING
	void SimulateClick();
#endif // !UE_BUILD_SHIPPING

public:

	//~ SWidget overrides
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual bool SupportsKeyboardFocus() const override;
	virtual void OnFocusLost( const FFocusEvent& InFocusEvent ) override;
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) override;
	virtual FReply OnKeyUp( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) override;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent ) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual bool IsInteractable() const override;
#if WITH_ACCESSIBILITY
	virtual TSharedRef<FSlateAccessibleWidget> CreateAccessibleWidget() override;
#endif
private:
	virtual FVector2D ComputeDesiredSize(float) const override;
	//~ SWidget

protected:
	/** Press the button */
	virtual void Press();

	/** Release the button */
	virtual void Release();

	/** Execute the "OnClicked" delegate, and get the reply */
	FReply ExecuteOnClick();

	/** @return combines the user-specified margin and the button's internal margin. */
	FMargin GetCombinedPadding() const;

	/** @return True if the disabled effect should be shown. */
	bool GetShowDisabledEffect() const;

	/** Utility function to translate other input click methods to regular ones. */
	TEnumAsByte<EButtonClickMethod::Type> GetClickMethodFromInputType(const FPointerEvent& MouseEvent) const;

	/** Utility function to determine if the incoming mouse event is for a precise tap or click */
	bool IsPreciseTapOrClick(const FPointerEvent& MouseEvent) const;

	/** Play the pressed sound */
	void PlayPressedSound() const;

	/** Play the hovered sound */
	void PlayHoverSound() const;

	/** Set if this button can be focused */
	void SetIsFocusable(bool bInIsFocusable)
	{
		bIsFocusable = bInIsFocusable;
	}

	void ExecuteHoverStateChanged(bool bPlaySound);

protected:
	/** @return the BorderForegroundColor attribute. */
	TSlateAttributeRef<FSlateColor> GetBorderForegroundColorAttribute() const { return TSlateAttributeRef<FSlateColor>(SharedThis(this), BorderForegroundColorAttribute); }

	/** @return the ContentPadding attribute. */
	TSlateAttributeRef<FMargin> GetContentPaddingAttribute() const { return TSlateAttributeRef<FMargin>(SharedThis(this), ContentPaddingAttribute); }

	/** Set the AppearPressed look. */
	void SetAppearPressed(TAttribute<bool> InValue)
	{
		AppearPressedAttribute.Assign(*this, MoveTemp(InValue));
	}

	/** @return the AppearPressed attribute. */
	TSlateAttributeRef<bool> GetAppearPressedAttribute() const { return TSlateAttributeRef<bool>(SharedThis(this), AppearPressedAttribute); }

private:
	void UpdatePressStateChanged();

	void UpdatePadding();
	void UpdateShowDisabledEffect();
	void UpdateBorderImage();
	void UpdateForegroundColor();
	void UpdateDisabledForegroundColor();

protected:
#if WITH_EDITORONLY_DATA
	UE_DEPRECATED(5.0, "Direct access to ContentPadding is now deprecated. Use the setter or getter.")
	TSlateDeprecatedTAttribute<FMargin> ContentPadding;
	/** Brush resource that represents a button */
	UE_DEPRECATED(5.0, "NormalImage is now deprecated. Use the ButtonStyle.")
	const FSlateBrush* NormalImage;
	/** Brush resource that represents a button when it is hovered */
	UE_DEPRECATED(5.0, "HoverImage is now deprecated. Use the ButtonStyle.")
	const FSlateBrush* HoverImage;
	/** Brush resource that represents a button when it is pressed */
	UE_DEPRECATED(5.0, "PressedImage is now deprecated. Use the ButtonStyle.")
	const FSlateBrush* PressedImage;
	/** Brush resource that represents a button when it is disabled */
	UE_DEPRECATED(5.0, "DisabledImage is now deprecated. Use the ButtonStyle.")
	const FSlateBrush* DisabledImage;
	/** Padding that accounts for the button border */
	UE_DEPRECATED(5.0, "BorderPadding is now deprecated. Use the ButtonStyle.")
	FMargin BorderPadding;
	/** Padding that accounts for the button border when pressed */
	UE_DEPRECATED(5.0, "PressedBorderPadding is now deprecated. Use the ButtonStyle.")
	FMargin PressedBorderPadding;
#endif

private:
	/** The location in screenspace the button was pressed */
	FVector2D PressedScreenSpacePosition;

	/** Style resource for the button */
	const FButtonStyle* Style;
	/** The delegate to execute when the button is clicked */
	FOnClicked OnClicked;

	/** The delegate to execute when the button is pressed */
	FSimpleDelegate OnPressed;

	/** The delegate to execute when the button is released */
	FSimpleDelegate OnReleased;

	/** The delegate to execute when the button is hovered */
	FSimpleDelegate OnHovered;

	/** The delegate to execute when the button exit the hovered state */
	FSimpleDelegate OnUnhovered;

	/** The Sound to play when the button is hovered  */
	FSlateSound HoveredSound;

	/** The Sound to play when the button is pressed */
	FSlateSound PressedSound;

	/** Sets whether a click should be triggered on mouse down, mouse up, or that both a mouse down and up are required. */
	TEnumAsByte<EButtonClickMethod::Type> ClickMethod;

	/** How should the button be clicked with touch events? */
	TEnumAsByte<EButtonTouchMethod::Type> TouchMethod;

	/** How should the button be clicked with keyboard/controller button events? */
	TEnumAsByte<EButtonPressMethod::Type> PressMethod;

	/** Can this button be focused? */
	uint8 bIsFocusable:1;

	/** True if this button is currently in a pressed state */
	uint8 bIsPressed:1;

private:
	/** Optional foreground color that will be inherited by all of this widget's contents */
	TSlateAttribute<FSlateColor> BorderForegroundColorAttribute;
	/** Padding specified by the user; it will be combind with the button's internal padding. */
	TSlateAttribute<FMargin> ContentPaddingAttribute;
	/** Optional foreground color that will be inherited by all of this widget's contents */
	TSlateAttribute<bool> AppearPressedAttribute;
};
