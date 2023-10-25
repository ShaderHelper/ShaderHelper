#pragma once
#include "Renderer/Renderer.h"
#include "GpuApi/GpuResource.h"
#include "App/PreviewViewPort.h"
#include "Renderer/RenderResource/ArgumentBuffer.h"

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
		FString GetResourceDeclaration() const;

	private:
		void RenderBegin() override;
		void RenderEnd() override;
		void ReCreatePipelineState();

	public:
		PreviewViewPort* ViewPort;
		static const FString DefaultVertexShaderText;
		static const FString DefaultPixelShaderText;
		static const FString DefaultPixelShaderInput;

		TUniquePtr<ArgumentBuffer> CustomArgumentBuffer;

	private:
		TRefCountPtr<GpuTexture> FinalRT;
		TRefCountPtr<GpuShader> VertexShader;
		TRefCountPtr<GpuShader> PixelShader;
		TRefCountPtr<GpuPipelineState> PipelineState;
		
		
		TSharedPtr<UniformBuffer> BuiltInUniformBuffer;
		float iTime;
		Vector2f iResolution, iMouse;

		TUniquePtr<ArgumentBuffer> BuiltInArgumentBuffer;
		TUniquePtr<ArgumentBufferLayout> BuiltInArgumentBufferLayout;
	};
}


