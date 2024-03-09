#include "CommonHeader.h"
#include "AssetObject.h"
#include "Common/Util/Reflection.h"

namespace FRAMEWORK
{
	GLOBAL_REFLECTION_REGISTER(
		ShReflectToy::AddClass<AssetObject>();
	)

	AssetObject::AssetObject()
	{
		Guid = FGuid::NewGuid();
	}

	void AssetObject::Serialize(FArchive& Ar)
	{
		Ar << Guid;
	}

}