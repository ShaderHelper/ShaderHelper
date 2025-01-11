#pragma once
#include "UI/Widgets/Property/PropertyData/PropertyData.h"

namespace FW
{
	enum class FrameWorkVer
	{
		Initial,
	};

    enum class ObjectOwnerShip
    {
        Assign,
        Retain,
    };

	inline FrameWorkVer GFrameWorkVer = FrameWorkVer::Initial;

	class FRAMEWORK_API ShObject : FNoncopyable
	{
		REFLECTION_TYPE(ShObject)
	public:
		ShObject();
		virtual ~ShObject();
        
		FGuid GetGuid() const { return Guid; }
		//Use the serialization system from unreal engine
		virtual void Serialize(FArchive& Ar);
        //
        virtual TArray<TSharedRef<PropertyData>>* GetPropertyDatas() { return nullptr; };
    
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

	public:
		FText ObjectName;

	protected:
		FGuid Guid;
        TArray<TSharedRef<PropertyData>> PropertyDatas;
        mutable int32 NumRefs;
	};

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
