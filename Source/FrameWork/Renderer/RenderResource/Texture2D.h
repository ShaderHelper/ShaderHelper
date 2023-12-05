#pragma once
#include "AssetManager/AssetObject.h"

namespace FRAMEWORK
{
	class Texture2D : AssetObject
	{

	};

	namespace AssetImporter
	{
		template<>
		Texture2D* Import<Texture2D>(const FString& AssetPath)
		{
			return nullptr;
		}
	}
}