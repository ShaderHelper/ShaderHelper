#pragma once
#include "AssetViewItem.h"

namespace FRAMEWORK
{
	class AssetViewThumbnailItem : public AssetViewItem
	{
	public:
		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;
	};

}
