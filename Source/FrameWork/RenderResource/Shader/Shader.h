#pragma once
#include "GpuApi/GpuRhi.h"

namespace FRAMEWORK
{

	class Shader
	{
	public:
		virtual ~Shader() = default;
	};

	class BindingContext
	{
	public:
		//Group Binding by update frequency
		enum Slot
		{
			GlobalSlot = 0,
			PassSlot = 1,
			ShaderSlot = 2,
			ObjectSlot = 3,
		};

		void SetGlobalBindGroup(TRefCountPtr<GpuBindGroup> InBindGroup) {
			GlobalBindGroup = MoveTemp(InBindGroup);
		}
		void SetShaderBindGroup(TRefCountPtr<GpuBindGroup> InBindGroup) {
			ShaderBindGroup = MoveTemp(InBindGroup);
		}

		void SetGlobalBindGroupLayout(TRefCountPtr<GpuBindGroupLayout> InBindGroupLayout) {
			GlobalBindGroupLayout = MoveTemp(InBindGroupLayout);
		}
		void SetShaderBindGroupLayout(TRefCountPtr<GpuBindGroupLayout> InBindGroupLayout) {
			ShaderBindGroupLayout = MoveTemp(InBindGroupLayout);
		}

		void ApplyBindGroupLayout(GpuPipelineStateDesc& OutDesc)
		{
			OutDesc.BindGroupLayout0 = GlobalBindGroupLayout;
			OutDesc.BindGroupLayout2 = ShaderBindGroupLayout;
		}

		void ApplyBindGroup() const
		{
			GGpuRhi->SetBindGroups(GlobalBindGroup, nullptr, ShaderBindGroup, nullptr);
		}

	private:
		TRefCountPtr<GpuBindGroup> GlobalBindGroup;
		TRefCountPtr<GpuBindGroup> ShaderBindGroup;

		TRefCountPtr<GpuBindGroupLayout> GlobalBindGroupLayout;
		TRefCountPtr<GpuBindGroupLayout> ShaderBindGroupLayout;
	};

}