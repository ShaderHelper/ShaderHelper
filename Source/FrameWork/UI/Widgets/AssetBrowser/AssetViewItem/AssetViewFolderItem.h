#pragma once
#include "AssetViewItem.h"

#include <Widgets/Text/SInlineEditableTextBlock.h>
#include <Widgets/Views/STileView.h>

namespace FW
{
	class AssetViewFolderItem : public AssetViewItem
	{
        MANUAL_RTTI_TYPE(AssetViewFolderItem, AssetViewItem)
	public:
		AssetViewFolderItem(STileView<TSharedRef<AssetViewItem>>* InOwner, const FString& InPath);
        
		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;
		void EnterRenameState();
        FReply HandleOnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
        FReply HandleOnDrop(const FDragDropEvent& DragDropEvent);

		TFunction<void()> OnExitRenameState;

	private:
		STileView<TSharedRef<AssetViewItem>>* Owner;
		TSharedPtr<SInlineEditableTextBlock> FolderEditableTextBlock;
	};
}
