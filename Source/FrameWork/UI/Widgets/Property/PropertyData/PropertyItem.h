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
        void SetCanChangeToName(const TFunction<bool(const FString&)>& CanChange) { CanChangeToName = CanChange; }
        void SetOnDisplayNameChanged(const TFunction<void(const FString&)>& DisplayNameChanged) { OnDisplayNameChanged = DisplayNameChanged; }
        void SetEmbedWidget(TSharedPtr<SWidget> InWidget) { EmbedWidget = MoveTemp(InWidget); }
        void SetOnDelete(const TFunction<void()>& OnDeleteFunc) { OnDelete = OnDeleteFunc;}
        
        virtual TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
        {
            auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable);
            Row->SetEnabled(IsEnabled);
                
            SAssignNew(Item, SPropertyItem)
                    .CanChangeToName(CanChangeToName)
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
        TFunction<bool(const FString&)> CanChangeToName;
        TFunction<void(const FString&)> OnDisplayNameChanged;
    };


    class PropertyFloatItem : public PropertyItemBase
    {
        MANUAL_RTTI_TYPE(PropertyFloatItem, PropertyItemBase)
    public:
        PropertyFloatItem(ShObject* InOwner, FString InName, float* InValueRef = nullptr)
            : PropertyItemBase(InOwner, MoveTemp(InName))
            , ValueRef(InValueRef)
        {}
        
        TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
        {
            auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
            if(ValueRef)
            {
                auto ValueWidget = SNew(SSpinBox<float>)
                    .MaxFractionalDigits(3)
                    .OnValueChanged_Lambda([this](float NewValue) {
                        if(*ValueRef != NewValue)
                        {
                            *ValueRef = NewValue;
                            Owner->PostPropertyChanged(this);
                        }
                    })
                    .Value_Lambda([this] { return *ValueRef; });
                Item->AddWidget(MoveTemp(ValueWidget));
            }
            
            return Row;
        }

    private:
        float* ValueRef;
    };

    class PropertyVector2fItem : public PropertyItemBase
    {
        MANUAL_RTTI_TYPE(PropertyVector2fItem, PropertyItemBase)
    public:
        PropertyVector2fItem(ShObject* InOwner, FString InName, Vector2f* InValueRef = nullptr)
            : PropertyItemBase(InOwner, MoveTemp(InName))
            , ValueRef(InValueRef)
        {}
        
        TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
        {
            auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
            if(ValueRef)
            {
                auto ValueWidget = SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    [
                        SNew(SSpinBox<float>)
                        .MaxFractionalDigits(3)
                        .OnValueChanged_Lambda([this](float NewValue) {
                            if(ValueRef->x != NewValue)
                            {
                                ValueRef->x = NewValue;
                                Owner->PostPropertyChanged(this);
                            }
                        })
                        .Value_Lambda([this] { return ValueRef->x; })
                    ]
                    + SHorizontalBox::Slot()
                    [
                        SNew(SSpinBox<float>)
                        .MaxFractionalDigits(3)
                        .OnValueChanged_Lambda([this](float NewValue) {
                            if(ValueRef->y != NewValue)
                            {
                                ValueRef->y = NewValue;
                                Owner->PostPropertyChanged(this);
                            }
                        })
                        .Value_Lambda([this] { return ValueRef->y; })
                    ];
                Item->AddWidget(MoveTemp(ValueWidget));
            }

            return Row;
        }

    private:
        Vector2f* ValueRef;
    };

	class PropertyVector4fItem : public PropertyItemBase
	{
		MANUAL_RTTI_TYPE(PropertyVector4fItem, PropertyItemBase)
	public:
		PropertyVector4fItem(ShObject* InOwner, FString InName, Vector4f* InValueRef = nullptr)
			: PropertyItemBase(InOwner, MoveTemp(InName))
			, ValueRef(InValueRef)
		{
		}

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
		{
			auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
			if (ValueRef)
			{
				auto ValueWidget = SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<float>)
							.MaxFractionalDigits(3)
							.OnValueChanged_Lambda([this](float NewValue) {
								if (ValueRef->x != NewValue)
								{
									ValueRef->x = NewValue;
									Owner->PostPropertyChanged(this);
								}
							})
							.Value_Lambda([this] { return ValueRef->x; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<float>)
							.MaxFractionalDigits(3)
							.OnValueChanged_Lambda([this](float NewValue) {
								if (ValueRef->y != NewValue)
								{
									ValueRef->y = NewValue;
									Owner->PostPropertyChanged(this);
								}
							})
							.Value_Lambda([this] { return ValueRef->y; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<float>)
							.MaxFractionalDigits(3)
							.OnValueChanged_Lambda([this](float NewValue) {
								if (ValueRef->z != NewValue)
								{
									ValueRef->z = NewValue;
									Owner->PostPropertyChanged(this);
								}
							})
							.Value_Lambda([this] { return ValueRef->z; })
					]
					+SHorizontalBox::Slot()
					[
						SNew(SSpinBox<float>)
							.MaxFractionalDigits(3)
							.OnValueChanged_Lambda([this](float NewValue) {
								if (ValueRef->w != NewValue)
								{
									ValueRef->w = NewValue;
									Owner->PostPropertyChanged(this);
								}
							})
							.Value_Lambda([this] { return ValueRef->w; })
					];
				Item->AddWidget(MoveTemp(ValueWidget));
			}

			return Row;
		}

	private:
		Vector4f* ValueRef;
	};

}

