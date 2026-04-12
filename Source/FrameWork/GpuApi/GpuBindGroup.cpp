#include "CommonHeader.h"
#include "GpuBindGroup.h"
#include "GpuRhi.h"

namespace FW
{

	GpuBindGroupBuilder::GpuBindGroupBuilder(GpuBindGroupLayout* InLayout)
	{
		Desc.Layout = InLayout;
	}

	GpuBindGroupBuilder& GpuBindGroupBuilder::SetExistingBinding(int32 InSlotNum, BindingType InType, GpuResource* InResource, BindingShaderStage InStage)
	{
		Desc.Resources.Add(BindingSlot{InSlotNum, InType, InStage}, { InResource });
		return *this;
	}

	GpuBindGroupBuilder& GpuBindGroupBuilder::SetUniformBuffer(const FString& BindingName, GpuResource* InResource)
	{
		BindingSlot Slot = Desc.Layout->GetDesc().CodegenBindingNameToSlot[BindingName];
		Desc.Resources.Add(Slot, { InResource });
		return *this;
	}

	GpuBindGroupBuilder& GpuBindGroupBuilder::SetTexture(const FString& BindingName, GpuResource* InResource)
	{
		BindingSlot Slot = Desc.Layout->GetDesc().CodegenBindingNameToSlot[BindingName];
		Desc.Resources.Add(Slot, { InResource });
		return *this;
	}

	GpuBindGroupBuilder& GpuBindGroupBuilder::SetSampler(const FString& BindingName, GpuResource* InResource)
	{
		BindingSlot Slot = Desc.Layout->GetDesc().CodegenBindingNameToSlot[BindingName];
		Desc.Resources.Add(Slot, { InResource });
		return *this;
	}

	GpuBindGroupBuilder& GpuBindGroupBuilder::SetCombinedTextureSampler(const FString& BindingName, GpuResource* InTexture, GpuResource* InSampler)
	{
		BindingSlot Slot = Desc.Layout->GetDesc().CodegenBindingNameToSlot[BindingName];
		Desc.Resources.Add(Slot, { new GpuCombinedTextureSampler(InTexture, InSampler) });
		return *this;
	}

	TRefCountPtr<GpuBindGroup> GpuBindGroupBuilder::Build() const
	{
		return GGpuRhi->CreateBindGroup(Desc);
	}

}
