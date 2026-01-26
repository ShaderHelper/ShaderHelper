#pragma once
#include "ShaderAsset.h"

namespace SH
{
	class ShaderHeader : public ShaderAsset
	{
		REFLECTION_TYPE(ShaderHeader)
	public:
		ShaderHeader();

		//AssetObject interface
		void Serialize(FArchive& Ar) override;
		FString FileExtension() const override;
		const FSlateBrush* GetImage() const override;

		//ShaderAsset interface
		FString GetFullContent() const override;
		int32 GetExtraLineNum() const override { return 0; }
		FW::GpuShaderSourceDesc GetShaderDesc(const FString& InContent) const override;

	public:
		FW::GpuShaderLanguage Language;
	};

}
