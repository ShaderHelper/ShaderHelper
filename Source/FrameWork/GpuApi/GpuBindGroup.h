#pragma once
#include "GpuResourceCommon.h"
#include "GpuTexture.h"
#include "GpuBuffer.h"
#include "GpuBindGroupLayout.h"

namespace FW
{
	class GpuCombinedTextureSampler : public GpuResource
	{
	public:
		GpuCombinedTextureSampler(GpuResource* InTexture, GpuResource* InSampler)
			: GpuResource(GpuResourceType::CombinedTextureSampler)
			, Texture(InTexture), Sampler(InSampler)
		{}

		GpuResource* GetTexture() const { return Texture; }
		GpuResource* GetSampler() const { return Sampler; }

	private:
		TRefCountPtr<GpuResource> Texture;
		TRefCountPtr<GpuResource> Sampler;
	};

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

		GpuBindGroupBuilder& SetExistingBinding(int32 InSlotNum, BindingType InType, GpuResource* InResource, BindingShaderStage InStage);
		GpuBindGroupBuilder& SetUniformBuffer(const FString& BindingName, GpuResource* InResource);
		GpuBindGroupBuilder& SetTexture(const FString& BindingName, GpuResource* InResource);
		GpuBindGroupBuilder& SetSampler(const FString& BindingName, GpuResource* InResource);
		GpuBindGroupBuilder& SetCombinedTextureSampler(const FString& BindingName, GpuResource* InTexture, GpuResource* InSampler);

		TRefCountPtr<GpuBindGroup> Build() const;

		const GpuBindGroupDesc& GetDesc() const { return Desc; }

	private:
		GpuBindGroupDesc Desc;
	};

}
