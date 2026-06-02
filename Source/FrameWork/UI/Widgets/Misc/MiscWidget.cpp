#include "CommonHeader.h"
#include "MiscWidget.h"
#include "UI/Widgets/ColorPicker/SColorPicker.h"

#include <Widgets/Layout/SScaleBox.h>
#include <Widgets/Input/SComboButton.h>

namespace FW
{
	void SShToggleButton::Construct(const FArguments& InArgs)
	{
		ToggleColorAndOpacity = InArgs._ToggleColorAndOpacity;
		TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox);
		float Space = InArgs._Icon.IsSet() ? 6.0f : 0.0f;

		if (InArgs._Icon.IsSet())
		{
			HBox->AddSlot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SImage).Image(InArgs._Icon).ColorAndOpacity(InArgs._IconColorAndOpacity)
				];
		}

		if (!InArgs._Text.Get().IsEmpty())
		{
			HBox->AddSlot()
				.HAlign(HAlign_Center)
				.Padding(Space, 0.0f, 0.f, 0.f)
				[
					SNew(STextBlock).Text(InArgs._Text)
				];
		}

		if (!ToggleColorAndOpacity.IsBound())
		{
			ToggleColorAndOpacity = TAttribute<FSlateColor>::CreateLambda([this] {
				if (IsCheckboxChecked.Get() == ECheckBoxState::Checked)
				{
					return FStyleColors::Select.GetSpecifiedColor();
				}
				else if (IsHovered())
				{
					return FStyleColors::Hover.GetSpecifiedColor();
				}
				return FLinearColor::Transparent;
			});
		}

		IsCheckboxChecked = InArgs._IsChecked;
		OnCheckStateChanged = InArgs._OnCheckStateChanged;
		ChildSlot
			[
				SNew(SBorder)
					.Padding(4)
					.BorderImage(FAppCommonStyle::Get().GetBrush("Effect.Toggle"))
					.BorderBackgroundColor(ToggleColorAndOpacity)
					.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& MouseEvent) {
					if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
						{
							if (IsCheckboxChecked.Get() == ECheckBoxState::Checked)
							{
								OnCheckStateChanged.ExecuteIfBound(ECheckBoxState::Unchecked);
							}
							else
							{
								OnCheckStateChanged.ExecuteIfBound(ECheckBoxState::Checked);
							}
							return FReply::Handled();
						}
						return FReply::Unhandled();
					})
					[
						SNew(SScaleBox)
						[
							HBox
						]
						
					]
			];
	}

	void SIconButton::Construct(const FArguments& InArgs)
	{
		OnKeyDownHandler = InArgs._OnKeyDownHandler;
		TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox);
		float Space = InArgs._Icon.IsSet() ? 6.0f : 0.0f;

		if (InArgs._Icon.IsSet())
		{
			HBox->AddSlot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
						.ColorAndOpacity(InArgs._IconColorAndOpacity)
						.DesiredSizeOverride(InArgs._IconSize)
						.Image(InArgs._Icon)
				];
		}

		if (!InArgs._Label.IsEmpty())
		{
			HBox->AddSlot()
				.VAlign(VAlign_Center)
				.Padding(Space, 0.f, 0.f, 0.f)
				.AutoWidth()
				[
					SNew(STextBlock)
						.TextStyle(&FAppStyle::Get().GetWidgetStyle< FTextBlockStyle >("ButtonText"))
						.Justification(ETextJustify::Center)
						.Text(InArgs._Label)
				];
		}

		const FButtonStyle* Style = &FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton");
		if (InArgs._ButtonStyle)
		{
			Style = InArgs._ButtonStyle;
		}

		SButton::Construct(
			SButton::FArguments()
			.ButtonStyle(Style)
			.OnClicked(InArgs._OnClicked)
			.IsFocusable(InArgs._IsFocusable)
			.ContentPadding(FMargin{0})
			[
				SNew(SScaleBox)
				[
					HBox
				]
				
			]
		);
	}

	void SShSplitButton::Construct(const FArguments& InArgs)
	{
		CommandList = InArgs._CommandList;
		Command = InArgs._Command;
		ButtonToolTipText = InArgs._ButtonToolTipText;
		IsButtonEnabled = InArgs._IsButtonEnabled;
		IsMenuEnabled = InArgs._IsMenuEnabled;
		OnClicked = InArgs._OnClicked;
		OnGetMenuContent = InArgs._OnGetMenuContent;

		const FButtonStyle* ButtonStyle = &FAppCommonStyle::Get().GetWidgetStyle<FButtonStyle>("SuperSimpleButton");
		if (InArgs._ButtonStyle)
		{
			ButtonStyle = InArgs._ButtonStyle;
		}

		ChildSlot
		[
			SNew(SBorder)
			.Padding(0.0f)
			.BorderImage(FAppCommonStyle::Get().GetBrush("Effect.Toggle"))
			.BorderBackgroundColor_Lambda([this] {
				return IsHovered() ? FStyleColors::Hover.GetSpecifiedColor() : FLinearColor::Transparent;
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(ButtonStyle)
					.ContentPadding(InArgs._ButtonContentPadding)
					.ToolTipText(this, &SShSplitButton::GetButtonToolTipText)
					.IsEnabled(this, &SShSplitButton::CanExecuteButton)
					.OnClicked(this, &SShSplitButton::OnButtonClicked)
					[
						InArgs._ButtonContent.Widget
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SComboButton)
					.ButtonStyle(ButtonStyle)
					.ContentPadding(InArgs._MenuContentPadding)
					.ToolTipText(InArgs._MenuToolTipText)
					.IsEnabled(this, &SShSplitButton::CanOpenMenu)
					.ButtonContent()
					[
						InArgs._MenuButtonContent.Widget
					]
					.OnGetMenuContent(this, &SShSplitButton::MakeMenuContent)
				]
			]
		];
	}

	FText SShSplitButton::GetButtonToolTipText() const
	{
		FText ToolTipText = ButtonToolTipText.Get();
		if (!Command.IsValid())
		{
			return ToolTipText;
		}

		const TSharedPtr<const FInputChord> ActiveChord = Command->GetActiveChord(EMultipleKeyBindingIndex::Primary);
		if (!ActiveChord.IsValid() || ActiveChord->Key == EKeys::Invalid)
		{
			return ToolTipText;
		}

		const FText InputText = ActiveChord->GetInputText(false);
		if (InputText.IsEmpty())
		{
			return ToolTipText;
		}

		if (ToolTipText.IsEmpty())
		{
			return InputText;
		}
		return FText::Format(FText::FromString(TEXT("{0} ({1})")), ToolTipText, InputText);
	}

	bool SShSplitButton::CanExecuteButton() const
	{
		if (!IsButtonEnabled.Get())
		{
			return false;
		}

		if (CommandList.IsValid() && Command.IsValid())
		{
			return CommandList->CanExecuteAction(Command.ToSharedRef());
		}
		return OnClicked.IsBound();
	}

	FReply SShSplitButton::OnButtonClicked()
	{
		if (CommandList.IsValid() && Command.IsValid())
		{
			CommandList->ExecuteAction(Command.ToSharedRef());
			return FReply::Handled();
		}

		if (OnClicked.IsBound())
		{
			return OnClicked.Execute();
		}
		return FReply::Handled();
	}

	bool SShSplitButton::CanOpenMenu() const
	{
		return IsMenuEnabled.Get() && OnGetMenuContent.IsBound();
	}

	TSharedRef<SWidget> SShSplitButton::MakeMenuContent()
	{
		return OnGetMenuContent.Execute();
	}

	void SShColorBlockPicker::Construct(const FArguments& InArgs, TWeakPtr<SWindow> InParentWindow)
	{
		Color = InArgs._Color;
		OnColorChanged = InArgs._OnColorChanged;
		ParentWindow = InParentWindow;

		ChildSlot
		[
			SNew(SColorBlock)
			.AlphaDisplayMode(InArgs._AlphaDisplayMode)
			.ShowBackgroundForAlpha(InArgs._ShowBackgroundForAlpha)
			.Color(Color)
			.UseSRGB(InArgs._UseSRGB)
			.OnMouseButtonDown_Lambda([=, this](const FGeometry&, const FPointerEvent& MouseEvent)
			{
				if (!PickerWindow)
				{
					TSharedRef<SWidget> PickerContent = SNew(SColorPicker)
						.TargetColorAttribute(Color)
						.ShowAlpha(InArgs._ShowAlpha)
						.OnColorChanged(OnColorChanged)
						.PreviewSrgb(InArgs._UseSRGB.Get());

					PickerContent->SlatePrepass(FSlateApplication::Get().GetApplicationScale());
					const FVector2D ExpectedSize = PickerContent->GetDesiredSize();
					const FGeometry ColorBlockGeometry = GetTickSpaceGeometry();
					const FVector2D AnchorPos = ColorBlockGeometry.GetAbsolutePosition();
					const FVector2D AnchorSize = ColorBlockGeometry.GetAbsoluteSize();
					const FSlateRect Anchor(AnchorPos, AnchorPos + AnchorSize);
					const FVector2D WindowPos = FSlateApplication::Get().CalculatePopupWindowPosition(Anchor, ExpectedSize, false, FVector2D::ZeroVector, Orient_Vertical);

					SAssignNew(PickerWindow, SWindow)
					.Type(EWindowType::ToolTip)
					.CreateTitleBar(false)
					.IsTopmostWindow(true)
					.SupportsMaximize(false)
					.SupportsMinimize(false)
					.IsPopupWindow(true)
					.SizingRule(ESizingRule::Autosized)
					.ScreenPosition(WindowPos)
					.AutoCenter(EAutoCenter::None)
					.ClientSize(ExpectedSize)
					.ActivationPolicy(EWindowActivationPolicy::Never)
					[
						PickerContent
					];
					TSharedPtr<SWindow> OwningWindow = ParentWindow.Pin();
					if (!OwningWindow)
					{
						OwningWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
					}
					if (OwningWindow)
					{
						FSlateApplication::Get().AddWindowAsNativeChild(PickerWindow.ToSharedRef(), OwningWindow.ToSharedRef());
					}
					else
					{
						FSlateApplication::Get().AddWindow(PickerWindow.ToSharedRef());
					}
					PickerWindow->MoveWindowTo(WindowPos);
				}
				
				return FReply::Handled();
			})
		];
	}

	void SShColorBlockPicker::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		if (PickerWindow.IsValid())
		{
			FVector2D ScreenSpaceCursorPos = FSlateApplication::Get().GetCursorPos();
			FGeometry WindowGeometry = PickerWindow->GetWindowGeometryInScreen();
			bool bMouseInPickerWindow = PickerWindow->IsVisible() && WindowGeometry.IsUnderLocation(ScreenSpaceCursorPos);
			bool bMouseInColorBlock = AllottedGeometry.IsUnderLocation(ScreenSpaceCursorPos);
			if (!bMouseInPickerWindow && !bMouseInColorBlock)
			{
				PickerWindow->RequestDestroyWindow();
				PickerWindow.Reset();
			}
		}
	}
}
