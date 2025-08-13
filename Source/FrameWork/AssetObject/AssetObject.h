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
		virtual void Save();
		virtual void MarkDirty(bool IsDirty = true);
		bool IsDirty();

		virtual FString FileExtension() const = 0;

		//Determine the asset icon in the asset browser.
		virtual GpuTexture* GetThumbnail() const { return nullptr; }
        virtual const FSlateBrush* GetImage() const;
		//
        
        FString GetFileName() const;
        FString GetPath() const;
        
    public:
		FSimpleMulticastDelegate OnDestroy;
	};

    FRAMEWORK_API MetaType* GetAssetMetaType(const FString& InPath);
}
