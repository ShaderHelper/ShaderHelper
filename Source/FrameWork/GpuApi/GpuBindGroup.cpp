#include "CommonHeader.h"
#include "GpuBindGroup.h"
#include "GpuRhi.h"

namespace FW
{

	GpuBindGroupBuilder::GpuBindGroupBuilder(GpuBindGroupLayout* InLayout)
	{
		Desc.Layout = InLayout;
	}

	GpuBindGroupBuilder& GpuBindGroupBuilder::SetExistingBinding(BindingSlot InSlot, GpuResource* InResource)
	{
		Desc.Resources.Add(InSlot, { InResource });
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

	TRefCountPtr<GpuBindGroup> GpuBindGroupBuilder::Build()
	{
		return GGpuRhi->CreateBindGroup(Desc);
	}

}