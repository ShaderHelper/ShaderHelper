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
        //
        virtual TArray<TSharedRef<PropertyData>>* GetPropertyDatas();
    
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
        
        AssetObject* GetOuterMost();

	public:
		FText ObjectName;
        
        //For all Shobject except AssetObject
        //The OuterMost should be an AssetObject
        ShObject* Outer;

	protected:
		FGuid Guid;
        TArray<TSharedRef<PropertyData>> PropertyDatas;
        mutable int32 NumRefs;
	};

    template<typename T>
    ObjectPtr<T, ObjectOwnerShip::Retain> NewShObject(ShObject* InOuter = nullptr)
    {
        T* NewObj = new T;
        NewObj->Outer = InOuter;
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
