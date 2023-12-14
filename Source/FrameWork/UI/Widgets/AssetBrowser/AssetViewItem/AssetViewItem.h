#pragma once

namespace FRAMEWORK
{
	class AssetViewItem
	{
	public:
		MANUAL_RTTI_BASE_TYPE()

		virtual TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) = 0;
		virtual ~AssetViewItem() = default;
	};
}