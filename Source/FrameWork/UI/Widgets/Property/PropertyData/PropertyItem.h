#pragma once
#include "SPropertyItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "AssetManager/AssetManager.h"

#include <Widgets/Input/SSpinBox.h>

namespace FW
{
    //{ Name | Embed | Value } DeleteIcon
    class PropertyItemBase : public PropertyData
    {
        MANUAL_RTTI_TYPE(PropertyItemBase, PropertyData)
    public:
        using PropertyData::PropertyData;
        
        void SetEnabled(bool Enabled) { IsEnabled = Enabled; }
        void SetCanApplyName(const TFunction<bool(const FString&)>& CanApply) { CanApplyName = CanApply; }
        void SetOnDisplayNameChanged(const TFunction<void(const FString&)>& DisplayNameChanged) { OnDisplayNameChanged = DisplayNameChanged; }
        void SetEmbedWidget(TSharedPtr<SWidget> InWidget) { EmbedWidget = MoveTemp(InWidget); }
        void SetOnDelete(const TFunction<void()>& OnDeleteFunc) { OnDelete = OnDeleteFunc;}
        
        virtual TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
        {
            auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable);
            Row->SetEnabled(IsEnabled);
                
            SAssignNew(Item, SPropertyItem)
                    .CanApplyName(CanApplyName)
                    .DisplayName(&DisplayName)
                    .OnDisplayNameChanged(OnDisplayNameChanged)
                    .Indent(!!Parent);
            
            if(EmbedWidget)
            {
                Item->AddWidget(EmbedWidget.ToSharedRef());
            }
            
            Row->SetRowContent(
				SNew(SBorder)
				.Padding(FMargin{0,2,0,2})
				.BorderImage_Lambda([this] {
					if(Parent && Parent->IsOfType<PropertyCategory>() && static_cast<PropertyCategory*>(Parent)->IsComposite() )
					{
						return FAppCommonStyle::Get().GetBrush("PropertyView.CompositeItemColor");
					}
				   return FAppCommonStyle::Get().GetBrush("PropertyView.ItemColor");
				})
				[
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
				]
            );
            return Row;
        }
        
    protected:
        bool IsEnabled = true;
        TFunction<void()> OnDelete;
        TSharedPtr<SWidget> EmbedWidget;
        TSharedPtr<SPropertyItem> Item;
        TFunction<bool(const FString&)> CanApplyName;
        TFunction<void(const FString&)> OnDisplayNameChanged;
    };

	class PropertyEnumItem : public PropertyItemBase
	{
		MANUAL_RTTI_TYPE(PropertyEnumItem, PropertyItemBase)
	public:
		PropertyEnumItem(ShObject* InOwner, const FString& InName, TSharedPtr<FString> InEnumValueName,
						 const TMap<FString, TSharedPtr<void>>& InEnumEntries, const TFunction<void(void*)>& InSetter, bool InReadOnly = false)
		: PropertyItemBase(InOwner, InName)
		, EnumValueName(InEnumValueName)
		, EnumEntries(InEnumEntries)
		, Setter(InSetter)
		, ReadOnly(InReadOnly)
		{
			for(const auto& [EntryStr, _] : EnumEntries)
			{
				EnumItems.Add(MakeShared<FString>(EntryStr));
			}
		}
		
		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
		{
			auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
			auto ValueWidget = SNew(SComboBox<TSharedPtr<FString>>)
			.IsEnabled(!ReadOnly)
			.OptionsSource(&EnumItems)
			.OnSelectionChanged_Lambda([this](TSharedPtr<FString> InItem, ESelectInfo::Type){
				if(*InItem != *EnumValueName && Owner->CanChangeProperty(this))
				{
					EnumValueName = InItem;
					Setter(EnumEntries[*EnumValueName].Get());
					Owner->PostPropertyChanged(this);
				}
			})
			.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem){
				return SNew(STextBlock).Text(FText::FromString(*InItem));
			})
			[
				SNew(STextBlock).Text_Lambda([this] {
					return FText::FromString(*EnumValueName);
				})
			];
			Item->AddWidget(MoveTemp(ValueWidget));
			return Row;
		}
		
	private:
		TSharedPtr<FString> EnumValueName;
		TMap<FString, TSharedPtr<void>> EnumEntries;
		TFunction<void(void*)> Setter;
		bool ReadOnly;
		TArray<TSharedPtr<FString>> EnumItems;
	};

	template<typename T>
    class PropertyScalarItem : public PropertyItemBase
    {
        MANUAL_RTTI_TYPE(PropertyScalarItem<T>, PropertyItemBase)
    public:
		PropertyScalarItem(ShObject* InOwner, FString InName, T* InValueRef = nullptr, bool InReadOnly = false)
            : PropertyItemBase(InOwner, MoveTemp(InName))
            , ValueRef(InValueRef)
			, ReadOnly(InReadOnly)
        {}

		void SetMinValue(const T& InValue) { MinValue = InValue; }
		void SetMaxValue(const T& InValue) { MaxValue = InValue; }
        
        TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
        {
            auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
            if(ValueRef)
            {
                auto ValueWidget = SNew(SSpinBox<T>)
					.IsEnabled(!ReadOnly)
					.MinValue(MinValue)
					.MaxValue(MaxValue)
                    .OnValueChanged_Lambda([this](T NewValue) {
                        if(*ValueRef != NewValue && Owner->CanChangeProperty(this))
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
        T* ValueRef;
		bool ReadOnly;
		TOptional<T> MinValue, MaxValue;
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
                        .OnValueChanged_Lambda([this](float NewValue) {
                            if(ValueRef->x != NewValue && Owner->CanChangeProperty(this))
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
                        .OnValueChanged_Lambda([this](float NewValue) {
                            if(ValueRef->y != NewValue && Owner->CanChangeProperty(this))
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

	class PropertyVector3fItem : public PropertyItemBase
	{
		MANUAL_RTTI_TYPE(PropertyVector3fItem, PropertyItemBase)
	public:
		PropertyVector3fItem(ShObject* InOwner, FString InName, Vector3f* InValueRef = nullptr)
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
						.OnValueChanged_Lambda([this](float NewValue) {
							if(ValueRef->x != NewValue && Owner->CanChangeProperty(this))
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
						.OnValueChanged_Lambda([this](float NewValue) {
							if(ValueRef->y != NewValue && Owner->CanChangeProperty(this))
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
						.OnValueChanged_Lambda([this](float NewValue) {
							if(ValueRef->z != NewValue && Owner->CanChangeProperty(this))
							{
								ValueRef->z = NewValue;
								Owner->PostPropertyChanged(this);
							}
						})
						.Value_Lambda([this] { return ValueRef->z; })
					];
				Item->AddWidget(MoveTemp(ValueWidget));
			}

			return Row;
		}

	private:
		Vector3f* ValueRef;
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
							.OnValueChanged_Lambda([this](float NewValue) {
								if (ValueRef->x != NewValue && Owner->CanChangeProperty(this))
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
							.OnValueChanged_Lambda([this](float NewValue) {
								if (ValueRef->y != NewValue && Owner->CanChangeProperty(this))
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
							.OnValueChanged_Lambda([this](float NewValue) {
								if (ValueRef->z != NewValue && Owner->CanChangeProperty(this))
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
							.OnValueChanged_Lambda([this](float NewValue) {
								if (ValueRef->w != NewValue && Owner->CanChangeProperty(this))
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

