#include "CommonHeader.h"
#include "MiscWidget.h"
#include "UI/Widgets/ColorPicker/SColorPicker.h"

#include <Widgets/Layout/SScaleBox.h>

namespace FW
{
	void SShToggleButton::Construct(const FArguments& InArgs)
	{
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
					SNew(STextBlock).Text(InArgs._Text.Get())
				];
		}

		IsCheckboxChecked = InArgs._IsChecked;
		OnCheckStateChanged = InArgs._OnCheckStateChanged;
		ChildSlot
			[
				SNew(SBorder)
					.Padding(4)
					.BorderImage(FAppCommonStyle::Get().GetBrush("Effect.Toggle"))
					.BorderBackgroundColor_Lambda([this] {
					if (IsCheckboxChecked.Get() == ECheckBoxState::Checked)
					{
						return FLinearColor{ 0.1f,0.5f,0.9f,1.0f };
					}
					else if (IsHovered())
					{
						return FLinearColor{ 0.5f,0.5f,0.5f,1.0f };
					}
					return FLinearColor::Transparent;
						})
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
						HBox
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
			.OnMouseButtonDown_Lambda([=, this](const FGeometry&, const FPointerEvent& MouseEvent)
			{
				if (!PickerWindow)
				{
					SAssignNew(PickerWindow, SWindow)
						.Type(EWindowType::ToolTip)
						.CreateTitleBar(false)
						.IsTopmostWindow(true)
						.SupportsMaximize(false)
						.SupportsMinimize(false)
						.IsPopupWindow(true)
						.SizingRule(ESizingRule::FixedSize)
						.ClientSize(FVector2D(350.f, 160.f))
						.ActivationPolicy(EWindowActivationPolicy::Never)
						[
							SNew(SColorPicker)
								.TargetColorAttribute(Color)
								.ShowAlpha(InArgs._ShowAlpha)
								.OnColorChanged(OnColorChanged)
						];
					FSlateApplication::Get().AddWindowAsNativeChild(PickerWindow.ToSharedRef(), ParentWindow.Pin().ToSharedRef());
					FVector2D WindowPos = GetTickSpaceGeometry().GetAbsolutePosition();
					WindowPos.Y += GetTickSpaceGeometry().GetAbsoluteSize().Y;
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
