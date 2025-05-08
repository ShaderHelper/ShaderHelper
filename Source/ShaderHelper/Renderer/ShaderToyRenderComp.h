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
	};

	class ShaderToyRenderComp : public FW::RenderComponent
	{
	public:
		ShaderToyRenderComp(ShaderToy* InShaderToyGraph, FW::PreviewViewPort* InViewPort);
        ~ShaderToyRenderComp();

	public:
		void OnViewportResize(const FW::Vector2f& InResolution);
		void OnMouseDown(const FPointerEvent& MouseEvent);
		void OnMouseMove(const FPointerEvent& MouseEvent);
		void OnMouseUp(const FPointerEvent& MouseEvent);

		void RenderBegin();
		void RenderInternal() override;

	private:
		ShaderToy* ShaderToyGraph;
		FW::PreviewViewPort* ViewPort;
		ShaderToyExecContext Context;
        FDelegateHandle ResizeHandle;
		FDelegateHandle MouseDownHandle, MouseUpHandle;
		FDelegateHandle MouseMoveHandle;
	};
}
