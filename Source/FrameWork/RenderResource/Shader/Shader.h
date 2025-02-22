#pragma once
#include "GpuApi/GpuRhi.h"

namespace FW
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
			OtherSlot = 3,
		};

		void SetGlobalBindGroup(TRefCountPtr<GpuBindGroup> InBindGroup) {
			GlobalBindGroup = MoveTemp(InBindGroup);
		}
		void SetPassBindGroup(TRefCountPtr<GpuBindGroup> InBindGroup) {
			PassBindGroup = MoveTemp(InBindGroup);
		}
		void SetShaderBindGroup(TRefCountPtr<GpuBindGroup> InBindGroup) {
			ShaderBindGroup = MoveTemp(InBindGroup);
		}

		void SetGlobalBindGroupLayout(TRefCountPtr<GpuBindGroupLayout> InBindGroupLayout) {
			GlobalBindGroupLayout = MoveTemp(InBindGroupLayout);
		}
		void SetPassBindGroupLayout(TRefCountPtr<GpuBindGroupLayout> InBindGroupLayout) {
			PassBindGroupLayout = MoveTemp(InBindGroupLayout);
		}
		void SetShaderBindGroupLayout(TRefCountPtr<GpuBindGroupLayout> InBindGroupLayout) {
			ShaderBindGroupLayout = MoveTemp(InBindGroupLayout);
		}

		void ApplyBindGroupLayout(GpuRenderPipelineStateDesc& OutDesc)
		{
			OutDesc.BindGroupLayout0 = GlobalBindGroupLayout;
			OutDesc.BindGroupLayout1 = PassBindGroupLayout;
			OutDesc.BindGroupLayout2 = ShaderBindGroupLayout;
		}

		void ApplyBindGroup(GpuRenderPassRecorder* InPassRecorder) const
		{
			InPassRecorder->SetBindGroups(GlobalBindGroup, PassBindGroup, ShaderBindGroup, nullptr);
		}

		void EnumerateBinding(TFunctionRef<void(const ResourceBinding&, const LayoutBinding&)> Func);

	private:
		TRefCountPtr<GpuBindGroup> GlobalBindGroup;
		TRefCountPtr<GpuBindGroup> PassBindGroup;
		TRefCountPtr<GpuBindGroup> ShaderBindGroup;

		TRefCountPtr<GpuBindGroupLayout> GlobalBindGroupLayout;
		TRefCountPtr<GpuBindGroupLayout> PassBindGroupLayout;
		TRefCountPtr<GpuBindGroupLayout> ShaderBindGroupLayout;
	};

}