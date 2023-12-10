#pragma once

namespace FRAMEWORK
{
	class AssetObject
	{
	public:
		AssetObject() = default;
		virtual ~AssetObject() = default;

		//Use the serialization system from unreal engine
		virtual void Serialize(FArchive& Ar) = 0;
		virtual void PostLoad() {}

		virtual FString FileExtension() const = 0;
		virtual class GpuTexture* Gethumbnail() const { return nullptr; }
		virtual struct FSlateBrush* GetImage() const { return nullptr; }
		virtual TSharedRef<class SWidget> GetPropertyView() const = 0;
	};

}