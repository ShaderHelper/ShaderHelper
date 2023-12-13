#pragma once

struct FSlateBrush;
class SWidget;

namespace FRAMEWORK
{
	class GpuTexture;

	class AssetObject
	{
	public:
		AssetObject() = default;
		virtual ~AssetObject() = default;

		//Use the serialization system from unreal engine
		virtual void Serialize(FArchive& Ar) = 0;
		virtual void PostLoad() {}

		virtual FString FileExtension() const = 0;
		virtual GpuTexture* Gethumbnail() const { return nullptr; }
		virtual FSlateBrush* GetImage() const { return nullptr; }
		virtual TSharedRef<SWidget> GetPropertyView() const = 0;
	};

}