#include "CommonHeader.h"
#include "FrameWorkCore.h"
#include "ProjectManager/ProjectManager.h"

namespace FW
{
	GLOBAL_REFLECTION_REGISTER(AddClass<ShObject>())
	GLOBAL_REFLECTION_REGISTER(AddClass<ShObjectOp>())

    TArray<ShObject*> GlobalValidShObjects;

	ShObject::ShObject() 
		: ObjectName(FText::FromString("Unknown"))
		, Guid(FGuid::NewGuid())
        , NumRefs(0)
	{
        GlobalValidShObjects.Add(this);
	}

    ShObject::~ShObject()
    {
        check(NumRefs == 0);
        GlobalValidShObjects.Remove(this);
    }

	void ShObject::Serialize(FArchive& Ar)
	{
		Ar << Guid;
		Ar << ObjectName;
		Ar << GFrameWorkVer << GProjectVer;
	}
    
    ShObjectOp* GetShObjectOp(ShObject* InObject)
    {
        return GetDefaultObject<ShObjectOp>([InObject](ShObjectOp* CurShObjectOp) {
            return CurShObjectOp->SupportType() == InObject->DynamicMetaType();
        });
    }
}
