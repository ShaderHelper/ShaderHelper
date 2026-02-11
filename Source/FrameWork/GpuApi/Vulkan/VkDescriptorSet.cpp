#include "CommonHeader.h"
#include "VkDescriptorSet.h"
#include "VkBuffer.h"
#include "VkTexture.h"
#include "VkMap.h"
#include "VkUtil.h"

namespace FW
{
	VulkanBindGroupLayout::VulkanBindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc, VkDescriptorSetLayout InVkLayout)
		: GpuBindGroupLayout(LayoutDesc)
		, VkLayout(InVkLayout)
	{

	}

	VulkanBindGroup::VulkanBindGroup(const GpuBindGroupDesc& InDesc, VkDescriptorSet InDescriptorSet)
		: GpuBindGroup(InDesc)
		, DescriptorSet(InDescriptorSet)
	{
		GVkDeferredReleaseManager.AddResource(this);
	}
	
	VkDescriptorType MapBindingType(BindingType InType)
	{
		switch (InType)
		{
		case BindingType::UniformBuffer:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case BindingType::Texture:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case BindingType::Sampler:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case BindingType::StructuredBuffer:
		case BindingType::RWStructuredBuffer:
		case BindingType::RawBuffer:
		case BindingType::RWRawBuffer:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		default:
			AUX::Unreachable();
		}
	}

	VkShaderStageFlags MapShaderStage(BindingShaderStage InStage)
	{
		VkShaderStageFlags Result = 0;
		if (EnumHasAnyFlags(InStage, BindingShaderStage::Vertex))
		{
			Result |= VK_SHADER_STAGE_VERTEX_BIT;
		}
		if (EnumHasAnyFlags(InStage, BindingShaderStage::Pixel))
		{
			Result |= VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		if (EnumHasAnyFlags(InStage, BindingShaderStage::Compute))
		{
			Result |= VK_SHADER_STAGE_COMPUTE_BIT;
		}
		return Result;
	}

	TRefCountPtr<VulkanBindGroupLayout> CreateVulkanBindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc)
	{
		TArray<VkDescriptorSetLayoutBinding> VkBindings;
		VkBindings.Reserve(LayoutDesc.Layouts.Num());

		for (const auto& [Slot, LayoutBindingEntry] : LayoutDesc.Layouts)
		{
			VkDescriptorSetLayoutBinding Binding{};
			Binding.binding = (uint32)Slot;
			Binding.descriptorType = MapBindingType(LayoutBindingEntry.Type);
			Binding.descriptorCount = 1;
			Binding.stageFlags = MapShaderStage(LayoutBindingEntry.Stage);
			Binding.pImmutableSamplers = nullptr;

			VkBindings.Add(Binding);
		}

		VkDescriptorSetLayoutCreateInfo LayoutInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = (uint32)VkBindings.Num(),
			.pBindings = VkBindings.GetData()
		};

		VkDescriptorSetLayout VkLayout = VK_NULL_HANDLE;
		VkCheck(vkCreateDescriptorSetLayout(GDevice, &LayoutInfo, nullptr, &VkLayout));

		return new VulkanBindGroupLayout(LayoutDesc, VkLayout);
	}

	TRefCountPtr<VulkanBindGroup> CreateVulkanBindGroup(const GpuBindGroupDesc& Desc)
	{
		if (!GDescriptorPool)
		{
			VkDescriptorPoolSize PoolSizes[] = {
						{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 256 },
						{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 256 },
						{ VK_DESCRIPTOR_TYPE_SAMPLER, 256 },
						{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 256 },
			};
			VkDescriptorPoolCreateInfo PoolInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
				.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
				.maxSets = 256,
				.poolSizeCount = UE_ARRAY_COUNT(PoolSizes),
				.pPoolSizes = PoolSizes
			};
			VkDescriptorPool Pool = VK_NULL_HANDLE;
			VkCheck(vkCreateDescriptorPool(GDevice, &PoolInfo, nullptr, &GDescriptorPool));
		}

		VulkanBindGroupLayout* Layout = static_cast<VulkanBindGroupLayout*>(Desc.Layout.GetReference());
		const GpuBindGroupLayoutDesc& LayoutDesc = Layout->GetDesc();

		VkDescriptorSetLayout SetLayout = Layout->GetLayout();
		VkDescriptorSetAllocateInfo AllocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = GDescriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &SetLayout
		};
		VkDescriptorSet Set = VK_NULL_HANDLE;
		VkCheck(vkAllocateDescriptorSets(GDevice, &AllocInfo, &Set));

		TArray<VkWriteDescriptorSet> Writes;
		TArray<VkDescriptorBufferInfo> BufferInfos;
		TArray<VkDescriptorImageInfo> ImageInfos;

		for (const auto& [Slot, ResourceBindingEntry] : Desc.Resources)
		{
			GpuResource* Resource = ResourceBindingEntry.Resource;
			VkDescriptorType DescType = MapBindingType(LayoutDesc.GetBindingType(Slot));

			VkWriteDescriptorSet Write{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = Set,
				.dstBinding = (uint32)Slot,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = DescType,
			};

			if (Resource->GetType() == GpuResourceType::Buffer)
			{
				VulkanBuffer* Buffer = static_cast<VulkanBuffer*>(Resource);
				BufferInfos.Add({
					.buffer = Buffer->GetBuffer(),
					.offset = 0,
					.range = VK_WHOLE_SIZE
				});
				Write.pBufferInfo = &BufferInfos.Last();
			}
			else if (Resource->GetType() == GpuResourceType::Texture)
			{
				VulkanTexture* Texture = static_cast<VulkanTexture*>(Resource);
				ImageInfos.Add({
					.imageView = Texture->GetView(),
					.imageLayout = MapImageLayout(Texture->State)
				});
				Write.pImageInfo = &ImageInfos.Last();
			}
			else if (Resource->GetType() == GpuResourceType::Sampler)
			{
				VulkanSampler* Sampler = static_cast<VulkanSampler*>(Resource);
				ImageInfos.Add({
					.sampler = Sampler->GetSampler(),
				});
				Write.pImageInfo = &ImageInfos.Last();
			}

			Writes.Add(Write);
		}

		vkUpdateDescriptorSets(GDevice, (uint32)Writes.Num(), Writes.GetData(), 0, nullptr);

		return new VulkanBindGroup(Desc, Set);
	}
}
