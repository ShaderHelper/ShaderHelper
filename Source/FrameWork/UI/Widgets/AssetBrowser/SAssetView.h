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

		virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
		virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
		virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	private:
		void OnMouseButtonDoubleClick(TSharedRef<AssetViewItem> ViewItem);
		void OnHandleDeleteAction();
		void OnHandleRenameAction();
		void OnHandleSaveAction();
		void OnHandleOpenAction(TSharedRef<AssetViewItem> ViewItem);

	private:
		SAssetBrowser* Browser;
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
