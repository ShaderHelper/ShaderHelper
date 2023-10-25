#pragma once
#include "GpuResourceCommon.h"
#include "GpuTexture.h"
#include "GpuBuffer.h"

namespace FRAMEWORK
{
	struct ResourceBinding
	{
		BindingSlot Slot;
		GpuResource* Resource;
	};

	struct GpuBindGroupDesc
	{
		GpuBindGroupLayout* Layout;
		TArray<ResourceBinding> Resources;
	};
	
	class GpuBindGroup : public GpuResource
	{
	public:
		GpuBindGroup(GpuBindGroupLayout* InLayout) 
			: GpuResource(GpuResourceType::BindGroup)
			, Layout(InLayout)
		{}

		GpuBindGroupLayout* GetLayout() const { return Layout; }

	private:
		GpuBindGroupLayout* Layout;
	};
}