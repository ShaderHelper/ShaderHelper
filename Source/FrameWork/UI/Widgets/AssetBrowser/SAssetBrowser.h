#pragma once
#include "SDirectoryTree.h"
struct FFileChangeData;

namespace FRAMEWORK
{
	class FRAMEWORK_API SAssetBrowser : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SAssetBrowser) {}
			SLATE_ARGUMENT(FString, ContentPathShowed)
			SLATE_ARGUMENT(FString, InitialSelectedDirectory)
			SLATE_ARGUMENT(TArray<FString>, InitialDirectoriesToExpand)
			SLATE_EVENT(SelectedDirectoryChangedDelegate, OnSelectedDirectoryChanged)
			SLATE_EVENT(ExpandedDirectoriesChangedDelegate, OnExpandedDirectoriesChanged)
		SLATE_END_ARGS()

		~SAssetBrowser();

		void Construct(const FArguments& InArgs);
		void OnFileChanged(const TArray<FFileChangeData>& InFileChanges);
		void OnDirectoryTreeSelectionChanged(const FString& SelectedDirectory);

	private:
		TSharedPtr<SDirectoryTree> DirectoryTree;
		TSharedPtr<class SAssetView> AssetView;
		FDelegateHandle DirectoryWatcherHandle;
		FString ContentPathShowed;
		SelectedDirectoryChangedDelegate OnSelectedDirectoryChanged;
		ExpandedDirectoriesChangedDelegate OnExpandedDirectoriesChanged;
	};
}


