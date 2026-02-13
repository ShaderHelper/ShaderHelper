#pragma once
#include "VkDevice.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
	class VulkanBindGroupLayout : public GpuBindGroupLayout
	{
	public:
		VulkanBindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc, VkDescriptorSetLayout InVkLayout);
		~VulkanBindGroupLayout() {
			vkDestroyDescriptorSetLayout(GDevice, VkLayout, nullptr);
		}
		VkDescriptorSetLayout GetLayout() const { return VkLayout; }

	private:
		VkDescriptorSetLayout VkLayout;
	};

	inline VkDescriptorPool GDescriptorPool;

	class VulkanBindGroup : public GpuBindGroup
	{
	public:
		VulkanBindGroup(const GpuBindGroupDesc& InDesc, VkDescriptorSet InDescriptorSet);
		~VulkanBindGroup() {
			vkFreeDescriptorSets(GDevice, GDescriptorPool, 1, &DescriptorSet);
		}
		VkDescriptorSet GetDescriptorSet() const { return DescriptorSet; }

	private:
		VkDescriptorSet DescriptorSet;
	};

	TRefCountPtr<VulkanBindGroupLayout> CreateVulkanBindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc);
	TRefCountPtr<VulkanBindGroup> CreateVulkanBindGroup(const GpuBindGroupDesc& Desc);
}
