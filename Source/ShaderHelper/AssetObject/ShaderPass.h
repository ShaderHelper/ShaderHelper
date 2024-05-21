#pragma once
#include "AssetObject/AssetObject.h"

namespace SH
{
	class ShaderPass : public FRAMEWORK::AssetObject
	{
	public:
		ShaderPass();

	public:
		void Serialize(FArchive& Ar) override;
		FString FileExtension() const override;
		const FSlateBrush* GetImage() const override;

	private:
		FString PixelShaderBody;
	};

}
