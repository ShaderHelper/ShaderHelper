#pragma once
#include "AssetObject.h"

namespace FRAMEWORK
{
	class Texture2D : public AssetObject
	{
	public:
		Texture2D();
		Texture2D(int32 InWidth, int32 InHeight, const TArray<uint8>& InRawData);

	public:
		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		FString FileExtension() const override;
		GpuTexture* GetThumbnail() const override;

	private:
		int32 Width, Height;
		//BGRA8
		TArray<uint8> RawData;
		TRefCountPtr<GpuTexture> GpuData;
	};

}
