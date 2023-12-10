#pragma once

namespace FRAMEWORK
{
	struct DirectoryData
	{
		bool IsRootDirectory = false;
		FString DirectoryPath;
		TArray<TSharedRef<DirectoryData>> Children;
	};

	class FRAMEWORK_API SDirectoryTree : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDirectoryTree) {}
			SLATE_ARGUMENT(FString, DirectoryShowed)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		TSharedRef<ITableRow> OnGenerateRow(TSharedRef<DirectoryData> InTreeNode, const TSharedRef<STableViewBase>& OwnerTable);
		void OnGetChildren(TSharedRef<DirectoryData> InTreeNode, TArray<TSharedRef<DirectoryData>>& OutChildren);
		void PopulateDirectoryData(TSharedRef<DirectoryData> InDirectoryData, const FString& DirectoryPath);

		TSharedPtr<DirectoryData> FindTreeItemFromTree(const FString& TargetDirPath, TSharedRef<DirectoryData> StartTreeItem);
		void AddDirectory(const FString& DirectoryPath);
		void RemoveDirectory(const FString& DirectoryPath);

	private:
		TArray<TSharedRef<DirectoryData>> DirectoryDatas;
		TSharedPtr<STreeView<TSharedRef<DirectoryData>>> DirectoryTree;
		FString DirectoryShowed;
	};
}
