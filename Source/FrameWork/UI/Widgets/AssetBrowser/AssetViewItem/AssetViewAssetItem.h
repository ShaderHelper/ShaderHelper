#pragma once
#include "AssetViewItem.h"

namespace FRAMEWORK
{
	class AssetViewAssetItem : public AssetViewItem
	{
	public:
		MANUAL_RTTI_TYPE(AssetViewAssetItem, AssetViewItem)

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;
	};

}
