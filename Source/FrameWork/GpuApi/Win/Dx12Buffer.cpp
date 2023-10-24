#include "CommonHeader.h"
#include "Dx12Buffer.h"
namespace FRAMEWORK
{

	TRefCountPtr<Dx12Buffer> CreateDx12Buffer(uint32 ByteSize, GpuBufferUsage Usage)
	{
		if (EnumHasAnyFlags(Usage, GpuBufferUsage::PersistentUniform))
		{
			BuddyAllocationData AllocationData = GPersistantUniformBufferAllocator[GetCurFrameSourceIndex()]->Alloc(ByteSize);
			return new Dx12Buffer{ Usage, AllocationData};
		}
		else if (EnumHasAnyFlags(Usage, GpuBufferUsage::TemporaryUniform))
		{
			BumpAllocationData AllocationData = GTempUniformBufferAllocator[GetCurFrameSourceIndex()]->Alloc(ByteSize);
			return new Dx12Buffer{ Usage, AllocationData};
		}
		else if (EnumHasAnyFlags(Usage, GpuBufferUsage::Dynamic))
		{
			CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(ByteSize, D3D12_HEAP_TYPE_UPLOAD);
			return new Dx12Buffer{ Usage, AllocationData};	
		}
		else if (EnumHasAnyFlags(Usage, GpuBufferUsage::Static))
		{
			CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(ByteSize, D3D12_HEAP_TYPE_DEFAULT);
			return new Dx12Buffer{ Usage, AllocationData};
		}
		else if (EnumHasAnyFlags(Usage, GpuBufferUsage::Staging))
		{
			CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(ByteSize, D3D12_HEAP_TYPE_READBACK);
			return new Dx12Buffer{ Usage, AllocationData};
		}

		checkf(false, TEXT("Invalid GpuBufferUsage"));
		return TRefCountPtr<Dx12Buffer>{};
	}

}
