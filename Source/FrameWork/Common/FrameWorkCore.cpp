#include "CommonHeader.h"
#include "FrameWorkCore.h"
#include "ProjectManager/ProjectManager.h"

namespace FW
{
	GLOBAL_REFLECTION_REGISTER(AddClass<ShObject>())
	GLOBAL_REFLECTION_REGISTER(AddClass<ShObjectOp>())

	ShObject::ShObject() 
		: ObjectName(FText::FromString("Unknown"))
		, Guid(FGuid::NewGuid())
	{

	}

	void ShObject::Serialize(FArchive& Ar)
	{
		Ar << Guid;
		Ar << ObjectName;
		Ar << GFrameWorkVer << GProjectVer;
	}

}