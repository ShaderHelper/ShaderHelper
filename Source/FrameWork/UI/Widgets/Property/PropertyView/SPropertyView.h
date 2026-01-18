#pragma once
#include "UI/Widgets/Property/PropertyData/PropertyData.h"

namespace FW
{
	class FRAMEWORK_API SPropertyView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPropertyView)
			: _ObjectData(nullptr)
		{}
            SLATE_ARGUMENT(ShObject*, ObjectData)
		SLATE_END_ARGS()

        void Construct(const FArguments& InArgs);
	public:
        void SetObjectData(ShObject* InObjectData);

		TArray<TSharedRef<PropertyData>> GetSelectedItems() const
		{
			return PropertyTree->GetSelectedItems();
		}
        
        TArray<TSharedRef<PropertyData>> GetLinearizedItems(TSharedRef<PropertyData> InItem) const;

		void Refresh(bool bClear = false)
		{
            if(PropertyTree)
            {
				if (bClear)
				{
					ObjectData = nullptr;
					PropertyDatas = nullptr;
				}
				else
				{
					for (const auto& Data : *PropertyDatas)
					{
						TryExpandItemRecursively(Data);
					}
				}
                PropertyTree->RequestTreeRefresh();
            }

		}

		bool IsLocked() const { return Locked; }

		void TryExpandItemRecursively(TSharedRef<PropertyData> InItem)
		{
            if(InItem->Expanded)
            {
                ExpandItem(InItem);
            }

			TArray<TSharedRef<PropertyData>> Children;
			InItem->GetChildren(Children);
			for (const auto& Child : Children)
			{
                TryExpandItemRecursively(Child);
			}
		}

		void ExpandItem(TSharedRef<PropertyData> InItem)
		{
			PropertyTree->SetItemExpansion(InItem, true);
		}

	private:
        ObserverObjectPtr<ShObject> ObjectData;
        bool Locked = false;
		TArray<TSharedRef<PropertyData>>* PropertyDatas;
		TSharedPtr<STreeView<TSharedRef<PropertyData>>> PropertyTree;
		
	};
}


