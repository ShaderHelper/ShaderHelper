#include "CommonHeader.h"
#include "AssetViewFolderItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Widgets/Text/SInlineEditableTextBlock.h>
#include <Styling/StyleColors.h>

namespace FRAMEWORK
{

	AssetViewFolderItem::AssetViewFolderItem(const FString& InFolderPath)
		: FolderPath(InFolderPath)
	{

	}

	TSharedRef<ITableRow> AssetViewFolderItem::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = SNew(STableRow<TSharedRef<AssetViewItem>>, OwnerTable);

		Row->SetContent(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
				.Padding(FMargin(5.0f, 0.0f, 5.0f, 0.0f))
				[
					SNew(SImage)
					.Image(FAppCommonStyle::Get().GetBrush("AssetBrowser.Folder"))
					.ColorAndOpacity_Lambda([Row] {
						if (Row->IsSelected())
						{
							return FSlateColor{FLinearColor{0.7f, 0.7f, 0.9f}};
						}
						else
						{
							return FStyleColors::AccentWhite;
						}
					})
				]
			]
				
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SInlineEditableTextBlock)
				.IsSelected_Lambda([] {return true; })
				.Font(FAppStyle::Get().GetFontStyle("SmallFont"))
				.Text(FText::FromString(FPaths::GetBaseFilename(FolderPath)))
				.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
				})
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
			]
		);
		return Row;
	}

}