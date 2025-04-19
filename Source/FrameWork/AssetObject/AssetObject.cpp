#include "CommonHeader.h"
#include "AssetObject.h"
#include "Common/Util/Reflection.h"
#include "AssetManager/AssetManager.h"
#include "ProjectManager/ProjectManager.h"
#include "UI/Styles/FAppCommonStyle.h"

namespace FW
{
    REFLECTION_REGISTER(AddClass<AssetObject>()
								.BaseClass<ShObject>())

    MetaType* GetAssetMetaType(const FString& InPath)
    {
        FString Ext = FPaths::GetExtension(InPath);
        return GetDefaultObject<AssetObject>([&](AssetObject* CurCDO){
            return CurCDO->FileExtension() == Ext;
        })->DynamicMetaType();
    }

    AssetObject::~AssetObject()
    {
        TSingleton<AssetManager>::Get().RemoveAsset(this);
    }

	void AssetObject::Serialize(FArchive& Ar)
	{
		ShObject::Serialize(Ar);
	}

    void AssetObject::PostLoad()
    {
        ShObject::PostLoad();
        ObjectName = FText::FromString(GetFileName());
    }

	void AssetObject::Save()
	{
		TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*GetPath()));
		Serialize(*Ar);

		GProject->RemovePendingAsset(this);
	}

	void AssetObject::MarkDirty(bool IsDirty)
	{
        if(IsDirty)
        {
            GProject->AddPendingAsset(this);
        }
        else
        {
            GProject->RemovePendingAsset(this);
        }
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

    const FSlateBrush* AssetObject::GetImage() const
    {
        return FAppCommonStyle::Get().GetBrush("Icons.File");
    }

}
