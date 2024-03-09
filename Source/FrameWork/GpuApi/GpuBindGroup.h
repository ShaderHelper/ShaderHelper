#pragma once
#include "GpuResourceCommon.h"
#include "GpuTexture.h"
#include "GpuBuffer.h"
#include "GpuBindGroupLayout.h"

namespace FRAMEWORK
{
	struct ResourceBinding
	{
		GpuResource* Resource;
	};

	struct GpuBindGroupDesc
	{
		GpuBindGroupLayout* Layout;
		TSortedMap<BindingSlot, ResourceBinding> Resources;
	};
	
	class GpuBindGroup : public GpuResource
	{
	public:
		GpuBindGroup(const GpuBindGroupDesc& InDesc)
			: GpuResource(GpuResourceType::BindGroup)
            , Desc(InDesc)
		{}

		GpuBindGroupLayout* GetLayout() const { return Desc.Layout; }
        const GpuBindGroupDesc& GetDesc() const { return Desc; }

	protected:
        GpuBindGroupDesc Desc;
	};

	class FRAMEWORK_API GpuBindGrouprBuilder
	{
	public:
		GpuBindGrouprBuilder(GpuBindGroupLayout* InLayout);

		GpuBindGrouprBuilder& SetExistingBinding(BindingSlot InSlot, GpuResource* InResource);
		GpuBindGrouprBuilder& SetUniformBuffer(const FString& BindingName, GpuResource* InResource);
		GpuBindGrouprBuilder& SetTexture(const FString& BindingName, GpuResource* InResource);
		GpuBindGrouprBuilder& SetSampler(const FString& BindingName, GpuResource* InResource);

		TRefCountPtr<GpuBindGroup> Build();

	private:
		GpuBindGroupDesc Desc;
	};

}
