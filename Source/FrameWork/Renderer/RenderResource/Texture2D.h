#pragma once
#include "AssetManager/AssetObject.h"

namespace FRAMEWORK
{
	class Texture2D : public AssetObject
	{
	public:
		virtual void Serialize(FArchive& Ar);
		virtual void PostLoad();
		virtual FString FileExtension() const;
		//virtual TUniquePtr<class AssetFactory> Factory();
		//virtual TSharedPtr<class AssetViewItem> AssetViewItem() const;
	};

}