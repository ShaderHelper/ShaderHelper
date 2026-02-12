#pragma once
#include "VkDevice.h"
#include "GpuApi/GpuResource.h"
#include "GpuApi/GpuSampler.h"

namespace FW
{
	class VulkanSampler : public GpuSampler
	{
	public:
		VulkanSampler(VkSampler InSampler, const GpuSamplerDesc& InDesc);
		~VulkanSampler() {
			vkDestroySampler(GDevice, Sampler, nullptr);
		}
		VkSampler GetSampler() const { return Sampler; }

	private:
		VkSampler Sampler;
	};

	class VulkanTexture : public GpuTexture
	{
	public:
		VulkanTexture(const GpuTextureDesc& InDesc, GpuResourceState InResourceState, VkImage InImage, VkImageView InImageView, VmaAllocation InAllocation);
		~VulkanTexture() {
			vkDestroyImageView(GDevice, ImageView, nullptr);
			vmaDestroyImage(GAllocator, Image, Allocation);
			CloseHandle(SharedTextureHandle);
		}

		VkImage GetImage() const { return Image; }
		VkImageView GetView() const { return ImageView; }
		uint32 GetAllocationSize() const override {
			VmaAllocationInfo AllocInfo;
			vmaGetAllocationInfo(GAllocator, Allocation, &AllocInfo);
			return (uint32)AllocInfo.size;
		}

		void* GetSharedHandle()
		{
			if (SharedTextureHandle == nullptr)
			{
#if PLATFORM_WINDOWS
				VkCheck(vmaGetMemoryWin32Handle2(GAllocator, Allocation, ExternalMemoryHandleType, nullptr, &SharedTextureHandle));
#endif
			}
			return SharedTextureHandle;
		}

		TRefCountPtr<GpuBuffer> UploadBuffer;
		TRefCountPtr<GpuBuffer> ReadBackBuffer;
		bool bIsMappingForWriting = false;

	private:
		VkImage Image;
		VkImageView ImageView;
		VmaAllocation Allocation;
		HANDLE SharedTextureHandle;
	};

	TRefCountPtr<VulkanTexture> CreateVulkanTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState);
	TRefCountPtr<VulkanSampler> CreateVulkanSampler(const GpuSamplerDesc& InSamplerDesc);
}
