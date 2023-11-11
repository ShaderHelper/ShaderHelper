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
		void UpdatePixelShader(TRefCountPtr<GpuShader> NewPixelShader);
		FString GetResourceDeclaration() const;
		void SetCustomArgumentBuffer(TUniquePtr<ArgumentBuffer> InArgumentBuffer, TUniquePtr<ArgumentBufferLayout> InArgumentBufferLayout) 
		{
			CustomArgumentBuffer = MoveTemp(InArgumentBuffer);
			CustomArgumentBufferLayout = MoveTemp(InArgumentBufferLayout);
		}
		TArray<PropertyData> GetBuiltInPropertyDatas() const;

	private:
		void RenderBegin() override;
		void RenderEnd() override;
		void ReCreatePipelineState();

	public:
		static const FString DefaultVertexShaderText;
		static const FString DefaultPixelShaderText;
		static const FString DefaultPixelShaderInput;
		static const FString DefaultPixelShaderMacro;

	private:
		PreviewViewPort* ViewPort;
		TRefCountPtr<GpuTexture> FinalRT;
		TRefCountPtr<GpuShader> VertexShader;
		TRefCountPtr<GpuShader> PixelShader;
		TRefCountPtr<GpuPipelineState> PipelineState;
		
		
		TSharedPtr<UniformBuffer> BuiltInUniformBuffer;
		float iTime;
		Vector2f iResolution;

		TUniquePtr<ArgumentBuffer> BuiltInArgumentBuffer;
		TUniquePtr<ArgumentBufferLayout> BuiltInArgumentBufferLayout;

		TUniquePtr<ArgumentBuffer> CustomArgumentBuffer;
		TUniquePtr<ArgumentBufferLayout> CustomArgumentBufferLayout;
	};
}


