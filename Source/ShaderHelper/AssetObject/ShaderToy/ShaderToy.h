#pragma once
#include "AssetObject/Graph.h"

namespace SH
{
	enum class ShaderToyFormat
	{
		//Note: BGRA8_UNORM is default framebuffer format in ue standalone renderer framework.
		B8G8R8A8_UNORM = (int)FW::GpuTextureFormat::B8G8R8A8_UNORM,
	};

	class ShaderToy : public FW::Graph
	{
		REFLECTION_TYPE(ShaderToy)
	public:
		ShaderToy() = default;
        ~ShaderToy();

	public:
		void Serialize(FArchive& Ar) override;
		FString FileExtension() const override;
		TArray<FW::MetaType*> SupportNodes() const override;
	};
}
