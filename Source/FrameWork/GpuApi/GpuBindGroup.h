#pragma once
#include "GpuResourceCommon.h"
#include "GpuTexture.h"
#include "GpuBuffer.h"
#include "GpuBindGroupLayout.h"

namespace FW
{
	struct ResourceBinding
	{
		TRefCountPtr<GpuResource> Resource;
	};

	struct GpuBindGroupDesc
	{
		TRefCountPtr<GpuBindGroupLayout> Layout;
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

	class FRAMEWORK_API GpuBindGroupBuilder
	{
	public:
		GpuBindGroupBuilder(GpuBindGroupLayout* InLayout);

		GpuBindGroupBuilder& SetExistingBinding(BindingSlot InSlot, GpuResource* InResource);
		GpuBindGroupBuilder& SetUniformBuffer(const FString& BindingName, GpuResource* InResource);
		GpuBindGroupBuilder& SetTexture(const FString& BindingName, GpuResource* InResource);
		GpuBindGroupBuilder& SetSampler(const FString& BindingName, GpuResource* InResource);

		TRefCountPtr<GpuBindGroup> Build() const;

		const GpuBindGroupDesc& GetDesc() const { return Desc; }

	private:
		GpuBindGroupDesc Desc;
	};

}
