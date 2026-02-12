#include "CommonHeader.h"
#include "VkBuffer.h"
#include "VkGpuRhiBackend.h"
#include "VkUtil.h"

namespace FW
{
	VulkanBuffer::VulkanBuffer(const GpuBufferDesc& InDesc, GpuResourceState InResourceState, VkBuffer InBuffer, VmaAllocation InAllocation)
		: GpuBuffer(InDesc, InResourceState)
		, Buffer(InBuffer), Allocation(InAllocation)
	{
		GVkDeferredReleaseManager.AddResource(this);
	}

	TRefCountPtr<VulkanBuffer> CreateVulkanBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState)
	{
		bool bHasInitialData = !InBufferDesc.InitialData.IsEmpty();

		VkBufferCreateInfo BufferInfo = { 
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = InBufferDesc.ByteSize,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};
		VmaAllocationCreateInfo AllocInfo{ .usage = VMA_MEMORY_USAGE_AUTO };

		if (EnumHasAnyFlags(InBufferDesc.Usage, GpuBufferUsage::RWStructured | GpuBufferUsage::Structured | GpuBufferUsage::Raw | GpuBufferUsage::RWRaw))
		{
			BufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			if (bHasInitialData)
			{
				BufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			}
		}
		else if (EnumHasAllFlags(InBufferDesc.Usage, GpuBufferUsage::Upload))
		{
			BufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			AllocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		else if (EnumHasAllFlags(InBufferDesc.Usage, GpuBufferUsage::Uniform))
		{
			BufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			AllocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		else if (EnumHasAllFlags(InBufferDesc.Usage, GpuBufferUsage::ReadBack))
		{
			BufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			AllocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		}

		VkBuffer Buffer;
		VmaAllocation Allocation;
		VkCheck(vmaCreateBuffer(GAllocator, &BufferInfo, &AllocInfo, &Buffer, &Allocation, nullptr));

		TRefCountPtr<VulkanBuffer> RetBuffer = new VulkanBuffer(InBufferDesc, InResourceState, Buffer, Allocation);

		if (bHasInitialData)
		{
			if (EnumHasAnyFlags(InBufferDesc.Usage, GpuBufferUsage::DynamicMask))
			{
				void* MappedData = nullptr;
				VkCheck(vmaMapMemory(GAllocator, Allocation, &MappedData));
				FMemory::Memcpy(MappedData, InBufferDesc.InitialData.GetData(), InBufferDesc.InitialData.Num());
				vmaUnmapMemory(GAllocator, Allocation);
			}
			else
			{
				TRefCountPtr<VulkanBuffer> UploadBuffer = CreateVulkanBuffer(
					{ .ByteSize = InBufferDesc.ByteSize, .Usage = GpuBufferUsage::Upload },
					GpuResourceState::CopySrc
				);

				void* MappedData = nullptr;
				VkCheck(vmaMapMemory(GAllocator, UploadBuffer->GetAllocation(), &MappedData));
				FMemory::Memcpy(MappedData, InBufferDesc.InitialData.GetData(), InBufferDesc.InitialData.Num());
				vmaUnmapMemory(GAllocator, UploadBuffer->GetAllocation());

				GpuCmdRecorder* CmdRecorder = GVkGpuRhi->BeginRecording();
				{
					CmdRecorder->Barriers({
						{ RetBuffer, GpuResourceState::CopyDst }
					});
					CmdRecorder->CopyBufferToBuffer(UploadBuffer, 0, RetBuffer, 0, InBufferDesc.ByteSize);
					CmdRecorder->Barriers({
						{ RetBuffer, InResourceState }
					});
				}
				GVkGpuRhi->EndRecording(CmdRecorder);
				GVkGpuRhi->Submit({ CmdRecorder });
			}
		}

		return RetBuffer;
	}
}
