#pragma once
#include "SDirectoryTree.h"
#include "SAssetView.h"
struct FFileChangeData;

namespace FW
{
    struct AssetBrowserDirectory
    {
        FString Path;
        TArray<AssetBrowserDirectory> Directories;
        TArray<FString> Files;
        
        bool operator==(const AssetBrowserDirectory& Rhs) const
        {
            return Path == Rhs.Path;
        }
    };

    struct AssetBrowserPersistentState
    {
        DirectoryTreePersistentState DirectoryTreeState;
        AssetViewPersistentState AssetViewState;
    };

    extern TMap<FString, FString> RenamedOrMovedFolderMap;

	class FRAMEWORK_API SAssetBrowser : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SAssetBrowser) {}
			SLATE_ARGUMENT(FString, ContentPathShowed)
            SLATE_ARGUMENT(AssetBrowserPersistentState*, State)
		SLATE_END_ARGS()

		~SAssetBrowser();

		void Construct(const FArguments& InArgs);
		void OnFileChanged(const TArray<FFileChangeData>& InFileChanges);
		void OnDirectoryTreeSelectionChanged(const FString& SelectedDirectory);
        
        void SetCurrentDisplyPath(const FString& DisplayDir);
        
    private:
        void InitDirectory(AssetBrowserDirectory& OutDirectory, const FString& Dir);
        AssetBrowserDirectory* FindBrowserDirectory(AssetBrowserDirectory& CurDirectory, const FString& SearchDir);
        void AddFile(const FString& File);
        void RemoveFile(const FString& File);
        void AddBrowserDirectory(const FString& DirName);
        void RemoveBrowserDirectory(const FString& DirName);
        TArray<FString> FindFilesUnderBrowserDirectory(const FString& DirName);

	private:
		TSharedPtr<SDirectoryTree> DirectoryTree;
		TSharedPtr<SAssetView> AssetView;
		FDelegateHandle DirectoryWatcherHandle;
		FString ContentPathShowed;
        
        AssetBrowserDirectory ContentDirectory;
        AssetBrowserPersistentState* State;
	};
}


