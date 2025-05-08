#pragma once
#include "PropertyItem.h"
#include "RenderResource/UniformBuffer.h"

namespace FW
{
    template<typename T>
    class PropertyUniformItem : public PropertyItemBase
    {
    public:
        PropertyUniformItem(ShObject* InOwner, FString InName, UniformBufferMemberWrapper<T> InValueRef)
            : PropertyItemBase(InOwner, MoveTemp(InName))
            , ValueRef(InValueRef)
        {}

        TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;

    private:
        UniformBufferMemberWrapper<T> ValueRef;
    };

    template<>
    inline TSharedRef<ITableRow> PropertyUniformItem<float>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
    {
        auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
        auto ValueWidget = SNew(SSpinBox<float>)
            .MaxFractionalDigits(3)
            .OnValueChanged_Lambda([this](float NewValue) {
                if(ValueRef != NewValue)
                {
                    ValueRef = NewValue;
                    Owner->PostPropertyChanged(this);
                }
            })
            .Value_Lambda([this] { return ValueRef; });
        Item->AddWidget(MoveTemp(ValueWidget));
        return Row;
    }

    template<>
    inline TSharedRef<ITableRow> PropertyUniformItem<Vector2f>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
    {
        auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
        auto ValueWidget = SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            [
                SNew(SSpinBox<float>)
                .MaxFractionalDigits(3)
                .OnValueChanged_Lambda([this](float NewValue) {
                    Vector2f& Value = ValueRef;
                    if(Value.x != NewValue)
                    {
                        ValueRef = {NewValue, Value.y};
                        Owner->PostPropertyChanged(this);
                    }
                })
                .Value_Lambda([this] { return ((Vector2f)ValueRef).x; })
            ]
            + SHorizontalBox::Slot()
            [
                SNew(SSpinBox<float>)
                .MaxFractionalDigits(3)
                .OnValueChanged_Lambda([this](float NewValue) {
                    Vector2f& Value = ValueRef;
                    if(Value.y != NewValue)
                    {
                        ValueRef = {Value.x, NewValue};
                        Owner->PostPropertyChanged(this);
                    }
                })
                .Value_Lambda([this] { return ((Vector2f&)ValueRef).y; })
            ];
        Item->AddWidget(MoveTemp(ValueWidget));
        return Row;
    }

	template<>
	inline TSharedRef<ITableRow> PropertyUniformItem<Vector4f>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
		auto ValueWidget = SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<float>)
							.MaxFractionalDigits(3)
							.OnValueChanged_Lambda([this](float NewValue) {
							Vector4f& Value = ValueRef;
							if (Value.x != NewValue)
							{
								ValueRef = { NewValue, Value.y, Value.z, Value.w };
								Owner->PostPropertyChanged(this);
							}
								})
							.Value_Lambda([this] { return ((Vector4f)ValueRef).x; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<float>)
							.MaxFractionalDigits(3)
							.OnValueChanged_Lambda([this](float NewValue) {
							Vector4f& Value = ValueRef;
							if (Value.y != NewValue)
							{
								ValueRef = { Value.x, NewValue, Value.z, Value.w };
								Owner->PostPropertyChanged(this);
							}
								})
							.Value_Lambda([this] { return ((Vector4f&)ValueRef).y; })
					]
			]
			+SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				[
					SNew(SSpinBox<float>)
						.MaxFractionalDigits(3)
						.OnValueChanged_Lambda([this](float NewValue) {
						Vector4f& Value = ValueRef;
						if (Value.z != NewValue)
						{
							ValueRef = { Value.x, Value.y, NewValue, Value.w };
							Owner->PostPropertyChanged(this);
						}
							})
						.Value_Lambda([this] { return ((Vector4f&)ValueRef).z; })
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SSpinBox<float>)
						.MaxFractionalDigits(3)
						.OnValueChanged_Lambda([this](float NewValue) {
						Vector4f& Value = ValueRef;
						if (Value.w != NewValue)
						{
							ValueRef = { Value.x, Value.y, Value.z, NewValue };
							Owner->PostPropertyChanged(this);
						}
							})
						.Value_Lambda([this] { return ((Vector4f&)ValueRef).w; })
				]
			];
		Item->AddWidget(MoveTemp(ValueWidget));
		return Row;
	}
}
