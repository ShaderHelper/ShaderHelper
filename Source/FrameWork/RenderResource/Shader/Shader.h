#pragma once
#include "GpuApi/GpuRhi.h"
#include <set>

namespace FW
{

	template<typename ShaderType>
	ShaderType* GetShader(const std::set<FString>& VariantDefinitions = {})
	{
		uint32 Hash = 0;
		for (const FString& D : VariantDefinitions)
		{
			Hash = HashCombine(Hash, GetTypeHash(D));
		}
		static TMap<uint32, TUniquePtr<ShaderType>> Variants;
		if (!Variants.Contains(Hash))
		{
			Variants.Add(Hash, MakeUnique<ShaderType>(VariantDefinitions));
		}
		return Variants[Hash].Get();
	}

}
