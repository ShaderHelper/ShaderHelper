#pragma once
#include "GpuApi/GpuTexture.h"

struct FSlateBrush;

namespace FW
{
	class FRAMEWORK_API AssetObject
	{
		REFLECTION_TYPE(AssetObject)
	public:
		AssetObject();
		virtual ~AssetObject() = default;

		//Use the serialization system from unreal engine
		virtual void Serialize(FArchive& Ar);
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
		FGuid GetGuid() const { return Guid; }

	protected:
		FGuid Guid;
	};

}
