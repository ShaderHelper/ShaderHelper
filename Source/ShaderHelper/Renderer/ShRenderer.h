#pragma once
#include "Renderer/Renderer.h"
#include "GpuApi/GpuResource.h"
#include "App/PreviewViewPort.h"
#include "Renderer/RenderResource/UniformBuffer.h"

namespace SH
{
	class ShRenderer : public Renderer
	{
	public:
		ShRenderer();
		
	public:
		void RenderInternal() override;
		void OnViewportResize();
		void UpdatePixelShader(TRefCountPtr<GpuShader> NewPixelShader);

	private:
		void RenderBegin() override;
		void RenderEnd() override;
		void ReCreatePipelineState();

	public:
		PreviewViewPort* ViewPort;
		static const FString DefaultVertexShaderText;
		static const FString DefaultPixelShaderText;
		static const FString DefaultPixelShaderInput;

	private:
		TRefCountPtr<GpuTexture> FinalRT;
		TRefCountPtr<GpuShader> VertexShader;
		TRefCountPtr<GpuShader> PixelShader;
		TRefCountPtr<GpuPipelineState> PipelineState;
		
		TUniquePtr<UniformBuffer> BuiltInUniformBuffer;
		TRefCountPtr<GpuBindGroup> CustomBindGroup;
		TRefCountPtr<GpuBindGroup> BuiltInBindGroup;
	};
}


