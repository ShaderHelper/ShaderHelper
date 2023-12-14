#pragma once
#include "AssetViewItem/AssetViewItem.h"
#include <Slate/Widgets/Views/STileView.h>

namespace FRAMEWORK
{

	class FRAMEWORK_API SAssetView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SAssetView) {}
			SLATE_ARGUMENT(TFunction<void(const FString&)>, OnFolderDoubleClick)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		void SetNewViewDirectory(const FString& NewViewDirectory);
		void PopulateAssetView(const FString& ViewDirectory);
		TSharedPtr<SWidget> CreateContextMenu();
		void AddFolder(const FString& InFolderName);
		void RemoveFolder(const FString& InFolderName);
		void AddFile(const FString& InFileName);
		void RemoveFile(const FString& InFileName);
		void ImportAsset();
	private:
		void OnMouseButtonDoubleClick(TSharedRef<AssetViewItem> ViewItem);

	private:
		TSharedPtr<STileView<TSharedRef<AssetViewItem>>> AssetTileView;
		TArray<TSharedRef<AssetViewItem>> AssetViewItems;
		FString CacheImportAssetPath;
		FString CurViewDirectory;
		TFunction<void(const FString&)> OnFolderDoubleClick;
	};

}
