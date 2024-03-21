#include "CommonHeader.h"
#include "SDirectoryTree.h"
#include <DesktopPlatformModule.h>
#include "UI/Styles/FAppCommonStyle.h"
#include <Styling/StyleColors.h>
#include "UI/Widgets/Misc/CommonTableRow.h"
#include "AssetViewItem/AssetViewItem.h"

namespace FRAMEWORK
{
	void SDirectoryTree::Construct(const FArguments& InArgs)
	{
		ContentPathShowed = InArgs._ContentPathShowed;
		OnSelectedDirectoryChanged = InArgs._OnSelectedDirectoryChanged;
		OnExpandedDirectoriesChanged = InArgs._OnExpandedDirectoriesChanged;
		CurSelectedDirectory = InArgs._InitialSelectedDirectory;
		DirectoriesToExpand = InArgs._InitialDirectoriesToExpand;

		ChildSlot
		[
			SAssignNew(DirectoryTree, STreeView<TSharedRef<DirectoryData>>)
			.TreeItemsSource(&DirectoryDatas)
			.OnGenerateRow(this, &SDirectoryTree::OnGenerateRow)
			.OnGetChildren(this, &SDirectoryTree::OnGetChildren)
			.SelectionMode(ESelectionMode::Single)
			.OnSelectionChanged_Lambda([this](TSharedPtr<DirectoryData> SelectedData, ESelectInfo::Type) {
				if (SelectedData) 
				{
					CurSelectedDirectory = SelectedData->DirectoryPath;
					OnSelectedDirectoryChanged.ExecuteIfBound(SelectedData->DirectoryPath);
				}
				else
				{
					SetSelection(CurSelectedDirectory);
				}
			})
			.OnExpansionChanged_Lambda([this](TSharedRef<DirectoryData> InData, bool bExpanded){
				if (bExpanded) {
					DirectoriesToExpand.Add(InData->DirectoryPath);
				}
				else {
					DirectoriesToExpand.Remove(InData->DirectoryPath);
				}
				OnExpandedDirectoriesChanged.ExecuteIfBound(DirectoriesToExpand);
			})
		];

		if (CurSelectedDirectory.IsEmpty())
		{
			CurSelectedDirectory = ContentPathShowed;
		}
		
		auto RootDirectoryData = MakeShared<DirectoryData>();
		RootDirectoryData->IsRootDirectory = true;
		DirectoryDatas.Add(RootDirectoryData);
		PopulateDirectoryData(RootDirectoryData, ContentPathShowed);
	}

	void SDirectoryTree::PopulateDirectoryData(TSharedRef<DirectoryData> InDirectoryData, const FString& DirectoryPath)
	{
		InDirectoryData->Children.Empty();
		InDirectoryData->DirectoryPath = DirectoryPath;

		TArray<FString> SubDirectoryNames;
		IFileManager::Get().FindFiles(SubDirectoryNames, *(DirectoryPath / TEXT("*")), false, true);

		for (const FString& SubDirectoryName : SubDirectoryNames)
		{
			TSharedRef<DirectoryData> SubDirectoryData = MakeShared<DirectoryData>();
			SubDirectoryData->Parent = &*InDirectoryData;
			InDirectoryData->Children.Add(SubDirectoryData);
			PopulateDirectoryData(SubDirectoryData, DirectoryPath / SubDirectoryName);
		}
        SortSubDirectory(DirectoryPath);

		if (DirectoriesToExpand.Contains(DirectoryPath))
		{
			DirectoryTree->SetItemExpansion(InDirectoryData, true);
		}

		if (CurSelectedDirectory == DirectoryPath)
		{
			DirectoryTree->SetSelection(InDirectoryData);
		}
	}

    FReply SDirectoryTree::HandleOnDrop(const FDragDropEvent& DragDropEvent, FString DropTargetPath)
    {
        TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
        FString DropFilePath = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Path;
        FString NewFilePath = DropTargetPath / FPaths::GetCleanFilename(DropFilePath);
        IFileManager::Get().Move(*NewFilePath, *DropFilePath);
        return FReply::Handled();
    }

	TSharedRef<ITableRow> SDirectoryTree::OnGenerateRow(TSharedRef<DirectoryData> InTreeNode, const TSharedRef<STableViewBase>& OwnerTable)
	{
		FSlateColor DirectoryTextColor = FLinearColor{ 0.8f,0.8f,0.8f };
		FSlateFontInfo DirectoryTextFont = FAppStyle::Get().GetFontStyle("SmallFont");

		if (InTreeNode->IsRootDirectory)
		{
			DirectoryTextColor = FLinearColor::White;
			DirectoryTextFont = FAppStyle::Get().GetFontStyle("SmallFontBold");
		}
        
        auto Row = SNew(SDropTargetTableRow<TSharedRef<DirectoryData>>, OwnerTable)
            .Style(&FAppCommonStyle::Get().GetWidgetStyle<FTableRowStyle>("DirectoryTreeView.Row"))
            .OnDrop_Raw(this, &SDirectoryTree::HandleOnDrop, InTreeNode->DirectoryPath);
        
        Row->SetDragFilter([=](const TSharedPtr<FDragDropOperation>& Operation){
            if(Operation.IsValid())
            {
                if(Operation->IsOfType<AssetViewItemDragDropOp>())
                {
                    FString DropFilePath = StaticCastSharedPtr<AssetViewItemDragDropOp>(Operation)->Path;
                    if(FPaths::GetExtension(DropFilePath).IsEmpty() && DropFilePath == InTreeNode->DirectoryPath)
                    {
                        return false;
                    }
                    else if(FPaths::GetPath(DropFilePath) == InTreeNode->DirectoryPath)
                    {
                        return false;
                    }
                    return true;
                }
            }
            return false;
        });
        
        Row->SetContent(
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0, 4, 0)
            .VAlign(VAlign_Center)
            [
                SNew(SImage)
                .DesiredSizeOverride(FVector2D(10.0f, 10.0f))
                .Image_Lambda([=]() {
                        if (DirectoryTree->IsItemExpanded(InTreeNode))
                        {
                            return FAppStyle::Get().GetBrush("Icons.FolderOpen");
                        }
                        else
                        {
                            return FAppStyle::Get().GetBrush("Icons.FolderClosed");
                        }
                    })
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(FPaths::GetBaseFilename(InTreeNode->DirectoryPath)))
                .Font(MoveTemp(DirectoryTextFont))
                .ColorAndOpacity(DirectoryTextColor)
            ]
        );

        return Row;
				
	}

	void SDirectoryTree::OnGetChildren(TSharedRef<DirectoryData> InTreeNode, TArray<TSharedRef<DirectoryData>>& OutChildren)
	{
		OutChildren = InTreeNode->Children;
	}

	TSharedPtr<DirectoryData> SDirectoryTree::FindTreeItemFromTree(const FString& TargetDirPath, TSharedRef<DirectoryData> StartTreeItem)
	{
		if (TargetDirPath == StartTreeItem->DirectoryPath)
		{
			return StartTreeItem;
		}
		else
		{
			for (const auto& Child : StartTreeItem->Children)
			{
				TSharedPtr<DirectoryData> Result =  FindTreeItemFromTree(TargetDirPath, Child);
				if (Result)
				{
					return Result;
				}
			}
		}

		return nullptr;
	}

	void SDirectoryTree::AddDirectory(const FString& DirectoryPath)
	{
		FString ParentDirectoryPath = FPaths::GetPath(DirectoryPath);
		if (TSharedPtr<DirectoryData> ParentDirTreeItem = FindTreeItemFromTree(ParentDirectoryPath, DirectoryDatas[0]))
		{
			TSharedRef<DirectoryData> NewDirTreeItem = MakeShared<DirectoryData>();
			PopulateDirectoryData(NewDirTreeItem, DirectoryPath);
			ParentDirTreeItem->Children.Add(NewDirTreeItem);
			SortSubDirectory(ParentDirectoryPath);
			DirectoryTree->RequestTreeRefresh();
		}
		else if (DirectoryPath == DirectoryDatas[0]->DirectoryPath)
		{
			PopulateDirectoryData(DirectoryDatas[0], DirectoryDatas[0]->DirectoryPath);
			DirectoryTree->RequestTreeRefresh();
		}
	}

	void SDirectoryTree::RemoveDirectory(const FString& DirectoryPath)
	{
		FString ParentDirectoryPath = FPaths::GetPath(DirectoryPath);
		if (TSharedPtr<DirectoryData> ParentDirTreeItem = FindTreeItemFromTree(ParentDirectoryPath, DirectoryDatas[0]))
		{
			for (auto It = ParentDirTreeItem->Children.CreateIterator(); It; It++)
			{
				if ((*It)->DirectoryPath == DirectoryPath)
				{
					It.RemoveCurrent();
					break;
				}
			}
            
            if(ParentDirTreeItem->Children.Num() == 0)
            {
                DirectoryTree->SetItemExpansion(ParentDirTreeItem.ToSharedRef(), false);
            }

			DirectoryTree->RequestTreeRefresh();
		}
		else if (DirectoryPath == DirectoryDatas[0]->DirectoryPath)
		{
			PopulateDirectoryData(DirectoryDatas[0], DirectoryDatas[0]->DirectoryPath);
			DirectoryTree->RequestTreeRefresh();
		}
	}

	void SDirectoryTree::SetSelection(const FString& SelectedDirectory)
	{
		if (TSharedPtr<DirectoryData> Data = FindTreeItemFromTree(SelectedDirectory, DirectoryDatas[0]))
		{
			DirectoryTree->SetSelection(Data.ToSharedRef());
		}
	}

	void SDirectoryTree::SetExpansion(const FString& ExpandedDirectory)
	{
		if (TSharedPtr<DirectoryData> Data = FindTreeItemFromTree(ExpandedDirectory, DirectoryDatas[0]))
		{
			DirectoryTree->SetItemExpansion(Data.ToSharedRef(), true);
		}
	}

	void SDirectoryTree::SortSubDirectory(const FString& ParentDirectory)
	{
		if (TSharedPtr<DirectoryData> ParentDirTreeItem = FindTreeItemFromTree(ParentDirectory, DirectoryDatas[0]))
		{
			ParentDirTreeItem->Children.Sort([](const TSharedRef<DirectoryData>& A, const TSharedRef<DirectoryData>& B) {
				return A->DirectoryPath < B->DirectoryPath;
			});
		}
	}

}
