#pragma once
#include "AssetObject.h"

namespace FRAMEWORK
{
	class Texture2D : public AssetObject
	{
		REFLECTION_TYPE(Texture2D)
	public:
		Texture2D();
		Texture2D(uint32 InWidth, uint32 InHeight, const TArray<uint8>& InRawData);

	public:
		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		FString FileExtension() const override;
		GpuTexture* GetThumbnail() const override;

	private:
        uint32 Width, Height;
		//BGRA8
		TArray<uint8> RawData;
		TRefCountPtr<GpuTexture> GpuData;
	};

}
