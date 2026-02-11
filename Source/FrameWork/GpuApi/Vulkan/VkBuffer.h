#pragma once
#include "VkDevice.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
	class VulkanBuffer : public GpuBuffer
	{
	public:
		VulkanBuffer(const GpuBufferDesc& InDesc, GpuResourceState InResourceState, VkBuffer InBuffer, VmaAllocation InAllocation);
		~VulkanBuffer() {
			vmaDestroyBuffer(GAllocator, Buffer, Allocation);
		}
		VkBuffer GetBuffer() const { return Buffer; }
		VmaAllocation GetAllocation() const { return Allocation; }

	private:
		VkBuffer Buffer;
		VmaAllocation Allocation;
	};

	TRefCountPtr<VulkanBuffer> CreateVulkanBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState);
}
