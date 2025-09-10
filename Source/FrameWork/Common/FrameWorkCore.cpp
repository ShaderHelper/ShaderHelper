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
	
	TArray<TSharedRef<PropertyData>> GeneratePropertyDatas(ShObject* InObject, const MetaMemberData* MetaMemData, void* Instance)
	{
		TArray<TSharedRef<PropertyData>> Datas;
		
		MetaType* MemberMetaType = MetaMemData->GetMetaType();
		TSharedPtr<PropertyData> Item;
		bool ReadOnly = EnumHasAnyFlags(MetaMemData->InfoType, MetaInfo::ReadOnly);
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
			});
		}
		else if(MetaMemData->IsType<int32>())
		{
			int* IntValue = (int*)MetaMemData->Get(Instance);
			Item = MakeShared<PropertyScalarItem<int32>>(InObject, MetaMemData->MemberName, IntValue, ReadOnly);
		}
		else if(MetaMemData->IsType<uint32>())
		{
			uint32* UIntValue = (uint32*)MetaMemData->Get(Instance);
			Item = MakeShared<PropertyScalarItem<uint32>>(InObject, MetaMemData->MemberName, UIntValue, ReadOnly);
		}
		else if(MetaMemData->IsType<float>())
		{
			float* FloatValue = (float*)MetaMemData->Get(Instance);
			Item = MakeShared<PropertyScalarItem<float>>(InObject, MetaMemData->MemberName, FloatValue, ReadOnly);
		}
		else if(MetaMemData->IsType<double>())
		{
			double* DoubleValue = (double*)MetaMemData->Get(Instance);
			Item = MakeShared<PropertyScalarItem<double>>(InObject, MetaMemData->MemberName, DoubleValue, ReadOnly);
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
		Ar << ObjectName;
		Ar << GFrameWorkVer << GProjectVer;
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
