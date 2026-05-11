#pragma once

namespace FW
{
	class PropertyData;
	class AssetObject;
	struct MetaType;
	class ShObject;
	FRAMEWORK_API extern TArray<ShObject*> GlobalValidShObjects;
}

#include "Common/ObjectPtr.h"

namespace FW
{
	DECLARE_LOG_CATEGORY_EXTERN(LogFrameWorkCore, Log, All);
	inline DEFINE_LOG_CATEGORY(LogFrameWorkCore);

	struct ObjectPtrFixupRequest
	{
		void* ObjectPtrAddress = nullptr;
		FGuid Guid;
		MetaType* TargetMetaType = nullptr;
		void(*AssignResolvedObject)(void*, ShObject*) = nullptr;
	};

	FRAMEWORK_API void BeginObjectPtrFixup();
	FRAMEWORK_API void ResolveObjectPtrFixups();
	FRAMEWORK_API void EndObjectPtrFixup();
	FRAMEWORK_API void RegisterLoadedShObject(ShObject* InObject);
	FRAMEWORK_API void RegisterObjectPtrFixup(const ObjectPtrFixupRequest& InRequest);
	FString GetRegisteredName(MetaType* InMt);
	MetaType* GetMetaType(const FString& InRegisteredName);
      
    //Note: All ShObject must be default constructible.
	class FRAMEWORK_API ShObject : FNoncopyable
	{
		REFLECTION_TYPE(ShObject)

		template<typename T, ObjectOwnerShip>
		friend class ObjectPtr;
		template<typename T, typename... ArgTypes>
		friend ObjectPtr<T, ObjectOwnerShip::Retain> NewShObject(ShObject* InOuter, ArgTypes&&... InArgs);
		template<typename T>
		friend ObjectPtr<T, ObjectOwnerShip::Retain> NewShObject(MetaType* InMetaType, ShObject* InOuter);
	protected:
		ShObject();
		virtual ~ShObject();
		void Destroy();
		//Init only be called when a non-default object is created by NewShObject
		virtual void Init() {}

		int32 Add()
		{
			return ++NumRefs;
		}
		int32 Release()
		{
			int32 Refs = --NumRefs;
			check(Refs >= 0);
			if (Refs == 0)
			{
				Destroy();
			}
			return Refs;
		}

    public:
		FGuid GetGuid() const { return Guid; }
		void RegenerateGuid() { Guid = FGuid::NewGuid(); }
		void RegenerateGuidRecursive();
		//Use the serialization system from unreal engine
		virtual void Serialize(FArchive& Ar);
        virtual void PostLoad();
        //
        virtual TArray<TSharedRef<PropertyData>> GeneratePropertyDatas();
		virtual bool CanChangeProperty(PropertyData* InProperty) { return true; };
        virtual void PostPropertyChanged(PropertyData* InProperty);
        // Called once per completed property edit (slider drag, typed commit, checkbox toggle, etc.).
        // OldData is the serialized state of this object captured just before the edit began.
        virtual void PostPropertyEdit(PropertyData* InProperty, TArray<uint8>&& OldData) {}
    
        int32 GetRefCount() const
        {
            return NumRefs;
        }
        
        void SetOuter(ShObject* InOuter);
        ShObject* GetOuter() const { return Outer; }
        AssetObject* GetOuterMost();

	public:
		FText ObjectName;
		FSimpleMulticastDelegate OnDestroy;

	protected:
		FGuid Guid;
        std::atomic<int32> NumRefs;


    private:
        //For all Shobject except AssetObject
        //The OuterMost should be an AssetObject
        ShObject* Outer;

        TArray<ObserverObjectPtr<ShObject>> SubObjects;
		bool IsDefaultObject{};
	};

    template<typename T, typename... ArgTypes>
    ObjectPtr<T, ObjectOwnerShip::Retain> NewShObject(ShObject* InOuter, ArgTypes&&... InArgs)
    {
        T* NewObj = new T(std::forward<ArgTypes>(InArgs)...);
        NewObj->SetOuter(InOuter);
		NewObj->Init();
        return NewObj;
    }

	template<typename T>
	ObjectPtr<T, ObjectOwnerShip::Retain> NewShObject(MetaType* InMetaType, ShObject* InOuter)
	{
		T* NewObj = static_cast<T*>(InMetaType->Construct());
		NewObj->SetOuter(InOuter);
		NewObj->Init();
		return NewObj;
	}

	template<typename T>
	void SerializePolymorphicObjectArray(FArchive& Ar, TArray<ObjectPtr<T, ObjectOwnerShip::Retain>>& Objects, ShObject* InOuter)
	{
		int32 ObjectNum = Objects.Num();
		Ar << ObjectNum;
		if (Ar.IsSaving())
		{
			for (const auto& Object : Objects)
			{
				FString TypeName = GetRegisteredName(Object->DynamicMetaType());
				Ar << TypeName;
				Object->Serialize(Ar);
			}
		}
		else
		{
			Objects.Reset();
			Objects.Reserve(ObjectNum);
			for (int32 Index = 0; Index < ObjectNum; ++Index)
			{
				FString TypeName;
				Ar << TypeName;
				auto Object = NewShObject<T>(GetMetaType(TypeName), InOuter);
				Object->Serialize(Ar);
				Objects.Add(MoveTemp(Object));
			}
		}
	}

	class FRAMEWORK_API ShObjectOp
	{
		REFLECTION_TYPE(ShObjectOp)
	public:
		ShObjectOp() = default;
		virtual ~ShObjectOp() = default;
        
        virtual MetaType* SupportType() = 0;
		virtual void OnCancelSelect(ShObject* InObject) {}
		virtual void OnSelect(ShObject* InObject) {}
	};

	class ShObjectDragDropOp : public FDragDropOperation
	{
	public:
		DRAG_DROP_OPERATOR_TYPE(ShObjectDragDropOp, FDragDropOperation)

			static TSharedRef<ShObjectDragDropOp> New(ShObject* InObject)
		{
			TSharedRef<ShObjectDragDropOp> Operation = MakeShareable(new ShObjectDragDropOp(InObject));
			Operation->MouseCursor = EMouseCursor::GrabHandClosed;
			Operation->Construct();
			return Operation;
		}

		ShObject* Object = nullptr;

	private:
		explicit ShObjectDragDropOp(ShObject* InObject)
			: Object(InObject)
		{
		}
	};

    FRAMEWORK_API ShObjectOp* GetShObjectOp(ShObject* InObject);

	FRAMEWORK_API TArray<TSharedRef<PropertyData>> GeneratePropertyDatas(ShObject* InObject, const MetaMemberData* MetaMemData, void* Instance, bool bForce = false);
	FRAMEWORK_API TArray<TSharedRef<PropertyData>> GeneratePropertyDatas(ShObject* InObject, MetaType* InMetaType);
}
