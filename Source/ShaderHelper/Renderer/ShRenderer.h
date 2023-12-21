#pragma once
#include "Renderer/Renderer.h"
#include "GpuApi/GpuResource.h"
#include "RenderResource/ArgumentBuffer.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"

namespace SH
{
	class ShRenderer : public Renderer
	{
	public:
		ShRenderer();
		
	public:
		void RenderInternal() override;
		void OnViewportResize(const Vector2f& InResolution);
		void UpdatePixelShader(TRefCountPtr<GpuShader> InNewPixelShader);
		void UpdateCustomArgumentBuffer(TSharedPtr<ArgumentBuffer> InBuffer) { NewCustomArgumentBuffer = MoveTemp(InBuffer); }
		void UpdateCustomArgumentBufferLayout(TSharedPtr<ArgumentBufferLayout> InLayout) { NewCustomArgumentBufferLayout = MoveTemp(InLayout); }
		FString GetPixelShaderDeclaration() const;
		FString GetDefaultPixelShaderBody() const;
		GpuTexture* GetFinalRT() const { return FinalRT; }
		TArray<TSharedRef<PropertyData>> GetBuiltInPropertyDatas() const;

	private:
		void RenderBegin() override;
		void RenderEnd() override;
		void ReCreatePipelineState();
		void RenderNewRenderPass();
		void RenderOldRenderPass();

	private:
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

		TRefCountPtr<GpuShader> NewPixelShader;
		TRefCountPtr<GpuShader> OldPixelShader;

		TSharedPtr<ArgumentBuffer> NewCustomArgumentBuffer;
		TSharedPtr<ArgumentBuffer> OldCustomArgumentBuffer;

		TSharedPtr<ArgumentBufferLayout> NewCustomArgumentBufferLayout;
		TSharedPtr<ArgumentBufferLayout> OldCustomArgumentBufferLayout;
	};
}


