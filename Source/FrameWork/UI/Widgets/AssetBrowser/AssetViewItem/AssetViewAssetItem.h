#pragma once
#include "AssetViewItem.h"
#include "Editor/PreviewViewPort.h"

namespace FRAMEWORK
{

	class AssetViewAssetItem : public AssetViewItem
	{
	public:
		AssetViewAssetItem(const FString& InPath);

		MANUAL_RTTI_TYPE(AssetViewAssetItem, AssetViewItem)

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;

	private:
		TSharedPtr<class SInlineEditableTextBlock> FolderEditableTextBlock;
		TSharedPtr<PreviewViewPort> ThumbnailViewport;

		GpuTexture* AssetThumbnail = nullptr;
		const FSlateBrush* ImageBrush = nullptr;
	};

}
