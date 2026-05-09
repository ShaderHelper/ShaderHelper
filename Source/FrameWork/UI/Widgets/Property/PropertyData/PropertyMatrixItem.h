#pragma once
#include "PropertyItem.h"

namespace FW
{
	class PropertyMatrix4x4fItem : public PropertyItemBase
	{
		MANUAL_RTTI_TYPE(PropertyMatrix4x4fItem, PropertyItemBase)
	public:
		PropertyMatrix4x4fItem(ShObject* InOwner, const FString& InName, float* InValues = nullptr, bool InReadOnly = false)
			: PropertyMatrix4x4fItem(InOwner, FText::FromString(InName), InValues, InReadOnly)
		{}
		PropertyMatrix4x4fItem(ShObject* InOwner, FText InName, float* InValues = nullptr, bool InReadOnly = false)
			: PropertyItemBase(InOwner, MoveTemp(InName))
			, Values(InValues)
			, ReadOnly(InReadOnly)
		{}
		PropertyMatrix4x4fItem(ShObject* InOwner, const FString& InName, FMatrix44f* InValueRef, bool InReadOnly = false)
			: PropertyMatrix4x4fItem(InOwner, FText::FromString(InName), InValueRef, InReadOnly)
		{}
		PropertyMatrix4x4fItem(ShObject* InOwner, FText InName, FMatrix44f* InValueRef, bool InReadOnly = false)
			: PropertyMatrix4x4fItem(InOwner, MoveTemp(InName), reinterpret_cast<float*>(InValueRef), InReadOnly)
		{}

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
		{
			auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
			if (Values)
			{
				auto ValueWidget = SNew(SVerticalBox);
				for (int32 RowIndex = 0; RowIndex < 4; ++RowIndex)
				{
					auto RowWidget = SNew(SHorizontalBox);
					for (int32 ColumnIndex = 0; ColumnIndex < 4; ++ColumnIndex)
					{
						const int32 ValueIndex = RowIndex * 4 + ColumnIndex;
						RowWidget->AddSlot()
						[
							SNew(SSpinBox<float>)
							.IsEnabled(!ReadOnly)
							.OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
							.OnValueChanged_Lambda([this, ValueIndex](float NewValue) {
								if (Values[ValueIndex] != NewValue && Owner->CanChangeProperty(this))
								{
									BeginEdit();
									Values[ValueIndex] = NewValue;
									Owner->PostPropertyChanged(this);
								}
							})
							.Value_Lambda([this, ValueIndex] { return Values[ValueIndex]; })
						];
					}
					ValueWidget->AddSlot().AutoHeight()[RowWidget];
				}
				Item->AddWidget(MoveTemp(ValueWidget));
			}

			return Row;
		}

	private:
		float* Values;
		bool ReadOnly;
	};
}
