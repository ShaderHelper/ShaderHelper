#pragma once
#include "AssetViewItem.h"
#include "Editor/PreviewViewPort.h"

namespace FW
{

	class AssetViewAssetItem : public AssetViewItem
	{
        MANUAL_RTTI_TYPE(AssetViewAssetItem, AssetViewItem)
	public:
		AssetViewAssetItem(const FString& InPath);
		AssetViewAssetItem(const FString& InPath, AssetObject* InAsset);
        
		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;
        void EnterRenameState();
        
        FReply HandleOnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
        
	private:
		TSharedPtr<class SInlineEditableTextBlock> AssetEditableTextBlock;
		TSharedPtr<PreviewViewPort> ThumbnailViewport = MakeShared<PreviewViewPort>();

		GpuTexture* AssetThumbnail = nullptr;
		const FSlateBrush* ImageBrush = nullptr;

		TSharedPtr<SBorder> PreviewBox;
	};

}
