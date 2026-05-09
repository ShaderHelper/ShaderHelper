#pragma once
#include "PropertyData.h"
#include "SPropertyItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "AssetManager/AssetManager.h"
#include "magic_enum.hpp"

#include <Framework/MultiBox/MultiBoxBuilder.h>
#include <Widgets/Input/SEditableTextBox.h>
#include <Widgets/Input/SSpinBox.h>
#include <Widgets/Layout/SWidgetSwitcher.h>
#include <Widgets/SBoxPanel.h>

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
		void SetEmbedWidget(TSharedPtr<SWidget> InWidget, bool bAutoWidth = false) { EmbedWidget = MoveTemp(InWidget); bEmbedWidgetAutoWidth = bAutoWidth; }
        void SetOnDelete(const TFunction<void()>& OnDeleteFunc) { OnDelete = OnDeleteFunc;}
		void SetContextMenuExtender(TFunction<void(FMenuBuilder&)> InContextMenuExtender) { ContextMenuExtender = MoveTemp(InContextMenuExtender); }
        
        virtual TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
        {
            auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable);
            Row->SetEnabled(IsEnabled);
			const bool bArrayElement = HasArrayElementStyle();
			const bool bCompositeMember = Parent && Parent->IsOfType<PropertyCategory>() && static_cast<PropertyCategory*>(Parent)->IsComposite();
			const FMargin RowPadding = bArrayElement ? FMargin(3.0f, 1.0f, 3.0f, 0.0f) :
				(bCompositeMember ? FMargin(0.0f) : FMargin(0.0f, 2.0f, 0.0f, 2.0f));
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
				Item->AddWidget(EmbedWidget, bEmbedWidgetAutoWidth);
            }
            
            Row->SetRowContent(
				SNew(SBorder)
				.Padding(RowPadding)
				.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& MouseEvent) {
					if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
					{
						if (TSharedPtr<SWidget> ContextMenu = CreateContextMenu())
						{
							FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();
							FSlateApplication::Get().PushMenu(Item.ToSharedRef(), WidgetPath, ContextMenu.ToSharedRef(), FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
							return FReply::Handled();
						}
					}
					return FReply::Unhandled();
				})
				.BorderImage_Lambda([Row, bArrayElement, RowBorder, NoBrush] {
					return bArrayElement && Row->IsSelected() ? NoBrush : RowBorder;
				})
				[
					SNew(SBorder)
					.BorderImage_Lambda([Row, bArrayElement, bCompositeMember, NoBrush] {
						if (bArrayElement && Row->IsSelected())
						{
							return NoBrush;
						}
						if (bCompositeMember)
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
		void AppendContextMenuExtender(FMenuBuilder& MenuBuilder)
		{
			if (ContextMenuExtender)
			{
				ContextMenuExtender(MenuBuilder);
			}
		}

		virtual TSharedPtr<SWidget> CreateContextMenu()
		{
			if (!ContextMenuExtender)
			{
				return nullptr;
			}
			FMenuBuilder MenuBuilder(true, nullptr);
			AppendContextMenuExtender(MenuBuilder);
			return MenuBuilder.MakeWidget();
		}

        bool IsEnabled = true;
        TFunction<void()> OnDelete;
		TFunction<void(FMenuBuilder&)> ContextMenuExtender;
		TSharedPtr<SWidget> EmbedWidget;
		bool bEmbedWidgetAutoWidth = false;
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

		void SetUseColorBlockPicker(bool bInUseColorBlockPicker) { bUseColorBlockPicker = bInUseColorBlockPicker; }
		void SetValueEnabled(TFunction<bool()> InValueEnabled) { ValueEnabled = MoveTemp(InValueEnabled); }
		
		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
		{
			auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
			if(ValueRef)
			{
				auto ValueWidget = SNew(SWidgetSwitcher)
					.WidgetIndex_Lambda([this] { return bUseColorBlockPicker ? 1 : 0; })
					+ SWidgetSwitcher::Slot()
					[
						MakeComponentWidget()
					]
					+ SWidgetSwitcher::Slot()
					[
						MakeColorPickerWidget()
					];
				Item->AddWidget(MoveTemp(ValueWidget));
			}

			return Row;
		}

	protected:
		TSharedPtr<SWidget> CreateContextMenu() override
		{
			if (!ValueRef)
			{
				return PropertyItemBase::CreateContextMenu();
			}

			FMenuBuilder MenuBuilder(true, nullptr);
			MenuBuilder.AddMenuEntry(
				LOCALIZATION("Value"),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateLambda([this] { bUseColorBlockPicker = false; }),
					FCanExecuteAction(),
					FIsActionChecked::CreateLambda([this] { return !bUseColorBlockPicker; })
				),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);
			MenuBuilder.AddMenuEntry(
				LOCALIZATION("Color"),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateLambda([this] { bUseColorBlockPicker = true; }),
					FCanExecuteAction(),
					FIsActionChecked::CreateLambda([this] { return bUseColorBlockPicker; })
				),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);
			AppendContextMenuExtender(MenuBuilder);
			return MenuBuilder.MakeWidget();
		}

	private:
		bool CanEditValue() const { return !ReadOnly && (!ValueEnabled || ValueEnabled()); }

		float GetComponent(int32 ComponentIdx) const
		{
			return ComponentIdx == 0 ? ValueRef->x : ComponentIdx == 1 ? ValueRef->y : ValueRef->z;
		}

		void SetComponent(int32 ComponentIdx, float NewValue)
		{
			if (ComponentIdx == 0)
			{
				ValueRef->x = NewValue;
			}
			else if (ComponentIdx == 1)
			{
				ValueRef->y = NewValue;
			}
			else
			{
				ValueRef->z = NewValue;
			}
		}

		TSharedRef<SWidget> MakeSpinBox(int32 ComponentIdx)
		{
			return SNew(SSpinBox<float>)
				.IsEnabled_Lambda([this] { return CanEditValue(); })
				.OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
				.OnValueChanged_Lambda([this, ComponentIdx](float NewValue) {
					if (GetComponent(ComponentIdx) != NewValue && Owner->CanChangeProperty(this))
					{
						BeginEdit();
						SetComponent(ComponentIdx, NewValue);
						Owner->PostPropertyChanged(this);
					}
				})
				.Value_Lambda([this, ComponentIdx] { return GetComponent(ComponentIdx); });
		}

		TSharedRef<SWidget> MakeComponentWidget()
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					MakeSpinBox(0)
				]
				+ SHorizontalBox::Slot()
				[
					MakeSpinBox(1)
				]
				+ SHorizontalBox::Slot()
				[
					MakeSpinBox(2)
				];
		}

		TSharedRef<SWidget> MakeColorPickerWidget()
		{
			TWeakPtr<SWindow> ParentWindow;
			return SNew(SShColorBlockPicker, ParentWindow)
				.IsEnabled_Lambda([this] { return CanEditValue(); })
				.UseSRGB(false)
				.Color_Lambda([this] {
					return FLinearColor{ ValueRef->x, ValueRef->y, ValueRef->z, 1.0f };
				})
				.OnColorChanged_Lambda([this](const FLinearColor& InColor) {
					if ((ValueRef->x != InColor.R || ValueRef->y != InColor.G || ValueRef->z != InColor.B) && Owner->CanChangeProperty(this))
					{
						BeginEdit();
						ValueRef->x = InColor.R;
						ValueRef->y = InColor.G;
						ValueRef->z = InColor.B;
						Owner->PostPropertyChanged(this);
						EndEdit();
					}
				});
		}

		Vector3f* ValueRef;
		bool ReadOnly;
		bool bUseColorBlockPicker = false;
		TFunction<bool()> ValueEnabled;
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

		void SetUseColorBlockPicker(bool bInUseColorBlockPicker) { bUseColorBlockPicker = bInUseColorBlockPicker; }
		void SetValueEnabled(TFunction<bool()> InValueEnabled) { ValueEnabled = MoveTemp(InValueEnabled); }

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
		{
			auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
			if (ValueRef)
			{
				auto ValueWidget = SNew(SWidgetSwitcher)
					.WidgetIndex_Lambda([this] { return bUseColorBlockPicker ? 1 : 0; })
					+ SWidgetSwitcher::Slot()
					[
						MakeComponentWidget()
					]
					+ SWidgetSwitcher::Slot()
					[
						MakeColorPickerWidget()
					];
				Item->AddWidget(MoveTemp(ValueWidget));
			}

			return Row;
		}

	protected:
		TSharedPtr<SWidget> CreateContextMenu() override
		{
			if (!ValueRef)
			{
				return PropertyItemBase::CreateContextMenu();
			}

			FMenuBuilder MenuBuilder(true, nullptr);
			MenuBuilder.AddMenuEntry(
				LOCALIZATION("Value"),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateLambda([this] { bUseColorBlockPicker = false; }),
					FCanExecuteAction(),
					FIsActionChecked::CreateLambda([this] { return !bUseColorBlockPicker; })
				),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);
			MenuBuilder.AddMenuEntry(
				LOCALIZATION("Color"),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateLambda([this] { bUseColorBlockPicker = true; }),
					FCanExecuteAction(),
					FIsActionChecked::CreateLambda([this] { return bUseColorBlockPicker; })
				),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);
			AppendContextMenuExtender(MenuBuilder);
			return MenuBuilder.MakeWidget();
		}

	private:
		bool CanEditValue() const { return !ReadOnly && (!ValueEnabled || ValueEnabled()); }

		float GetComponent(int32 ComponentIdx) const
		{
			switch (ComponentIdx)
			{
			case 0: return ValueRef->x;
			case 1: return ValueRef->y;
			case 2: return ValueRef->z;
			default: return ValueRef->w;
			}
		}

		void SetComponent(int32 ComponentIdx, float NewValue)
		{
			switch (ComponentIdx)
			{
			case 0: ValueRef->x = NewValue; break;
			case 1: ValueRef->y = NewValue; break;
			case 2: ValueRef->z = NewValue; break;
			default: ValueRef->w = NewValue; break;
			}
		}

		TSharedRef<SWidget> MakeSpinBox(int32 ComponentIdx)
		{
			return SNew(SSpinBox<float>)
				.IsEnabled_Lambda([this] { return CanEditValue(); })
				.OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
				.OnValueChanged_Lambda([this, ComponentIdx](float NewValue) {
					if (GetComponent(ComponentIdx) != NewValue && Owner->CanChangeProperty(this))
					{
						BeginEdit();
						SetComponent(ComponentIdx, NewValue);
						Owner->PostPropertyChanged(this);
					}
				})
				.Value_Lambda([this, ComponentIdx] { return GetComponent(ComponentIdx); });
		}

		TSharedRef<SWidget> MakeComponentWidget()
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					MakeSpinBox(0)
				]
				+ SHorizontalBox::Slot()
				[
					MakeSpinBox(1)
				]
				+ SHorizontalBox::Slot()
				[
					MakeSpinBox(2)
				]
				+ SHorizontalBox::Slot()
				[
					MakeSpinBox(3)
				];
		}

		TSharedRef<SWidget> MakeColorPickerWidget()
		{
			TWeakPtr<SWindow> ParentWindow;
			return SNew(SShColorBlockPicker, ParentWindow)
				.IsEnabled_Lambda([this] { return CanEditValue(); })
				.UseSRGB(false)
				.AlphaDisplayMode(EColorBlockAlphaDisplayMode::Separate)
				.ShowBackgroundForAlpha(true)
				.ShowAlpha(true)
				.Color_Lambda([this] {
					return FLinearColor{ ValueRef->x, ValueRef->y, ValueRef->z, ValueRef->w };
				})
				.OnColorChanged_Lambda([this](const FLinearColor& InColor) {
					if ((ValueRef->x != InColor.R || ValueRef->y != InColor.G || ValueRef->z != InColor.B || ValueRef->w != InColor.A) && Owner->CanChangeProperty(this))
					{
						BeginEdit();
						ValueRef->x = InColor.R;
						ValueRef->y = InColor.G;
						ValueRef->z = InColor.B;
						ValueRef->w = InColor.A;
						Owner->PostPropertyChanged(this);
						EndEdit();
					}
				});
		}

		Vector4f* ValueRef;
		bool ReadOnly;
		bool bUseColorBlockPicker = false;
		TFunction<bool()> ValueEnabled;
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

