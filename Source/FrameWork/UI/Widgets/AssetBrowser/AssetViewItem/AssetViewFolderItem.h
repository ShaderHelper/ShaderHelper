#pragma once
#include "AssetViewItem.h"

namespace FRAMEWORK
{
	class AssetViewFolderItem : public AssetViewItem
	{
	public:
		MANUAL_RTTI_TYPE(AssetViewFolderItem, AssetViewItem)

		AssetViewFolderItem(const FString& InFolderPath);
		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;
		FString GetFolderPath() const { return FolderPath; }
		
	private:
		FString FolderPath;
	};
}