#include "CommonHeader.h"
#include "AssetObject.h"
#include "Common/Path/PathHelper.h"
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

	void AssetObject::Destroy()
	{
		GProject->RemovePendingAsset(this);
		ShObject::Destroy();
	}

    AssetObject::~AssetObject()
    {
        TSingleton<AssetManager>::Get().RemoveAsset(this);
    }

	void AssetObject::Serialize(FArchive& Ar)
	{
        AssetHeader Header;
        if (Ar.IsSaving())
        {
            Header.Guid = Guid;
            Header.DependencyGuids = CollectReflectedDependencyGuids();
            Header.FileAssetVer = GAssetVer;
        }

        Ar << Header;

        Guid = Header.Guid;
        FileAssetVer = Header.FileAssetVer;
	}

    void AssetObject::PostLoad()
    {
        ShObject::PostLoad();
        ObjectName = FText::FromString(GetFileName());
    }

	void AssetObject::PostPropertyChanged(PropertyData* InProperty)
	{
		ShObject::PostPropertyChanged(InProperty);
		RegisterReflectedDependencies();
	}

    TArray<FGuid> AssetObject::CollectReflectedDependencyGuids() const
	{
		TArray<FGuid> DependencyGuids;
		MetaType* Mt = DynamicMetaType();
		while (Mt && !Mt->IsType<AssetObject>())
		{
			for (const MetaMemberData& Member : Mt->Datas)
			{
				if (Member.IsAssetRef())
				{
                    auto& AssetRef = *(AssetPtr<AssetObject>*)Member.Get(const_cast<AssetObject*>(this));
                    if (AssetRef && !DependencyGuids.Contains(AssetRef->GetGuid()))
					{
                        DependencyGuids.Add(AssetRef->GetGuid());
					}
				}
			}
			Mt = Mt->GetBaseClass();
		}
        return DependencyGuids;
    }

    void AssetObject::RegisterReflectedDependencies()
    {
        TSingleton<AssetManager>::Get().RegisterAssetDependencies(GetGuid(), CollectReflectedDependencyGuids());
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

    bool AssetObject::IsBuiltInAsset() const
    {
        return FPaths::IsUnderDirectory(GetPath(), PathHelper::BuiltinDir());
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

    GpuTexture* AssetObject::GetThumbnail() const
    {
        if (GpuTexture* Thumbnail = TSingleton<AssetManager>::Get().FindAssetThumbnail(Guid))
        {
            return Thumbnail;
        }

        TRefCountPtr<GpuTexture> Thumbnail = GenerateThumbnail();
        if (Thumbnail.IsValid())
        {
            TSingleton<AssetManager>::Get().AddAssetThumbnail(Guid, Thumbnail);
            return Thumbnail;
        }

        return nullptr;
    }

}
