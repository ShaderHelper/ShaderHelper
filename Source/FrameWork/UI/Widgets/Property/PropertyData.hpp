#pragma once
#include "SPropertyItem.h"
#include "SPropertyCategory.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Widgets/Input/SSpinBox.h>

namespace FRAMEWORK
{
	inline TSharedRef<ITableRow> PropertyCategory::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable);

		TSharedRef<SPropertyCatergory> RowContent = SNew(SPropertyCatergory, Row)
			.DisplayName(DisplayName)
			.IsRootCategory(IsRootCategory())
			.AddMenuWidget(AddMenuWidget);
		Row->SetRowContent(RowContent);
		return Row;
	}

	template<> 
	inline void PropertyNumber<float>::AddToUniformBuffer(UniformBufferBuilder& Builder)
	{
		Builder.AddFloat(DisplayName);
	}
	template<>
	inline void PropertyNumber<Vector2f>::AddToUniformBuffer(UniformBufferBuilder& Builder)
	{
		Builder.AddVector2f(DisplayName);
	}

	template<>
	inline TSharedRef<ITableRow> PropertyNumber<float>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable);

		auto ValueWidget = SNew(SSpinBox<float>)
			.MaxFractionalDigits(3)
			.OnValueChanged_Lambda([this](float NewValue) { 
				ValueAttribute = NewValue; 
				if (OnValueChanged) { OnValueChanged(ValueAttribute.Get()); }
			})
			.Value_Lambda([this] { return ValueAttribute.Get(); });

		Row->SetRowContent(
			SNew(SPropertyItem)
			.DisplayName(DisplayName)
			.ValueWidget(MoveTemp(ValueWidget))
			.IsEnabled(IsEnabled)
		);
		return Row;
	}

	template<>
	inline TSharedRef<ITableRow> PropertyNumber<Vector2f>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable);

		auto ValueWidget = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SSpinBox<float>)
				.MaxFractionalDigits(3)
				.OnValueChanged_Lambda([this](float NewValue) { 
					ValueAttribute = Vector2f{NewValue, ValueAttribute.Get().y}; 
					if (OnValueChanged) { OnValueChanged(ValueAttribute.Get()); }
				})
				.Value_Lambda([this] { return ValueAttribute.Get().x; })
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SSpinBox<float>)
				.MaxFractionalDigits(2)
				.OnValueChanged_Lambda([this](float NewValue) { 
					ValueAttribute = Vector2f{ValueAttribute.Get().x, NewValue}; 
					if (OnValueChanged) { OnValueChanged(ValueAttribute.Get()); };
				})
				.Value_Lambda([this] { return ValueAttribute.Get().y; })
			];

		Row->SetRowContent(
			SNew(SPropertyItem)
			.DisplayName(DisplayName)
			.ValueWidget(MoveTemp(ValueWidget))
			.IsEnabled(IsEnabled)
		);
		return Row;
	}
}


