#include "CommonHeader.h"
#include "VkTexture.h"
#include "VkMap.h"
#include "VkGpuRhiBackend.h"
#include "VkUtil.h"
#include "VkBuffer.h"

namespace FW
{
	VulkanSampler::VulkanSampler(VkSampler InSampler, const GpuSamplerDesc& InDesc)
		: GpuSampler(InDesc)
		, Sampler(InSampler)
	{
		GVkDeferredReleaseManager.AddResource(this);
	}

	VulkanTexture::VulkanTexture(const GpuTextureDesc& InDesc, GpuResourceState InResourceState, VkImage InImage, VkImageView InImageView, VmaAllocation InAllocation)
		: GpuTexture(InDesc, InResourceState)
		, Image(InImage), ImageView(InImageView)
		, Allocation(InAllocation)
		, SharedTextureHandle(nullptr)
	{
		GVkDeferredReleaseManager.AddResource(this);
	}

	static VmaPool ExternalPool = VK_NULL_HANDLE;
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

		if (EnumHasAnyFlags(InTexDesc.Usage, GpuTextureUsage::Shared))
		{
			VkPhysicalDeviceExternalImageFormatInfo ExternalImgFormatInfo = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO,
				.handleType = ExternalMemoryHandleType
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

			static constexpr VkExportMemoryAllocateInfo ExportAllocInfo = {
				.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
				.handleTypes = ExternalMemoryHandleType
			};

			static constexpr VkExternalMemoryImageCreateInfo ExternalImgCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
				.handleTypes = ExternalMemoryHandleType
			};

			ImgCreateInfo.pNext = &ExternalImgCreateInfo;

			static bool InitExternalPool = [&] {
				uint32_t MemTypeIndex = 0;
				VkCheck(vmaFindMemoryTypeIndexForImageInfo(GAllocator, &ImgCreateInfo, &AllocCreateInfo, &MemTypeIndex));
				VmaPoolCreateInfo PoolCreateInfo = {
					.memoryTypeIndex = MemTypeIndex,
					.pMemoryAllocateNext = (void*)&ExportAllocInfo
				};
				VkCheck(vmaCreatePool(GAllocator, &PoolCreateInfo, &ExternalPool));
				return true;
			}();

			AllocCreateInfo.pool = ExternalPool;
			AllocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		}

		VkImage Image;
		VmaAllocation Allocation;
		VkCheck(vmaCreateImage(GAllocator, &ImgCreateInfo, &AllocCreateInfo, &Image, &Allocation, nullptr));
		VkImageViewCreateInfo ViewInfo = { 
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, 
			.image = Image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = ImgCreateInfo.format,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};
		VkImageView ImageView;
		VkCheck(vkCreateImageView(GDevice, &ViewInfo, nullptr, &ImageView));

		VulkanTexture* NewTex = new VulkanTexture(InTexDesc, InitState, Image, ImageView, Allocation);
		//Vulkan requires the initialLayout member of VkImageCreateInfo must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED..
		//but we want to create the texture in the correct initial state, so we need an explicit layout transition after creation.
		NewTex->State = GpuResourceState::Unknown;

		if (!InTexDesc.InitialData.IsEmpty())
		{
			const uint32 UploadSize = InTexDesc.Width * InTexDesc.Height * GetTextureFormatByteSize(InTexDesc.Format);
			TRefCountPtr<VulkanBuffer> UploadBuffer = CreateVulkanBuffer(
				{ .ByteSize = UploadSize, .Usage = GpuBufferUsage::Upload },
				GpuResourceState::CopySrc
			);

			void* MappedData = nullptr;
			VkCheck(vmaMapMemory(GAllocator, UploadBuffer->GetAllocation(), &MappedData));
			FMemory::Memcpy(MappedData, InTexDesc.InitialData.GetData(), UploadSize);
			vmaUnmapMemory(GAllocator, UploadBuffer->GetAllocation());

			GpuCmdRecorder* CmdRecorder = GVkGpuRhi->BeginRecording();
			{
				CmdRecorder->Barriers({
					{NewTex, GpuResourceState::CopyDst}
				});
				CmdRecorder->CopyBufferToTexture(UploadBuffer, NewTex);
				CmdRecorder->Barriers({
					{NewTex, InitState}
				});
			}
			GVkGpuRhi->EndRecording(CmdRecorder);
			GVkGpuRhi->Submit({ CmdRecorder });
		}
		else
		{
			GpuCmdRecorder* CmdRecorder = GVkGpuRhi->BeginRecording();
			{
				CmdRecorder->Barriers({
					{NewTex, InitState}
				});
			}
			GVkGpuRhi->EndRecording(CmdRecorder);
			GVkGpuRhi->Submit({ CmdRecorder });
		}

		return NewTex;
	}

	TRefCountPtr<VulkanSampler> CreateVulkanSampler(const GpuSamplerDesc& InSamplerDesc)
	{
		bool ComparisonEnable = InSamplerDesc.Compare != CompareMode::Never;

		VkFilter MinFilter = VK_FILTER_NEAREST;
		VkFilter MagFilter = VK_FILTER_NEAREST;
		VkSamplerMipmapMode MipMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		switch (InSamplerDesc.Filter)
		{
		case SamplerFilter::Point:
			break;
		case SamplerFilter::Bilinear:
			MinFilter = VK_FILTER_LINEAR;
			MagFilter = VK_FILTER_LINEAR;
			break;
		case SamplerFilter::Trilinear:
			MinFilter = VK_FILTER_LINEAR;
			MagFilter = VK_FILTER_LINEAR;
			MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			break;
		}

		VkSamplerCreateInfo SamplerInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = MagFilter,
			.minFilter = MinFilter,
			.mipmapMode = MipMode,
			.addressModeU = MapSamplerAddressMode(InSamplerDesc.AddressU),
			.addressModeV = MapSamplerAddressMode(InSamplerDesc.AddressV),
			.addressModeW = MapSamplerAddressMode(InSamplerDesc.AddressW),
			.mipLodBias = 0.0f,
			.anisotropyEnable = VK_FALSE,
			.maxAnisotropy = 1.0f,
			.compareEnable = ComparisonEnable ? VK_TRUE : VK_FALSE,
			.compareOp = MapCompareOp(InSamplerDesc.Compare),
			.minLod = 0.0f,
			.maxLod = VK_LOD_CLAMP_NONE,
			.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
		};

		VkSampler Sampler = VK_NULL_HANDLE;
		VkCheck(vkCreateSampler(GDevice, &SamplerInfo, nullptr, &Sampler));

		return new VulkanSampler(Sampler, InSamplerDesc);
	}
}
