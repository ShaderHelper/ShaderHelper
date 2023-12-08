#pragma once

namespace FRAMEWORK
{
	class AssetViewItem
	{
	public:
		virtual TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) = 0;
		virtual ~AssetViewItem() = default;
	};

}
