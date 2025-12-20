#pragma once
#include "AssetViewItem.h"
#include "Editor/PreviewViewPort.h"

namespace FW
{

	class AssetViewAssetItem : public AssetViewItem
	{
        MANUAL_RTTI_TYPE(AssetViewAssetItem, AssetViewItem)
	public:
		AssetViewAssetItem(STileView<TSharedRef<AssetViewItem>>* InOwner, const FString& InPath);
        
		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;
        void EnterRenameState();
		void SetAssetThumbnail(GpuTexture* InAssetThumbnail) { 
			AssetThumbnail = InAssetThumbnail; 
			ThumbnailViewport->SetViewPortRenderTexture(AssetThumbnail);
		}
		void SetAssetImage(const FSlateBrush* InImage) { ImageBrush = InImage; };
        FReply HandleOnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
        
	private:
		TSharedPtr<class SInlineEditableTextBlock> AssetEditableTextBlock;
		TSharedPtr<PreviewViewPort> ThumbnailViewport = MakeShared<PreviewViewPort>();

		GpuTexture* AssetThumbnail = nullptr;
		const FSlateBrush* ImageBrush = nullptr;

		TSharedPtr<SBorder> PreviewBox;
	};

}
