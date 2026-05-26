#pragma once
#include "VkDevice.h"
#include "GpuApi/GpuResource.h"

namespace FW::VK
{
	class VulkanBuffer : public GpuBuffer
	{
	public:
		VulkanBuffer(const GpuBufferDesc& InDesc, GpuResourceState InResourceState, VkBuffer InBuffer, VmaAllocation InAllocation);
		~VulkanBuffer() {
			if (View != VK_NULL_HANDLE)
			{
				vkDestroyBufferView(GDevice, View, nullptr);
			}
			vmaDestroyBuffer(GAllocator, Buffer, Allocation);
		}
		VkBuffer GetBuffer() const { return Buffer; }
		VmaAllocation GetAllocation() const { return Allocation; }
		const VkBufferView& GetView() const { return View; }
		void SetView(VkBufferView InView) { View = InView; }

	public:
		TRefCountPtr<VulkanBuffer> ReadBackBuffer;

	private:
		VkBuffer Buffer;
		VmaAllocation Allocation;
		VkBufferView View = VK_NULL_HANDLE;
	};

	TRefCountPtr<VulkanBuffer> CreateVulkanBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState);
}
