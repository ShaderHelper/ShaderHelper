#pragma once
#include "GpuApi/GpuTexture.h"

struct FSlateBrush;

namespace FRAMEWORK
{
	class FRAMEWORK_API AssetObject
	{
	public:
		AssetObject();
		virtual ~AssetObject() = default;

		//Use the serialization system from unreal engine
		virtual void Serialize(FArchive& Ar);
		virtual void PostLoad() {}

		virtual FString FileExtension() const = 0;
		virtual GpuTexture* GetThumbnail() const { return nullptr; }
		virtual const FSlateBrush* GetImage() const { return nullptr; }
        
        FString GetFileName() const;
		FGuid GetGuid() const { return Guid; }

	protected:
		FGuid Guid;
	};

}
