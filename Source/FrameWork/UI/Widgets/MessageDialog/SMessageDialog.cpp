#include "CommonHeader.h"
#include "SMessageDialog.h"
#include "UI/Styles/FAppCommonStyle.h"

namespace FRAMEWORK::MessageDialog
{
	class FRAMEWORK_API SMessageDialog : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SMessageDialog) {}
			SLATE_ARGUMENT(SWindow*, ParentWindow)
			SLATE_ARGUMENT(FString, Message)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
	private:
		SWindow* ParentWindow;
	};

	void SMessageDialog::Construct(const FArguments& InArgs)
	{
		ParentWindow = InArgs._ParentWindow;
		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Title"))
			.Padding(16.f)
			[
		
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(SImage)
					.DesiredSizeOverride(FVector2D(64.f, 64.f))
					.Image(FAppCommonStyle::Get().GetBrush("MessageDialog.Boqi"))
				]

				+ SHorizontalBox::Slot()
				.Padding(16.f, 0.f, 0.f, 0.f)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(InArgs._Message))
						.ColorAndOpacity(FLinearColor::White)
					]

					+ SVerticalBox::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Bottom)
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
						.Text(FText::FromString("OK"))
						.OnClicked_Lambda([this] {
							ParentWindow->RequestDestroyWindow();
							return FReply::Handled();
						})
					]
				]

			]

		];
	}

	void Open(const FString& InMessage)
	{
		TSharedRef<SWindow> ModalWindow = SNew(SWindow)
			.CreateTitleBar(false)
			.SizingRule(ESizingRule::Autosized)
			.AutoCenter(EAutoCenter::PreferredWorkArea)
			.HasCloseButton(false)
			.SupportsMinimize(false).SupportsMaximize(false)
			.AdjustInitialSizeAndPositionForDPIScale(false);

		TSharedRef<SMessageDialog> MessageDialog = SNew(SMessageDialog)
			.ParentWindow(&*ModalWindow)
			.Message(InMessage);

		ModalWindow->SetContent(MessageDialog);
		FSlateApplication::Get().AddModalWindow(ModalWindow, nullptr);
	}

}