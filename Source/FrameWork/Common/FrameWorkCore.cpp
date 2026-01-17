#include "CommonHeader.h"
#include "FrameWorkCore.h"
#include "ProjectManager/ProjectManager.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "UI/Widgets/Property/PropertyData/PropertyAssetItem.h"
#include "PluginManager/PluginManager.h"

namespace FW
{
    REFLECTION_REGISTER(AddClass<ShObject>())
    REFLECTION_REGISTER(AddClass<ShObjectOp>())
	
	TArray<TSharedRef<PropertyData>> GeneratePropertyDatas(ShObject* InObject, const MetaMemberData* MetaMemData, void* Instance, bool bForce)
	{
		if (!bForce && !EnumHasAnyFlags(MetaMemData->InfoType, MetaInfo::Property))
		{
			return {};
		}

		TArray<TSharedRef<PropertyData>> Datas;
		
		MetaType* MemberMetaType = MetaMemData->GetMetaType();
		TSharedPtr<PropertyData> Item;
		bool ReadOnly = EnumHasAnyFlags(MetaMemData->InfoType, MetaInfo::ReadOnly);

		const MetaPropertyData& PropertyData = MetaMemData->PropertyData;
		if(MetaMemData->IsAssetRef())
		{
			void* AssetPtrRef = MetaMemData->Get(Instance);
			Item = MakeShared<PropertyAssetItem>(InObject, MetaMemData->MemberName, MemberMetaType, AssetPtrRef);
		}
		//IsEnum
		else if(MetaMemData->GetEnumValueName)
		{
			auto EnumValueName = MakeShared<FString>(MetaMemData->GetEnumValueName(Instance));
			Item = MakeShared<PropertyEnumItem>(InObject, MetaMemData->MemberName, EnumValueName, MetaMemData->EnumEntries, [=](void* NewValue){
				MetaMemData->Set(Instance, NewValue);
			}, ReadOnly);
		}
		else if(MetaMemData->IsType<int32>())
		{
			int32* IntValue = (int32*)MetaMemData->Get(Instance);
			auto ScalarItem = MakeShared<PropertyScalarItem<int32>>(InObject, MetaMemData->MemberName, IntValue, ReadOnly);
			if (PropertyData.Min)
			{
				ScalarItem->SetMinValue(std::any_cast<int32>(PropertyData.Min(InObject)));
			}
			if (PropertyData.Max)
			{
				ScalarItem->SetMaxValue(std::any_cast<int32>(PropertyData.Max(InObject)));
			}
			Item = MoveTemp(ScalarItem);
		}
		else if(MetaMemData->IsType<uint32>())
		{
			uint32* UIntValue = (uint32*)MetaMemData->Get(Instance);
			auto ScalarItem = MakeShared<PropertyScalarItem<uint32>>(InObject, MetaMemData->MemberName, UIntValue, ReadOnly);
			if (PropertyData.Min)
			{
				ScalarItem->SetMinValue(std::any_cast<uint32>(PropertyData.Min(InObject)));
			}
			if (PropertyData.Max)
			{
				ScalarItem->SetMaxValue(std::any_cast<uint32>(PropertyData.Max(InObject)));
			}
			Item = MoveTemp(ScalarItem);
		}
		else if(MetaMemData->IsType<float>())
		{
			float* FloatValue = (float*)MetaMemData->Get(Instance);
			auto ScalarItem = MakeShared<PropertyScalarItem<float>>(InObject, MetaMemData->MemberName, FloatValue, ReadOnly);
			if (PropertyData.Min)
			{
				ScalarItem->SetMinValue(std::any_cast<float>(PropertyData.Min(InObject)));
			}
			if (PropertyData.Max)
			{
				ScalarItem->SetMaxValue(std::any_cast<float>(PropertyData.Max(InObject)));
			}
			Item = MoveTemp(ScalarItem);
		}
		else if(MetaMemData->IsType<double>())
		{
			double* DoubleValue = (double*)MetaMemData->Get(Instance);
			auto ScalarItem = MakeShared<PropertyScalarItem<double>>(InObject, MetaMemData->MemberName, DoubleValue, ReadOnly);
			if (PropertyData.Min)
			{
				ScalarItem->SetMinValue(std::any_cast<double>(PropertyData.Min(InObject)));
			}
			if (PropertyData.Max)
			{
				ScalarItem->SetMaxValue(std::any_cast<double>(PropertyData.Max(InObject)));
			}
			Item = MoveTemp(ScalarItem);
		}
		//Struct/Class
		else if(MemberMetaType && MemberMetaType->Datas.Num() > 0)
		{
			Item = MakeShared<PropertyCategory>(InObject, MetaMemData->MemberName, true);
			void* CompositeInstance = MetaMemData->Get(Instance);
			for(MetaMemberData* ProperyMember : GetProperties(MemberMetaType))
			{
				if(MemberMetaType->IsDerivedFrom<ShObject>())
				{
					Item->AddChilds(GeneratePropertyDatas(static_cast<ShObject*>(CompositeInstance), ProperyMember, CompositeInstance));
				}
				else
				{
					Item->AddChilds(GeneratePropertyDatas(InObject, ProperyMember, CompositeInstance));
				}
				
			}
			
		}
		
		if(Item) {
			Datas.Add(Item.ToSharedRef());
		}
		
		return Datas;
	}

    TArray<TSharedRef<PropertyData>> GeneratePropertyDatas(ShObject* InObject, MetaType* InMetaType)
    {
        TArray<TSharedRef<PropertyData>> Datas;
        TArray<MetaMemberData*> MetaMemDatas = GetProperties(InMetaType);
        for(MetaMemberData* MetaMemData : MetaMemDatas)
        {
			Datas.Append(GeneratePropertyDatas(InObject, MetaMemData, InObject));
        }

		for (PropertyExt* Ext : PropertyExts)
		{
			auto Widget = Ext->CreateWidget();
			if(Ext->TargetType == InMetaType && Widget)
			{
				Datas.Add(MakeShared<PropertyCustomWidget>(Widget->GetSlateWidget()));
			}
		}
        
        return Datas;
    }

    TArray<ShObject*> GlobalValidShObjects;

	ShObject::ShObject() 
		: ObjectName(FText::FromString("Unknown"))
		, Guid(FGuid::NewGuid())
        , NumRefs(0)
        , Outer(nullptr)
	{
        GlobalValidShObjects.Add(this);
	}

    ShObject::~ShObject()
    {
        check(NumRefs == 0);
        GlobalValidShObjects.Remove(this);
    }

	void ShObject::Destroy()
	{
		if (ShObjectOp* Op = GetShObjectOp(this))
		{
			Op->OnCancelSelect(this);
		}
		OnDestroy.Broadcast();
		NumRefs = 0;
		delete this;
	}

    void ShObject::SetOuter(ShObject* InOuter)
    {
        Outer = InOuter;
        if(Outer)
        {
            Outer->SubObjects.Add(this);
        }
    }

    AssetObject* ShObject::GetOuterMost()
    {
		if (IsDefaultObject)
		{
			return nullptr;
		}

        ShObject* Cur = this;
        ShObject* CurOuter = GetOuter();
        while(CurOuter)
        {
            Cur = CurOuter;
            CurOuter = CurOuter->GetOuter();
        }
        
        checkf(Cur->DynamicMetaType()->IsDerivedFrom<AssetObject>(), TEXT("The OuterMost should be an AssetObject"));
        return static_cast<AssetObject*>(Cur);
    }

	void ShObject::Serialize(FArchive& Ar)
	{
		Ar << Guid;
		ShSerializeText(Ar, ObjectName);
	}

    void ShObject::PostLoad()
    {
        //PostLoad subobjects
        for(auto It = SubObjects.CreateIterator(); It; It++)
        {
            if(*It)
            {
                (*It)->PostLoad();
            }
            else
            {
                It.RemoveCurrent();
            }
        }
    }

    void ShObject::PostPropertyChanged(PropertyData* InProperty)
    {
        GetOuterMost()->MarkDirty();
    }

    TArray<TSharedRef<PropertyData>>* ShObject::GetPropertyDatas()
    {
        PropertyDatas = GeneratePropertyDatas(this, this->DynamicMetaType());
        return &PropertyDatas;
    }
    
    ShObjectOp* GetShObjectOp(ShObject* InObject)
    {
        return GetDefaultObject<ShObjectOp>([InObject](ShObjectOp* CurShObjectOp) {
            return CurShObjectOp->SupportType() == InObject->DynamicMetaType();
        });
    }
}
