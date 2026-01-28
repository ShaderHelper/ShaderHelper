#include "CommonHeader.h"
#include "ShaderHeader.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<ShaderHeader>("Shader Header")
		.BaseClass<ShaderAsset>()
		.Data<&ShaderHeader::Language, MetaInfo::Property>(LOCALIZATION("Language"))
	)

	const FString DefaultHeader = "#pragma once\n";

	ShaderHeader::ShaderHeader() : Language(GpuShaderLanguage::HLSL)
	{
		EditorContent = DefaultHeader;
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
					FScopeLock ScopeLock(&GAssetCS);
					AssetPtr<ShaderHeader> HeaderAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<ShaderHeader>(IncludePath);
					return HeaderAsset ? HeaderAsset->GetFullContent() : "";
				}
				FString Content;
				FFileHelper::LoadFileToString(Content, *IncludePath);
				return Content;
			},
		};
		return Desc;
	}

	bool ShaderHeader::CanChangeProperty(FW::PropertyData* InProperty)
	{
		if (InProperty->IsOfType<PropertyEnumItem>() && InProperty->GetDisplayName().EqualTo(LOCALIZATION("Language")))
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			auto Ret = MessageDialog::Open(MessageDialog::OkCancel, MessageDialog::Shocked, ShEditor->GetMainWindow(), LOCALIZATION("ShaderLanguageTip"));
			if (Ret == MessageDialog::MessageRet::Cancel)
			{
				return false;
			}
		}
		return true;
	}

	void ShaderHeader::PostPropertyChanged(FW::PropertyData* InProperty)
	{
		if (InProperty->IsOfType<PropertyEnumItem>() && InProperty->GetDisplayName().EqualTo(LOCALIZATION("Language")))
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			GpuShaderLanguage NewLanguage = *static_cast<GpuShaderLanguage*>(static_cast<PropertyEnumItem*>(InProperty)->GetEnum());
			if (NewLanguage == GpuShaderLanguage::HLSL)
			{
				ShEditor->OpenShaderTab(this);
				ShEditor->GetShaderEditor(this)->SetText(FText::FromString(DefaultHeader));
			}
			else if (NewLanguage == GpuShaderLanguage::GLSL)
			{
				ShEditor->OpenShaderTab(this);
				ShEditor->GetShaderEditor(this)->SetText(FText::FromString(DefaultHeader));
			}
		}
	}
}
