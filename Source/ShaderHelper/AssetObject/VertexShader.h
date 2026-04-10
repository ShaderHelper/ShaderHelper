#pragma once
#include "ShaderAsset.h"

namespace SH
{
	class VertexShader : public ShaderAsset
	{
		REFLECTION_TYPE(VertexShader)
	public:
		VertexShader();

		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		FString FileExtension() const override;
		const FSlateBrush* GetImage() const override;
		TArray<TSharedRef<FW::PropertyData>> GeneratePropertyDatas() override;

		FString GetFullContent() const override;
		int32 GetExtraLineNum() const override { return 0; }
		FW::GpuShaderSourceDesc GetShaderDesc(const FString& InContent) const override;

		bool CanChangeProperty(FW::PropertyData* InProperty) override;
		void PostPropertyChanged(FW::PropertyData* InProperty) override;
		void HandleShaderRefreshed();

	public:
		FW::GpuShaderLanguage Language;
		FString EntryPoint;
	};
}
