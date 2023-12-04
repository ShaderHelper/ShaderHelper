#include "CommonHeader.h"
#include "SDirectoryTree.h"
#include <DesktopPlatform/DesktopPlatformModule.h>
#include "ProjectManager/ProjectManager.h"
#include <DirectoryWatcher/DirectoryWatcherModule.h>
#include <DirectoryWatcher/IDirectoryWatcher.h>

namespace FRAMEWORK
{
	SDirectoryTree::~SDirectoryTree()
	{
		if (FDirectoryWatcherModule* Module = FModuleManager::GetModulePtr<FDirectoryWatcherModule>(TEXT("DirectoryWatcher")))
		{
			if (IDirectoryWatcher* DirectoryWatcher = Module->Get())
			{
				DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(DirectoryShowed, DirectoryWatcherHandle);
			}
		}
	}

	void SDirectoryTree::Construct(const FArguments& InArgs)
	{
		DirectoryShowed = InArgs._DirectoryShowed;

		FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
		IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

		DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(DirectoryShowed,
			IDirectoryWatcher::FDirectoryChanged::CreateRaw(this, &SDirectoryTree::OnDirectoryChanged), 
			DirectoryWatcherHandle, IDirectoryWatcher::WatchOptions::IncludeDirectoryChanges);

		auto ContentDirectoryData = MakeShared<DirectoryData>();
		DirectoryDatas.Add(ContentDirectoryData);
		PopulateDirectoryData(ContentDirectoryData, DirectoryShowed);

		ChildSlot
		[
			SAssignNew(DirectoryTree, STreeView<TSharedRef<DirectoryData>>)
			.TreeItemsSource(&DirectoryDatas)
			.OnGenerateRow(this, &SDirectoryTree::OnGenerateRow)
			.OnGetChildren(this, &SDirectoryTree::OnGetChildren)
			.SelectionMode(ESelectionMode::Single)
		];

		DirectoryTree->SetItemExpansion(ContentDirectoryData, true);
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
			InDirectoryData->Children.Add(SubDirectoryData);
			PopulateDirectoryData(SubDirectoryData, DirectoryPath / SubDirectoryName);
		}
	}

	TSharedRef<ITableRow> SDirectoryTree::OnGenerateRow(TSharedRef<DirectoryData> InTreeNode, const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(STableRow<TSharedRef<DirectoryData>>, OwnerTable)
			.Style(&FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("SimpleTableView.Row"))
			[
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
					.Font(FAppStyle::Get().GetFontStyle("SmallFont"))
				]
			];
	}

	void SDirectoryTree::OnGetChildren(TSharedRef<DirectoryData> InTreeNode, TArray<TSharedRef<DirectoryData>>& OutChildren)
	{
		OutChildren = InTreeNode->Children;
	}

	void SDirectoryTree::OnDirectoryChanged(const TArray<FFileChangeData>& InFileChanges)
	{
		for (const FFileChangeData& FileChange : InFileChanges)
		{
			if (FPaths::GetExtension(FileChange.Filename).IsEmpty())
			{
				if (FileChange.Action == FFileChangeData::FCA_Added)
				{
					AddDirectory(FPaths::ConvertRelativePathToFull(FileChange.Filename));
				}
				else if (FileChange.Action == FFileChangeData::FCA_Removed)
				{
					RemoveDirectory(FPaths::ConvertRelativePathToFull(FileChange.Filename));
				}
			}
		}
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

			DirectoryTree->RequestTreeRefresh();
		}
	}

}