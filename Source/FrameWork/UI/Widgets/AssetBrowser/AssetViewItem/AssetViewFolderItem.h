#pragma once
#include "AssetViewItem.h"

namespace FRAMEWORK
{
	class AssetViewFolderItem : public AssetViewItem
	{
	public:
		using AssetViewItem::AssetViewItem;

		MANUAL_RTTI_TYPE(AssetViewFolderItem, AssetViewItem)

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;
		void EnterRenameState();

	private:
		TSharedPtr<class SInlineEditableTextBlock> FolderEditableTextBlock;
	};
}