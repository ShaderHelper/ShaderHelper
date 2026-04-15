#pragma once
#include "ShaderAsset.h"

namespace SH
{
	enum class ShaderStageFlag : uint8
	{
		None = 0,
		Vertex = 1 << 0,
		Pixel = 1 << 1,
		Compute = 1 << 2,
	};
	ENUM_CLASS_FLAGS(ShaderStageFlag)

	class Shader : public ShaderAsset
	{
		REFLECTION_TYPE(Shader)
	public:
		static constexpr int32 StageCount = (int32)magic_enum::enum_count<FW::ShaderType>();

		Shader();

		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		FString FileExtension() const override;
		const FSlateBrush* GetImage() const override;
		TArray<TSharedRef<FW::PropertyData>> GeneratePropertyDatas() override;

		FString GetFullContent() const override;
		int32 GetExtraLineNum() const override { return 0; }
		ShaderDesc GetShaderDesc(const FString& InContent, FW::ShaderType InStage = FW::ShaderType::Vertex) const override;

		bool CanChangeProperty(FW::PropertyData* InProperty) override;
		void PostPropertyChanged(FW::PropertyData* InProperty) override;
		void HandleShaderRefreshed();

		bool IsCompilable() const override { return EnabledStages != ShaderStageFlag::None; }
		bool CompileShader(FString& OutError, FString& OutWarn) override;
		bool IsCompilationSucceeded() const override;

		bool IsStageEnabled(FW::ShaderType InStage) const;
		FW::GpuShader* GetCompiledShader(FW::ShaderType InStage) const;
		TArray<FW::ShaderType> GetEnabledStageList() const;

		static ShaderStageFlag StageToFlag(FW::ShaderType InStage);

	public:
		FW::GpuShaderLanguage Language;
		ShaderStageFlag EnabledStages;
		FString EntryPoints[StageCount];
		TArray<FString> StageMacros[StageCount];
		TRefCountPtr<FW::GpuShader> CompiledShaders[StageCount];
		bool StageCompileResults[StageCount]{};
	};
}
