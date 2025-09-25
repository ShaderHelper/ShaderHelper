#pragma once
#include "AssetObject.h"

namespace FW
{
	class FRAMEWORK_API Texture2D : public AssetObject
	{
		REFLECTION_TYPE(Texture2D)
	public:
		Texture2D();
		Texture2D(uint32 InWidth, uint32 InHeight, GpuTextureFormat InFormat, const TArray<uint8>& InRawData);

	public:
		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		
		void InitGpudata();
		FString FileExtension() const override;
		GpuTexture* GetThumbnail() const override;
		GpuTexture* GetGpuData() const { return GpuData; }
		uint32 GetWidth() const { return Width; }
		uint32 GetHeight() const { return Height; }
		GpuTextureFormat GetFormat() const { return Format; }

	private:
		GpuTextureFormat Format;
        uint32 Width, Height;
		TArray<uint8> RawData;
		TRefCountPtr<GpuTexture> GpuData;
	};

}
