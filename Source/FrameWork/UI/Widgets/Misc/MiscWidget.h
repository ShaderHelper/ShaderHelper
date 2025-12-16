#pragma once
#include "UI/Styles/FAppCommonStyle.h"

#include <Widgets/Text/SInlineEditableTextBlock.h>
#include <Widgets/Colors/SColorBlock.h>
#include <Styling/StyleColors.h>

namespace FW
{
	class SShInlineEditableTextBlock : public SInlineEditableTextBlock
	{
	public:
		void Construct(const FArguments& InArgs)
		{
			SInlineEditableTextBlock::Construct(InArgs);
		}
		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override { return FReply::Unhandled(); }
		virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override
		{
			if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !bIsReadOnly.Get())
			{
				EnterEditingMode();
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}
	};

	class FRAMEWORK_API SIconButton : public SButton
	{
	private:
		FOnKeyDown OnKeyDownHandler;

	public:
		SLATE_BEGIN_ARGS(SIconButton) : _ButtonStyle(nullptr), _Icon(nullptr), _IconColorAndOpacity(FStyleColors::Foreground){}
			SLATE_STYLE_ARGUMENT(FButtonStyle, ButtonStyle)
			SLATE_ATTRIBUTE(const FSlateBrush*, Icon)
			SLATE_ARGUMENT(TOptional<FVector2D>, IconSize)
			SLATE_ATTRIBUTE(FSlateColor, IconColorAndOpacity)
			SLATE_ARGUMENT(FText, Label)
			SLATE_ARGUMENT(bool, IsFocusable)
			SLATE_EVENT(FOnClicked, OnClicked)
			SLATE_EVENT(FOnKeyDown, OnKeyDownHandler)
		SLATE_END_ARGS()

		virtual bool SupportsKeyboardFocus() const override { return true; }
		FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
		{
			if (OnKeyDownHandler.IsBound())
			{
				return OnKeyDownHandler.Execute(MyGeometry, InKeyEvent);
			}
			return FReply::Unhandled();
		}

		void Construct(const FArguments& InArgs);
	};

	class FRAMEWORK_API SShToggleButton : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SShToggleButton) 
			: _IconColorAndOpacity(FStyleColors::Foreground)
			, _OnCheckStateChanged()
			, _IsChecked(ECheckBoxState::Unchecked)
			{}
			SLATE_ATTRIBUTE(const FSlateBrush*, Icon)
			SLATE_ATTRIBUTE(FSlateColor, IconColorAndOpacity)
			SLATE_ATTRIBUTE(FText, Text)
			SLATE_EVENT(FOnCheckStateChanged, OnCheckStateChanged)
			SLATE_ATTRIBUTE(ECheckBoxState, IsChecked)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

		TAttribute<ECheckBoxState> IsCheckboxChecked;
		FOnCheckStateChanged OnCheckStateChanged;
	};

	class FRAMEWORK_API SShColorBlockPicker : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SShColorBlockPicker)
			: _Color(FLinearColor::White)
			, _AlphaDisplayMode(EColorBlockAlphaDisplayMode::Ignore)
			, _ShowBackgroundForAlpha(false)
			, _ShowAlpha(false)
		{}
			SLATE_ATTRIBUTE(FLinearColor, Color)
			SLATE_ATTRIBUTE(EColorBlockAlphaDisplayMode, AlphaDisplayMode)
			SLATE_ATTRIBUTE(bool, ShowBackgroundForAlpha)
			SLATE_ARGUMENT(bool, ShowAlpha)
			SLATE_EVENT(FOnLinearColorValueChanged, OnColorChanged)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, TWeakPtr<SWindow> InParentWindow);
		void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	private:
		TAttribute<FLinearColor> Color;
		FOnLinearColorValueChanged OnColorChanged;
		TSharedPtr<SWindow> PickerWindow;
		TWeakPtr<SWindow> ParentWindow;
	};
}
