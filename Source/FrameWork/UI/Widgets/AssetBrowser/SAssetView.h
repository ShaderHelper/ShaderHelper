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

	public:
        SLATE_BEGIN_ARGS(SAssetView) : _State(nullptr)
        {}
			SLATE_ARGUMENT(FString, ContentPathShowed)
			SLATE_ARGUMENT(TFunction<void(const FString&)>, OnFolderOpen)
            SLATE_ARGUMENT(AssetViewPersistentState*, State)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		void SetNewViewDirectory(const FString& NewViewDirectory);
		void PopulateAssetView(const FString& ViewDirectory);
		TSharedPtr<SWidget> CreateContextMenu();
		TSharedPtr<SWidget> CreateItemContextMenu(TSharedRef<AssetViewItem> ViewItem);
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

		FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	private:
		void OnMouseButtonDoubleClick(TSharedRef<AssetViewItem> ViewItem);
		void OnHandleDeleteAction();
		void OnHandleRenameAction();
		void OnHandleSaveAction();
		void OnHandleOpenAction(TSharedRef<AssetViewItem> ViewItem);

	private:
		TSharedPtr<STileView<TSharedRef<AssetViewItem>>> AssetTileView;
		TArray<TSharedRef<AssetViewItem>> AssetViewItems;
		FString CacheImportAssetPath;
		FString CurViewDirectory;
		TFunction<void(const FString&)> OnFolderOpen;
		TSharedPtr<FUICommandList> UICommandList;
		FString ContentPathShowed;

        AssetViewPersistentState* State;
	};

}
