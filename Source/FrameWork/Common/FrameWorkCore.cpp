#include "CommonHeader.h"
#include "FrameWorkCore.h"
#include "ProjectManager/ProjectManager.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "UI/Widgets/Property/PropertyData/PropertyArrayItem.h"
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

	namespace
	{
		struct PropertyValueAccess
		{
			FText DisplayName;
			FString TypeName;
			void* ValuePtr = nullptr;
			TFunction<void(void*)> SetValue;
			TFunction<MetaType*()> GetMetaType;
			TFunction<ShObject*()> GetReferencedShObject;
			TFunction<void(ShObject*)> SetReferencedShObject;
			TFunction<FString()> GetEnumValueName;
			TMap<FString, TSharedPtr<void>> EnumEntries;
			const MetaPropertyData* PropertyData = nullptr;
			void* VisibilityInstance = nullptr;
			bool ReadOnly = false;
		};

		template<typename ValueType>
		ValueType* CastPropertyValue(void* ValuePtr)
		{
			return static_cast<ValueType*>(ValuePtr);
		}

		TSharedPtr<PropertyData> GeneratePropertyDataForValue(ShObject* InObject, const PropertyValueAccess& Access)
		{
			MetaType* MemberMetaType = Access.GetMetaType ? Access.GetMetaType() : nullptr;
			TSharedPtr<PropertyData> Item;

			if (MemberMetaType && MemberMetaType->IsDerivedFrom<AssetObject>() && Access.GetReferencedShObject)
			{
				Item = MakeShared<PropertyAssetItem>(InObject, Access.DisplayName, MemberMetaType, Access.ValuePtr);
			}
			else if (MemberMetaType && MemberMetaType->IsDerivedFrom<ShObject>() && Access.GetReferencedShObject)
			{
				Item = MakeShared<PropertyObjectItem>(
					InObject,
					Access.DisplayName,
					MemberMetaType,
					Access.GetReferencedShObject,
					Access.SetReferencedShObject,
					Access.ReadOnly
				);
			}
			else if (Access.GetEnumValueName)
			{
				auto EnumValueName = MakeShared<FString>(Access.GetEnumValueName());
				Item = MakeShared<PropertyEnumItem>(InObject, Access.DisplayName, EnumValueName, Access.EnumEntries, Access.SetValue, Access.ReadOnly);
			}
			else if (Access.TypeName == AUX::TypeName<bool>)
			{
				Item = MakeShared<PropertyScalarItem<bool>>(InObject, Access.DisplayName, CastPropertyValue<bool>(Access.ValuePtr), Access.ReadOnly);
			}
			else if (Access.TypeName == AUX::TypeName<int32>)
			{
				auto ScalarItem = MakeShared<PropertyScalarItem<int32>>(InObject, Access.DisplayName, CastPropertyValue<int32>(Access.ValuePtr), Access.ReadOnly);
				if (Access.PropertyData && Access.PropertyData->Min)
				{
					ScalarItem->SetMinValue(std::any_cast<int32>(Access.PropertyData->Min(InObject)));
				}
				if (Access.PropertyData && Access.PropertyData->Max)
				{
					ScalarItem->SetMaxValue(std::any_cast<int32>(Access.PropertyData->Max(InObject)));
				}
				Item = MoveTemp(ScalarItem);
			}
			else if (Access.TypeName == AUX::TypeName<uint32>)
			{
				auto ScalarItem = MakeShared<PropertyScalarItem<uint32>>(InObject, Access.DisplayName, CastPropertyValue<uint32>(Access.ValuePtr), Access.ReadOnly);
				if (Access.PropertyData && Access.PropertyData->Min)
				{
					ScalarItem->SetMinValue(std::any_cast<uint32>(Access.PropertyData->Min(InObject)));
				}
				if (Access.PropertyData && Access.PropertyData->Max)
				{
					ScalarItem->SetMaxValue(std::any_cast<uint32>(Access.PropertyData->Max(InObject)));
				}
				Item = MoveTemp(ScalarItem);
			}
			else if (Access.TypeName == AUX::TypeName<float>)
			{
				auto ScalarItem = MakeShared<PropertyScalarItem<float>>(InObject, Access.DisplayName, CastPropertyValue<float>(Access.ValuePtr), Access.ReadOnly);
				if (Access.PropertyData && Access.PropertyData->Min)
				{
					ScalarItem->SetMinValue(std::any_cast<float>(Access.PropertyData->Min(InObject)));
				}
				if (Access.PropertyData && Access.PropertyData->Max)
				{
					ScalarItem->SetMaxValue(std::any_cast<float>(Access.PropertyData->Max(InObject)));
				}
				Item = MoveTemp(ScalarItem);
			}
			else if (Access.TypeName == AUX::TypeName<double>)
			{
				auto ScalarItem = MakeShared<PropertyScalarItem<double>>(InObject, Access.DisplayName, CastPropertyValue<double>(Access.ValuePtr), Access.ReadOnly);
				if (Access.PropertyData && Access.PropertyData->Min)
				{
					ScalarItem->SetMinValue(std::any_cast<double>(Access.PropertyData->Min(InObject)));
				}
				if (Access.PropertyData && Access.PropertyData->Max)
				{
					ScalarItem->SetMaxValue(std::any_cast<double>(Access.PropertyData->Max(InObject)));
				}
				Item = MoveTemp(ScalarItem);
			}
			else if (Access.TypeName == AUX::TypeName<FString>)
			{
				Item = MakeShared<PropertyStringItem>(InObject, Access.DisplayName, CastPropertyValue<FString>(Access.ValuePtr), Access.ReadOnly);
			}
			else if (Access.TypeName == AUX::TypeName<Vector2f>)
			{
				Item = MakeShared<PropertyVector2fItem>(InObject, Access.DisplayName, CastPropertyValue<Vector2f>(Access.ValuePtr), Access.ReadOnly);
			}
			else if (Access.TypeName == AUX::TypeName<Vector3f>)
			{
				Item = MakeShared<PropertyVector3fItem>(InObject, Access.DisplayName, CastPropertyValue<Vector3f>(Access.ValuePtr), Access.ReadOnly);
			}
			else if (Access.TypeName == AUX::TypeName<Vector4f>)
			{
				Item = MakeShared<PropertyVector4fItem>(InObject, Access.DisplayName, CastPropertyValue<Vector4f>(Access.ValuePtr), Access.ReadOnly);
			}
			else if (Access.TypeName == AUX::TypeName<Vector2i>)
			{
				Vector2i* VecValue = CastPropertyValue<Vector2i>(Access.ValuePtr);
				Item = MakeShared<PropertyVector2iItem>(InObject, Access.DisplayName, &VecValue->x, Access.ReadOnly);
			}
			else if (Access.TypeName == AUX::TypeName<Vector3i>)
			{
				Vector3i* VecValue = CastPropertyValue<Vector3i>(Access.ValuePtr);
				Item = MakeShared<PropertyVector3iItem>(InObject, Access.DisplayName, &VecValue->x, Access.ReadOnly);
			}
			else if (Access.TypeName == AUX::TypeName<Vector4i>)
			{
				Vector4i* VecValue = CastPropertyValue<Vector4i>(Access.ValuePtr);
				Item = MakeShared<PropertyVector4iItem>(InObject, Access.DisplayName, &VecValue->x, Access.ReadOnly);
			}
			else if (MemberMetaType && MemberMetaType->Datas.Num() > 0)
			{
				Item = MakeShared<PropertyCategory>(InObject, Access.DisplayName, true);
				void* CompositeInstance = Access.ValuePtr;
				for (MetaMemberData* ProperyMember : GetProperties(MemberMetaType))
				{
					if (MemberMetaType->IsDerivedFrom<ShObject>())
					{
						Item->AddChilds(GeneratePropertyDatas(static_cast<ShObject*>(CompositeInstance), ProperyMember, CompositeInstance));
					}
					else
					{
						Item->AddChilds(GeneratePropertyDatas(InObject, ProperyMember, CompositeInstance));
					}
				}
			}

			return Item;
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
		bool ReadOnly = EnumHasAnyFlags(MetaMemData->InfoType, MetaInfo::ReadOnly);
		TSharedPtr<PropertyData> Item;

		if (MetaMemData->IsArray())
		{
			auto ArrayItem = MakeShared<PropertyArrayItem>(
				InObject,
				MetaMemData->MemberName,
				[MetaMemData, Instance]() -> int32 {
					return MetaMemData->GetArrayNum ? MetaMemData->GetArrayNum(Instance) : 0;
				},
				[MetaMemData, Instance](int32 NewNum) {
					if (MetaMemData->SetArrayNum)
					{
						MetaMemData->SetArrayNum(Instance, NewNum);
					}
				},
				ReadOnly
			);
			ArrayItem->SetRemoveAt([MetaMemData, Instance](int32 Index) {
				if (MetaMemData->RemoveArrayElement)
				{
					MetaMemData->RemoveArrayElement(Instance, Index);
				}
			});

			ArrayItem->SetRebuildChildren([InObject, MetaMemData, Instance, ReadOnly](PropertyArrayItem& InArrayItem) {
				const MetaMemberData* ElementMetaData = MetaMemData->ArrayElementData.Get();
				if (!ElementMetaData)
				{
					return;
				}
				const int32 NumElements = MetaMemData->GetArrayNum ? MetaMemData->GetArrayNum(Instance) : 0;
				for (int32 ElementIndex = 0; ElementIndex < NumElements; ++ElementIndex)
				{
					void* ElementPtr = MetaMemData->GetArrayElement ? MetaMemData->GetArrayElement(Instance, ElementIndex) : nullptr;
					if (!ElementPtr)
					{
						continue;
					}

					PropertyValueAccess ElementAccess;
					ElementAccess.DisplayName = FText::Format(LOCALIZATION("Element {0}"), FText::AsNumber(ElementIndex));
					ElementAccess.TypeName = ElementMetaData->TypeName;
					ElementAccess.ValuePtr = ElementMetaData->Get(ElementPtr);
					ElementAccess.SetValue = [ElementMetaData, ElementPtr](void* NewValue) {
						ElementMetaData->Set(ElementPtr, NewValue);
					};
					ElementAccess.GetMetaType = [ElementMetaData] {
						return ElementMetaData->GetMetaType ? ElementMetaData->GetMetaType() : nullptr;
					};
					if (ElementMetaData->GetReferencedShObject)
					{
						ElementAccess.GetReferencedShObject = [ElementMetaData, ElementPtr] {
							return ElementMetaData->GetReferencedShObject(ElementPtr);
						};
						ElementAccess.SetReferencedShObject = [ElementMetaData, ElementPtr](ShObject* NewObject) {
							if (ElementMetaData->SetReferencedShObject)
							{
								ElementMetaData->SetReferencedShObject(ElementPtr, NewObject);
							}
						};
					}
					if (ElementMetaData->GetEnumValueName)
					{
						ElementAccess.GetEnumValueName = [ElementMetaData, ElementPtr] {
							return ElementMetaData->GetEnumValueName(ElementPtr);
						};
						ElementAccess.EnumEntries = ElementMetaData->EnumEntries;
					}
					ElementAccess.PropertyData = &ElementMetaData->PropertyData;
					ElementAccess.ReadOnly = ReadOnly;

					if (TSharedPtr<PropertyData> ElementItem = GeneratePropertyDataForValue(InObject, ElementAccess))
					{
						ElementItem->SetArrayElementStyle(true);
						InArrayItem.AddChild(ElementItem.ToSharedRef());
					}
				}
			});
			Item = MoveTemp(ArrayItem);
		}
		else
		{
			PropertyValueAccess Access;
			Access.DisplayName = MetaMemData->MemberName;
			Access.TypeName = MetaMemData->TypeName;
			Access.ValuePtr = MetaMemData->Get(Instance);
			Access.SetValue = [MetaMemData, Instance](void* NewValue) {
				MetaMemData->Set(Instance, NewValue);
			};
			Access.GetMetaType = [MetaMemData] {
				return MetaMemData->GetMetaType ? MetaMemData->GetMetaType() : nullptr;
			};
			if (MetaMemData->GetReferencedShObject)
			{
				Access.GetReferencedShObject = [MetaMemData, Instance] {
					return MetaMemData->GetReferencedShObject(Instance);
				};
				Access.SetReferencedShObject = [MetaMemData, Instance](ShObject* NewObject) {
					if (MetaMemData->SetReferencedShObject)
					{
						MetaMemData->SetReferencedShObject(Instance, NewObject);
					}
				};
			}
			if (MetaMemData->GetEnumValueName)
			{
				Access.GetEnumValueName = [MetaMemData, Instance] {
					return MetaMemData->GetEnumValueName(Instance);
				};
				Access.EnumEntries = MetaMemData->EnumEntries;
			}
			Access.PropertyData = &MetaMemData->PropertyData;
			Access.VisibilityInstance = Instance;
			Access.ReadOnly = ReadOnly;
			Item = GeneratePropertyDataForValue(InObject, Access);
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
			GlobalValidShObjects.Remove(this);
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
