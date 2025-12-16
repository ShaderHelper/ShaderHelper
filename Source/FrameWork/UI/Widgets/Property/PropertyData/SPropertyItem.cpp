#include "CommonHeader.h"
#include "SPropertyItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "UI/Widgets/Misc/MiscWidget.h"

namespace FW
{
    void SPropertyItem::AddWidget(TSharedPtr<SWidget> InWidget)
    {
        HBox->AddSlot()
            .VAlign(VAlign_Center)
        [
            InWidget.ToSharedRef()
        ];
    }

	void SPropertyItem::Construct(const FArguments& InArgs)
	{
        DisplayName = InArgs._DisplayName;
		auto ItemTextBlock = SNew(SShInlineEditableTextBlock)
			.IsReadOnly(!InArgs._OnDisplayNameChanged)
			.Text_Lambda([this] {
			return FText::FromString(*DisplayName);
				})
			.OnTextCommitted_Lambda([this, InArgs](const FText& NewText, ETextCommit::Type) {
			if (NewText.ToString().IsEmpty())
			{
				return;
			}
			if (*DisplayName != NewText.ToString() && InArgs._OnDisplayNameChanged)
			{
				if (InArgs._CanApplyName)
				{
					if (InArgs._CanApplyName(NewText.ToString()))
					{
						*DisplayName = NewText.ToString();
						InArgs._OnDisplayNameChanged(NewText.ToString());
					}
				}
				else
				{
					*DisplayName = NewText.ToString();
					InArgs._OnDisplayNameChanged(NewText.ToString());
				}
			}
				})
			.Font(FAppStyle::Get().GetFontStyle("NormalFont"));
        
        float Indent = InArgs._Indent ? 22.0f : 4.0f;
		HBox = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(Indent, 0.0f, 0.0f, 0.0f)
			.FillWidth(0.45f)
			[
                ItemTextBlock
            ];

		ChildSlot
		[
			HBox.ToSharedRef()
		];
	}

}
