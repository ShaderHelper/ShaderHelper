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
		GpuBindGroup(const GpuBindGroupDesc& InDesc)
			: GpuResource(GpuResourceType::BindGroup)
            , Desc(InDesc)
		{}

		GpuBindGroupLayout* GetLayout() const { return Desc.Layout; }
        GpuBindGroupDesc GetDesc() const { return Desc; }

	protected:
        GpuBindGroupDesc Desc;
	};
}
