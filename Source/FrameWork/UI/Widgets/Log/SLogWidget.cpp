#include "CommonHeader.h"
#include "SLogWidget.h"
#include "UI/Styles/FAppCommonStyle.h"

namespace FW
{
	FLogWidgetMarshaller::FLogWidgetMarshaller()
		:TextLayout(nullptr)
	{

	}

	FLogWidgetMarshaller::~FLogWidgetMarshaller()
	{

	}

	void FLogWidgetMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FTextLayout::FLineModel>&& OldLineModels)
	{
		TextLayout = &TargetTextLayout;
	}

	void FLogWidgetMarshaller::GetText(FString& TargetString, const FTextLayout& SourceTextLayout)
	{
		SourceTextLayout.GetAsText(TargetString);
	}

	void SLogWidget::Construct(const FArguments& InArgs)
	{
		Marshaller = InArgs._Marshaller ? InArgs._Marshaller : MakeShared<FLogWidgetMarshaller>();

		MessagesTextBox = SNew(SMultiLineEditableTextBox)
			.Style(FAppCommonStyle::Get(), "Log.TextBox")
			.TextStyle(FAppCommonStyle::Get(), "Log.Normal")
			.Marshaller(Marshaller)
			.IsReadOnly(true)
			.AlwaysShowScrollbars(true);
		
		TSharedPtr<SVerticalBox> SBox = SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1)
			[
				MessagesTextBox.ToSharedRef()
			];
		
		ChildSlot
		.Padding(3)
		[
			SBox.ToSharedRef()
		];

		SBox->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 4.0f, 0.0f, 4.0f))
			[
				InArgs._ExtraContentArea.Widget
			];
	}

}
