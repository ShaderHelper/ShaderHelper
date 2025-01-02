#pragma once
#include "AssetViewItem.h"
#include "Editor/PreviewViewPort.h"

namespace FW
{

	class AssetViewAssetItem : public AssetViewItem
	{
	public:
		AssetViewAssetItem(const FString& InPath);

		MANUAL_RTTI_TYPE(AssetViewAssetItem, AssetViewItem)

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;
        void EnterRenameState();
        
        FReply HandleOnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
        
	private:
		TSharedPtr<class SInlineEditableTextBlock> AssetEditableTextBlock;
		TSharedPtr<PreviewViewPort> ThumbnailViewport;

		GpuTexture* AssetThumbnail = nullptr;
		const FSlateBrush* ImageBrush = nullptr;

		TSharedPtr<SBorder> PreviewBox;
	};

}
