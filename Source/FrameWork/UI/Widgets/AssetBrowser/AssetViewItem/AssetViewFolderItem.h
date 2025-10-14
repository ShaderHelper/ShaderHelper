#pragma once
#include "AssetViewItem.h"
#include <Widgets/Text/SInlineEditableTextBlock.h>

namespace FW
{
	class AssetViewFolderItem : public AssetViewItem
	{
        MANUAL_RTTI_TYPE(AssetViewFolderItem, AssetViewItem)
	public:
		AssetViewFolderItem(const FString& InPath);
        
		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;
		void EnterRenameState();
        FReply HandleOnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
        FReply HandleOnDrop(const FDragDropEvent& DragDropEvent);

		TFunction<void()> OnExitRenameState;
	private:
		TSharedPtr<SInlineEditableTextBlock> FolderEditableTextBlock;
	};
}
