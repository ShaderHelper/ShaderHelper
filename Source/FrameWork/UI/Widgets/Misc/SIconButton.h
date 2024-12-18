#pragma once

namespace FRAMEWORK
{
	class SIconButton : public SButton
	{
	public:
		SLATE_BEGIN_ARGS(SIconButton) : _Icon(nullptr) {}
			SLATE_ARGUMENT(const FSlateBrush*, Icon)
			SLATE_ARGUMENT(TOptional<FVector2D>, IconSize)
			SLATE_ARGUMENT(FText, Label)
			SLATE_EVENT(FOnClicked, OnClicked)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox);
			float Space = InArgs._Icon ? 6.0f : 0.0f;

			if (InArgs._Icon)
			{
				HBox->AddSlot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
							.DesiredSizeOverride(InArgs._IconSize)
							.Image(InArgs._Icon)
					];
			}

			if (!InArgs._Label.IsEmpty())
			{
				HBox->AddSlot()
					.VAlign(VAlign_Center)
					.Padding(Space, 0.5f, 0.f, 0.f)
					.AutoWidth()
					[
						SNew(STextBlock)
							.TextStyle(&FAppStyle::Get().GetWidgetStyle< FTextBlockStyle >("ButtonText"))
							.Justification(ETextJustify::Center)
							.Text(InArgs._Label)
					];
			}

			SButton::Construct(
				SButton::FArguments()
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
				.OnClicked(InArgs._OnClicked)
				[
					HBox
				]
			);
		}
	};
}