#pragma once
#include "AssetViewItem.h"

namespace FW
{
	class AssetViewFolderItem : public AssetViewItem
	{
        MANUAL_RTTI_TYPE(AssetViewFolderItem, AssetViewItem)
	public:
		using AssetViewItem::AssetViewItem;
        
		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;
		void EnterRenameState();
        FReply HandleOnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
        FReply HandleOnDrop(const FDragDropEvent& DragDropEvent);

	private:
		TSharedPtr<class SInlineEditableTextBlock> FolderEditableTextBlock;
	};
}
