#pragma once
#include "SPropertyItem.h"
#include "SPropertyCategory.h"
#include "UI/Styles/FAppCommonStyle.h"

namespace FRAMEWORK
{
	inline TSharedRef<ITableRow> PropertyCategory::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable)
			.Style(FAppCommonStyle::Get(), "PropertyView.TableRow")
			.ShowSelection(false);

		TSharedRef<SPropertyCatergory> RowContent = SNew(SPropertyCatergory, Row)
			.DisplayName(DisplayName)
			.AddMenuWidget(AddMenuWidget);
		Row->SetRowContent(RowContent);
		return Row;
	}

	template<>
	inline TSharedRef<ITableRow> PropertyNumber<float>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable)
			[
				SNew(SPropertyItem)
			];
	}

	template<>
	inline TSharedRef<ITableRow> PropertyNumber<Vector2f>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable)
			[
				SNew(SPropertyItem)
			];
	}
}


