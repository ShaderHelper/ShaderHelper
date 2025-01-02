#include "CommonHeader.h"
#include "AssetViewFolderItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Widgets/Text/SInlineEditableTextBlock.h>
#include <Styling/StyleColors.h>
#include "UI/Widgets/Misc/CommonTableRow.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "ProjectManager/ProjectManager.h"
#include "App/App.h"

namespace FW
{

    FReply AssetViewFolderItem::HandleOnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
    {
        return FReply::Handled().BeginDragDrop(AssetViewItemDragDropOp::New(Path));
    }

    FReply AssetViewFolderItem::HandleOnDrop(const FDragDropEvent& DragDropEvent)
    {
        TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
        FString DropFilePath = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Path;

		bool CanDo = true;
		if (GProject->IsPendingAsset(DropFilePath))
		{
			CanDo = MessageDialog::Open(MessageDialog::OkCancel, GApp->GetEditor()->GetMainWindow(), LOCALIZATION("OpPendingAssetTip"));
		}

		if (CanDo)
		{
			FString NewFilePath = Path / FPaths::GetCleanFilename(DropFilePath);
			IFileManager::Get().Move(*NewFilePath, *DropFilePath);
		}
        return FReply::Handled();
    }

	TSharedRef<ITableRow> AssetViewFolderItem::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = SNew(SDropTargetTableRow<TSharedRef<AssetViewItem>>, OwnerTable)
            .Style(&FAppCommonStyle::Get().GetWidgetStyle<FTableRowStyle>("AssetView.Row"))
            .OnDragDetected_Raw(this, &AssetViewFolderItem::HandleOnDragDetected)
            .OnDrop_Raw(this, &AssetViewFolderItem::HandleOnDrop);
        
        Row->SetDragFilter([this](const TSharedPtr<FDragDropOperation>& Operation){
            if(Operation.IsValid())
            {
                if(Operation->IsOfType<AssetViewItemDragDropOp>())
                {
                    FString DropFilePath = StaticCastSharedPtr<AssetViewItemDragDropOp>(Operation)->Path;
                    if(DropFilePath == Path)
                    {
                        return false;
                    }
                    return true;
                }
            }
            return false;
        });

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
							return FSlateColor{FLinearColor{0.75f, 0.9f, 0.98f, 1.0f}};
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
				SNew(SBox)
					.Padding(FMargin(2.0f, 0.0f, 2.0f, 0.0f))
					[
                    SAssignNew(FolderEditableTextBlock, SInlineEditableTextBlock)
                    .Font(FAppStyle::Get().GetFontStyle("SmallFont"))
                    .Text(FText::FromString(FPaths::GetBaseFilename(Path)))
                    .OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
                        FString NewFolderPath =  FPaths::GetPath(Path) / NewText.ToString();
                        if(NewFolderPath != Path)
                        {
                            if(IFileManager::Get().DirectoryExists(*NewFolderPath))
                            {
                                MessageDialog::Open(MessageDialog::Ok, GApp->GetEditor()->GetMainWindow(), LOCALIZATION("AssetRenameFailure"));
                            }
                            else
                            {
                                IFileManager::Get().Move(*NewFolderPath, *Path);
                            }
                        }
                    })
                    .OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                ]
				
			]
		);

		return Row;
	}

	void AssetViewFolderItem::EnterRenameState()
	{
		FolderEditableTextBlock->EnterEditingMode();
	}

}
