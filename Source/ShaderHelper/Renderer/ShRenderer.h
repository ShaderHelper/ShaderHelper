#pragma once
#include "Renderer/Renderer.h"
#include "GpuApi/GpuResource.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"

namespace SH
{
	class ShRenderer : public FRAMEWORK::Renderer
	{
	public:
		ShRenderer();
		
	public:
		void RenderInternal() override;
		void OnViewportResize(const FRAMEWORK::Vector2f& InResolution);
		void UpdatePixelShader(TRefCountPtr<FRAMEWORK::GpuShader> InNewPixelShader);
		void UpdateCustomBindGroup(TRefCountPtr<FRAMEWORK::GpuBindGroup> InBindGroup) { CustomBindGroup = MoveTemp(InBindGroup); }
		void UpdateCustomBindGroupLayout(TRefCountPtr<FRAMEWORK::GpuBindGroupLayout> InBindGroupLayout) { CustomBindGroupLayout = MoveTemp(InBindGroupLayout); }
		FString GetPixelShaderDeclaration() const;
		FString GetDefaultPixelShaderBody() const;
        FRAMEWORK::GpuTexture* GetFinalRT() const { return FinalRT; }


		//Deprecated
		TArray<TSharedRef<FRAMEWORK::PropertyData>> GetBuiltInPropertyDatas() const;

	private:
		void RenderBegin() override;
		void RenderEnd() override;
		void ReCreatePipelineState();

	private:
		TRefCountPtr<FRAMEWORK::GpuTexture> FinalRT;
		TRefCountPtr<FRAMEWORK::GpuShader> VertexShader;
		TRefCountPtr<FRAMEWORK::GpuShader> PixelShader;
		TRefCountPtr<FRAMEWORK::GpuPipelineState> PipelineState;

		float iTime;
        FRAMEWORK::Vector2f iResolution;
		TUniquePtr<FRAMEWORK::UniformBuffer> BuiltInUniformBuffer;

		TRefCountPtr<FRAMEWORK::GpuBindGroup> BuiltInBindGroup;
		TRefCountPtr<FRAMEWORK::GpuBindGroupLayout> BuiltInBindGroupLayout;

		TRefCountPtr<FRAMEWORK::GpuBindGroup> CustomBindGroup;
		TRefCountPtr<FRAMEWORK::GpuBindGroupLayout> CustomBindGroupLayout;
	};
}


