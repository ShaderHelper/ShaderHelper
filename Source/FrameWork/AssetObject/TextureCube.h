#pragma once
#include "AssetObject.h"

namespace FW
{
	class FRAMEWORK_API TextureCube : public AssetObject
	{
		REFLECTION_TYPE(TextureCube)
	public:
		TextureCube();
		TextureCube(uint32 InSize, GpuFormat InFormat, const TArray<TArray<uint8>>& InFaceData);

	public:
		void Serialize(FArchive& Ar) override;
		void PostLoad() override;

		void InitGpuData();
		FString FileExtension() const override;
		GpuTexture* GetThumbnail() const override;
		GpuTexture* GetGpuData() const { return GpuData; }
		GpuTexture* GetPreviewTexture() const { return PreviewData; }
		uint32 GetSize() const { return Size; }
		GpuFormat GetFormat() const { return Format; }
		const TArray<TArray<uint8>>& GetFaceData() const { return FaceData; }

		void SetFaceData(int32 FaceIndex, const TArray<uint8>& InData);
		void SetData(uint32 InSize, GpuFormat InFormat, TArray<TArray<uint8>> InFaceData);

	public:
		GpuFormat Format;
		uint32 Size; // cubemap faces are square: Width == Height == Size

	private:
		TArray<TArray<uint8>> FaceData; // 6 faces: +X, -X, +Y, -Y, +Z, -Z
		TRefCountPtr<GpuTexture> GpuData;
		TRefCountPtr<GpuTexture> PreviewData;
	};
}
