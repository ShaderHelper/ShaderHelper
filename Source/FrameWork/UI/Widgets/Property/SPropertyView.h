#pragma once
#include "PropertyData.h"
namespace FRAMEWORK
{
	class SPropertyView : public SCompoundWidget
	{
		using PropertyDataType = TSharedRef<PropertyData>;
	public:
		SLATE_BEGIN_ARGS(SPropertyView)
			: _PropertyDatas(nullptr)
		{}
			SLATE_ARGUMENT(TArray<PropertyDataType>*, PropertyDatas)
			SLATE_ARGUMENT(bool, IsExpandAll)
			SLATE_EVENT(FOnContextMenuOpening, OnContextMenuOpening)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			PropertyDatas = InArgs._PropertyDatas;
			ChildSlot
			[
				SNew(SBorder)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
					.Padding(0)
					[
						SAssignNew(PropertyTree, STreeView<PropertyDataType>)
						.TreeItemsSource(InArgs._PropertyDatas)
						.OnGetChildren_Lambda([](PropertyDataType InTreeNode, TArray<PropertyDataType>& OutChildren) {
							InTreeNode->GetChildren(OutChildren);
						})
						.OnGenerateRow_Lambda([](PropertyDataType InTreeNode, const TSharedRef<STableViewBase>& OwnerTable) {
							return InTreeNode->GenerateWidgetForTableView(OwnerTable);
						})
						.SelectionMode(ESelectionMode::Single)
						.OnContextMenuOpening(InArgs._OnContextMenuOpening)
					]
				]
			
			
			];

			if (InArgs._IsExpandAll)
			{
				ExpandAllPropertyData(true);
			}
		}
	public:

		TArray<PropertyDataType> GetSelectedItems() const
		{
			return PropertyTree->GetSelectedItems();
		}

		void Refresh()
		{
			PropertyTree->RequestTreeRefresh();
		}

		void ExpandItemRecursively(TSharedRef<PropertyData> InItem)
		{
			PropertyTree->SetItemExpansion(InItem, true);

			TArray<TSharedRef<PropertyData>> Children;
			InItem->GetChildren(Children);
			for (const auto& Child : Children)
			{
				ExpandItemRecursively(Child);
			}
		}

		void ExpandItem(TSharedRef<PropertyData> InItem)
		{
			PropertyTree->SetItemExpansion(InItem, true);
		}

		void ExpandAllPropertyData(bool IsRecursive)
		{
			for (const auto& Data : *PropertyDatas)
			{
				if (IsRecursive)
				{
					ExpandItemRecursively(Data);
				}
				else
				{
					ExpandItem(Data);
				}
			}
		}

	private:
		TArray<PropertyDataType>* PropertyDatas;
		TSharedPtr<STreeView<PropertyDataType>> PropertyTree;
		
	};
}


