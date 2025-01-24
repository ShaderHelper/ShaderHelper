#pragma once
#include "SPropertyItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Widgets/Input/SSpinBox.h>
#include "UI/Widgets/Misc/SIconButton.h"
#include "AssetManager/AssetManager.h"

namespace FW
{
    //{ Name | Embed | Value } DeleteIcon
    class PropertyItemBase : public PropertyData
    {
        MANUAL_RTTI_TYPE(PropertyItemBase, PropertyData)
    public:
        using PropertyData::PropertyData;
        
        void SetEnabled(bool Enabled) { IsEnabled = Enabled; }
        void SetOnDisplayNameChanged(const TFunction<void(const FString&)> DisplayNameChanged) { OnDisplayNameChanged = DisplayNameChanged; }
        void SetEmbedWidget(TSharedPtr<SWidget> InWidget) { EmbedWidget = MoveTemp(InWidget); }
        void SetOnDelete(const TFunction<void()>& OnDeleteFunc) { OnDelete = OnDeleteFunc;}
        
        virtual TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
        {
            auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable);
            Row->SetEnabled(IsEnabled);
                
            SAssignNew(Item, SPropertyItem)
                    .DisplayName(&DisplayName)
                    .OnDisplayNameChanged(OnDisplayNameChanged)
                    .Indent(!!Parent);
            
            if(EmbedWidget)
            {
                Item->AddWidget(EmbedWidget.ToSharedRef());
            }
            
            Row->SetRowContent(
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot()
                [
                    Item.ToSharedRef()
                ]
                +SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SIconButton).Icon(FAppStyle::Get().GetBrush("Icons.Delete"))
                        .Visibility_Lambda([this]{ return OnDelete? EVisibility::Visible : EVisibility::Hidden; })
                        .OnClicked_Lambda([this]{
                            if(OnDelete) OnDelete();
                            return FReply::Handled();
                        })
                ]
            );
            return Row;
        }
        
    protected:
        bool IsEnabled = true;
        TFunction<void()> OnDelete;
        TSharedPtr<SWidget> EmbedWidget;
        TSharedPtr<SPropertyItem> Item;
        TFunction<void(const FString&)> OnDisplayNameChanged;
    };


    template<typename T>
    class PropertyItem : public PropertyItemBase
    {
        MANUAL_RTTI_TYPE(PropertyItem<T>, PropertyItemBase)
    public:
        PropertyItem(ShObject* InOwner, FString InName, T* InValueRef = nullptr)
            : PropertyItemBase(InOwner, MoveTemp(InName))
            , ValueRef(InValueRef)
        {}

        void SetOnValueChanged(const TFunction<void(T)>& ValueChanged) { OnValueChanged = ValueChanged; }
        TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;

    private:
        T* ValueRef;
        TFunction<void(T)> OnValueChanged;
    };

	template<>
	inline TSharedRef<ITableRow> PropertyItem<float>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
        auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
//        if(ValueRef)
//        {
//            auto ValueWidget = SNew(SSpinBox<float>)
//                .MaxFractionalDigits(3)
//                .OnValueChanged_Lambda([this](float NewValue) {
//                    *ValueRef = NewValue;
//                    if (OnValueChanged) { OnValueChanged(NewValue); }
//                })
//                .Value_Lambda([this] { return *ValueRef; });
//            Item->AddWidget(MoveTemp(ValueWidget));
//        }
        
		return Row;
	}

	template<>
	inline TSharedRef<ITableRow> PropertyItem<Vector2f>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
        auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
//        if(ValueRef)
//        {
//            auto ValueWidget = SNew(SHorizontalBox)
//                + SHorizontalBox::Slot()
//                [
//                    SNew(SSpinBox<float>)
//                    .MaxFractionalDigits(3)
//                    .OnValueChanged_Lambda([this](float NewValue) {
//                        ValueRef->x = NewValue;
//                        if (OnValueChanged) { OnValueChanged(*ValueRef); }
//                    })
//                    .Value_Lambda([this] { return ValueRef->x; })
//                ]
//                + SHorizontalBox::Slot()
//                [
//                    SNew(SSpinBox<float>)
//                    .MaxFractionalDigits(3)
//                    .OnValueChanged_Lambda([this](float NewValue) {
//                        ValueRef->y = NewValue;
//                        if (OnValueChanged) { OnValueChanged(*ValueRef); };
//                    })
//                    .Value_Lambda([this] { return ValueRef->y; })
//                ];
//            Item->AddWidget(MoveTemp(ValueWidget));
//        }

		return Row;
	}
}

#include "PropertyAssetItem.h"

