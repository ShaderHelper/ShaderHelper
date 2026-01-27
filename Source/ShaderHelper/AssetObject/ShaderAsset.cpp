#include "CommonHeader.h"
#include "ShaderAsset.h"
#include "ProjectManager/ShProjectManager.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<ShaderAsset>("ShaderAsset")
								.BaseClass<AssetObject>()
	)

	ShaderAsset::ShaderAsset()
	{

	}

	void ShaderAsset::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);
		
		Ar << EditorContent;
		SavedEditorContent = EditorContent;
	}

	TArray<FString> ShaderAsset::GetIncludeDirs() const
	{
		return {
			PathHelper::ShaderDir(),
			TSingleton<ShProjectManager>::Get().GetActiveContentDirectory(),
			FPaths::GetPath(GetPath()),
		};
	}

	FString ShaderAsset::GetShaderName() const
	{
		return GetFileName() + "." + FileExtension();
	}

	AssetPtr<ShaderAsset> ShaderAsset::FindIncludeAsset(const FString& InShaderName)
	{
		if (InShaderName.IsEmpty())
		{
			return nullptr;
		}
		
		if (InShaderName == GetShaderName())
		{
			return this;
		}
		
		// Try to find in include directories
		for (const FString& IncludeDir : GetIncludeDirs())
		{
			FString FullPath = FPaths::Combine(IncludeDir, InShaderName);
			if (TSingleton<AssetManager>::Get().IsValidAsset(FullPath))
			{
				return TSingleton<AssetManager>::Get().LoadAssetByPath<ShaderAsset>(FullPath);
			}
		}
		
		return nullptr;
	}
}
