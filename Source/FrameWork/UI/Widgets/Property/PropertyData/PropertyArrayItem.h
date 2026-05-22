#pragma once

#include "PropertyData.h"

#include "UI/Styles/FAppCommonStyle.h"
#include "UI/Widgets/Misc/MiscWidget.h"

#include <Widgets/Input/SNumericEntryBox.h>
#include <Widgets/Views/STreeView.h>

namespace FW
{
	class PropertyArrayControlsItem : public PropertyData
	{
	public:
		PropertyArrayControlsItem(ShObject* InOwner, TFunction<void()> InAdd, TFunction<void()> InRemove, TFunction<bool()> InCanRemove)
			: PropertyData(InOwner, FText::GetEmpty())
			, Add(MoveTemp(InAdd))
			, Remove(MoveTemp(InRemove))
			, CanRemove(MoveTemp(InCanRemove))
		{
			check(Add);
			check(Remove);
			check(CanRemove);
		}

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
		{
			auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable).Padding(FMargin{0, 0, 0, 2});
			Row->SetExpanderArrowVisibility(EVisibility::Collapsed);
			Row->SetRowContent(
				SNew(SBorder)
				.Padding(FMargin{3.0f, 0.0f, 3.0f, 2.0f})
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Input"))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNullWidget::NullWidget
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SIconButton)
						.Icon(FAppStyle::Get().GetBrush("Icons.Plus"))
						.IconSize(FVector2D(10.0f, 10.0f))
						.IsFocusable(false)
						.OnClicked_Lambda([this] {
							Add();
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SIconButton)
						.Icon(FAppStyle::Get().GetBrush("Icons.Minus"))
						.IconSize(FVector2D(10.0f, 10.0f))
						.IsFocusable(false)
						.IsEnabled_Lambda([this] { return CanRemove(); })
						.OnClicked_Lambda([this] {
							Remove();
							return FReply::Handled();
						})
					]
				]
			);
			return Row;
		}

	private:
		TFunction<void()> Add;
		TFunction<void()> Remove;
		TFunction<bool()> CanRemove;
	};

	class PropertyArrayItem : public PropertyData
	{
		MANUAL_RTTI_TYPE(PropertyArrayItem, PropertyData)
	public:
		PropertyArrayItem(ShObject* InOwner, const FString& InName, TFunction<int32()> InGetNum, TFunction<void(int32)> InSetNum, bool InReadOnly = false)
			: PropertyArrayItem(InOwner, FText::FromString(InName), MoveTemp(InGetNum), MoveTemp(InSetNum), InReadOnly)
		{}

		PropertyArrayItem(ShObject* InOwner, FText InName, TFunction<int32()> InGetNum, TFunction<void(int32)> InSetNum, bool InReadOnly = false)
			: PropertyData(InOwner, MoveTemp(InName))
			, GetNumFunc(MoveTemp(InGetNum))
			, SetNum(MoveTemp(InSetNum))
			, ReadOnly(InReadOnly)
		{
			check(GetNumFunc);
			check(SetNum);
			Expanded = true;
		}

		void SetRebuildChildren(TFunction<void(PropertyArrayItem&)> InRebuildChildren)
		{
			check(InRebuildChildren);
			RebuildChildren = MoveTemp(InRebuildChildren);
			RefreshChildren();
		}

		void SetRemoveAt(TFunction<void(int32)> InRemoveAt)
		{
			check(InRemoveAt);
			RemoveAt = MoveTemp(InRemoveAt);
		}

		int32 GetNum() const { return GetNumFunc(); }
		int32 GetPendingNum() const { return PendingNum; }

		int32 GetSelectedElementIndex() const
		{
			const int32 NumElements = GetNum();
			if (TSharedPtr<STreeView<TSharedRef<PropertyData>>> OwnerTree = OwnerTreeWeak.Pin())
			{
				const TArray<TSharedRef<PropertyData>> SelectedItems = OwnerTree->GetSelectedItems();
				for (const TSharedRef<PropertyData>& SelectedItem : SelectedItems)
				{
					PropertyData* CurItem = &SelectedItem.Get();
					while (CurItem && CurItem->GetParent() != this)
					{
						CurItem = CurItem->GetParent();
					}
					if (CurItem)
					{
						const int32 ElementIndex = Children.IndexOfByPredicate([CurItem](const TSharedRef<PropertyData>& Child) {
							return &Child.Get() == CurItem;
						});
						if (ElementIndex >= 0 && ElementIndex < NumElements)
						{
							CachedSelectedElementIndex = ElementIndex;
							return ElementIndex;
						}
						if (ElementIndex == NumElements && CachedSelectedElementIndex >= 0 && CachedSelectedElementIndex < NumElements)
						{
							return CachedSelectedElementIndex;
						}
					}
				}
			}
			return INDEX_NONE;
		}

		void RefreshChildren()
		{
			Children.Reset();
			RebuildChildren(*this);
			if (!ReadOnly)
			{
				AddChild(MakeShared<PropertyArrayControlsItem>(Owner,
					[this] { ApplyNum(GetNum() + 1); },
					[this] { RemoveSelectedElement(); },
					[this] { return GetSelectedElementIndex() != INDEX_NONE; }
				));
			}
		}

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
		{
			OwnerTableWeak = OwnerTable;
			OwnerTreeWeak = StaticCastSharedRef<STreeView<TSharedRef<PropertyData>>>(OwnerTable);
			auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable);

			const bool bRootCategory = Parent == nullptr;
			const FSlateBrush* CategoryBrush = bRootCategory
				? FAppCommonStyle::Get().GetBrush("PropertyView.CategoryColor")
				: FAppCommonStyle::Get().GetBrush("PropertyView.ItemColor");
			FSlateFontInfo CategoryTextFont = bRootCategory
				? FAppStyle::Get().GetFontStyle("NormalFontBold")
				: FAppStyle::Get().GetFontStyle("NormalFont");

			Row->SetRowContent(
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
				.Padding(FMargin{0.0f, 3.0f, 0.0f, 0.0f})
				[
					SNew(SBorder)
					.BorderImage(CategoryBrush)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SExpanderArrow, Row)
							.IndentAmount(0)
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						.Padding(4.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(DisplayName)
							.Font(MoveTemp(CategoryTextFont))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4.0f, 1.0f)
						[
							SNew(SNumericEntryBox<int32>)
							.IsEnabled(!ReadOnly)
							.AllowSpin(false)
							.MinValue(0)
							.MinDesiredValueWidth(58.0f)
							.Value_Lambda([this] { return GetNum(); })
							.OnValueCommitted_Lambda([this](int32 NewNum, ETextCommit::Type) {
								ApplyNum(NewNum);
							})
						]
					]
				]
			);

			return Row;
		}

	private:
		void RemoveSelectedElement()
		{
			const int32 ElementIndex = GetSelectedElementIndex();
			if (ElementIndex == INDEX_NONE || ReadOnly)
			{
				return;
			}
			PendingNum = GetNum() - 1;
			const bool bCanChange = !Owner || Owner->CanChangeProperty(this);
			if (!bCanChange)
			{
				return;
			}

			BeginEdit();
			RemoveAt(ElementIndex);
			RefreshChildren();
			if (Owner)
			{
				Owner->PostPropertyChanged(this);
			}
			EndEdit();

			if (TSharedPtr<STableViewBase> OwnerTable = OwnerTableWeak.Pin())
			{
				OwnerTable->RequestListRefresh();
			}
		}

		void ApplyNum(int32 NewNum)
		{
			if (ReadOnly)
			{
				return;
			}
			if (NewNum == GetNum() || NewNum == PendingNum)
			{
				return;
			}
			PendingNum = NewNum;
			const bool bCanChange = !Owner || Owner->CanChangeProperty(this);
			if (!bCanChange)
			{
				return;
			}

			BeginEdit();
			SetNum(NewNum);
			RefreshChildren();
			if (Owner)
			{
				Owner->PostPropertyChanged(this);
			}
			EndEdit();

			if (TSharedPtr<STableViewBase> OwnerTable = OwnerTableWeak.Pin())
			{
				OwnerTable->RequestListRefresh();
			}

		}

		TFunction<int32()> GetNumFunc;
		TFunction<void(int32)> SetNum;
		TFunction<void(int32)> RemoveAt;
		TFunction<void(PropertyArrayItem&)> RebuildChildren;
		TWeakPtr<STableViewBase> OwnerTableWeak;
		TWeakPtr<STreeView<TSharedRef<PropertyData>>> OwnerTreeWeak;
		mutable int32 CachedSelectedElementIndex = INDEX_NONE;
		int32 PendingNum = INDEX_NONE;
		bool ReadOnly;
	};
}
