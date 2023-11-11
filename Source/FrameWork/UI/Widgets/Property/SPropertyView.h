#pragma once
#include "PropertyData.h"
#include <type_traits>
namespace FRAMEWORK
{
	template<typename PropertyDataType, typename = void>
	class SPropertyView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPropertyView)
			: _PropertyDatas(nullptr)
		{}
			SLATE_ARGUMENT(const TArray<PropertyDataType>*, PropertyDatas)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ChildSlot
			[
				SAssignNew(PropertyTree, STreeView<PropertyDataType>)
				.TreeItemsSource(InArgs._PropertyDatas)
				.OnGetChildren([](PropertyDataType InTreeNode, TArray<PropertyDataType>& OutChildren) {
					InTreeNode->GetChildren(OutChildren);
				})
				.OnGenerateRow([](PropertyDataType InTreeNode, const TSharedRef<STableViewBase>& OwnerTable) {
					return InTreeNode->GenerateWidgetForTableView();
				})
			];
		}

	private:
		TSharedPtr<STreeView<PropertyDataType>> PropertyTree;
		
	};

	template<typename PropertyDataType>
	class SPropertyView<PropertyDataType, std::enable_if_t<!std::is_base_of_v<IPropertyData, PropertyDataType>>>
	{
		static_assert(AUX::AlwaysFalse<PropertyDataType>, "PropertyDataType must be derived from IPropertyData.");
	};
}


