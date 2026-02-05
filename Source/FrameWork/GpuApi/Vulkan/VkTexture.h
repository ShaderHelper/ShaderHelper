#pragma once
#include "VkDevice.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
#if PLATFORM_WINDOWS
	constexpr VkExternalMemoryHandleTypeFlagBits ExternalHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#endif

	class VulkanTexture : public GpuTexture
	{
	public:
		VulkanTexture(const GpuTextureDesc& InDesc, GpuResourceState InResourceState, VkImage InImage, VmaAllocation InAllocation, VmaPool InPool) 
			: GpuTexture(InDesc, InResourceState)
			, Image(InImage), Allocation(InAllocation), Pool(InPool), Handle(nullptr)
		{}
		~VulkanTexture() {
			vmaDestroyImage(GAllocator, Image, Allocation);
			CloseHandle(Handle);
		}

		void* GetSharedHandle()
		{
			VkCheck(vmaGetMemoryWin32Handle2(GAllocator, Allocation, ExternalHandleType, nullptr, &Handle));
			return Handle;
		}

	private:
		VkImage Image;
		VmaAllocation Allocation;
		VmaPool Pool;
		HANDLE Handle;
	};

	TRefCountPtr<VulkanTexture> CreateVulkanTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState);
}