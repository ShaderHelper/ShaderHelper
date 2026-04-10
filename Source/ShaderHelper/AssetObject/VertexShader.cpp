#include "CommonHeader.h"
#include "VertexShader.h"
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
	REFLECTION_REGISTER(AddClass<VertexShader>("VertexShader")
		.BaseClass<ShaderAsset>()
		.Data<&VertexShader::Language, MetaInfo::Property>(LOCALIZATION("Language"))
	)

	const FString DefaultHlslVertexShader =
	R"(struct VSInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD0;
};

cbuffer TransformUb
{
	float4x4 Model;
	float4x4 ViewProj;
};

struct VSOutput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD0;
};

VSOutput main(VSInput Input)
{
	VSOutput Output;
	Output.Position = mul(mul(float4(Input.Position, 1.0), Model), ViewProj);
	Output.TexCoord = Input.TexCoord;
	return Output;
})";

	const FString DefaultGlslVertexShader =
	R"(#version 450
layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TexCoord;

layout(std140, binding = 0) uniform TransformUb
{
	mat4 Model;
	mat4 ViewProj;
};

layout(location = 0) out vec2 OutTexCoord;

void main()
{
	OutTexCoord = TexCoord;
	gl_Position = ViewProj * Model * vec4(Position, 1.0);
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

		const FString& GetDefaultVertexShader(GpuShaderLanguage Language)
		{
			return Language == GpuShaderLanguage::GLSL ? DefaultGlslVertexShader : DefaultHlslVertexShader;
		}

		TSharedRef<PropertyData> CreateEntryPointProperty(VertexShader& InShaderAsset)
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

	VertexShader::VertexShader()
		: Language(GpuShaderLanguage::HLSL)
		, EntryPoint(TEXT("main"))
	{
		OnShaderRefreshed.AddRaw(this, &VertexShader::HandleShaderRefreshed);
		EditorContent = DefaultHlslVertexShader;
	}

	void VertexShader::Serialize(FArchive& Ar)
	{
		ShaderAsset::Serialize(Ar);
		Ar << Language;
		Ar << EntryPoint;
	}

	void VertexShader::PostLoad()
	{
		AssetObject::PostLoad();
		Shader = GGpuRhi->CreateShaderFromSource(GetShaderDesc(GetFullContent()));
		FString ErrorInfo;
		FString WarnInfo;
		bCompilationSucceed = GGpuRhi->CompileShader(Shader, ErrorInfo, WarnInfo);
	}

	FString VertexShader::FileExtension() const
	{
		return "vertexShader";
	}

	const FSlateBrush* VertexShader::GetImage() const
	{
		return FShaderHelperStyle::Get().GetBrush("AssetBrowser.Shader");
	}

	TArray<TSharedRef<PropertyData>> VertexShader::GeneratePropertyDatas()
	{
		TArray<TSharedRef<PropertyData>> Result = ShObject::GeneratePropertyDatas();
		Result.Add(CreateEntryPointProperty(*this));
		Result.Append(BuildBindingPropertyDatas());
		return Result;
	}

	FString VertexShader::GetFullContent() const
	{
		return EditorContent;
	}

	GpuShaderSourceDesc VertexShader::GetShaderDesc(const FString& InContent) const
	{
		return {
			.Name = GetShaderName(),
			.Source = InContent,
			.Type = ShaderType::VertexShader,
			.EntryPoint = EntryPoint,
			.Language = Language,
			.IncludeDirs = GetIncludeDirs(),
			.IncludeHandler = [](const FString& IncludePath) -> FString {
				return LoadIncludeFile(IncludePath);
			},
		};
	}

	bool VertexShader::CanChangeProperty(PropertyData* InProperty)
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

	void VertexShader::PostPropertyChanged(PropertyData* InProperty)
	{
        ShObject::PostPropertyChanged(InProperty);
		if (InProperty->IsOfType<PropertyEnumItem>() && InProperty->GetDisplayName().EqualTo(LOCALIZATION("Language")))
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			GpuShaderLanguage NewLanguage = *static_cast<GpuShaderLanguage*>(static_cast<PropertyEnumItem*>(InProperty)->GetEnum());
			EntryPoint = TEXT("main");
			ShEditor->GetShaderEditor(this)->SetText(FText::FromString(GetDefaultVertexShader(NewLanguage)));
			ShEditor->RefreshProperty();
		}
	}

	void VertexShader::HandleShaderRefreshed()
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
