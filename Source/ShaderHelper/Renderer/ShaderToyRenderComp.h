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
		int FrameCount = 0;
		FW::Vector3f iResolution{};
		FW::Vector4f iMouse{};
		FW::RenderGraph* RG = nullptr;
        TRefCountPtr<FW::GpuTexture> FinalRT;
		TRefCountPtr<FW::GpuTexture> Keyboard;
		FW::PreviewViewPort* ViewPort;
		TArray<ShaderToyOutputDesc> Ontputs;
		bool bResetPreviousFrame{};
	};

	class ShaderToyRenderComp : public FW::RenderComponent
	{
	public:
		ShaderToyRenderComp(ShaderToy* InShaderToyGraph, FW::PreviewViewPort* InViewPort);
        ~ShaderToyRenderComp();

	public:
		void RenderBegin();
		void RenderInternal() override;
		void RefreshKeyboard();

		ShaderToyExecContext Context;

	private:
		ShaderToy* ShaderToyGraph;
		TSet<uint32> PressedKeys;
		FDelegateHandle ResizeHandle;
		FDelegateHandle FocusLostHandle;
		FDelegateHandle KeyDownHandle, KeyUpHandle;
		FDelegateHandle MouseDownHandle, MouseUpHandle;
		FDelegateHandle MouseMoveHandle;
	};
}
