#pragma once
#include "PropertyData.h"
#include "SPropertyItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "AssetManager/AssetManager.h"
#include "magic_enum.hpp"

#include <Widgets/Input/SEditableTextBox.h>
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
        void SetCanApplyName(const TFunction<bool(const FText&)>& CanApply) { CanApplyName = CanApply; }
        void SetOnDisplayNameChanged(const TFunction<void(const FText&)>& DisplayNameChanged) { OnDisplayNameChanged = DisplayNameChanged; }
        void SetEmbedWidget(TSharedPtr<SWidget> InWidget) { EmbedWidget = MoveTemp(InWidget); }
        void SetOnDelete(const TFunction<void()>& OnDeleteFunc) { OnDelete = OnDeleteFunc;}
        
        virtual TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
        {
            auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable);
            Row->SetEnabled(IsEnabled);
			const bool bArrayElement = HasArrayElementStyle();
			const FMargin RowPadding = bArrayElement ? FMargin(3.0f, 1.0f, 3.0f, 0.0f) : FMargin(0.0f, 2.0f, 0.0f, 2.0f);
			const FSlateBrush* RowBorder = bArrayElement
				? FAppStyle::Get().GetBrush("Brushes.Input")
				: FAppCommonStyle::Get().GetBrush("PropertyView.ItemColor");
			const FSlateBrush* NoBrush = FAppStyle::Get().GetBrush("NoBrush");
                
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
				.Padding(RowPadding)
				.BorderImage_Lambda([Row, bArrayElement, RowBorder, NoBrush] {
					return bArrayElement && Row->IsSelected() ? NoBrush : RowBorder;
				})
				[
					SNew(SBorder)
					.BorderImage_Lambda([this, Row, bArrayElement, NoBrush] {
						if (bArrayElement && Row->IsSelected())
						{
							return NoBrush;
						}
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
				]
            );
            return Row;
        }
        
    protected:
        bool IsEnabled = true;
        TFunction<void()> OnDelete;
        TSharedPtr<SWidget> EmbedWidget;
        TSharedPtr<SPropertyItem> Item;
        TFunction<bool(const FText&)> CanApplyName;
        TFunction<void(const FText&)> OnDisplayNameChanged;
    };

	class PropertyEnumItem : public PropertyItemBase
	{
		MANUAL_RTTI_TYPE(PropertyEnumItem, PropertyItemBase)
	public:
		PropertyEnumItem(ShObject* InOwner, const FText& InName, TSharedPtr<FString> InEnumValueName,
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
			SAssignNew(ValueWidget, SComboBox<TSharedPtr<FString>>)
			.IsEnabled(!ReadOnly)
			.OptionsSource(&EnumItems)
			.OnSelectionChanged_Lambda([this](TSharedPtr<FString> InItem, ESelectInfo::Type){
				if(InItem && *InItem != *EnumValueName)
				{
					if (Owner->CanChangeProperty(this))
					{
						BeginEdit();
						EnumValueName = InItem;
						Setter(EnumEntries[*EnumValueName].Get());
						Owner->PostPropertyChanged(this);
						EndEdit();
					}
					else
					{
						ValueWidget->SetSelectedItem(EnumValueName);
					}
				}
			})
			.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem){
				return SNew(STextBlock).Text(LOCALIZATION(*InItem));
			})
			[
				SNew(STextBlock).Text_Lambda([this] {
					return LOCALIZATION(*EnumValueName);
				})
			];
			Item->AddWidget(ValueWidget);
			return Row;
		}

		void* GetEnum() const { return EnumEntries[*EnumValueName].Get(); }
		
	private:
		TSharedPtr<SComboBox<TSharedPtr<FString>> > ValueWidget;
		TSharedPtr<FString> EnumValueName;
		TMap<FString, TSharedPtr<void>> EnumEntries;
		TFunction<void(void*)> Setter;
		bool ReadOnly;
		TArray<TSharedPtr<FString>> EnumItems;
	};

	template<typename EnumType>
	TMap<FString, TSharedPtr<void>> MakePropertyEnumEntries()
	{
		TMap<FString, TSharedPtr<void>> Entries;
		for (const auto& [EntryValue, EntryStr] : magic_enum::enum_entries<EnumType>())
		{
			Entries.Add(ANSI_TO_TCHAR(EntryStr.data()), MakeShared<EnumType>(EntryValue));
		}
		return Entries;
	}

	template<typename EnumType>
	TSharedPtr<FString> MakePropertyEnumValueName(EnumType Value)
	{
		return MakeShared<FString>(ANSI_TO_TCHAR(magic_enum::enum_name(Value).data()));
	}

	template<typename EnumType>
	TSharedRef<PropertyEnumItem> MakePropertyEnumItem(ShObject* InOwner, const FText& InName, EnumType Value,
		const TFunction<void(EnumType)>& InSetter, bool InReadOnly = false)
	{
		return MakeShared<PropertyEnumItem>(
			InOwner,
			InName,
			MakePropertyEnumValueName(Value),
			MakePropertyEnumEntries<EnumType>(),
			[InSetter](void* InValue) {
				InSetter(*static_cast<EnumType*>(InValue));
			},
			InReadOnly);
	}

	template<typename T>
    class PropertyScalarItem : public PropertyItemBase
    {
        MANUAL_RTTI_TYPE(PropertyScalarItem<T>, PropertyItemBase)
    public:
		PropertyScalarItem(ShObject* InOwner, const FString& InName, T* InValueRef = nullptr, bool InReadOnly = false) 
			: PropertyScalarItem(InOwner, FText::FromString(InName), InValueRef, InReadOnly)
		{}
		PropertyScalarItem(ShObject* InOwner, FText InName, T* InValueRef = nullptr, bool InReadOnly = false)
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
				TSharedPtr<SWidget> ValueWidget;
				if constexpr (std::is_same_v<T, bool>)
				{
					ValueWidget = SNew(SCheckBox).IsChecked_Lambda([this] {
						return *ValueRef ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;;
					})
					.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) {
						if (Owner->CanChangeProperty(this))
						{
							BeginEdit();
							*ValueRef = InState == ECheckBoxState::Checked;
							Owner->PostPropertyChanged(this);
							EndEdit();
						}
					});
				}
				else
				{
					ValueWidget = SNew(SSpinBox<T>)
					.IsEnabled(!ReadOnly)
					.MinValue(MinValue)
					.MaxValue(MaxValue)
					.OnValueCommitted_Lambda([this](T, ETextCommit::Type) { EndEdit(); })
					.OnValueChanged_Lambda([this](T NewValue) {
						if (*ValueRef != NewValue && Owner->CanChangeProperty(this))
						{
							BeginEdit();
							*ValueRef = NewValue;
							Owner->PostPropertyChanged(this);
						}
					})
					.Value_Lambda([this] { return *ValueRef; });
				}
                Item->AddWidget(MoveTemp(ValueWidget));
            }
            
            return Row;
        }

    private:
        T* ValueRef;
		bool ReadOnly;
		TOptional<T> MinValue, MaxValue;
    };

	class PropertyStringItem : public PropertyItemBase
	{
		MANUAL_RTTI_TYPE(PropertyStringItem, PropertyItemBase)
	public:
		PropertyStringItem(ShObject* InOwner, const FString& InName, FString* InValueRef = nullptr, bool InReadOnly = false)
			: PropertyStringItem(InOwner, FText::FromString(InName), InValueRef, InReadOnly)
		{}

		PropertyStringItem(ShObject* InOwner, FText InName, FString* InValueRef = nullptr, bool InReadOnly = false)
			: PropertyItemBase(InOwner, MoveTemp(InName))
			, ValueRef(InValueRef)
			, ReadOnly(InReadOnly)
		{}

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
		{
			auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
			if (ValueRef)
			{
				Item->AddWidget(
					SNew(SEditableTextBox)
					.IsReadOnly(ReadOnly)
					.Text_Lambda([this] {
						return FText::FromString(*ValueRef);
					})
					.OnTextCommitted_Lambda([this](const FText& InText, ETextCommit::Type InCommitType) {
						const FString NewValue = InText.ToString();
						if (*ValueRef != NewValue && Owner->CanChangeProperty(this))
						{
							BeginEdit();
							*ValueRef = NewValue;
							Owner->PostPropertyChanged(this);
							EndEdit();
						}
					})
				);
			}

			return Row;
		}

	private:
		FString* ValueRef;
		bool ReadOnly;
	};

    class PropertyVector2fItem : public PropertyItemBase
    {
        MANUAL_RTTI_TYPE(PropertyVector2fItem, PropertyItemBase)
    public:
		PropertyVector2fItem(ShObject* InOwner, const FString& InName, Vector2f* InValueRef = nullptr, bool InReadOnly = false)
			: PropertyVector2fItem(InOwner, FText::FromString(InName), InValueRef, InReadOnly)
		{}
		PropertyVector2fItem(ShObject* InOwner, FText InName, Vector2f* InValueRef = nullptr, bool InReadOnly = false)
            : PropertyItemBase(InOwner, MoveTemp(InName))
            , ValueRef(InValueRef)
			, ReadOnly(InReadOnly)
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
						.IsEnabled(!ReadOnly)
                        .OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
                        .OnValueChanged_Lambda([this](float NewValue) {
                            if(ValueRef->x != NewValue && Owner->CanChangeProperty(this))
                            {
                                BeginEdit();
                                ValueRef->x = NewValue;
                                Owner->PostPropertyChanged(this);
                            }
                        })
                        .Value_Lambda([this] { return ValueRef->x; })
                    ]
                    + SHorizontalBox::Slot()
                    [
                        SNew(SSpinBox<float>)
						.IsEnabled(!ReadOnly)
                        .OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
                        .OnValueChanged_Lambda([this](float NewValue) {
                            if(ValueRef->y != NewValue && Owner->CanChangeProperty(this))
                            {
                                BeginEdit();
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
		bool ReadOnly;
    };

	class PropertyVector3fItem : public PropertyItemBase
	{
		MANUAL_RTTI_TYPE(PropertyVector3fItem, PropertyItemBase)
	public:
		PropertyVector3fItem(ShObject* InOwner, const FString& InName, Vector3f* InValueRef = nullptr, bool InReadOnly = false)
			: PropertyVector3fItem(InOwner, FText::FromString(InName), InValueRef, InReadOnly)
		{}
		PropertyVector3fItem(ShObject* InOwner, FText InName, Vector3f* InValueRef = nullptr, bool InReadOnly = false)
			: PropertyItemBase(InOwner, MoveTemp(InName))
			, ValueRef(InValueRef)
			, ReadOnly(InReadOnly)
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
						.IsEnabled(!ReadOnly)
						.OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
						.OnValueChanged_Lambda([this](float NewValue) {
							if(ValueRef->x != NewValue && Owner->CanChangeProperty(this))
							{
								BeginEdit();
								ValueRef->x = NewValue;
								Owner->PostPropertyChanged(this);
							}
						})
						.Value_Lambda([this] { return ValueRef->x; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<float>)
						.IsEnabled(!ReadOnly)
						.OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
						.OnValueChanged_Lambda([this](float NewValue) {
							if(ValueRef->y != NewValue && Owner->CanChangeProperty(this))
							{
								BeginEdit();
								ValueRef->y = NewValue;
								Owner->PostPropertyChanged(this);
							}
						})
						.Value_Lambda([this] { return ValueRef->y; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<float>)
						.IsEnabled(!ReadOnly)
						.OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
						.OnValueChanged_Lambda([this](float NewValue) {
							if(ValueRef->z != NewValue && Owner->CanChangeProperty(this))
							{
								BeginEdit();
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
		bool ReadOnly;
	};

	class PropertyVector4fItem : public PropertyItemBase
	{
		MANUAL_RTTI_TYPE(PropertyVector4fItem, PropertyItemBase)
	public:
		PropertyVector4fItem(ShObject* InOwner, const FString& InName, Vector4f* InValueRef = nullptr, bool InReadOnly = false)
			: PropertyVector4fItem(InOwner, FText::FromString(InName), InValueRef, InReadOnly)
		{}
		PropertyVector4fItem(ShObject* InOwner, FText InName, Vector4f* InValueRef = nullptr, bool InReadOnly = false)
			: PropertyItemBase(InOwner, MoveTemp(InName))
			, ValueRef(InValueRef)
			, ReadOnly(InReadOnly)
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
							.IsEnabled(!ReadOnly)
							.OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
							.OnValueChanged_Lambda([this](float NewValue) {
								if (ValueRef->x != NewValue && Owner->CanChangeProperty(this))
								{
									BeginEdit();
									ValueRef->x = NewValue;
									Owner->PostPropertyChanged(this);
								}
							})
							.Value_Lambda([this] { return ValueRef->x; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<float>)
							.IsEnabled(!ReadOnly)
							.OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
							.OnValueChanged_Lambda([this](float NewValue) {
								if (ValueRef->y != NewValue && Owner->CanChangeProperty(this))
								{
									BeginEdit();
									ValueRef->y = NewValue;
									Owner->PostPropertyChanged(this);
								}
							})
							.Value_Lambda([this] { return ValueRef->y; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<float>)
							.IsEnabled(!ReadOnly)
							.OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
							.OnValueChanged_Lambda([this](float NewValue) {
								if (ValueRef->z != NewValue && Owner->CanChangeProperty(this))
								{
									BeginEdit();
									ValueRef->z = NewValue;
									Owner->PostPropertyChanged(this);
								}
							})
							.Value_Lambda([this] { return ValueRef->z; })
					]
					+SHorizontalBox::Slot()
					[
						SNew(SSpinBox<float>)
							.IsEnabled(!ReadOnly)
							.OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
							.OnValueChanged_Lambda([this](float NewValue) {
								if (ValueRef->w != NewValue && Owner->CanChangeProperty(this))
								{
									BeginEdit();
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
		bool ReadOnly;
	};

	class PropertyVector2iItem : public PropertyItemBase
	{
		MANUAL_RTTI_TYPE(PropertyVector2iItem, PropertyItemBase)
	public:
		PropertyVector2iItem(ShObject* InOwner, FText InName, int32* InValues = nullptr, bool InReadOnly = false)
			: PropertyItemBase(InOwner, MoveTemp(InName))
			, Values(InValues)
			, ReadOnly(InReadOnly)
		{}

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
		{
			auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
			if (Values)
			{
				auto ValueWidget = SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<int32>)
						.IsEnabled(!ReadOnly)
						.OnValueCommitted_Lambda([this](int32, ETextCommit::Type) { EndEdit(); })
						.OnValueChanged_Lambda([this](int32 NewValue) {
							if (Values[0] != NewValue && Owner->CanChangeProperty(this))
							{
								BeginEdit();
								Values[0] = NewValue;
								Owner->PostPropertyChanged(this);
							}
						})
						.Value_Lambda([this] { return Values[0]; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<int32>)
						.IsEnabled(!ReadOnly)
						.OnValueCommitted_Lambda([this](int32, ETextCommit::Type) { EndEdit(); })
						.OnValueChanged_Lambda([this](int32 NewValue) {
							if (Values[1] != NewValue && Owner->CanChangeProperty(this))
							{
								BeginEdit();
								Values[1] = NewValue;
								Owner->PostPropertyChanged(this);
							}
						})
						.Value_Lambda([this] { return Values[1]; })
					];
				Item->AddWidget(MoveTemp(ValueWidget));
			}
			return Row;
		}

	private:
		int32* Values;
		bool ReadOnly;
	};

	class PropertyVector3iItem : public PropertyItemBase
	{
		MANUAL_RTTI_TYPE(PropertyVector3iItem, PropertyItemBase)
	public:
		PropertyVector3iItem(ShObject* InOwner, FText InName, int32* InValues = nullptr, bool InReadOnly = false)
			: PropertyItemBase(InOwner, MoveTemp(InName))
			, Values(InValues)
			, ReadOnly(InReadOnly)
		{}

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
		{
			auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
			if (Values)
			{
				auto ValueWidget = SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<int32>)
						.IsEnabled(!ReadOnly)
						.OnValueCommitted_Lambda([this](int32, ETextCommit::Type) { EndEdit(); })
						.OnValueChanged_Lambda([this](int32 NewValue) {
							if (Values[0] != NewValue && Owner->CanChangeProperty(this))
							{
								BeginEdit();
								Values[0] = NewValue;
								Owner->PostPropertyChanged(this);
							}
						})
						.Value_Lambda([this] { return Values[0]; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<int32>)
						.IsEnabled(!ReadOnly)
						.OnValueCommitted_Lambda([this](int32, ETextCommit::Type) { EndEdit(); })
						.OnValueChanged_Lambda([this](int32 NewValue) {
							if (Values[1] != NewValue && Owner->CanChangeProperty(this))
							{
								BeginEdit();
								Values[1] = NewValue;
								Owner->PostPropertyChanged(this);
							}
						})
						.Value_Lambda([this] { return Values[1]; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<int32>)
						.IsEnabled(!ReadOnly)
						.OnValueCommitted_Lambda([this](int32, ETextCommit::Type) { EndEdit(); })
						.OnValueChanged_Lambda([this](int32 NewValue) {
							if (Values[2] != NewValue && Owner->CanChangeProperty(this))
							{
								BeginEdit();
								Values[2] = NewValue;
								Owner->PostPropertyChanged(this);
							}
						})
						.Value_Lambda([this] { return Values[2]; })
					];
				Item->AddWidget(MoveTemp(ValueWidget));
			}
			return Row;
		}

	private:
		int32* Values;
		bool ReadOnly;
	};

	class PropertyVector4iItem : public PropertyItemBase
	{
		MANUAL_RTTI_TYPE(PropertyVector4iItem, PropertyItemBase)
	public:
		PropertyVector4iItem(ShObject* InOwner, FText InName, int32* InValues = nullptr, bool InReadOnly = false)
			: PropertyItemBase(InOwner, MoveTemp(InName))
			, Values(InValues)
			, ReadOnly(InReadOnly)
		{}

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
		{
			auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
			if (Values)
			{
				auto ValueWidget = SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<int32>)
						.IsEnabled(!ReadOnly)
						.OnValueCommitted_Lambda([this](int32, ETextCommit::Type) { EndEdit(); })
						.OnValueChanged_Lambda([this](int32 NewValue) {
							if (Values[0] != NewValue && Owner->CanChangeProperty(this))
							{
								BeginEdit();
								Values[0] = NewValue;
								Owner->PostPropertyChanged(this);
							}
						})
						.Value_Lambda([this] { return Values[0]; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<int32>)
						.IsEnabled(!ReadOnly)
						.OnValueCommitted_Lambda([this](int32, ETextCommit::Type) { EndEdit(); })
						.OnValueChanged_Lambda([this](int32 NewValue) {
							if (Values[1] != NewValue && Owner->CanChangeProperty(this))
							{
								BeginEdit();
								Values[1] = NewValue;
								Owner->PostPropertyChanged(this);
							}
						})
						.Value_Lambda([this] { return Values[1]; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<int32>)
						.IsEnabled(!ReadOnly)
						.OnValueCommitted_Lambda([this](int32, ETextCommit::Type) { EndEdit(); })
						.OnValueChanged_Lambda([this](int32 NewValue) {
							if (Values[2] != NewValue && Owner->CanChangeProperty(this))
							{
								BeginEdit();
								Values[2] = NewValue;
								Owner->PostPropertyChanged(this);
							}
						})
						.Value_Lambda([this] { return Values[2]; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<int32>)
						.IsEnabled(!ReadOnly)
						.OnValueCommitted_Lambda([this](int32, ETextCommit::Type) { EndEdit(); })
						.OnValueChanged_Lambda([this](int32 NewValue) {
							if (Values[3] != NewValue && Owner->CanChangeProperty(this))
							{
								BeginEdit();
								Values[3] = NewValue;
								Owner->PostPropertyChanged(this);
							}
						})
						.Value_Lambda([this] { return Values[3]; })
					];
				Item->AddWidget(MoveTemp(ValueWidget));
			}
			return Row;
		}

	private:
		int32* Values;
		bool ReadOnly;
	};

}

