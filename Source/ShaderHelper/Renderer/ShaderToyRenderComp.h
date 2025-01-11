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
		float iTime{ 0 };
		FW::Vector2f iResolution{ 0 };
		FW::RenderGraph* RG = nullptr;
		TRefCountPtr<FW::GpuTexture> FinalRT;
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
		FW::PreviewViewPort* ViewPort;
		ShaderToyExecContext Context;
        FDelegateHandle ResizeHandle;
	};
}
