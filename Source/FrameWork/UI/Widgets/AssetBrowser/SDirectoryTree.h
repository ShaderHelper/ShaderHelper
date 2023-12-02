#pragma once

struct FFileChangeData;

namespace FRAMEWORK
{
	struct DirectoryData
	{
		FString DirectoryPath;
		TArray<TSharedRef<DirectoryData>> Children;
	};

	class FRAMEWORK_API SDirectoryTree : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDirectoryTree) {}
		SLATE_END_ARGS()

		~SDirectoryTree();

		void Construct(const FArguments& InArgs);
		TSharedRef<ITableRow> OnGenerateRow(TSharedRef<DirectoryData> InTreeNode, const TSharedRef<STableViewBase>& OwnerTable);
		void OnGetChildren(TSharedRef<DirectoryData> InTreeNode, TArray<TSharedRef<DirectoryData>>& OutChildren);
		void PopulateDirectoryData(TSharedRef<DirectoryData> InDirectoryData, const FString& DirectoryPath);
		void OnDirectoryChanged(const TArray<FFileChangeData>& InFileChanges);

		TSharedPtr<DirectoryData> FindTreeItemFromTree(const FString& TargetDirPath, TSharedRef<DirectoryData> StartTreeItem);
		void AddDirectory(const FString& DirectoryPath);
		void RemoveDirectory(const FString& DirectoryPath);

	private:
		FDelegateHandle DirectoryWatcherHandle;
		TArray<TSharedRef<DirectoryData>> DirectoryDatas;
		TSharedPtr<STreeView<TSharedRef<DirectoryData>>> DirectoryTree;
	};
}
