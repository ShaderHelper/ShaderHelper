#pragma once

namespace FRAMEWORK
{
	class AssetViewItem
	{
	public:
		MANUAL_RTTI_BASE_TYPE()

		AssetViewItem(const FString& InPath) : Path(InPath) {}
		virtual ~AssetViewItem() = default;
		virtual TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) = 0;
		FString GetPath() const { return Path; }

	protected:
		FString Path;
	};
}