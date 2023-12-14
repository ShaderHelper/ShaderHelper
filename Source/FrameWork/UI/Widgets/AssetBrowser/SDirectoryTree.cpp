#include "CommonHeader.h"
#include "SDirectoryTree.h"
#include <DesktopPlatform/DesktopPlatformModule.h>
#include "UI/Styles/FAppCommonStyle.h"
#include <Styling/StyleColors.h>

namespace FRAMEWORK
{
	void SDirectoryTree::Construct(const FArguments& InArgs)
	{
		ContentPathShowed = InArgs._ContentPathShowed;
		OnSelectionChanged = InArgs._OnSelectionChanged;
		CurSelectedDirectory = InArgs._InitialDirectory;

		auto RootDirectoryData = MakeShared<DirectoryData>();
		RootDirectoryData->IsRootDirectory = true;
		DirectoryDatas.Add(RootDirectoryData);
		PopulateDirectoryData(RootDirectoryData, ContentPathShowed);

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
					OnSelectionChanged.ExecuteIfBound(SelectedData->DirectoryPath);
				}
				else
				{
					SetSelection(CurSelectedDirectory);
				}
			})
		];

		DirectoryTree->SetItemExpansion(RootDirectoryData, true);

		if (CurSelectedDirectory.IsEmpty())
		{
			DirectoryTree->SetSelection(RootDirectoryData);
		}
		else
		{
			SetSelection(CurSelectedDirectory);
		}
		
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
		FSlateColor DirectoryTextColor = FLinearColor{ 0.8f,0.8f,0.8f };
		FSlateFontInfo DirectoryTextFont = FAppStyle::Get().GetFontStyle("SmallFont");

		if (InTreeNode->IsRootDirectory)
		{
			DirectoryTextColor = FLinearColor::White;
			DirectoryTextFont = FAppStyle::Get().GetFontStyle("SmallFontBold");
		}

		return SNew(STableRow<TSharedRef<DirectoryData>>, OwnerTable)
			.Style(&FAppCommonStyle::Get().GetWidgetStyle<FTableRowStyle>("DirectoryTreeView.Row"))
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
					.Font(MoveTemp(DirectoryTextFont))
					.ColorAndOpacity(DirectoryTextColor)
				]
			];
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

}