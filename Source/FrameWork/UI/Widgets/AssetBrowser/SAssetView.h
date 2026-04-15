#pragma once
#include "AssetViewItem/AssetViewItem.h"

#include <Widgets/Views/STileView.h>

namespace FW
{
	class SAssetBrowser;

    struct AssetViewPersistentState
    {
        float AssetViewSize = 60;
    };

	class FRAMEWORK_API SAssetView : public SCompoundWidget
	{
	public:
		static constexpr float MinAssetViewSize = 50;
		static constexpr float MaxAssetViewSize = 150;
		static constexpr int32 MaxThumbnailLoadRequests = 1;
		static constexpr float ThumbnailRefreshDelay = 0.3f;

		struct PendingThumbnailLoad
		{
			TWeakPtr<class AssetViewAssetItem> Item;
			FString Path;
		};

	public:
        SLATE_BEGIN_ARGS(SAssetView) : _State(nullptr)
        {}
			SLATE_ARGUMENT(FString, ContentPathShowed)
			SLATE_ARGUMENT(FString, BuiltInDir)
			SLATE_ARGUMENT(TFunction<void(const FString&)>, OnFolderOpen)
            SLATE_ARGUMENT(AssetViewPersistentState*, State)
		SLATE_END_ARGS()

		~SAssetView();
		void Construct(const FArguments& InArgs, SAssetBrowser* InBrowser);
		void SetNewViewDirectory(const FString& NewViewDirectory);
		FString GetViewDirectory() const { return CurViewDirectory; }
		void PopulateAssetView(const FString& ViewDirectory, const FText& InFilterText = FText::GetEmpty());
		TSharedPtr<SWidget> CreateContextMenu();
		TSharedPtr<SWidget> CreateItemContextMenu(const TArray<TSharedRef<AssetViewItem>>& ViewItems);
		void AddFolder(const FString& InFolderName);
		void RemoveFolder(const FString& InFolderName);
		void AddFile(const FString& InFileName);
		void RemoveFile(const FString& InFileName);
		void ImportAsset();
		void SortViewItems();

		void SetAssetViewSize(float InAssetViewSize) { 
            State->AssetViewSize = InAssetViewSize;
			AssetTileView->RequestListRefresh();
		}

		const FString& GetBuiltInDir() const { return BuiltInDir; }
		auto GetSelectedItems() const
		{
			return AssetTileView->GetSelectedItems();
		}
		void SetSelectedItem(const FString& InItem);
		void RefreshAssetThumbnail(const FString& InAssetPath);

		virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
		virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
		virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	private:
		void OnMouseButtonDoubleClick(TSharedRef<AssetViewItem> ViewItem);
		void OnHandleDeleteAction();
		void OnHandleRenameAction();
		void OnHandleSaveAction();
		void OnHandleOpenAction(TSharedRef<AssetViewItem> ViewItem);
		void SetAssetIcon(TSharedRef<class AssetViewAssetItem> ViewItem);
		void TickAssetIconLoading(float DeltaTime);
		void FlushDirtyThumbnails(float DeltaTime);
		void ResetAssetIconLoadingState();

	private:
		SAssetBrowser* Browser;
		TSharedPtr<STileView<TSharedRef<AssetViewItem>>> AssetTileView;
		TArray<TSharedRef<AssetViewItem>> AssetViewItems;
		FString CacheImportAssetPath;
		FString CurViewDirectory;
		TFunction<void(const FString&)> OnFolderOpen;
		TSharedPtr<FUICommandList> UICommandList;
		FString ContentPathShowed, BuiltInDir;

        AssetViewPersistentState* State;
		TArray<PendingThumbnailLoad> PendingThumbnailLoads;
		int32 NextPendingThumbnailLoad = 0;
		int32 InflightThumbnailLoads = 0;
		FDelegateHandle ThumbnailLoadTickerHandle;
		TSet<FString> DirtyThumbnailPaths;
		float ThumbnailRefreshAccumulator = 0.0f;
	};

}
