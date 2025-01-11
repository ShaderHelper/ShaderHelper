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
        void Destroy()
        {
            NumRefs = 0;
            delete this;
        }

		virtual void Serialize(FArchive& Ar) override;
        virtual void PostLoad();
		virtual void Save();
		virtual void MarkDirty();
		bool IsDirty();

		virtual FString FileExtension() const = 0;

		//Determine the asset icon in the asset browser.
		virtual GpuTexture* GetThumbnail() const { return nullptr; }
		virtual const FSlateBrush* GetImage() const { return nullptr; }
		//
        
        FString GetFileName() const;
        FString GetPath() const;
	};

}
