#pragma once
#include "AssetObject.h"

namespace FW
{
	class FRAMEWORK_API Texture3D : public AssetObject
	{
		REFLECTION_TYPE(Texture3D)
	public:
		Texture3D();
		Texture3D(uint32 InWidth, uint32 InHeight, uint32 InDepth, GpuFormat InFormat, const TArray<uint8>& InRawData);

	public:
		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		void PostPropertyChanged(PropertyData* InProperty) override;

		void InitGpuData();
		FString FileExtension() const override;
		GpuTexture* GetGpuData() const { return GpuData; }
		GpuTexture* GetPreviewTexture() const { return PreviewData; }
		const TArray<uint8>& GetRawData() const { return RawData; }
		uint32 GetWidth() const { return Width; }
		uint32 GetHeight() const { return Height; }
		uint32 GetDepth() const { return Depth; }
		GpuFormat GetFormat() const { return Format; }
		void SetData(uint32 InWidth, uint32 InHeight, uint32 InDepth, GpuFormat InFormat, TArray<uint8> InRawData);

		TRefCountPtr<GpuTexture> GenerateThumbnail() const override;

	public:
		GpuFormat Format;
		uint32 Width, Height, Depth;
		bool GenerateMipmap = false;

	private:
		TArray<uint8> RawData;
		TRefCountPtr<GpuTexture> GpuData;
		TRefCountPtr<GpuTexture> PreviewData;
	};
}
