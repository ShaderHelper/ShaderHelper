#include "CommonHeader.h"
#include "Dx12Buffer.h"
namespace FRAMEWORK
{

	TRefCountPtr<Dx12Buffer> CreateDx12Buffer(uint32 ByteSize, GpuBufferUsage Usage)
	{
		if (EnumHasAnyFlags(Usage, GpuBufferUsage::Dynamic))
		{
			if (EnumHasAnyFlags(Usage, GpuBufferUsage::Uniform)) {
				
				if (EnumHasAnyFlags(Usage, GpuBufferUsage::Persistent))
				{
					BuddyAllocationData AllocationData = GPersistantUniformBufferAllocator->Alloc(ByteSize);
					return new Dx12Buffer{ Usage, ResourceAllocation{AllocationData} };
				}
				else if (EnumHasAnyFlags(Usage, GpuBufferUsage::Temporary))
				{
					BumpAllocationData AllocationData = GTempUniformBufferAllocator[GetCurFrameSourceIndex()]->Alloc(ByteSize);
					return new Dx12Buffer{ Usage, ResourceAllocation{AllocationData} };
				}
			}
			else
			{
				CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(ByteSize, 0, D3D12_HEAP_TYPE_UPLOAD);
				return new Dx12Buffer{ Usage, ResourceAllocation{AllocationData} };
			}
			
		}
		else if (EnumHasAnyFlags(Usage, GpuBufferUsage::Static))
		{
			CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(ByteSize, 0, D3D12_HEAP_TYPE_DEFAULT);
			return new Dx12Buffer{ Usage, ResourceAllocation{AllocationData} };
		}
		else if (EnumHasAnyFlags(Usage, GpuBufferUsage::Staging))
		{
			CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(ByteSize, 0, D3D12_HEAP_TYPE_READBACK);
			return new Dx12Buffer{ Usage, ResourceAllocation{AllocationData} };
		}
		else
		{
			checkf(false, TEXT("Invalid GpuBufferUsage"));
			return TRefCountPtr<Dx12Buffer>{};
		}
	}

}
