#include "CommonHeader.h"
#include "VkTexture.h"
#include "VkMap.h"

namespace FW
{
	VkImageUsageFlags DetermineTextureUsage(const GpuTextureDesc& InTexDesc)
	{
		VkImageUsageFlags Usage = 0;
		if (EnumHasAnyFlags(InTexDesc.Usage, GpuTextureUsage::RenderTarget))
		{
			Usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		if (EnumHasAnyFlags(InTexDesc.Usage, GpuTextureUsage::ShaderResource))
		{
			Usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		if (!InTexDesc.InitialData.IsEmpty())
		{
			Usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
				
		check(Usage != 0);
		return Usage;
	}

	TRefCountPtr<VulkanTexture> CreateVulkanTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState)
	{
		VkImageCreateInfo ImgCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		ImgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImgCreateInfo.extent.width = InTexDesc.Width;
		ImgCreateInfo.extent.height = InTexDesc.Height;
		ImgCreateInfo.extent.depth = 1;
		ImgCreateInfo.mipLevels = 1;
		ImgCreateInfo.arrayLayers = 1;
		ImgCreateInfo.format = MapTextureFormat(InTexDesc.Format);
		ImgCreateInfo.usage = DetermineTextureUsage(InTexDesc);
		ImgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		VmaAllocationCreateInfo AllocCreateInfo = {};
		AllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

		VkImage Image;
		VmaAllocation Allocation;
		VmaPool Pool = VK_NULL_HANDLE;
		if (EnumHasAnyFlags(InTexDesc.Usage, GpuTextureUsage::Shared))
		{
			VkPhysicalDeviceExternalImageFormatInfo ExternalImgFormatInfo = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO,
				.handleType = ExternalHandleType
			};
			VkPhysicalDeviceImageFormatInfo2 ImgFormatInfo2 = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
				.pNext = &ExternalImgFormatInfo,
				.format = ImgCreateInfo.format,
				.type = ImgCreateInfo.imageType,
				.tiling = ImgCreateInfo.tiling,
				.usage = ImgCreateInfo.usage,
				.flags = ImgCreateInfo.flags
			};
			VkExternalImageFormatProperties ExternalImgFormatProps = {
				.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES
			};
			VkImageFormatProperties2 ImgFormatProps2 = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
				.pNext = &ExternalImgFormatProps
			};
			vkGetPhysicalDeviceImageFormatProperties2(GPhysicalDevice, &ImgFormatInfo2, &ImgFormatProps2);
			check((ExternalImgFormatProps.externalMemoryProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT) != 0);
			bool DedicatedAlloc = (ExternalImgFormatProps.externalMemoryProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT) != 0;

			static constexpr VkExportMemoryAllocateInfo ExportAllocInfo = {
				.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
				.handleTypes = ExternalHandleType
			};

			static constexpr VkExternalMemoryImageCreateInfo ExternalImgCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
				.handleTypes = ExternalHandleType
			};

			ImgCreateInfo.pNext = &ExternalImgCreateInfo;

			uint32_t MemTypeIndex = 0;
			VkCheck(vmaFindMemoryTypeIndexForImageInfo(GAllocator, &ImgCreateInfo, &AllocCreateInfo, &MemTypeIndex));
			VmaPoolCreateInfo PoolCreateInfo = {
				.memoryTypeIndex = MemTypeIndex,
				.pMemoryAllocateNext = (void*)&ExportAllocInfo
			};

			VkCheck(vmaCreatePool(GAllocator, &PoolCreateInfo, &Pool));
			AllocCreateInfo.pool = Pool;
			if (DedicatedAlloc)
			{
				AllocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
			}
		}

		VkCheck(vmaCreateImage(GAllocator, &ImgCreateInfo, &AllocCreateInfo, &Image, &Allocation, nullptr));
		return new VulkanTexture(InTexDesc, InitState, Image, Allocation, Pool);
	}
}
