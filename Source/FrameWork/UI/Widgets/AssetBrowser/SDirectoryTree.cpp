#include "CommonHeader.h"
#include "SDirectoryTree.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "UI/Widgets/Misc/CommonTableRow.h"
#include "AssetViewItem/AssetViewItem.h"
#include "ProjectManager/ProjectManager.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "App/App.h"
#include "UI/Widgets/AssetBrowser/SAssetBrowser.h"

#include <DesktopPlatformModule.h>
#include <Styling/StyleColors.h>

namespace FW
{
	void SDirectoryTree::Construct(const FArguments& InArgs)
	{
		ContentPathShowed = InArgs._ContentPathShowed;
		BuiltInDir = InArgs._BuiltInDir;
        State = InArgs._State;
		OnSelectedDirectoryChanged = InArgs._OnSelectedDirectoryChanged;

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
                    State->CurSelectedDirectory = SelectedData->DirectoryPath;
					OnSelectedDirectoryChanged.ExecuteIfBound(SelectedData->DirectoryPath);
				}
				else
				{
					SetSelection(State->CurSelectedDirectory);
				}
			})
			.OnExpansionChanged_Lambda([this](TSharedRef<DirectoryData> InData, bool bExpanded){
				if (bExpanded) {
                    State->DirectoriesToExpand.AddUnique(InData->DirectoryPath);
				}
				else {
                    State->DirectoriesToExpand.Remove(InData->DirectoryPath);
				}
			})
		];

		if (State->CurSelectedDirectory.IsEmpty())
		{
            State->CurSelectedDirectory = ContentPathShowed;
		}

		if (!BuiltInDir.IsEmpty())
		{
			auto RootDirectoryData = MakeShared<DirectoryData>();
			RootDirectoryData->IsRootDirectory = true;
			DirectoryDatas.Add(RootDirectoryData);
			PopulateDirectoryData(RootDirectoryData, BuiltInDir);
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

		if (State->DirectoriesToExpand.Contains(DirectoryPath))
		{
			DirectoryTree->SetItemExpansion(InDirectoryData, true);
		}

		if (State->CurSelectedDirectory == DirectoryPath)
		{
			DirectoryTree->SetSelection(InDirectoryData);
		}
	}

    FReply SDirectoryTree::HandleOnDrop(const FDragDropEvent& DragDropEvent, FString DropTargetPath)
    {
        TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
        if(DragDropOp->IsOfType<AssetViewItemDragDropOp>())
        {
			TArray<FString> DropFilePaths = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Paths;
			for (const auto& DropFilePath : DropFilePaths)
			{
				FString NewFilePath = DropTargetPath / FPaths::GetCleanFilename(DropFilePath);
				if (FPaths::GetExtension(DropFilePath).IsEmpty())
				{
					RenamedOrMovedFolderMap.Add(DropFilePath, NewFilePath);
				}

				IFileManager::Get().Move(*NewFilePath, *DropFilePath);
			}

        }
        return FReply::Handled();
    }

	TSharedRef<ITableRow> SDirectoryTree::OnGenerateRow(TSharedRef<DirectoryData> InTreeNode, const TSharedRef<STableViewBase>& OwnerTable)
	{
		FSlateFontInfo DirectoryTextFont = FAppStyle::Get().GetFontStyle("SmallFont");

		if (InTreeNode->IsRootDirectory)
		{
			DirectoryTextFont = FAppStyle::Get().GetFontStyle("SmallFontBold");
		}
        
        auto Row = SNew(SDropTargetTableRow<TSharedRef<DirectoryData>>, OwnerTable)
            .Style(&FAppCommonStyle::Get().GetWidgetStyle<FTableRowStyle>("DirectoryTreeView.Row"))
            .OnDrop_Raw(this, &SDirectoryTree::HandleOnDrop, InTreeNode->DirectoryPath);
        
        Row->SetDragFilter([this, InTreeNode](const TSharedPtr<FDragDropOperation>& Operation){
			if (FPaths::IsUnderDirectory(InTreeNode->DirectoryPath, BuiltInDir))
			{
				return false;
			}

            if(Operation.IsValid())
            {
                if(Operation->IsOfType<AssetViewItemDragDropOp>())
                {
					TArray<FString> DropFilePaths = StaticCastSharedPtr<AssetViewItemDragDropOp>(Operation)->Paths;
					for (const auto& DropFilePath : DropFilePaths)
					{
						if (FPaths::GetExtension(DropFilePath).IsEmpty() && FPaths::IsUnderDirectory(InTreeNode->DirectoryPath, DropFilePath))
						{
							return false;
						}
						else if (FPaths::GetPath(DropFilePath) == InTreeNode->DirectoryPath)
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
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0, 4, 0)
            .VAlign(VAlign_Center)
            [
                SNew(SImage)
				.ColorAndOpacity(FStyleColors::Foreground)
                .DesiredSizeOverride(FVector2D(10.0f, 10.0f))
                .Image_Lambda([this, InTreeNode]() {
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
				.ColorAndOpacity_Lambda([this, InTreeNode] {
					return FPaths::IsUnderDirectory(InTreeNode->DirectoryPath,BuiltInDir) ? FStyleColors::Foreground.GetSpecifiedColor().CopyWithNewOpacity(0.5f) : FStyleColors::Foreground;
				})
                .Text(FText::FromString(FPaths::GetBaseFilename(InTreeNode->DirectoryPath)))
                .Font(MoveTemp(DirectoryTextFont))
            ]
        );

        return Row;
				
	}

	void SDirectoryTree::OnGetChildren(TSharedRef<DirectoryData> InTreeNode, TArray<TSharedRef<DirectoryData>>& OutChildren)
	{
		OutChildren = InTreeNode->Children;
	}

	namespace
	{
		TSharedPtr<DirectoryData> FindRecursive(const TSharedRef<DirectoryData>& TreeNode, const FString& TargetPath)
		{
			if (TargetPath == TreeNode->DirectoryPath)
			{
				return TreeNode;
			}
			
			for (const auto& Child : TreeNode->Children)
			{
				if (TSharedPtr<DirectoryData> Result = FindRecursive(Child, TargetPath))
				{
					return Result;
				}
			}
			
			return nullptr;
		}
	}

	TSharedPtr<DirectoryData> SDirectoryTree::FindTreeItemFromTree(const FString& TargetDirPath)
	{
		for (const auto& RootItem : DirectoryDatas)
		{
			if (TSharedPtr<DirectoryData> Result = FindRecursive(RootItem, TargetDirPath))
			{
				return Result;
			}
		}

		return nullptr;
	}

	void SDirectoryTree::AddDirectory(const FString& DirectoryPath)
	{
		FString ParentDirectoryPath = FPaths::GetPath(DirectoryPath);
		if (TSharedPtr<DirectoryData> ParentDirTreeItem = FindTreeItemFromTree(ParentDirectoryPath))
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
        for(auto It = State->DirectoriesToExpand.CreateIterator(); It; It++)
        {
            if(FPaths::IsUnderDirectory(*It, DirectoryPath))
            {
                It.RemoveCurrent();
            }
        }
        
		FString ParentDirectoryPath = FPaths::GetPath(DirectoryPath);
		if (TSharedPtr<DirectoryData> ParentDirTreeItem = FindTreeItemFromTree(ParentDirectoryPath))
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
		if (TSharedPtr<DirectoryData> Data = FindTreeItemFromTree(SelectedDirectory))
		{
			DirectoryTree->SetSelection(Data.ToSharedRef());
		}
	}

	void SDirectoryTree::SetExpansion(const FString& ExpandedDirectory)
	{
		if (TSharedPtr<DirectoryData> Data = FindTreeItemFromTree(ExpandedDirectory))
		{
			DirectoryTree->SetItemExpansion(Data.ToSharedRef(), true);
		}
	}

    void SDirectoryTree::SetExpansionRecursive(const FString& ExpandedDirectory)
    {
        if (TSharedPtr<DirectoryData> Data = FindTreeItemFromTree(ExpandedDirectory))
        {
            DirectoryTree->SetItemExpansion(Data.ToSharedRef(), true);
            SetExpansionRecursive(FPaths::GetPath(ExpandedDirectory));
        }
    }

	void SDirectoryTree::SortSubDirectory(const FString& ParentDirectory)
	{
		if (TSharedPtr<DirectoryData> ParentDirTreeItem = FindTreeItemFromTree(ParentDirectory))
		{
			ParentDirTreeItem->Children.Sort([](const TSharedRef<DirectoryData>& A, const TSharedRef<DirectoryData>& B) {
				return A->DirectoryPath < B->DirectoryPath;
			});
		}
	}

}
