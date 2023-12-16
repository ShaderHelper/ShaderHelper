#include "CommonHeader.h"
#include "AssetViewFolderItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Widgets/Text/SInlineEditableTextBlock.h>
#include <Styling/StyleColors.h>

namespace FRAMEWORK
{

	TSharedRef<ITableRow> AssetViewFolderItem::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = SNew(STableRow<TSharedRef<AssetViewItem>>, OwnerTable)
			.Style(&FAppCommonStyle::Get().GetWidgetStyle<FTableRowStyle>("AssetView.Row"));

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
							return FSlateColor{FLinearColor{0.5f, 0.5f, 1.0f}};
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
				SAssignNew(FolderEditableTextBlock, SInlineEditableTextBlock)
				.Font(FAppStyle::Get().GetFontStyle("SmallFont"))
				.Text(FText::FromString(FPaths::GetBaseFilename(Path)))
				.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
					FString NewFolderPath =  FPaths::GetPath(Path) / NewText.ToString();
					IFileManager::Get().Move(*NewFolderPath, *Path);
				})
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
			]
		);

		return Row;
	}

	void AssetViewFolderItem::EnterRenameState()
	{
		FolderEditableTextBlock->EnterEditingMode();
	}

}