#pragma once
#include "GpuApi/GpuTexture.h"
#include "Common/FrameWorkCore.h"
struct FSlateBrush;

namespace FW
{
	class FRAMEWORK_API AssetObject : public ShObject
	{
		REFLECTION_TYPE(AssetObject)
	public:
		AssetObject() = default;

		virtual void Serialize(FArchive& Ar) override;
		virtual void PostLoad() {}
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
