#pragma once
#include "SPropertyCategory.h"
#include "UI/Styles/FAppCommonStyle.h"

namespace FW
{
	class PropertyData
	{
        MANUAL_RTTI_BASE_TYPE()
	public:
		PropertyData(ShObject* InOwner, const FString& InName) : PropertyData(InOwner, FText::FromString(InName)) {}
		PropertyData(ShObject* InOwner, FText InName)
            : Owner(InOwner)
            , DisplayName(MoveTemp(InName))
		{}
		virtual ~PropertyData() = default;


		virtual TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) = 0;
		void GetChildren(TArray<TSharedRef<PropertyData>>& OutChildren) const { OutChildren = Children; };
		int32 GetChildrenNum() const { return Children.Num(); }
		FText GetDisplayName() const { return DisplayName; }

		PropertyData* GetParent() const { return Parent; }

		void AddChild(TSharedRef<PropertyData> InChild) 
		{ 
			InChild->Parent = this;
			Children.Add(InChild); 
		}
		
		void AddChilds(const TArray<TSharedRef<PropertyData>>& InChilds)
		{
			for(const auto& Child : InChilds)
			{
				AddChild(Child);
			}
		}

		void Remove()
		{
			check(Parent);
			Parent->RemoveChild(this);
		}

		void RemoveChild(PropertyData* InChild)
		{
            Children.RemoveAll([=](TSharedRef<PropertyData>& Item){
                return InChild == &*Item;
            });
		}
        
		TSharedPtr<PropertyData> GetData(const FString& InName) const { return GetData(FText::FromString(InName)); }
        TSharedPtr<PropertyData> GetData(const FText& InName) const
        {
            for(auto Child : Children)
            {
                if(Child->GetDisplayName().EqualTo(InName))
                {
                    return Child;
                }
            }
            return {};
        }
    public:
        bool Expanded = true;

	protected:
        ShObject* Owner;
		FText DisplayName;
		PropertyData* Parent = nullptr;
		TArray<TSharedRef<PropertyData>> Children;
	};

	class PropertyCategory : public PropertyData
	{
        MANUAL_RTTI_TYPE(PropertyCategory, PropertyData)
	public:
		PropertyCategory(ShObject* InOwner, const FString& InName, bool IsComposite = false) : PropertyCategory(InOwner, FText::FromString(InName), IsComposite) {}
		PropertyCategory(ShObject* InOwner, FText InName, bool IsComposite = false)
		: PropertyData(InOwner, MoveTemp(InName))
		, bComposite(IsComposite)
		{
			Expanded = !IsComposite;
		}

		void SetAddMenuWidget(TSharedPtr<SWidget> InWidget) { AddMenuWidget = MoveTemp(InWidget); }
		bool IsRootCategory() const { return Parent == nullptr; }
		bool IsComposite() const { return bComposite; }

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
        {
            auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable);
			
			const FSlateBrush* CategoryBrush = nullptr;
			if(bComposite)
			{
				CategoryBrush = FAppCommonStyle::Get().GetBrush("PropertyView.CompositeItemColor");
			}

            TSharedRef<SPropertyCatergory> RowContent = SNew(SPropertyCatergory, Row)
                .DisplayName(DisplayName)
                .IsRootCategory(IsRootCategory())
				.CategoryBrush(CategoryBrush)
                .AddMenuWidget(AddMenuWidget);
			
            Row->SetRowContent(RowContent);
            return Row;
        }
	private:
		bool bComposite;
		TSharedPtr<SWidget> AddMenuWidget;
	};

	class PropertyCustomWidget : public PropertyData
	{
		MANUAL_RTTI_TYPE(PropertyCustomWidget, PropertyData)
	public:
		PropertyCustomWidget(TSharedRef<SWidget> InCustomWidget)
		: PropertyData(nullptr, FText::GetEmpty())
		, CustomWidget(InCustomWidget)
		{
			
		}

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
		{
			auto Row = SNew(STableRow<TSharedRef<PropertyData>>, OwnerTable).Padding(FMargin{ 0,4,0,4 });
			Row->SetExpanderArrowVisibility(EVisibility::Collapsed);
			Row->SetContent(CustomWidget.ToSharedRef());
			return Row;
		}
	private:
		TSharedPtr<SWidget> CustomWidget;
	};
}



