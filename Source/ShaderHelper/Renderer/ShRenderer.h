#pragma once
#include "Renderer/Renderer.h"
#include "GpuApi/GpuResource.h"
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
		void UpdateCustomBindGroup(TRefCountPtr<GpuBindGroup> InBindGroup) { NewCustomBindGroup = MoveTemp(InBindGroup); }
		void UpdateCustomBindGroupLayout(TRefCountPtr<GpuBindGroupLayout> InBindGroupLayout) { NewCustomBindGroupLayout = MoveTemp(InBindGroupLayout); }
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
		TUniquePtr<UniformBuffer> BuiltInUniformBuffer;
		float iTime;
		Vector2f iResolution;

		TRefCountPtr<GpuBindGroup> BuiltInBindGroup;
		TRefCountPtr<GpuBindGroupLayout> BuiltInBindGroupLayout;

		//keep previous state if failed to compile new pixel shader.
		bool bCompileSuccessful;

		TRefCountPtr<GpuPipelineState> NewPipelineState;
		TRefCountPtr<GpuPipelineState> OldPipelineState;

		TRefCountPtr<GpuShader> NewPixelShader;
		TRefCountPtr<GpuShader> OldPixelShader;

		TRefCountPtr<GpuBindGroup> NewCustomBindGroup;
		TRefCountPtr<GpuBindGroup> OldCustomBindGroup;

		TRefCountPtr<GpuBindGroupLayout> NewCustomBindGroupLayout;
		TRefCountPtr<GpuBindGroupLayout> OldCustomBindGroupLayout;
	};
}


