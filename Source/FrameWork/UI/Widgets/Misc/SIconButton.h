#pragma once

namespace FW
{
	class SIconButton : public SButton
	{
	private:
		FOnKeyDown OnKeyDownHandler;

	public:
		SLATE_BEGIN_ARGS(SIconButton) : _ButtonStyle(nullptr), _Icon(nullptr) {}
			SLATE_STYLE_ARGUMENT(FButtonStyle, ButtonStyle)
            SLATE_ATTRIBUTE(const FSlateBrush*, Icon)
			SLATE_ARGUMENT(TOptional<FVector2D>, IconSize)
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

		void Construct(const FArguments& InArgs)
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
			const FButtonStyle* DefaultStyle = &FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton");
			if (InArgs._ButtonStyle)
			{
				DefaultStyle = InArgs._ButtonStyle;
			}

			SButton::Construct(
				SButton::FArguments()
				.ButtonStyle(DefaultStyle)
				.OnClicked(InArgs._OnClicked)
				.IsFocusable(InArgs._IsFocusable)
				[
					HBox
				]
			);
		}
	};
}
