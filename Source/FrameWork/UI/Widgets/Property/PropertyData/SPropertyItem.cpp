#include "CommonHeader.h"
#include "SPropertyItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Widgets/Text/SInlineEditableTextBlock.h>

namespace FW
{
    void SPropertyItem::SetValueWidget(TSharedPtr<SWidget> InWidget)
    {
        HBox->AddSlot().AttachWidget(InWidget.ToSharedRef());
    }

	void SPropertyItem::Construct(const FArguments& InArgs)
	{
        DisplayName = InArgs._DisplayName;
		OnDisplayNameChanged = InArgs._OnDisplayNameChanged;
        auto ItemTextBlock = SNew(SInlineEditableTextBlock)
            .IsReadOnly(!OnDisplayNameChanged)
            .Text_Lambda([this] {
                return FText::FromString(*DisplayName);
            })
            .OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
                *DisplayName = NewText.ToString();
                if (OnDisplayNameChanged)
                {
                    OnDisplayNameChanged(NewText.ToString());
                }
            })
            .Font(FAppStyle::Get().GetFontStyle("NormalFont"))
            .ColorAndOpacity(FSlateColor{ FLinearColor{0.5f,0.5f,0.5f} });
        
        float Indent = InArgs._Indent ? 21.0f : 4.0f;
		HBox = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(Indent, 0.0f, 8.0f, 0.0f)
			[
                ItemTextBlock
            ];

		HBox->SetEnabled(InArgs._IsEnabled);

		ChildSlot
		[
	
            SNew(SBorder)
            .Padding(FMargin{0,4,0,4})
            .BorderImage_Lambda([this] {
                return FAppCommonStyle::Get().GetBrush("PropertyView.ItemColor");
            })
            [
                HBox.ToSharedRef()
            ]

			
		];
	}

}
