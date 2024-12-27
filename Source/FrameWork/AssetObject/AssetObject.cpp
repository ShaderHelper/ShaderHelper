#include "CommonHeader.h"
#include "AssetObject.h"
#include "Common/Util/Reflection.h"
#include "AssetManager/AssetManager.h"
#include "ProjectManager/ProjectManager.h"
#include "Common/FrameWorkCore.h"

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
		Ar << GFrameWorkVer << GProjectVer;
	}

	void AssetObject::Save()
	{
		TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*GetPath()));
		Serialize(*Ar);

		GProject->RemovePendingAsset(this);
	}

	void AssetObject::MarkDirty()
	{
		GProject->AddPendingAsset(this);
	}

	bool AssetObject::IsDirty() const
	{
		return GProject->IsPendingAsset(this);
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
