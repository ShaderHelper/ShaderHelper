#include "CommonHeader.h"
#include "FrameWorkCore.h"
#include "ProjectManager/ProjectManager.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "UI/Widgets/Property/PropertyData/PropertyAssetItem.h"

namespace FW
{
    REFLECTION_REGISTER(AddClass<ShObject>())
    REFLECTION_REGISTER(AddClass<ShObjectOp>())

    TArray<TSharedRef<PropertyData>> GeneratePropertyDatas(ShObject* InObject)
    {
        TArray<TSharedRef<PropertyData>> Datas;
        TArray<MetaMemberData*> MetaMemDatas = GetProperties(InObject->DynamicMetaType());
        for(MetaMemberData* MetaMemData : MetaMemDatas)
        {
            if(MetaMemData->IsAssetRef())
            {
                void* AssetPtrRef = MetaMemData->Get(InObject);
                TSharedRef<PropertyData> Item = MakeShared<PropertyAssetItem>(InObject, MetaMemData->MemberName, MetaMemData->GetShObjectMetaType(), AssetPtrRef);
                Datas.Add(MoveTemp(Item));
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
        if(PropertyDatas.IsEmpty())
        {
            PropertyDatas = GeneratePropertyDatas(this);
        }
        return &PropertyDatas;
    }
    
    ShObjectOp* GetShObjectOp(ShObject* InObject)
    {
        return GetDefaultObject<ShObjectOp>([InObject](ShObjectOp* CurShObjectOp) {
            return CurShObjectOp->SupportType() == InObject->DynamicMetaType();
        });
    }
}
