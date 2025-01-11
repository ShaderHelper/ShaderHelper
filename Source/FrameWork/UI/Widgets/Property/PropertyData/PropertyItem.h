#pragma once
#include "SPropertyItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Widgets/Input/SSpinBox.h>

namespace FW
{
    //|Name | Value | Embed |
    template<typename T>
    class PropertyItem : public PropertyData
    {
        MANUAL_RTTI_TYPE(PropertyItem<T>, PropertyData)
    public:
        PropertyItem(FString InName, T* InValueRef = nullptr)
            : PropertyData(MoveTemp(InName))
            , ValueRef(InValueRef)
        {}

        void SetEnabled(bool Enabled) { IsEnabled = Enabled; }
        void SetOnValueChanged(const TFunction<void(T)>& ValueChanged) { OnValueChanged = ValueChanged; }
        void SetOnDisplayNameChanged(const TFunction<void(const FString&)> DisplayNameChanged) { OnDisplayNameChanged = DisplayNameChanged; }
        void SetEmbedWidget(TSharedPtr<SWidget> InWidget) { EmbedWidget = MoveTemp(InWidget); }
        TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;

    private:
        T* ValueRef;
        bool IsEnabled = true;
        TSharedPtr<SWidget> EmbedWidget;
        TFunction<void(T)> OnValueChanged;
        TFunction<void(const FString&)> OnDisplayNameChanged;
    };

	template<>
	inline TSharedRef<ITableRow> PropertyItem<float>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
        auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable);
            
        auto Item = SNew(SPropertyItem)
                    .DisplayName(&DisplayName)
                    .Indent(!!Parent)
                    .IsEnabled(IsEnabled);

		Row->SetRowContent(
			Item
		);
        
        if(ValueRef)
        {
            auto ValueWidget = SNew(SSpinBox<float>)
                .MaxFractionalDigits(3)
                .OnValueChanged_Lambda([this](float NewValue) {
                    *ValueRef = NewValue;
                    if (OnValueChanged) { OnValueChanged(NewValue); }
                })
                .Value_Lambda([this] { return *ValueRef; });
            Item->SetValueWidget(MoveTemp(ValueWidget));
        }
        
		return Row;
	}

	template<>
	inline TSharedRef<ITableRow> PropertyItem<Vector2f>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
        auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable);
        
        auto Item = SNew(SPropertyItem)
            .DisplayName(&DisplayName)
            .Indent(!!Parent)
            .IsEnabled(IsEnabled);
        
        Row->SetRowContent(
            Item
        );
        
        if(ValueRef)
        {
            auto ValueWidget = SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                [
                    SNew(SSpinBox<float>)
                    .MaxFractionalDigits(3)
                    .OnValueChanged_Lambda([this](float NewValue) {
                        ValueRef->x = NewValue;
                        if (OnValueChanged) { OnValueChanged(*ValueRef); }
                    })
                    .Value_Lambda([this] { return ValueRef->x; })
                ]
                + SHorizontalBox::Slot()
                [
                    SNew(SSpinBox<float>)
                    .MaxFractionalDigits(2)
                    .OnValueChanged_Lambda([this](float NewValue) {
                        ValueRef->y = NewValue;
                        if (OnValueChanged) { OnValueChanged(*ValueRef); };
                    })
                    .Value_Lambda([this] { return ValueRef->y; })
                ];
            Item->SetValueWidget(MoveTemp(ValueWidget));
        }

		return Row;
	}
}


