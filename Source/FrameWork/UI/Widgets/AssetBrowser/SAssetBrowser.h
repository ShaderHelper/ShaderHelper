#pragma once

struct FFileChangeData;

namespace FRAMEWORK
{
	class FRAMEWORK_API SAssetBrowser : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SAssetBrowser) {}
			SLATE_ARGUMENT(FString, DirectoryShowed)
		SLATE_END_ARGS()

		~SAssetBrowser();

		void Construct(const FArguments& InArgs);
		void OnFileChanged(const TArray<FFileChangeData>& InFileChanges);
		TSharedPtr<SWidget> CreateContextMenu();
		void ImportAsset();

	private:
		TSharedPtr<class SDirectoryTree> DirectoryTree;
		TSharedPtr<class SAssetView> AssetView;
		FDelegateHandle DirectoryWatcherHandle;
		FString DirectoryShowed;
		FString SelectedDirectory;
	};
}


