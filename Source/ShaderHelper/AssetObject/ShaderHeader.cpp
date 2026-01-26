#include "CommonHeader.h"
#include "ShaderHeader.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "AssetManager/AssetManager.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<ShaderHeader>("Shader Header")
		.BaseClass<ShaderAsset>()
		.Data<&ShaderHeader::Language, MetaInfo::Property>(LOCALIZATION("Language"))
	)

	ShaderHeader::ShaderHeader() : Language(GpuShaderLanguage::HLSL)
	{
	}

	void ShaderHeader::Serialize(FArchive& Ar)
	{
		ShaderAsset::Serialize(Ar);
		Ar << Language;
	}

	FString ShaderHeader::FileExtension() const
	{
		return "header";
	}

	const FSlateBrush* ShaderHeader::GetImage() const
	{
		return FShaderHelperStyle::Get().GetBrush("AssetBrowser.Header");
	}

	FString ShaderHeader::GetFullContent() const
	{
		return EditorContent;
	}

	GpuShaderSourceDesc ShaderHeader::GetShaderDesc(const FString& InContent) const
	{
		auto Desc = GpuShaderSourceDesc{
			.Name = GetShaderName(),
			.Source = InContent,
			.Language = Language,
			.IncludeDirs = GetIncludeDirs(),
			.IncludeHandler = [](const FString& IncludePath) -> FString {
				if (FPaths::GetExtension(IncludePath) == TEXT("header"))
				{
					AssetPtr<ShaderHeader> HeaderAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<ShaderHeader>(IncludePath);
					if (HeaderAsset)
					{
						return HeaderAsset->GetFullContent();
					}
				}
				FString Content;
				FFileHelper::LoadFileToString(Content, *IncludePath);
				return Content;
			},
		};
		return Desc;
	}
}
