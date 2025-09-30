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
		
		template<typename T, typename... ArgTypes>
		friend ObjectPtr<T, ObjectOwnerShip::Retain> NewShObject(ShObject* InOuter, ArgTypes&&... InArgs);
	protected:
		ShObject();
	public:
		virtual ~ShObject();
		//Init will be called when a non-default object is created
		virtual void Init() {}
        
    public:
		FGuid GetGuid() const { return Guid; }
		//Use the serialization system from unreal engine
		virtual void Serialize(FArchive& Ar);
        virtual void PostLoad();
        //
        virtual TArray<TSharedRef<PropertyData>>* GetPropertyDatas();
		virtual bool CanChangeProperty(PropertyData* InProperty) { return true; };
        virtual void PostPropertyChanged(PropertyData* InProperty);
    
        uint32 Add() const
        {
            return uint32(++NumRefs);
        }
        uint32 Release() const
        {
            uint32 Refs = uint32(--NumRefs);
            if(Refs == 0)
            {
                delete this;
            }
            return Refs;
        }
        uint32 GetRefCount() const
        {
            return uint32(NumRefs);
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
        mutable int32 NumRefs;

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
