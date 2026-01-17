#pragma once

namespace FW
{
	struct DirectoryData
	{
		bool IsRootDirectory = false;
		FString DirectoryPath;
		TArray<TSharedRef<DirectoryData>> Children;
		DirectoryData* Parent = nullptr;
	};

    struct DirectoryTreePersistentState
    {
        FString CurSelectedDirectory;
        TArray<FString> DirectoriesToExpand;
    };

    DECLARE_DELEGATE_OneParam(SelectedDirectoryChangedDelegate, const FString&)

	class FRAMEWORK_API SDirectoryTree : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDirectoryTree) : _State(nullptr)
        {}
			SLATE_ARGUMENT(FString, ContentPathShowed)
			SLATE_ARGUMENT(FString, BuiltInDir)
            SLATE_ARGUMENT(DirectoryTreePersistentState*, State)
            SLATE_EVENT(SelectedDirectoryChangedDelegate, OnSelectedDirectoryChanged)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		TSharedRef<ITableRow> OnGenerateRow(TSharedRef<DirectoryData> InTreeNode, const TSharedRef<STableViewBase>& OwnerTable);
		void OnGetChildren(TSharedRef<DirectoryData> InTreeNode, TArray<TSharedRef<DirectoryData>>& OutChildren);
		void PopulateDirectoryData(TSharedRef<DirectoryData> InDirectoryData, const FString& DirectoryPath);

		TSharedPtr<DirectoryData> FindTreeItemFromTree(const FString& TargetDirPath);
		void AddDirectory(const FString& DirectoryPath);
		void RemoveDirectory(const FString& DirectoryPath);

		void SetSelection(const FString& SelectedDirectory);
		void SetExpansion(const FString& ExpandedDirectory);
        void SetExpansionRecursive(const FString& ExpandedDirectory);
		void SortSubDirectory(const FString& ParentDirectory);
        
        FReply HandleOnDrop(const FDragDropEvent& DragDropEvent, FString DropTargetPath);

	private:
		TArray<TSharedRef<DirectoryData>> DirectoryDatas;
		TSharedPtr<STreeView<TSharedRef<DirectoryData>>> DirectoryTree;
		FString ContentPathShowed, BuiltInDir;
        SelectedDirectoryChangedDelegate OnSelectedDirectoryChanged;
        DirectoryTreePersistentState* State;
	};
}
