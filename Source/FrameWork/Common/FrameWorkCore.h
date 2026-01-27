#pragma once

namespace FW
{
    class PropertyData;
    class AssetObject;
    
    enum class ObjectOwnerShip
    {
        Assign,
        Retain,
    };

    template<typename T, ObjectOwnerShip> class ObjectPtr;

    template<typename T>
    using ObserverObjectPtr = ObjectPtr<T, ObjectOwnerShip::Assign>;
      
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
		//Use the serialization system from unreal engine
		virtual void Serialize(FArchive& Ar);
        virtual void PostLoad();
        //
        virtual TArray<TSharedRef<PropertyData>>* GetPropertyDatas();
		virtual bool CanChangeProperty(PropertyData* InProperty) { return true; };
        virtual void PostPropertyChanged(PropertyData* InProperty);
    
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
        TArray<TSharedRef<PropertyData>> PropertyDatas;
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

    FRAMEWORK_API ShObjectOp* GetShObjectOp(ShObject* InObject);

    FRAMEWORK_API extern TArray<ShObject*> GlobalValidShObjects;

	FRAMEWORK_API TArray<TSharedRef<PropertyData>> GeneratePropertyDatas(ShObject* InObject, const MetaMemberData* MetaMemData, void* Instance, bool bForce = false);
	FRAMEWORK_API TArray<TSharedRef<PropertyData>> GeneratePropertyDatas(ShObject* InObject, MetaType* InMetaType);
}
