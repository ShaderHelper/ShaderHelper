#pragma once
#include "AssetManager/AssetObject.h"

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
		GpuTexture* Gethumbnail() const override;
		TSharedRef<SWidget> GetPropertyView() const override;

	private:
		int32 Width, Height;
		//BGRA8
		TArray<uint8> RawData;
		TRefCountPtr<GpuTexture> GpuData;
	};

}