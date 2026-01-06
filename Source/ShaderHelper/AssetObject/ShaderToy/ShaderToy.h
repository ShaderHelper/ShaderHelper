#pragma once
#include "AssetObject/Graph.h"

namespace SH
{
	enum class ShaderToyFormat
	{
		//Note: BGRA8_UNORM is default framebuffer format in ue standalone renderer framework.
		B8G8R8A8_UNORM = (int)FW::GpuTextureFormat::B8G8R8A8_UNORM,
		R32G32B32A32_FLOAT = (int)FW::GpuTextureFormat::R32G32B32A32_FLOAT
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
		
		void OnDragEnter(TSharedPtr<FDragDropOperation> DragDropOp) override;
		void OnDrop(TSharedPtr<FDragDropOperation> DragDropOp, const FW::Vector2D& Pos) override;
	};
}
