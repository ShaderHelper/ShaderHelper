#pragma once
#include "GpuApi/GpuTexture.h"

struct FSlateBrush;

namespace FW
{
	class FRAMEWORK_API AssetObject : public ShObject
	{
		REFLECTION_TYPE(AssetObject)
	public:
		AssetObject() = default;
        ~AssetObject();
        
        //The asset can be deleted outside
		void Destroy();

		virtual void Serialize(FArchive& Ar) override;
        virtual void PostLoad() override;
		virtual void PostPropertyChanged(PropertyData* InProperty) override;
		virtual void Save();
		virtual void MarkDirty(bool IsDirty = true);
		bool IsDirty();

		virtual FString FileExtension() const = 0;
		bool IsBuiltInAsset() const;

		//Determine the asset icon in the asset browser.
		GpuTexture* GetThumbnail() const;
        virtual const FSlateBrush* GetImage() const;
		int GetFileAssetVer() const { return FileAssetVer; }
        
        FString GetFileName() const;
        FString GetPath() const;

	protected:
		virtual TRefCountPtr<GpuTexture> GenerateThumbnail() const { return nullptr; }
		int FileAssetVer = 0;

	private:
		TArray<FGuid> CollectReflectedDependencyGuids() const;
		void RegisterReflectedDependencies();
	};

    FRAMEWORK_API MetaType* GetAssetMetaType(const FString& InPath);
}
