#pragma once
#include "SPropertyCategory.h"

namespace FW
{
	class PropertyData : public TSharedFromThis<PropertyData>
	{
        MANUAL_RTTI_BASE_TYPE()
	public:
		PropertyData(FString InName) : DisplayName(MoveTemp(InName))
		{}
		virtual ~PropertyData() = default;


		virtual TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) = 0;
		void GetChildren(TArray<TSharedRef<PropertyData>>& OutChildren) const { OutChildren = Children; };
		int32 GetChildrenNum() const { return Children.Num(); }
		FString GetDisplayName() const { return DisplayName; }
		PropertyData* GetParent() const { return Parent; }

		void AddChild(TSharedRef<PropertyData> InChild) 
		{ 
			InChild->Parent = this;
			Children.Add(InChild); 
		}

		void Remove()
		{
			check(Parent);
			Parent->RemoveChild(AsShared());
		}

		void RemoveChild(TSharedRef<PropertyData> InChild)
		{
			Children.Remove(InChild);
		}
        
        TSharedPtr<PropertyData> GetData(const FString& InName) const
        {
            for(auto Child : Children)
            {
                if(Child->GetDisplayName() == InName)
                {
                    return Child;
                }
            }
            return {};
        }
    public:
        bool Expanded = false;

	protected:
		FString DisplayName;
		PropertyData* Parent = nullptr;
		TArray<TSharedRef<PropertyData>> Children;
	};

	class PropertyCategory : public PropertyData
	{
        MANUAL_RTTI_TYPE(PropertyCategory, PropertyData)
	public:
		PropertyCategory(FString InName)
			: PropertyData(MoveTemp(InName))
		{}

		void SetAddMenuWidget(TSharedPtr<SWidget> InWidget) { AddMenuWidget = MoveTemp(InWidget); }
		bool IsRootCategory() const { return Parent == nullptr; }

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
        {
            auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable);

            TSharedRef<SPropertyCatergory> RowContent = SNew(SPropertyCatergory, Row)
                .DisplayName(DisplayName)
                .IsRootCategory(IsRootCategory())
                .AddMenuWidget(AddMenuWidget);
            Row->SetRowContent(RowContent);
            return Row;
        }
	private:
		TSharedPtr<SWidget> AddMenuWidget;
	};
}



