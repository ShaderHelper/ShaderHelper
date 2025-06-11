#include "CommonHeader.h"
#include "SMessageDialog.h"
#include "UI/Styles/FAppCommonStyle.h"

namespace FW::MessageDialog
{
	class FRAMEWORK_API SMessageDialog : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SMessageDialog) {}
			SLATE_ARGUMENT(SWindow*, ParentWindow)
			SLATE_ARGUMENT(FText, Message)
			SLATE_ARGUMENT(MessageType, MsgType)
			SLATE_ARGUMENT(MessageRet*, ReturnResult)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

	private:
		MessageType MsgType;
		SWindow* ParentWindow;
		MessageRet* ReturnResult;
	};

	void SMessageDialog::Construct(const FArguments& InArgs)
	{
		MsgType = InArgs._MsgType;
		ParentWindow = InArgs._ParentWindow;
		ReturnResult = InArgs._ReturnResult;

		TSharedPtr<SHorizontalBox> ButtonBox;

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
					.Image_Lambda([this] {
						if (MsgType == MessageType::Ok)
						{	
							return FAppCommonStyle::Get().GetBrush("MessageDialog.Boqi");
						}
						else
						{
							return FAppCommonStyle::Get().GetBrush("MessageDialog.Boqi2");
						}
					})
				]

				+ SHorizontalBox::Slot()
				.Padding(16.f, 0.f, 0.f, 0.f)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(InArgs._Message)
						.WrapTextAt(250.0f)
						.ColorAndOpacity(FLinearColor::White)
					]

					+ SVerticalBox::Slot()
					.VAlign(VAlign_Bottom)
					.HAlign(HAlign_Right)
					[
						SAssignNew(ButtonBox, SHorizontalBox)
					]
				]

			]

		];

		if (MsgType & MessageType::Ok)
		{
			ButtonBox->AddSlot()
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
				.Text(LOCALIZATION("Ok"))
				.OnClicked_Lambda([this] {
					*ReturnResult = MessageRet::Ok;
					ParentWindow->RequestDestroyWindow();
					return FReply::Handled();
				})
			];
		}
		
		if(MsgType & MessageType::No)
		{
			ButtonBox->AddSlot()
			.AutoWidth()
			.Padding(FMargin{10,0,0,0})
			[
				SNew(SButton)
				.Text(LOCALIZATION("No"))
				.OnClicked_Lambda([this] {
					*ReturnResult = MessageRet::No;
					ParentWindow->RequestDestroyWindow();
					return FReply::Handled();
				})
			];
		}
		
		if(MsgType & MessageType::Cancel)
		{
		
			ButtonBox->AddSlot()
			.AutoWidth()
			.Padding(FMargin{10,0,0,0})
			[
				SNew(SButton)
				.Text(LOCALIZATION("Cancel"))
				.OnClicked_Lambda([this] {
					*ReturnResult = MessageRet::Cancel;
					ParentWindow->RequestDestroyWindow();
					return FReply::Handled();
				})
			];
		}

	}

	MessageRet Open(MessageType MsgType, TSharedPtr<SWindow> Parent, const TAttribute<FText>& InMessage)
	{
		MessageRet Result;

		TSharedRef<SWindow> ModalWindow = SNew(SWindow)
			.bDragAnywhere(true)
			.CreateTitleBar(false)
			.SizingRule(ESizingRule::Autosized)
			.AutoCenter(EAutoCenter::PreferredWorkArea)
			.HasCloseButton(false)
			.SupportsMinimize(false).SupportsMaximize(false);

		TSharedRef<SMessageDialog> MessageDialog = SNew(SMessageDialog)
			.ReturnResult(&Result)
			.MsgType(MsgType)
			.ParentWindow(&*ModalWindow)
			.Message(InMessage.Get());

		ModalWindow->SetContent(MessageDialog);
		FSlateApplication::Get().AddModalWindow(ModalWindow, Parent);
		return Result;
	}

}
