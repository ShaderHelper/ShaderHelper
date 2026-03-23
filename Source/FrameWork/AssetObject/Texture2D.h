#pragma once
#include "AssetObject.h"

namespace FW
{
	class FRAMEWORK_API Texture2D : public AssetObject
	{
		REFLECTION_TYPE(Texture2D)
	public:
		Texture2D();
		Texture2D(uint32 InWidth, uint32 InHeight, GpuFormat InFormat, const TArray<uint8>& InRawData);

	public:
		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		void PostPropertyChanged(PropertyData* InProperty) override;
		
		void InitGpudata();
		FString FileExtension() const override;
		GpuTexture* GetThumbnail() const override;
		GpuTexture* GetGpuData() const { return GpuData; }
		GpuTexture* GetPreviewTexture() const { return PreviewTex; }
		const TArray<uint8>& GetRawData() const { return RawData; }
		uint32 GetWidth() const { return Width; }
		uint32 GetHeight() const { return Height; }
		GpuFormat GetFormat() const { return Format; }

	public:
		GpuFormat Format;
		uint32 Width, Height;
		bool GenerateMipmap = false;

	private:
		TArray<uint8> RawData;
		TRefCountPtr<GpuTexture> GpuData;
		TRefCountPtr<GpuTexture> PreviewTex;
	};

}
