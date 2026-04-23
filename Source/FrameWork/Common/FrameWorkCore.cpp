#include "CommonHeader.h"
#include "FrameWorkCore.h"
#include "ProjectManager/ProjectManager.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "UI/Widgets/Property/PropertyData/PropertyAssetItem.h"
#include "UI/Widgets/Property/PropertyData/PropertyObjectItem.h"
#include "PluginManager/PluginManager.h"

namespace FW
{
    REFLECTION_REGISTER(AddClass<ShObject>())
    REFLECTION_REGISTER(AddClass<ShObjectOp>())

	namespace
	{
		struct ObjectPtrFixupContext
		{
			TMap<FGuid, ShObject*> GuidToObject;
			TArray<ObjectPtrFixupRequest> Requests;
		};

		TArray<TUniquePtr<ObjectPtrFixupContext>>& GetObjectPtrFixupStack()
		{
			static TArray<TUniquePtr<ObjectPtrFixupContext>> FixupStack;
			return FixupStack;
		}

		ObjectPtrFixupContext* GetCurrentObjectPtrFixupContext()
		{
			auto& FixupStack = GetObjectPtrFixupStack();
			return FixupStack.IsEmpty() ? nullptr : FixupStack.Last().Get();
		}
	}

	void BeginObjectPtrFixup()
	{
		GetObjectPtrFixupStack().Add(MakeUnique<ObjectPtrFixupContext>());
	}

	void ResolveObjectPtrFixups()
	{
		ObjectPtrFixupContext* Context = GetCurrentObjectPtrFixupContext();
		if (!Context)
		{
			return;
		}

		for (const ObjectPtrFixupRequest& Request : Context->Requests)
		{
			if (!Request.Guid.IsValid())
			{
				continue;
			}

			ShObject** ResolvedObjectPtr = Context->GuidToObject.Find(Request.Guid);
			if (!ResolvedObjectPtr || !*ResolvedObjectPtr)
			{
				SH_LOG(LogFrameWorkCore, Warning, TEXT("Failed to resolve object pointer guid: %s"), *Request.Guid.ToString());
				continue;
			}

			bool bTypeMatched = false;
			for (MetaType* CurMetaType = (*ResolvedObjectPtr)->DynamicMetaType(); CurMetaType; CurMetaType = CurMetaType->GetBaseClass())
			{
				if (CurMetaType == Request.TargetMetaType)
				{
					bTypeMatched = true;
					break;
				}
			}

			if (!bTypeMatched)
			{
				SH_LOG(LogFrameWorkCore, Warning, TEXT("Resolved guid %s with mismatched type."), *Request.Guid.ToString());
				continue;
			}

			Request.AssignResolvedObject(Request.ObjectPtrAddress, *ResolvedObjectPtr);
		}
	}

	void EndObjectPtrFixup()
	{
		auto& FixupStack = GetObjectPtrFixupStack();
		check(!FixupStack.IsEmpty());
		FixupStack.Pop();
	}

	void RegisterLoadedShObject(ShObject* InObject)
	{
		if (ObjectPtrFixupContext* Context = GetCurrentObjectPtrFixupContext())
		{
			Context->GuidToObject.FindOrAdd(InObject->GetGuid()) = InObject;
		}
	}

	void RegisterObjectPtrFixup(const ObjectPtrFixupRequest& InRequest)
	{
		if (ObjectPtrFixupContext* Context = GetCurrentObjectPtrFixupContext())
		{
			Context->Requests.Add(InRequest);
		}
	}
	
	TArray<TSharedRef<PropertyData>> GeneratePropertyDatas(ShObject* InObject, const MetaMemberData* MetaMemData, void* Instance, bool bForce)
	{
		if (!bForce && !EnumHasAnyFlags(MetaMemData->InfoType, MetaInfo::Property))
		{
			return {};
		}

		const MetaPropertyData& PropData = MetaMemData->PropertyData;
		if (PropData.Visibility)
		{
			bool bVisible = std::any_cast<bool>(PropData.Visibility(Instance));
			if (!bVisible)
			{
				return {};
			}
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
		else if (MetaMemData->IsShObjectRef())
		{
			Item = MakeShared<PropertyObjectItem>(
				InObject,
				MetaMemData->MemberName,
				MemberMetaType,
				[MetaMemData, Instance]() {
					return MetaMemData->GetReferencedShObject ? MetaMemData->GetReferencedShObject(Instance) : nullptr;
				},
				[MetaMemData, Instance](ShObject* NewObject) {
					if (MetaMemData->SetReferencedShObject)
					{
						MetaMemData->SetReferencedShObject(Instance, NewObject);
					}
				},
				ReadOnly
			);
		}
		//IsEnum
		else if(MetaMemData->GetEnumValueName)
		{
			auto EnumValueName = MakeShared<FString>(MetaMemData->GetEnumValueName(Instance));
			Item = MakeShared<PropertyEnumItem>(InObject, MetaMemData->MemberName, EnumValueName, MetaMemData->EnumEntries, [=](void* NewValue){
				MetaMemData->Set(Instance, NewValue);
			}, ReadOnly);
		}
		else if (MetaMemData->IsType<bool>())
		{
			bool* BoolValue = (bool*)MetaMemData->Get(Instance);
			Item = MakeShared<PropertyScalarItem<bool>>(InObject, MetaMemData->MemberName, BoolValue, ReadOnly);
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
		else if (MetaMemData->IsType<FString>())
		{
			FString* StringValue = (FString*)MetaMemData->Get(Instance);
			Item = MakeShared<PropertyStringItem>(InObject, MetaMemData->MemberName, StringValue, ReadOnly);
		}
		else if(MetaMemData->IsType<Vector2f>())
		{
			Vector2f* VecValue = (Vector2f*)MetaMemData->Get(Instance);
			Item = MakeShared<PropertyVector2fItem>(InObject, MetaMemData->MemberName, VecValue, ReadOnly);
		}
		else if(MetaMemData->IsType<Vector3f>())
		{
			Vector3f* VecValue = (Vector3f*)MetaMemData->Get(Instance);
			Item = MakeShared<PropertyVector3fItem>(InObject, MetaMemData->MemberName, VecValue, ReadOnly);
		}
		else if(MetaMemData->IsType<Vector4f>())
		{
			Vector4f* VecValue = (Vector4f*)MetaMemData->Get(Instance);
			Item = MakeShared<PropertyVector4fItem>(InObject, MetaMemData->MemberName, VecValue, ReadOnly);
		}
		else if(MetaMemData->IsType<Vector2i>())
		{
			Vector2i* VecValue = (Vector2i*)MetaMemData->Get(Instance);
			Item = MakeShared<PropertyVector2iItem>(InObject, MetaMemData->MemberName, &VecValue->x, ReadOnly);
		}
		else if(MetaMemData->IsType<Vector3i>())
		{
			Vector3i* VecValue = (Vector3i*)MetaMemData->Get(Instance);
			Item = MakeShared<PropertyVector3iItem>(InObject, MetaMemData->MemberName, &VecValue->x, ReadOnly);
		}
		else if(MetaMemData->IsType<Vector4i>())
		{
			Vector4i* VecValue = (Vector4i*)MetaMemData->Get(Instance);
			Item = MakeShared<PropertyVector4iItem>(InObject, MetaMemData->MemberName, &VecValue->x, ReadOnly);
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

		// Collect properties from base classes first
		MetaType* BaseType = InMetaType->GetBaseClass();
		if (BaseType)
		{
			Datas.Append(GeneratePropertyDatas(InObject, BaseType));
		}

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
		if (GlobalValidShObjects.Contains(this))
		{
			OnDestroy.Broadcast();
			NumRefs = 0;
			delete this;
		}
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
		if (Ar.IsLoading())
		{
			RegisterLoadedShObject(this);
		}
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

    TArray<TSharedRef<PropertyData>> ShObject::GeneratePropertyDatas()
    {
        return FW::GeneratePropertyDatas(this, this->DynamicMetaType());
    }
    
    ShObjectOp* GetShObjectOp(ShObject* InObject)
    {
		if (!InObject) return nullptr;

        return GetDefaultObject<ShObjectOp>([InObject](ShObjectOp* CurShObjectOp) {
            return CurShObjectOp->SupportType() == InObject->DynamicMetaType();
        });
    }
}
