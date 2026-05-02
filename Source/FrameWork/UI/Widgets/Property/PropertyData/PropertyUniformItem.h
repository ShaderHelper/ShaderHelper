#pragma once
#include "PropertyItem.h"
#include "RenderResource/UniformBuffer.h"

namespace FW
{
    template<typename T>
    class PropertyUniformItem : public PropertyItemBase
    {
    public:
        PropertyUniformItem(ShObject* InOwner, FString InName, UniformBufferMemberWrapper<T> InValueRef, const TAttribute<bool>& InWritable = false)
            : PropertyItemBase(InOwner, MoveTemp(InName))
            , ValueRef(InValueRef)
			, Writable(InWritable)
        {}

		void SetUseColorBlockPicker(bool bInUseColorBlockPicker) { bUseColorBlockPicker = bInUseColorBlockPicker; }

        TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;

	protected:
		TSharedPtr<SWidget> CreateContextMenu() override
		{
			if constexpr (std::is_same_v<T, Vector3f> || std::is_same_v<T, Vector4f>)
			{
				FMenuBuilder MenuBuilder(true, nullptr);
				MenuBuilder.AddMenuEntry(
					FText::FromString(TEXT("SpinBox")),
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
					FText::FromString(TEXT("Color Picker")),
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
				return MenuBuilder.MakeWidget();
			}

			return nullptr;
		}

    private:
        UniformBufferMemberWrapper<T> ValueRef;
		TAttribute<bool> Writable;
		bool bUseColorBlockPicker = false;
    };

	bool IsProperyUniformItem(PropertyData* InProprety)
	{
		return InProprety->IsOfType<PropertyUniformItem<float>>() || InProprety->IsOfType<PropertyUniformItem<int32>>()
			|| InProprety->IsOfType<PropertyUniformItem<Vector2f>>() 
			|| InProprety->IsOfType<PropertyUniformItem<Vector3f>>() || InProprety->IsOfType<PropertyUniformItem<Vector4f>>();
	}

    template<>
    inline TSharedRef<ITableRow> PropertyUniformItem<float>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
    {
        auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
        auto ValueWidget = SNew(SSpinBox<float>)
            .OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
            .OnValueChanged_Lambda([this](float NewValue) {
                if(ValueRef != NewValue && Owner->CanChangeProperty(this))
                {
                    BeginEdit();
                    ValueRef = NewValue;
                    Owner->PostPropertyChanged(this);
                }
            })
            .Value_Lambda([this] { return ValueRef; });
		ValueWidget->SetEnabled(Writable);
        Item->AddWidget(MoveTemp(ValueWidget));
        return Row;
    }

	template<>
	inline TSharedRef<ITableRow> PropertyUniformItem<int32>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
		auto ValueWidget = SNew(SSpinBox<int32>)
			.OnValueCommitted_Lambda([this](int32, ETextCommit::Type) { EndEdit(); })
			.OnValueChanged_Lambda([this](int32 NewValue) {
				if (ValueRef != NewValue && Owner->CanChangeProperty(this))
				{
					BeginEdit();
					ValueRef = NewValue;
					Owner->PostPropertyChanged(this);
				}
			})
			.Value_Lambda([this] { return ValueRef; });
		ValueWidget->SetEnabled(Writable);
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
                .OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
                .OnValueChanged_Lambda([this](float NewValue) {
                    Vector2f& Value = ValueRef;
                    if(Value.x != NewValue && Owner->CanChangeProperty(this))
                    {
                        BeginEdit();
                        ValueRef = {NewValue, Value.y};
                        Owner->PostPropertyChanged(this);
                    }
                })
                .Value_Lambda([this] { return ((Vector2f)ValueRef).x; })
            ]
            + SHorizontalBox::Slot()
            [
                SNew(SSpinBox<float>)
                .OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
                .OnValueChanged_Lambda([this](float NewValue) {
                    Vector2f& Value = ValueRef;
                    if(Value.y != NewValue && Owner->CanChangeProperty(this))
                    {
                        BeginEdit();
                        ValueRef = {Value.x, NewValue};
                        Owner->PostPropertyChanged(this);
                    }
                })
                .Value_Lambda([this] { return ((Vector2f&)ValueRef).y; })
            ];
		ValueWidget->SetEnabled(Writable);
        Item->AddWidget(MoveTemp(ValueWidget));
        return Row;
    }

	template<>
	inline TSharedRef<ITableRow> PropertyUniformItem<Vector3f>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
		auto MakeSpinBox = [this](int32 ComponentIdx) {
			return SNew(SSpinBox<float>)
				.OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
				.OnValueChanged_Lambda([this, ComponentIdx](float NewValue) {
					Vector3f CurrentValue = ValueRef;
					float* ComponentValue = ComponentIdx == 0 ? &CurrentValue.x : ComponentIdx == 1 ? &CurrentValue.y : &CurrentValue.z;
					if (*ComponentValue != NewValue && Owner->CanChangeProperty(this))
					{
						BeginEdit();
						*ComponentValue = NewValue;
						ValueRef = CurrentValue;
						Owner->PostPropertyChanged(this);
					}
				})
				.Value_Lambda([this, ComponentIdx] {
					Vector3f CurrentValue = ValueRef;
					return ComponentIdx == 0 ? CurrentValue.x : ComponentIdx == 1 ? CurrentValue.y : CurrentValue.z;
				});
		};

		auto ValueWidget = SNew(SWidgetSwitcher)
			.WidgetIndex_Lambda([this] { return bUseColorBlockPicker ? 1 : 0; })
			+ SWidgetSwitcher::Slot()
			[
				SNew(SHorizontalBox)
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
			]
			+ SWidgetSwitcher::Slot()
			[
				SNew(SShColorBlockPicker, TWeakPtr<SWindow>())
				.UseSRGB(false)
				.Color_Lambda([this] {
					Vector3f CurrentValue = ValueRef;
					return FLinearColor{ CurrentValue.x, CurrentValue.y, CurrentValue.z, 1.0f };
				})
				.OnColorChanged_Lambda([this](const FLinearColor& InColor) {
					Vector3f CurrentValue = ValueRef;
					if ((CurrentValue.x != InColor.R || CurrentValue.y != InColor.G || CurrentValue.z != InColor.B) && Owner->CanChangeProperty(this))
					{
						BeginEdit();
						ValueRef = { InColor.R, InColor.G, InColor.B };
						Owner->PostPropertyChanged(this);
						EndEdit();
					}
				})
			];
		ValueWidget->SetEnabled(Writable);
		Item->AddWidget(MoveTemp(ValueWidget));
		return Row;
	}

	template<>
	inline TSharedRef<ITableRow> PropertyUniformItem<Vector4f>::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
		auto MakeSpinBox = [this](int32 ComponentIdx) {
			return SNew(SSpinBox<float>)
				.OnValueCommitted_Lambda([this](float, ETextCommit::Type) { EndEdit(); })
				.OnValueChanged_Lambda([this, ComponentIdx](float NewValue) {
					Vector4f CurrentValue = ValueRef;
					float* ComponentValue = nullptr;
					switch (ComponentIdx)
					{
					case 0: ComponentValue = &CurrentValue.x; break;
					case 1: ComponentValue = &CurrentValue.y; break;
					case 2: ComponentValue = &CurrentValue.z; break;
					default: ComponentValue = &CurrentValue.w; break;
					}
					if (*ComponentValue != NewValue && Owner->CanChangeProperty(this))
					{
						BeginEdit();
						*ComponentValue = NewValue;
						ValueRef = CurrentValue;
						Owner->PostPropertyChanged(this);
					}
				})
				.Value_Lambda([this, ComponentIdx] {
					Vector4f CurrentValue = ValueRef;
					switch (ComponentIdx)
					{
					case 0: return CurrentValue.x;
					case 1: return CurrentValue.y;
					case 2: return CurrentValue.z;
					default: return CurrentValue.w;
					}
				});
		};

		auto ValueWidget = SNew(SWidgetSwitcher)
			.WidgetIndex_Lambda([this] { return bUseColorBlockPicker ? 1 : 0; })
			+ SWidgetSwitcher::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						MakeSpinBox(0)
					]
					+ SHorizontalBox::Slot()
					[
						MakeSpinBox(1)
					]
				]
				+ SVerticalBox::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						MakeSpinBox(2)
					]
					+ SHorizontalBox::Slot()
					[
						MakeSpinBox(3)
					]
				]
			]
			+ SWidgetSwitcher::Slot()
			[
				SNew(SShColorBlockPicker, TWeakPtr<SWindow>())
				.UseSRGB(false)
				.AlphaDisplayMode(EColorBlockAlphaDisplayMode::Separate)
				.ShowBackgroundForAlpha(true)
				.ShowAlpha(true)
				.Color_Lambda([this] {
					Vector4f CurrentValue = ValueRef;
					return FLinearColor{ CurrentValue.x, CurrentValue.y, CurrentValue.z, CurrentValue.w };
				})
				.OnColorChanged_Lambda([this](const FLinearColor& InColor) {
					Vector4f CurrentValue = ValueRef;
					if ((CurrentValue.x != InColor.R || CurrentValue.y != InColor.G || CurrentValue.z != InColor.B || CurrentValue.w != InColor.A) && Owner->CanChangeProperty(this))
					{
						BeginEdit();
						ValueRef = { InColor.R, InColor.G, InColor.B, InColor.A };
						Owner->PostPropertyChanged(this);
						EndEdit();
					}
				})
			];
		ValueWidget->SetEnabled(Writable);
		Item->AddWidget(MoveTemp(ValueWidget));
		return Row;
	}
}
