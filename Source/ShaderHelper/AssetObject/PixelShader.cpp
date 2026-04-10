#include "CommonHeader.h"
#include "PixelShader.h"
#include "AssetManager/AssetManager.h"
#include "AssetObject/ShaderHeader.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<PixelShader>("PixelShader")
		.BaseClass<ShaderAsset>()
		.Data<&PixelShader::Language, MetaInfo::Property>(LOCALIZATION("Language"))
	)

	const FString DefaultHlslPixelShader =
	R"(struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD0;
};

float4 main(PSInput Input) : SV_Target
{
	return float4(Input.TexCoord, 0.0, 1.0);
})";

	const FString DefaultGlslPixelShader =
	R"(#version 450
layout(location = 0) in vec2 OutTexCoord;
layout(location = 0) out vec4 FragColor;

void main()
{
	FragColor = vec4(OutTexCoord, 0.0, 1.0);
})";

	namespace
	{
		FString LoadIncludeFile(const FString& IncludePath)
		{
			if (FPaths::GetExtension(IncludePath) == TEXT("header"))
			{
				FScopeLock ScopeLock(&GAssetCS);
				AssetPtr<ShaderHeader> HeaderAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<ShaderHeader>(IncludePath);
				return HeaderAsset ? HeaderAsset->GetFullContent() : "";
			}

			FString Content;
			FFileHelper::LoadFileToString(Content, *IncludePath);
			return Content;
		}

		const FString& GetDefaultPixelShader(GpuShaderLanguage Language)
		{
			return Language == GpuShaderLanguage::GLSL ? DefaultGlslPixelShader : DefaultHlslPixelShader;
		}

		TSharedRef<PropertyData> CreateEntryPointProperty(PixelShader& InShaderAsset)
		{
			auto EntryPointProperty = MakeShared<PropertyItemBase>(&InShaderAsset, LOCALIZATION("EntryPoint"));
			PropertyData* EntryPointPropertyPtr = &EntryPointProperty.Get();
			EntryPointProperty->SetEmbedWidget(
				SNew(SEditableTextBox)
				.IsEnabled_Lambda([&InShaderAsset] {
					return InShaderAsset.Language != GpuShaderLanguage::GLSL;
				})
				.Text_Lambda([&InShaderAsset] {
					return FText::FromString(InShaderAsset.EntryPoint);
				})
				.OnTextCommitted_Lambda([EntryPointPropertyPtr, &InShaderAsset](const FText& InText, ETextCommit::Type InCommitType) {
					FString NewEntryPoint = InText.ToString().TrimStartAndEnd();
					if (InShaderAsset.EntryPoint == NewEntryPoint)
					{
						return;
					}

					const FString OldEntryPoint = InShaderAsset.EntryPoint;
					InShaderAsset.EntryPoint = NewEntryPoint;
					if (!InShaderAsset.CanChangeProperty(EntryPointPropertyPtr))
					{
						InShaderAsset.EntryPoint = OldEntryPoint;
						return;
					}
					InShaderAsset.PostPropertyChanged(EntryPointPropertyPtr);
				})
			);
			return EntryPointProperty;
		}
	}

	PixelShader::PixelShader()
		: Language(GpuShaderLanguage::HLSL)
		, EntryPoint(TEXT("main"))
	{
		OnShaderRefreshed.AddRaw(this, &PixelShader::HandleShaderRefreshed);
		EditorContent = DefaultHlslPixelShader;
	}

	void PixelShader::Serialize(FArchive& Ar)
	{
		ShaderAsset::Serialize(Ar);
		Ar << Language;
		Ar << EntryPoint;
	}

	void PixelShader::PostLoad()
	{
		AssetObject::PostLoad();
		Shader = GGpuRhi->CreateShaderFromSource(GetShaderDesc(GetFullContent()));
		FString ErrorInfo;
		FString WarnInfo;
		bCompilationSucceed = GGpuRhi->CompileShader(Shader, ErrorInfo, WarnInfo);
	}

	FString PixelShader::FileExtension() const
	{
		return "pixelShader";
	}

	const FSlateBrush* PixelShader::GetImage() const
	{
		return FShaderHelperStyle::Get().GetBrush("AssetBrowser.Shader");
	}

	TArray<TSharedRef<PropertyData>> PixelShader::GeneratePropertyDatas()
	{
		TArray<TSharedRef<PropertyData>> Result = ShObject::GeneratePropertyDatas();
		Result.Add(CreateEntryPointProperty(*this));
		Result.Append(BuildBindingPropertyDatas());
		return Result;
	}

	FString PixelShader::GetFullContent() const
	{
		return EditorContent;
	}

	GpuShaderSourceDesc PixelShader::GetShaderDesc(const FString& InContent) const
	{
		return {
			.Name = GetShaderName(),
			.Source = InContent,
			.Type = ShaderType::PixelShader,
			.EntryPoint = EntryPoint,
			.Language = Language,
			.IncludeDirs = GetIncludeDirs(),
			.IncludeHandler = [](const FString& IncludePath) -> FString {
				return LoadIncludeFile(IncludePath);
			},
		};
	}

	bool PixelShader::CanChangeProperty(PropertyData* InProperty)
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
		else if (InProperty->GetDisplayName().EqualTo(LOCALIZATION("EntryPoint")) && EntryPoint.TrimStartAndEnd().IsEmpty())
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, ShEditor->GetMainWindow(), LOCALIZATION("ShaderEntryPointEmptyTip"));
			return false;
		}
		return true;
	}

	void PixelShader::PostPropertyChanged(PropertyData* InProperty)
	{
        ShObject::PostPropertyChanged(InProperty);
		if (InProperty->IsOfType<PropertyEnumItem>() && InProperty->GetDisplayName().EqualTo(LOCALIZATION("Language")))
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			GpuShaderLanguage NewLanguage = *static_cast<GpuShaderLanguage*>(static_cast<PropertyEnumItem*>(InProperty)->GetEnum());
			EntryPoint = TEXT("main");
			ShEditor->GetShaderEditor(this)->SetText(FText::FromString(GetDefaultPixelShader(NewLanguage)));
			ShEditor->RefreshProperty();
		}
	}

	void PixelShader::HandleShaderRefreshed()
	{
		auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		auto& AM = TSingleton<AssetManager>::Get();
		TSet<FGuid> DependentGuids = AM.GetAssetDependents(GetGuid());
		if (FW::SAssetBrowser* AssetBrowser = ShEditor->GetAssetBrowser())
		{
			for (const FGuid& DepGuid : DependentGuids)
			{
				if (!AM.FindLoadedAsset(DepGuid) && AM.IsValidAsset(DepGuid))
				{
					AssetBrowser->RefreshAssetThumbnail(AM.GetPath(DepGuid));
				}
			}
		}

		ShEditor->RefreshProperty();
	}
}
