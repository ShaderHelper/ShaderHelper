#pragma once
#include "Renderer/Renderer.h"
#include "GpuApi/GpuResource.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"

namespace SH
{
	class ShRenderer : public FW::Renderer
	{
	public:
		ShRenderer();
		
	public:
		void RenderInternal() override;
		void OnViewportResize(const FW::Vector2f& InResolution);
		void UpdatePixelShader(TRefCountPtr<FW::GpuShader> InNewPixelShader);
		void UpdateCustomBindGroup(TRefCountPtr<FW::GpuBindGroup> InBindGroup) { CustomBindGroup = MoveTemp(InBindGroup); }
		void UpdateCustomBindGroupLayout(TRefCountPtr<FW::GpuBindGroupLayout> InBindGroupLayout) { CustomBindGroupLayout = MoveTemp(InBindGroupLayout); }

	private:
		void RenderBegin() override;
		void RenderEnd() override;
		void ReCreatePipelineState();

	private:
		TRefCountPtr<FW::GpuTexture> FinalRT;
		TRefCountPtr<FW::GpuShader> VertexShader;
		TRefCountPtr<FW::GpuShader> PixelShader;
		TRefCountPtr<FW::GpuPipelineState> PipelineState;

		float iTime;
        FW::Vector2f iResolution;
		TUniquePtr<FW::UniformBuffer> BuiltInUniformBuffer;

		TRefCountPtr<FW::GpuBindGroup> BuiltInBindGroup;
		TRefCountPtr<FW::GpuBindGroupLayout> BuiltInBindGroupLayout;

		TRefCountPtr<FW::GpuBindGroup> CustomBindGroup;
		TRefCountPtr<FW::GpuBindGroupLayout> CustomBindGroupLayout;
	};
}


