#pragma once

namespace FRAMEWORK
{
	class PropertyData
	{
	public:
		PropertyData(FString InName) : DisplayName(MoveTemp(InName))
		{}
		virtual ~PropertyData() = default;


		virtual TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) = 0;
		void GetChildren(TArray<TSharedRef<PropertyData>>& OutChildren) { OutChildren = Children; };
		void AddChild(TSharedRef<PropertyData> InChild) { Children.Add(InChild); }

	protected:
		FString DisplayName;
		TArray<TSharedRef<PropertyData>> Children;
	};

	class PropertyCategory : public PropertyData
	{
	public:
		PropertyCategory(FString InName)
			: PropertyData(MoveTemp(InName))
		{}

		void SetAddMenuWidget(TSharedPtr<SWidget> InWidget) { AddMenuWidget = MoveTemp(InWidget); }

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;

	private:
		TSharedPtr<SWidget> AddMenuWidget;
	};

	template<typename T>
	class PropertyNumber : public PropertyData
	{
	public:
		PropertyNumber(FString InName, const T& InValue)
			: PropertyData(MoveTemp(InName))
			, Value(InValue)
		{}

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;

	private:
		T Value;
	};

}

#include "PropertyData.hpp"


