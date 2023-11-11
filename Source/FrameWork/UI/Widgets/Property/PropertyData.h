#pragma once

namespace FRAMEWORK
{
	class IPropertyData
	{
	public:
		virtual TSharedRef<ITableRow> GenerateWidgetForTableView() = 0;
		virtual void GetChildren(TArray<TSharedRef<IPropertyData>>& OutChildren) = 0;
	};

	class PropertyData : public IPropertyData
	{
	public:
		TSharedRef<ITableRow> GenerateWidgetForTableView() override;
		void GetChildren(TArray<TSharedRef<IPropertyData>>& OutChildren) override;

	private:
		TArray<TSharedPtr<PropertyData>> Children;
	};
	
}


