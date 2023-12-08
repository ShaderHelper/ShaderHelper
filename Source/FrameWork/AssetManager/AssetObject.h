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
		virtual TUniquePtr<class AssetFactory> Factory() const = 0;
		virtual TSharedPtr<class AssetViewItem> AssetViewItem() const = 0;
	};

}