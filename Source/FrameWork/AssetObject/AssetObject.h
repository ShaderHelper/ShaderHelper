#pragma once
#include "GpuApi/GpuTexture.h"

struct FSlateBrush;

namespace FRAMEWORK
{
	class AssetObject
	{
	public:
		AssetObject();
		virtual ~AssetObject() = default;

		//Use the serialization system from unreal engine
		virtual void Serialize(FArchive& Ar);
		virtual void PostLoad() {}

		virtual FString FileExtension() const = 0;
		virtual GpuTexture* GetThumbnail() const { return nullptr; }
		virtual FSlateBrush* GetImage() const { return nullptr; }

		FGuid GetGuid() const { return Guid; }

	protected:
		FGuid Guid;
	};

}
