#pragma once

namespace FRAMEWORK
{
	struct DirectoryData
	{
		bool IsRootDirectory = false;
		FString DirectoryPath;
		TArray<TSharedRef<DirectoryData>> Children;
		DirectoryData* Parent = nullptr;
	};

	DECLARE_DELEGATE_OneParam(SelectedDirectoryChangedDelegate, const FString&)
	DECLARE_DELEGATE_OneParam(ExpandedDirectoriesChangedDelegate, const TArray<FString>&)

	class FRAMEWORK_API SDirectoryTree : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDirectoryTree) {}
			SLATE_ARGUMENT(FString, ContentPathShowed)
			SLATE_ARGUMENT(FString, InitialSelectedDirectory)
			SLATE_ARGUMENT(TArray<FString>, InitialDirectoriesToExpand)
			SLATE_EVENT(SelectedDirectoryChangedDelegate, OnSelectedDirectoryChanged)
			SLATE_EVENT(ExpandedDirectoriesChangedDelegate, OnExpandedDirectoriesChanged)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		TSharedRef<ITableRow> OnGenerateRow(TSharedRef<DirectoryData> InTreeNode, const TSharedRef<STableViewBase>& OwnerTable);
		void OnGetChildren(TSharedRef<DirectoryData> InTreeNode, TArray<TSharedRef<DirectoryData>>& OutChildren);
		void PopulateDirectoryData(TSharedRef<DirectoryData> InDirectoryData, const FString& DirectoryPath);

		TSharedPtr<DirectoryData> FindTreeItemFromTree(const FString& TargetDirPath, TSharedRef<DirectoryData> StartTreeItem);
		void AddDirectory(const FString& DirectoryPath);
		void RemoveDirectory(const FString& DirectoryPath);

		void SetSelection(const FString& SelectedDirectory);
		void SetExpansion(const FString& ExpandedDirectory);
		void SortSubDirectory(const FString& ParentDirectory);
        
        FReply HandleOnDrop(const FDragDropEvent& DragDropEvent, FString DropTargetPath);

	private:
		TArray<TSharedRef<DirectoryData>> DirectoryDatas;
		TSharedPtr<STreeView<TSharedRef<DirectoryData>>> DirectoryTree;
		FString ContentPathShowed;
		SelectedDirectoryChangedDelegate OnSelectedDirectoryChanged;
		ExpandedDirectoriesChangedDelegate OnExpandedDirectoriesChanged;
		FString CurSelectedDirectory;
		TArray<FString> DirectoriesToExpand;
	};
}
