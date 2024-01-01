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
		void UpdateCustomBindGroup(TRefCountPtr<GpuBindGroup> InBindGroup) { CustomBindGroup = MoveTemp(InBindGroup); }
		void UpdateCustomBindGroupLayout(TRefCountPtr<GpuBindGroupLayout> InBindGroupLayout) { CustomBindGroupLayout = MoveTemp(InBindGroupLayout); }
		FString GetPixelShaderDeclaration() const;
		FString GetDefaultPixelShaderBody() const;
		GpuTexture* GetFinalRT() const { return FinalRT; }


		//Deprecated
		TArray<TSharedRef<PropertyData>> GetBuiltInPropertyDatas() const;

	private:
		void RenderBegin() override;
		void RenderEnd() override;
		void ReCreatePipelineState();

	private:
		TRefCountPtr<GpuTexture> FinalRT;
		TRefCountPtr<GpuShader> VertexShader;
		TRefCountPtr<GpuShader> PixelShader;
		TRefCountPtr<GpuPipelineState> PipelineState;

		float iTime;
		Vector2f iResolution;
		TUniquePtr<UniformBuffer> BuiltInUniformBuffer;

		TRefCountPtr<GpuBindGroup> BuiltInBindGroup;
		TRefCountPtr<GpuBindGroupLayout> BuiltInBindGroupLayout;

		TRefCountPtr<GpuBindGroup> CustomBindGroup;
		TRefCountPtr<GpuBindGroupLayout> CustomBindGroupLayout;
	};
}


