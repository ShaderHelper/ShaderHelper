#pragma once
#include "Renderer/RenderComponent.h"
#include "Editor/PreviewViewPort.h"
#include "AssetObject/ShaderToy/ShaderToy.h"
#include "RenderResource/UniformBuffer.h"
#include "Renderer/RenderGraph.h"
#include "RenderResource/RenderPass/BlitPass.h"

namespace SH
{
	struct ShaderToyOutputDesc
	{
		FW::BlitPassInput Pass;
		int32 Layer;
	};

	struct ShaderToyExecContext : FW::GraphExecContext
	{
		float iTime{};
		FW::Vector3f iResolution{};
		FW::Vector4f iMouse{};
		FW::RenderGraph* RG = nullptr;
        TRefCountPtr<FW::GpuTexture> FinalRT;
		FW::PreviewViewPort* ViewPort;
		TArray<ShaderToyOutputDesc> Ontputs;
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
