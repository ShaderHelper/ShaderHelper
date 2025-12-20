#include "CommonHeader.h"
#include "AssetViewFolderItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "UI/Widgets/Misc/CommonTableRow.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "ProjectManager/ProjectManager.h"
#include "App/App.h"
#include "UI/Widgets/AssetBrowser/SAssetBrowser.h"

#include <Styling/StyleColors.h>

namespace FW
{
	AssetViewFolderItem::AssetViewFolderItem(STileView<TSharedRef<AssetViewItem>>* InOwner, const FString& InPath)
		: AssetViewItem(InOwner, InPath)
	{
		SAssignNew(FolderEditableTextBlock, SInlineEditableTextBlock)
			.Font(FAppStyle::Get().GetFontStyle("SmallFont"))
			.Text(FText::FromString(FPaths::GetBaseFilename(Path)))
			.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
				FString NewFolderPath = FPaths::GetPath(Path) / NewText.ToString();
				if (NewFolderPath != Path)
				{
					if (IFileManager::Get().DirectoryExists(*NewFolderPath))
					{
						MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, GApp->GetEditor()->GetMainWindow(), LOCALIZATION("AssetRenameFailure"));
					}
					else
					{
						RenamedOrMovedFolderMap.Add(Path, NewFolderPath);
						IFileManager::Get().Move(*NewFolderPath, *Path);
					}
				}
			})
			.OnExitEditingMode_Lambda([this] { 
				if (OnExitRenameState)
				{
					OnExitRenameState();
				}
			})
			.OverflowPolicy(ETextOverflowPolicy::Ellipsis);
	}

    FReply AssetViewFolderItem::HandleOnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
    {
		TArray<FString> SelectedPaths;
		for (const auto& Item : Owner->GetSelectedItems())
		{
			SelectedPaths.Add(Item->GetPath());
		}
        return FReply::Handled().BeginDragDrop(AssetViewItemDragDropOp::New(SelectedPaths));
    }

    FReply AssetViewFolderItem::HandleOnDrop(const FDragDropEvent& DragDropEvent)
    {
        TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
        if(DragDropOp->IsOfType<AssetViewItemDragDropOp>())
        {
			TArray<FString> DropFilePaths = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Paths;
			for (const auto& DropFilePath : DropFilePaths)
			{
				FString NewFilePath = Path / FPaths::GetCleanFilename(DropFilePath);
				if (FPaths::GetExtension(DropFilePath).IsEmpty())
				{
					RenamedOrMovedFolderMap.Add(DropFilePath, NewFilePath);
				}

				IFileManager::Get().Move(*NewFilePath, *DropFilePath);
			}

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
                    TArray<FString> DropFilePaths = StaticCastSharedPtr<AssetViewItemDragDropOp>(Operation)->Paths;
					for (const auto& DropFilePath : DropFilePaths)
					{
						if (DropFilePath == Path)
						{
							return false;
						}
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
						FolderEditableTextBlock.ToSharedRef()
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
