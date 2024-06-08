#include "CommonHeader.h"
#include "AssetObject.h"
#include "Common/Util/Reflection.h"
#include "AssetManager/AssetManager.h"

namespace FRAMEWORK
{
	GLOBAL_REFLECTION_REGISTER(AddClass<AssetObject>())

	AssetObject::AssetObject()
	{
		Guid = FGuid::NewGuid();
	}

	void AssetObject::Serialize(FArchive& Ar)
	{
		Ar << Guid;
	}

    FString AssetObject::GetFileName() const
    {
        return FPaths::GetBaseFilename(GetPath());
    }

    FString AssetObject::GetPath() const
    {
        return TSingleton<AssetManager>::Get().GetPath(Guid);
    }

}
