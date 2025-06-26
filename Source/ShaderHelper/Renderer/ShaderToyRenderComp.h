#pragma once
#include "Renderer/RenderComponent.h"
#include "Editor/PreviewViewPort.h"
#include "AssetObject/ShaderToy/ShaderToy.h"
#include "RenderResource/UniformBuffer.h"
#include "Renderer/RenderGraph.h"

namespace SH
{
	struct ShaderToyExecContext : FW::GraphExecContext
	{
		float iTime{};
		FW::Vector2f iResolution{};
		FW::Vector4f iMouse{};
		FW::RenderGraph* RG = nullptr;
        TRefCountPtr<FW::GpuTexture> FinalRT;
		FW::PreviewViewPort* ViewPort;
	};

	class ShaderToyRenderComp : public FW::RenderComponent
	{
	public:
		ShaderToyRenderComp(ShaderToy* InShaderToyGraph, FW::PreviewViewPort* InViewPort);
        ~ShaderToyRenderComp();

	public:
		void OnViewportResize(const FW::Vector2f& InResolution);

		void RenderBegin();
		void RenderInternal() override;

	private:
		ShaderToy* ShaderToyGraph;
		ShaderToyExecContext Context;
        FDelegateHandle ResizeHandle;
	};
}
