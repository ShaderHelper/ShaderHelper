#pragma once
#include "SPropertyItem.h"
#include "SPropertyCategory.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Widgets/Input/SSpinBox.h>

namespace FRAMEWORK
{
	class SNeverSelectedTableRow : public STableRow<TSharedRef<PropertyData>>
	{
	public:
		SLATE_BEGIN_ARGS(SNeverSelectedTableRow) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
		{
			STableRow<TSharedRef<PropertyData>>::Construct(
				STableRow<TSharedRef<PropertyData>>::FArguments(),
				OwnerTableView);
		}

	protected:
		virtual ESelectionMode::Type GetSelectionMode() const override
		{
			return ESelectionMode::None;
		}
	};

	inline TSharedRef<ITableRow> PropertyCategory::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = SNew(SNeverSelectedTableRow, OwnerTable);

		TSharedRef<SPropertyCatergory> RowContent = SNew(SPropertyCatergory, Row)
			.DisplayName(DisplayName)
			.IsRootCategory(IsRootCategory())
			.AddMenuWidget(AddMenuWidget);
		Row->SetRowContent(RowContent);
		return Row;
	}

	template<> 
	inline void PropertyItem<float>::AddToUniformBuffer(UniformBufferBuilder& Builder) const
	{
		Builder.AddFloat(DisplayName);
	}
	template<>
	inline void PropertyItem<float>::UpdateUniformBuffer(UniformBuffer* Buffer) const
	{
		Buffer->GetMember<float>(DisplayName) = ValueAttribute.Get();
	}

	template<>
	inline void PropertyItem<Vector2f>::AddToUniformBuffer(UniformBufferBuilder& Builder) const
	{
		Builder.AddVector2f(DisplayName);
	}
	template<>
	inline void PropertyItem<Vector2f>::UpdateUniformBuffer(UniformBuffer* Buffer) const
	{
		Buffer->GetMember<Vector2f>(DisplayName) = ValueAttribute.Get();
	}

	template<>
	inline TSharedRef<ITableRow> PropertyItem<float>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = IsEnabled ? 
			SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable) : SNew(SNeverSelectedTableRow, OwnerTable);

		auto ValueWidget = SNew(SSpinBox<float>)
			.MaxFractionalDigits(3)
			.OnValueChanged_Lambda([this](float NewValue) { 
				ValueAttribute = NewValue; 
				if (OnValueChanged) { OnValueChanged(ValueAttribute.Get()); }
			})
		/*	.OnValueCommitted_Lambda([this](float NewValue, ETextCommit::Type) {
				ValueAttribute = NewValue;
				if (OnValueChanged) { OnValueChanged(ValueAttribute.Get()); }
			})*/
			.Value_Lambda([this] { return ValueAttribute.Get(); });

		Row->SetRowContent(
			SNew(SPropertyItem)
			.DisplayName(DisplayName)
			.OnDisplayNameChanged([this](const FString& NewDisplayName) {
				DisplayName = NewDisplayName;
				this->OnDisplayNameChanged(NewDisplayName);
			})
			.ValueWidget(MoveTemp(ValueWidget))
			.IsEnabled(IsEnabled)
		);
		return Row;
	}

	template<>
	inline TSharedRef<ITableRow> PropertyItem<Vector2f>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = IsEnabled ?
			SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable) : SNew(SNeverSelectedTableRow, OwnerTable);

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
			.OnDisplayNameChanged([this](const FString& NewDisplayName) {
				DisplayName = NewDisplayName;
				this->OnDisplayNameChanged(NewDisplayName);
			})
			.ValueWidget(MoveTemp(ValueWidget))
			.IsEnabled(IsEnabled)
		);
		return Row;
	}
}


