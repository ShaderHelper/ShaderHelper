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
        
	enum class FrameWorkVer
	{
		Initial,
	};

	inline FrameWorkVer GFrameWorkVer = FrameWorkVer::Initial;

    //Note: All ShObject must be default constructible.
	class FRAMEWORK_API ShObject : FNoncopyable
	{
		REFLECTION_TYPE(ShObject)
	public:
		ShObject();
		virtual ~ShObject();
        
    public:
		FGuid GetGuid() const { return Guid; }
		//Use the serialization system from unreal engine
		virtual void Serialize(FArchive& Ar);
        virtual void PostLoad();
        //
        virtual TArray<TSharedRef<PropertyData>>* GetPropertyDatas();
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

	protected:
		FGuid Guid;
        TArray<TSharedRef<PropertyData>> PropertyDatas;
        mutable int32 NumRefs;

    private:
        //For all Shobject except AssetObject
        //The OuterMost should be an AssetObject
        ShObject* Outer;

        TArray<ObserverObjectPtr<ShObject>> SubObjects;
	};

    template<typename T>
    ObjectPtr<T, ObjectOwnerShip::Retain> NewShObject(ShObject* InOuter)
    {
        T* NewObj = new T;
        NewObj->SetOuter(InOuter);
        return NewObj;
    }

	class FRAMEWORK_API ShObjectOp
	{
		REFLECTION_TYPE(ShObjectOp)
	public:
		ShObjectOp() = default;
		virtual ~ShObjectOp() = default;
        
        virtual MetaType* SupportType() = 0;
		virtual void OnSelect(ShObject* InObject) {}
	};

    FRAMEWORK_API ShObjectOp* GetShObjectOp(ShObject* InObject);

    FRAMEWORK_API extern TArray<ShObject*> GlobalValidShObjects;
}
