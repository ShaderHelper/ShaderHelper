#include "CommonHeader.h"
#include "SColorPicker.h"
#include "UI/Widgets/Misc/MiscWidget.h"

#include <Widgets/Colors/SColorWheel.h>
#include <Widgets/Colors/SSimpleGradient.h>
#include <Widgets/Input/SSpinBox.h>
#include <Widgets/Input/SSlider.h>
#include <Widgets/Input/SSegmentedControl.h>
#include <Widgets/Input/SEditableTextBox.h>
#include <Widgets/Layout/SWidgetSwitcher.h>
#include <Widgets/Colors/SColorBlock.h>

namespace FW
{
	SColorPicker::~SColorPicker()
	{
		OnDestroyed.ExecuteIfBound();
	}

	void SColorPicker::Construct(const FArguments& InArgs)
	{
		OnColorChanged = InArgs._OnColorChanged;
		OnDestroyed = InArgs._OnDestroyed;
		bShowAlpha = InArgs._ShowAlpha;
		bPreviewSrgb = InArgs._PreviewSrgb;

		CurrentColor = InArgs._TargetColorAttribute.IsSet() ? InArgs._TargetColorAttribute.Get() : FLinearColor::White;
		SyncHSVColorFromCurrentColor();

		ChildSlot
		[
			SNew(SBox)
			.WidthOverride(350.f)
			.HeightOverride(190.f)
			[
				SNew(SBorder)
				.Padding(8.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride(160.f)
						.HeightOverride(160.f)
						[
							SNew(SColorWheel)
							.SelectedColor_Lambda([this] {
								return FLinearColor(CurrentHSVColor.R, CurrentHSVColor.G, 0.f, 1.f);
							})
							.OnValueChanged(FOnLinearColorValueChanged::CreateSP(this, &SColorPicker::HandleWheelColorChanged))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.MaxWidth(12.0f)
					.Padding(8, 0)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SSimpleGradient)
							.StartColor_Lambda([this]{ 
								return FLinearColor::White;
							})
							.EndColor_Lambda([this]{ 
								return FLinearColor::Black;
							})
							.Orientation(Orient_Horizontal)
						]
						+ SOverlay::Slot()
						[
							SNew(SSlider)
							.IndentHandle(false)
							.Orientation(Orient_Vertical)
							.SliderBarColor(FLinearColor::Transparent)
							.Style(&FAppStyle::Get().GetWidgetStyle<FSliderStyle>("ColorPicker.Slider"))
							.Value_Lambda([this]{ 
								return CurrentHSVColor.B;
							})
							.OnValueChanged_Lambda([this](float V){ HandleHSVChanged(V, 2); })
						]
					]
					+ SHorizontalBox::Slot()
					.Padding(8,0)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,6)
						[
							SNew(SSegmentedControl<int32>)
							.Value_Lambda([this]{ return static_cast<int32>(DisplayMode); })
							.OnValueChanged_Lambda([this](int32 NewValue){ DisplayMode = static_cast<EColorDisplayMode>(NewValue); })
							+ SSegmentedControl<int32>::Slot(0).Text(FText::FromString(TEXT("RGB")))
							+ SSegmentedControl<int32>::Slot(1).Text(FText::FromString(TEXT("HSV")))
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,6)
						[
							SNew(SWidgetSwitcher)
							.WidgetIndex_Lambda([this]{ return static_cast<int32>(DisplayMode); })
							+ SWidgetSwitcher::Slot()
							[
								SNew(SGridPanel).FillColumn(1, 1.0f)
								+ SGridPanel::Slot(0,0).VAlign(VAlign_Center)[ SNew(STextBlock).Text(FText::FromString(TEXT("R"))) ]
								+ SGridPanel::Slot(1,0).Padding(6,0,0,0)[ SNew(SSpinBox<float>).MinValue(0.f).MaxValue(1.f).Delta(0.01f).MaxFractionalDigits(3)
									.Value_Lambda([this]{ return CurrentColor.R; })
									.OnValueChanged_Lambda([this](float V){ HandleRGBChanged(V, 0); }) ]

								+ SGridPanel::Slot(0,1).VAlign(VAlign_Center)[ SNew(STextBlock).Text(FText::FromString(TEXT("G"))) ]
								+ SGridPanel::Slot(1,1).Padding(6,0,0,0)[ SNew(SSpinBox<float>).MinValue(0.f).MaxValue(1.f).Delta(0.01f).MaxFractionalDigits(3)
									.Value_Lambda([this]{ return CurrentColor.G; })
									.OnValueChanged_Lambda([this](float V){ HandleRGBChanged(V, 1); }) ]

								+ SGridPanel::Slot(0,2).VAlign(VAlign_Center)[ SNew(STextBlock).Text(FText::FromString(TEXT("B"))) ]
								+ SGridPanel::Slot(1,2).Padding(6,0,0,0)[ SNew(SSpinBox<float>).MinValue(0.f).MaxValue(1.f).Delta(0.01f).MaxFractionalDigits(3)
									.Value_Lambda([this]{ return CurrentColor.B; })
									.OnValueChanged_Lambda([this](float V){ HandleRGBChanged(V, 2); }) ]
							]
							+ SWidgetSwitcher::Slot()
							[
								SNew(SGridPanel).FillColumn(1, 1.0f)
								+ SGridPanel::Slot(0,0).VAlign(VAlign_Center)[ SNew(STextBlock).Text(FText::FromString(TEXT("H"))) ]
								+ SGridPanel::Slot(1,0).Padding(6,0,0,0)[ SNew(SSpinBox<float>).MinValue(0.f).MaxValue(1.f).Delta(0.01f).MaxFractionalDigits(3)
									.Value_Lambda([this]{ 
										return CurrentHSVColor.R / 360.f;
									})
									.OnValueChanged_Lambda([this](float V){ HandleHSVChanged(V, 0); }) ]

								+ SGridPanel::Slot(0,1).VAlign(VAlign_Center)[ SNew(STextBlock).Text(FText::FromString(TEXT("S"))) ]
								+ SGridPanel::Slot(1,1).Padding(6,0,0,0)[ SNew(SSpinBox<float>).MinValue(0.f).MaxValue(1.f).Delta(0.01f).MaxFractionalDigits(3)
									.Value_Lambda([this]{ 
										return CurrentHSVColor.G;
									})
									.OnValueChanged_Lambda([this](float V){ HandleHSVChanged(V, 1); }) ]

								+ SGridPanel::Slot(0,2).VAlign(VAlign_Center)[ SNew(STextBlock).Text(FText::FromString(TEXT("V"))) ]
								+ SGridPanel::Slot(1,2).Padding(6,0,0,0)[ SNew(SSpinBox<float>).MinValue(0.f).MaxValue(1.f).Delta(0.01f).MaxFractionalDigits(3)
									.Value_Lambda([this]{ 
										return CurrentHSVColor.B;
									})
									.OnValueChanged_Lambda([this](float V){ HandleHSVChanged(V, 2); }) ]
							]
						]

						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							.Visibility_Lambda([this]{ return bShowAlpha ? EVisibility::Visible : EVisibility::Collapsed; })
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[ SNew(STextBlock).Text(FText::FromString(TEXT("A"))) ]
							+ SHorizontalBox::Slot().Padding(6,0,0,0)
							[ SNew(SSpinBox<float>).MinValue(0.f).MaxValue(1.f).Delta(0.01f).MaxFractionalDigits(3)
								.Value_Lambda([this]{ return CurrentColor.A; })
								.OnValueChanged_Lambda([this](float V){ HandleAlphaChanged(V); }) ]
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Hex")))
							]
							+ SHorizontalBox::Slot().Padding(6, 0, 0, 0)
							[
								SNew(SEditableTextBox)
								.Text_Lambda([this] {
									return FText::FromString(CurrentColor.ToFColor(true).ToHex());
								})
								.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type CommitType) {
									FColor NewColor = FColor::FromHex(Text.ToString());
									CurrentColor = FLinearColor(NewColor);
									SyncHSVColorFromCurrentColor();
									if (OnColorChanged.IsBound()) OnColorChanged.Execute(CurrentColor);
								})
							]
						]
						+ SVerticalBox::Slot()
						.Padding(0, 2, 0, 0)
						.VAlign(VAlign_Center)
						[
							SNew(SBox).HeightOverride(18)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[
									SNew(SShToggleButton).Text_Lambda([this] { return bPreviewSrgb ? FText::FromString("SRGB") : FText::FromString("RGB"); })
									.ToggleColorAndOpacity_Lambda([this] {
										return CurrentColor;
									})
									.IsChecked_Lambda([this] { return bPreviewSrgb ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
									.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) {
										bPreviewSrgb = InState == ECheckBoxState::Checked;
									})
								]
								+ SHorizontalBox::Slot()
								[
									SNew(SColorBlock)
										.AlphaDisplayMode(EColorBlockAlphaDisplayMode::Separate)
										.ShowBackgroundForAlpha(true)
										.Color_Lambda([this] { return CurrentColor; })
										.UseSRGB_Lambda([this] { return bPreviewSrgb; })
								]
							]
						]
					]
				]
			]
		];
	}

	void SColorPicker::SyncHSVColorFromCurrentColor()
	{
		FLinearColor HSV = CurrentColor.LinearRGBToHSV();
		if (!FMath::IsNearlyZero(HSV.G))
		{
			CurrentHSVColor.R = HSV.R;
		}
		CurrentHSVColor.G = HSV.G;
		CurrentHSVColor.B = HSV.B;
		CurrentHSVColor.A = CurrentColor.A;
	}

	void SColorPicker::ApplyHSVColor()
	{
		CurrentColor = CurrentHSVColor.HSVToLinearRGB();
		CurrentColor.A = CurrentHSVColor.A;
		if (OnColorChanged.IsBound()) OnColorChanged.Execute(CurrentColor);
	}

	void SColorPicker::HandleWheelColorChanged(FLinearColor NewColor)
	{
		CurrentHSVColor.R = NewColor.R;
		CurrentHSVColor.G = FMath::Clamp(NewColor.G, 0.f, 1.f);
		if (FMath::IsNearlyZero(CurrentHSVColor.B))
		{
			CurrentHSVColor.B = 1.f;
		}
		ApplyHSVColor();
	}

	void SColorPicker::HandleAlphaChanged(float NewAlpha)
	{
		CurrentColor.A = FMath::Clamp(NewAlpha, 0.f, 1.f);
		CurrentHSVColor.A = CurrentColor.A;
		if (OnColorChanged.IsBound()) OnColorChanged.Execute(CurrentColor);
	}

	void SColorPicker::HandleRGBChanged(float NewValue, int32 Component)
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 1.f);
		if (Component == 0) CurrentColor.R = NewValue;
		else if (Component == 1) CurrentColor.G = NewValue;
		else if (Component == 2) CurrentColor.B = NewValue;
		SyncHSVColorFromCurrentColor();
		if (OnColorChanged.IsBound()) OnColorChanged.Execute(CurrentColor);
	}

	void SColorPicker::HandleHSVChanged(float NewValue, int32 Component)
	{
		if (Component == 0)
		{
			CurrentHSVColor.R = FMath::Clamp(NewValue, 0.f, 1.f) * 360.f;
		}
		else if (Component == 1)
		{
			CurrentHSVColor.G = FMath::Clamp(NewValue, 0.f, 1.f);
		}
		else if (Component == 2)
		{
			CurrentHSVColor.B = FMath::Clamp(NewValue, 0.f, 1.f);
		}
		ApplyHSVColor();
	}
}
