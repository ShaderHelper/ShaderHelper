#pragma once
#include "Renderer/Renderer.h"
#include "GpuApi/GpuResource.h"
#include "App/PreviewViewPort.h"
#include "Renderer/RenderResource/ArgumentBuffer.h"
#include "UI/Widgets/Property/PropertyData.h"

namespace SH
{
	class ShRenderer : public Renderer
	{
	public:
		ShRenderer(PreviewViewPort* InViewPort);
		
	public:
		void RenderInternal() override;
		void OnViewportResize();
		void UpdatePixelShader(TRefCountPtr<GpuShader> InNewPixelShader);
		void UpdateCustomArgumentBuffer(TSharedPtr<ArgumentBuffer> InBuffer) { NewCustomArgumentBuffer = MoveTemp(InBuffer); }
		void UpdateCustomArgumentBufferLayout(TSharedPtr<ArgumentBufferLayout> InLayout) { NewCustomArgumentBufferLayout = MoveTemp(InLayout); }
		FString GetResourceDeclaration() const;
		TArray<TSharedRef<PropertyData>> GetBuiltInPropertyDatas() const;

	private:
		void RenderBegin() override;
		void RenderEnd() override;
		void ReCreatePipelineState();
		void RenderNewRenderPass();
		void RenderOldRenderPass();

	public:
		static const FString DefaultVertexShaderText;
		static const FString DefaultPixelShaderText;
		static const FString DefaultPixelShaderInput;
		static const FString DefaultPixelShaderMacro;

	private:
		PreviewViewPort* ViewPort;
		TRefCountPtr<GpuTexture> FinalRT;
		TRefCountPtr<GpuShader> VertexShader;
		TSharedPtr<UniformBuffer> BuiltInUniformBuffer;
		float iTime;
		Vector2f iResolution;

		TUniquePtr<ArgumentBuffer> BuiltInArgumentBuffer;
		TUniquePtr<ArgumentBufferLayout> BuiltInArgumentBufferLayout;

		//keep previous state if failed to compile new pixel shader.
		bool bCompileSuccessful;

		TRefCountPtr<GpuPipelineState> NewPipelineState;
		TRefCountPtr<GpuPipelineState> OldPipelineState;

		TRefCountPtr<GpuShader> NewPixelShader; //nullptr if failed
		TRefCountPtr<GpuShader> OldPixelShader;

		TSharedPtr<ArgumentBuffer> NewCustomArgumentBuffer;
		TSharedPtr<ArgumentBuffer> OldCustomArgumentBuffer;

		TSharedPtr<ArgumentBufferLayout> NewCustomArgumentBufferLayout;
		TSharedPtr<ArgumentBufferLayout> OldCustomArgumentBufferLayout;
	};
}


