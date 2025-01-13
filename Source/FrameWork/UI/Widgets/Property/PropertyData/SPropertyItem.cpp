#include "CommonHeader.h"
#include "SPropertyItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Widgets/Text/SInlineEditableTextBlock.h>

namespace FW
{
    void SPropertyItem::AddWidget(TSharedPtr<SWidget> InWidget)
    {
        HBox->AddSlot().VAlign(VAlign_Center)
        [
            InWidget.ToSharedRef()
        ];
    }

	void SPropertyItem::Construct(const FArguments& InArgs)
	{
        DisplayName = InArgs._DisplayName;
        auto ItemTextBlock = SNew(SInlineEditableTextBlock)
            .IsSelected_Lambda([] {return true; })
            .IsReadOnly(!InArgs._OnDisplayNameChanged)
            .Text_Lambda([this] {
                return FText::FromString(*DisplayName);
            })
            .OnTextCommitted_Lambda([this, InArgs](const FText& NewText, ETextCommit::Type) {
                if (*DisplayName != NewText.ToString() && InArgs._OnDisplayNameChanged)
                {
                    *DisplayName = NewText.ToString();
                    InArgs._OnDisplayNameChanged(NewText.ToString());
                }
            })
            .Font(FAppStyle::Get().GetFontStyle("NormalFont"))
            .ColorAndOpacity(FSlateColor{ FLinearColor{0.5f,0.5f,0.5f} });
        
        float Indent = InArgs._Indent ? 22.0f : 4.0f;
		HBox = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(Indent, 0.0f, 8.0f, 0.0f)
			[
                ItemTextBlock
            ];

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
