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
		VulkanTexture(const GpuTextureDesc& InDesc, GpuResourceState InResourceState, VkImage InImage, VkImageView InImageView, VmaAllocation InAllocation)
			: GpuTexture(InDesc, InResourceState)
			, Image(InImage), ImageView(InImageView)
			, Allocation(InAllocation), Handle(nullptr)
		{}
		~VulkanTexture() {
			vkDestroyImageView(GDevice, ImageView, nullptr);
			vmaDestroyImage(GAllocator, Image, Allocation);
			CloseHandle(Handle);
		}

		VkImageView GetView() const { return ImageView; }

		void* GetSharedHandle()
		{
			if (Handle == nullptr)
			{
#if PLATFORM_WINDOWS
				VkCheck(vmaGetMemoryWin32Handle2(GAllocator, Allocation, ExternalHandleType, nullptr, &Handle));
#endif
			}
			return Handle;
		}

	private:
		VkImage Image;
		VkImageView ImageView;
		VmaAllocation Allocation;
		HANDLE Handle;
	};

	TRefCountPtr<VulkanTexture> CreateVulkanTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState);
}