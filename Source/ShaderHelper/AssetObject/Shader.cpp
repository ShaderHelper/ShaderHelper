#include "CommonHeader.h"
#include "Shader.h"
#include "AssetManager/AssetManager.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<Shader>("Shader")
		.BaseClass<ShaderAsset>()
		.Data<&Shader::Language, MetaInfo::Property>(LOCALIZATION("Language"))
	)

	const FString DefaultHlslShader =
R"(cbuffer Transform : register(b0)
{
    float4x4 MVP;
};

struct VSInput
{
    float3 Position : POSITION;
    float2 UV : TEXCOORD0;
};

struct VSOutput
{
    float4 Pos : SV_Position;
    float2 UV : TEXCOORD0;
};

VSOutput MainVS(VSInput In)
{
    VSOutput Out;
    Out.Pos = mul(float4(In.Position, 1.0), MVP);
    Out.UV = In.UV;
    return Out;
}

float4 MainPS(VSOutput In) : SV_Target
{
    return float4(In.UV, 0, 1);
})";

	const FString DefaultGlslShader =
R"(#version 450
layout(binding = 0) uniform Transform
{
    mat4 MVP;
};

#ifdef VERTEX
layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 UV;

layout(location = 0) out vec2 vUV;
void main()
{
    vUV = UV;
    gl_Position = MVP * vec4(Position, 1.0);
}
#endif

#ifdef PIXEL
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 fragColor;
void main()
{
    fragColor = vec4(vUV, 0, 1);
}
#endif)";

	namespace
	{
		const FString& GetDefaultShader(GpuShaderLanguage Language)
		{
			return Language == GpuShaderLanguage::GLSL ? DefaultGlslShader : DefaultHlslShader;
		}

		TSharedRef<PropertyData> CreateEntryPointProperty(Shader& InShaderAsset, int32 StageIndex)
		{
			auto EntryPointProperty = MakeShared<PropertyItemBase>(&InShaderAsset, LOCALIZATION("EntryPoint"));
			PropertyData* EntryPointPropertyPtr = &EntryPointProperty.Get();
			EntryPointProperty->SetEmbedWidget(
				SNew(SEditableTextBox)
				.IsEnabled_Lambda([&InShaderAsset] {
					return InShaderAsset.Language != GpuShaderLanguage::GLSL;
				})
				.Text_Lambda([&InShaderAsset, StageIndex] {
					return FText::FromString(InShaderAsset.EntryPoints[StageIndex]);
				})
				.OnTextCommitted_Lambda([EntryPointPropertyPtr, &InShaderAsset, StageIndex](const FText& InText, ETextCommit::Type InCommitType) {
					FString NewEntryPoint = InText.ToString().TrimStartAndEnd();
					if (InShaderAsset.EntryPoints[StageIndex] == NewEntryPoint)
					{
						return;
					}

					const FString OldEntryPoint = InShaderAsset.EntryPoints[StageIndex];
					InShaderAsset.EntryPoints[StageIndex] = NewEntryPoint;
					if (!InShaderAsset.CanChangeProperty(EntryPointPropertyPtr))
					{
						InShaderAsset.EntryPoints[StageIndex] = OldEntryPoint;
						return;
					}
					InShaderAsset.PostPropertyChanged(EntryPointPropertyPtr);
				})
			);
			return EntryPointProperty;
		}

		TSharedRef<PropertyData> CreateStageMacrosProperty(Shader& InShaderAsset, int32 StageIndex)
		{
			auto MacrosProperty = MakeShared<PropertyItemBase>(&InShaderAsset, LOCALIZATION("Macro"));
			PropertyData* MacrosPropertyPtr = &MacrosProperty.Get();
			MacrosProperty->SetEmbedWidget(
				SNew(SEditableTextBox)
				.Text_Lambda([&InShaderAsset, StageIndex] {
					return FText::FromString(FString::Join(InShaderAsset.StageMacros[StageIndex], TEXT(" ")));
				})
				.OnTextCommitted_Lambda([MacrosPropertyPtr, &InShaderAsset, StageIndex](const FText& InText, ETextCommit::Type InCommitType) {
					FString MacroString = InText.ToString().TrimStartAndEnd();
					TArray<FString> NewMacros;
					MacroString.ParseIntoArray(NewMacros, TEXT(" "), true);
					if (InShaderAsset.StageMacros[StageIndex] == NewMacros)
					{
						return;
					}

					InShaderAsset.StageMacros[StageIndex] = MoveTemp(NewMacros);
					InShaderAsset.PostPropertyChanged(MacrosPropertyPtr);
				})
			);
			return MacrosProperty;
		}

	}

	Shader::Shader()
		: Language(GpuShaderLanguage::HLSL)
	{
		EnabledStages = ShaderStageFlag::Vertex | ShaderStageFlag::Pixel;
		EntryPoints[0] = TEXT("MainVS");
		EntryPoints[1] = TEXT("MainPS");
		EntryPoints[2] = TEXT("main");
		EditorContent = DefaultHlslShader;
		OnShaderRefreshed.AddRaw(this, &Shader::HandleShaderRefreshed);
	}

	void Shader::Serialize(FArchive& Ar)
	{
		ShaderAsset::Serialize(Ar);
		Ar << Language;
		Ar << EnabledStages;
		for (int32 i = 0; i < StageCount; ++i)
		{
			Ar << EntryPoints[i];
			Ar << StageMacros[i];
		}
	}

	void Shader::PostLoad()
	{
		AssetObject::PostLoad();
		FString ErrorInfo, WarnInfo;
		CompileShader(ErrorInfo, WarnInfo);
	}

	bool Shader::CompileShader(FString& OutError, FString& OutWarn)
	{
		bool bAllSucceeded = true;
		for (ShaderType Stage : magic_enum::enum_values<ShaderType>())
		{
			if (!IsStageEnabled(Stage)) continue;
			int32 Index = (int32)Stage;
			ShaderDesc StageDesc = GetShaderDesc(GetFullContent(), Stage);
			CompiledShaders[Index] = GGpuRhi->CreateShaderFromSource(StageDesc.SourceDesc);
			FString StageError, StageWarn;
			StageCompileResults[Index] = GGpuRhi->CompileShader(CompiledShaders[Index], StageError, StageWarn, StageDesc.ExtraArgs);
			if (!StageError.IsEmpty())
			{
				if (!OutError.IsEmpty()) OutError += TEXT("\n");
				OutError += StageError;
			}
			if (!StageWarn.IsEmpty())
			{
				if (!OutWarn.IsEmpty()) OutWarn += TEXT("\n");
				OutWarn += StageWarn;
			}
			if (!StageCompileResults[Index])
			{
				bAllSucceeded = false;
			}
		}
		return bAllSucceeded;
	}

	bool Shader::IsCompilationSucceeded() const
	{
		for (ShaderType Stage : magic_enum::enum_values<ShaderType>())
		{
			if (IsStageEnabled(Stage) && !StageCompileResults[(int32)Stage])
				return false;
		}
		return EnabledStages != ShaderStageFlag::None;
	}

	bool Shader::IsStageEnabled(ShaderType InStage) const
	{
		return EnumHasAnyFlags(EnabledStages, StageToFlag(InStage));
	}

	GpuShader* Shader::GetCompiledShader(ShaderType InStage) const
	{
		GpuShader* Shader = CompiledShaders[(int32)InStage].GetReference();
		return (Shader && Shader->IsCompiled()) ? Shader : nullptr;
	}

	TArray<ShaderType> Shader::GetEnabledStageList() const
	{
		TArray<ShaderType> Result;
		for (ShaderType Stage : magic_enum::enum_values<ShaderType>())
		{
			if (IsStageEnabled(Stage))
				Result.Add(Stage);
		}
		return Result;
	}

	ShaderStageFlag Shader::StageToFlag(ShaderType InStage)
	{
		switch (InStage)
		{
		case ShaderType::Vertex:  return ShaderStageFlag::Vertex;
		case ShaderType::Pixel:   return ShaderStageFlag::Pixel;
		case ShaderType::Compute: return ShaderStageFlag::Compute;
		default: return ShaderStageFlag::None;
		}
	}

	FString Shader::FileExtension() const
	{
		return "shader";
	}

	const FSlateBrush* Shader::GetImage() const
	{
		return FShaderHelperStyle::Get().GetBrush("AssetBrowser.Shader");
	}

	TArray<TSharedRef<PropertyData>> Shader::GeneratePropertyDatas()
	{
		TArray<TSharedRef<PropertyData>> Result = ShObject::GeneratePropertyDatas();

		for (ShaderType Stage : magic_enum::enum_values<ShaderType>())
		{
			ShaderStageFlag Flag = StageToFlag(Stage);
			int32 Index = (int32)Stage;
			FString StageName = ANSI_TO_TCHAR(magic_enum::enum_name(Flag).data());
			auto StageCategory = MakeShared<PropertyCategory>(this, StageName);

			StageCategory->SetCheckBox(
				TAttribute<ECheckBoxState>::CreateLambda([this, Flag] {
					return EnumHasAnyFlags(EnabledStages, Flag) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				}),
				FOnCheckStateChanged::CreateLambda([this, Flag, Stage](ECheckBoxState NewState) {
					if (NewState == ECheckBoxState::Checked)
						EnabledStages |= Flag;
					else
					{
						EnabledStages &= ~Flag;
						int32 Idx = (int32)Stage;
						CompiledShaders[Idx].SafeRelease();
						StageCompileResults[Idx] = false;
					}

					MarkDirty();
					auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
					if (SShaderEditorBox* EditorBox = ShEditor->GetShaderEditor(this))
					{
						EditorBox->SyncStageSelectorOptions(GetEnabledStageList());
					}
					ShEditor->RefreshProperty();
				})
			);

			if (IsStageEnabled(Stage))
			{
				StageCategory->AddChild(CreateEntryPointProperty(*this, Index));
				StageCategory->AddChild(CreateStageMacrosProperty(*this, Index));

				GpuShader* Compiled = GetCompiledShader(Stage);
				if (Compiled)
				{
					StageCategory->AddChilds(BuildBindingPropertyDatas(Compiled));
				}
			}

			Result.Add(StageCategory);
		}

		return Result;
	}

	FString Shader::GetFullContent() const
	{
		return EditorContent;
	}

	ShaderDesc Shader::GetShaderDesc(const FString& InContent, ShaderType InStage) const
	{
		ShaderDesc Desc;
		Desc.SourceDesc = {
			.Name = GetShaderName(),
			.Source = InContent,
			.Type = InStage,
			.EntryPoint = EntryPoints[(int32)InStage],
			.Language = Language,
			.IncludeDirs = GetIncludeDirs(),
			.IncludeHandler = &ShaderAsset::LoadIncludeFile,
		};
		for (const FString& Macro : StageMacros[(int32)InStage])
		{
			Desc.ExtraArgs.Add(TEXT("-D"));
			Desc.ExtraArgs.Add(Macro);
		}

		return Desc;
	}

	bool Shader::CanChangeProperty(PropertyData* InProperty)
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

	void Shader::PostPropertyChanged(PropertyData* InProperty)
	{
		ShObject::PostPropertyChanged(InProperty);
		if (InProperty->IsOfType<PropertyEnumItem>() && InProperty->GetDisplayName().EqualTo(LOCALIZATION("Language")))
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			if (Language == GpuShaderLanguage::GLSL)
			{
				for (int32 i = 0; i < StageCount; ++i)
				{
					EntryPoints[i] = TEXT("main");
				}
				StageMacros[0] = { TEXT("VERTEX") };
				StageMacros[1] = { TEXT("PIXEL") };
				StageMacros[2] = { TEXT("COMPUTE") };
			}
			else
			{
				EntryPoints[0] = TEXT("MainVS");
				EntryPoints[1] = TEXT("MainPS");
				EntryPoints[2] = TEXT("MainCS");
				for (int32 i = 0; i < StageCount; ++i)
				{
					StageMacros[i].Empty();
				}
			}
			ShEditor->GetShaderEditor(this)->SetText(FText::FromString(GetDefaultShader(Language)));
			ShEditor->RefreshProperty();
		}
	}

	void Shader::HandleShaderRefreshed()
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
