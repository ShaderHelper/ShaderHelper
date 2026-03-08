#pragma once
#include "VkDevice.h"
#include "GpuApi/GpuResource.h"
#include "GpuApi/GpuSampler.h"
#include "VkUtil.h"

namespace FW
{
	class VulkanTextureView : public GpuTextureView
	{
	public:
		VulkanTextureView(GpuTextureViewDesc InDesc, VkImageView InImageView)
			: GpuTextureView(MoveTemp(InDesc))
			, ImageView(InImageView)
		{
			GVkDeferredReleaseManager.AddResource(this);
		}

		~VulkanTextureView()
		{
			vkDestroyImageView(GDevice, ImageView, nullptr);
		}

		VkImageView GetView() const { return ImageView; }

	private:
		VkImageView ImageView;
	};

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
		VulkanTexture(const GpuTextureDesc& InDesc, GpuResourceState InResourceState, VkImage InImage, VmaAllocation InAllocation);
		~VulkanTexture() {
			vmaDestroyImage(GAllocator, Image, Allocation);
			CloseHandle(SharedTextureHandle);
		}

		VkImage GetImage() const { return Image; }
		VulkanTextureView* GetVkDefaultView() const { return static_cast<VulkanTextureView*>(DefaultView.GetReference()); }
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
		VmaAllocation Allocation;
		HANDLE SharedTextureHandle;
	};

	TRefCountPtr<VulkanTexture> CreateVulkanTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState);
	TRefCountPtr<VulkanSampler> CreateVulkanSampler(const GpuSamplerDesc& InSamplerDesc);
}
