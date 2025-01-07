#include "CommonHeader.h"
#include "AssetObject.h"
#include "Common/Util/Reflection.h"
#include "AssetManager/AssetManager.h"
#include "ProjectManager/ProjectManager.h"
#include "Common/FrameWorkCore.h"

namespace FW
{
	GLOBAL_REFLECTION_REGISTER(AddClass<AssetObject>()
								.BaseClass<ShObject>())

	void AssetObject::Serialize(FArchive& Ar)
	{
		ShObject::Serialize(Ar);
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

	bool AssetObject::IsDirty()
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
