#pragma once
#include "AssetViewItem.h"

namespace FRAMEWORK
{
	class AssetViewImageItem : public AssetViewItem
	{
	public:
		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;
	};
}