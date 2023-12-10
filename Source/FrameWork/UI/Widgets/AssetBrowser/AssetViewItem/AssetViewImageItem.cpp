#include "CommonHeader.h"
#include "AssetViewImageItem.h"

namespace FRAMEWORK
{

	TSharedRef<ITableRow> AssetViewImageItem::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(STableRow<TSharedRef<AssetViewItem>>, OwnerTable);
	}

}