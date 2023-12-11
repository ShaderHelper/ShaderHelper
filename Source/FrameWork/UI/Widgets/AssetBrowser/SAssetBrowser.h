#pragma once

struct FFileChangeData;

namespace FRAMEWORK
{
	class FRAMEWORK_API SAssetBrowser : public SCompoundWidget
	{
	public:
		DECLARE_DELEGATE_OneParam(DirectoryChangedDelegate, const FString&)

		SLATE_BEGIN_ARGS(SAssetBrowser) {}
			SLATE_ARGUMENT(FString, ContentPathShowed)
			SLATE_ARGUMENT(FString, InitialDirectory)
			SLATE_EVENT(DirectoryChangedDelegate, OnDirectoryChanged)
		SLATE_END_ARGS()

		~SAssetBrowser();

		void Construct(const FArguments& InArgs);
		void OnFileChanged(const TArray<FFileChangeData>& InFileChanges);
		void OnDirectoryTreeSelectionChanged(const FString& SelectedDirectory);

	private:
		TSharedPtr<class SDirectoryTree> DirectoryTree;
		TSharedPtr<class SAssetView> AssetView;
		FDelegateHandle DirectoryWatcherHandle;
		FString ContentPathShowed;
		FString SelectedDirectory;
		DirectoryChangedDelegate OnDirectoryChanged;
	};
}


