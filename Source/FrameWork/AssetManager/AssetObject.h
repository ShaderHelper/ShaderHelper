#pragma once

namespace FRAMEWORK
{
	namespace AssetImporter
	{
		//Implement it if you have custom asset type.
		template<typename T>
		T* Import(const FString& AssetPath);
	}

	class AssetObject
	{
	public:
		virtual ~AssetObject() = default;
	};

}