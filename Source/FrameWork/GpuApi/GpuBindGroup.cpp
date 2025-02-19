#include "CommonHeader.h"
#include "GpuBindGroup.h"
#include "GpuRhi.h"

namespace FW
{

	GpuBindGrouprBuilder::GpuBindGrouprBuilder(GpuBindGroupLayout* InLayout)
	{
		Desc.Layout = InLayout;
	}

	GpuBindGrouprBuilder& GpuBindGrouprBuilder::SetExistingBinding(BindingSlot InSlot, GpuResource* InResource)
	{
		Desc.Resources.Add(InSlot, { InResource });
		return *this;
	}

	GpuBindGrouprBuilder& GpuBindGrouprBuilder::SetUniformBuffer(const FString& BindingName, GpuResource* InResource)
	{
		BindingSlot Slot = Desc.Layout->GetDesc().CodegenBindingNameToSlot[BindingName];
		Desc.Resources.Add(Slot, { InResource });
		return *this;
	}

	GpuBindGrouprBuilder& GpuBindGrouprBuilder::SetTexture(const FString& BindingName, GpuResource* InResource)
	{
		BindingSlot Slot = Desc.Layout->GetDesc().CodegenBindingNameToSlot[BindingName];
		Desc.Resources.Add(Slot, { InResource });
		return *this;
	}

	GpuBindGrouprBuilder& GpuBindGrouprBuilder::SetSampler(const FString& BindingName, GpuResource* InResource)
	{
		BindingSlot Slot = Desc.Layout->GetDesc().CodegenBindingNameToSlot[BindingName];
		Desc.Resources.Add(Slot, { InResource });
		return *this;
	}

	TRefCountPtr<GpuBindGroup> GpuBindGrouprBuilder::Build()
	{
		return GGpuRhi->CreateBindGroup(Desc);
	}

}