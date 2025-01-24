#include "CommonHeader.h"
#include "Dx12Buffer.h"
namespace FW
{

	TRefCountPtr<Dx12Buffer> CreateDx12Buffer(D3D12_RESOURCE_STATES InitState, uint32 ByteSize, GpuBufferUsage Usage, bool IsDeferred)
	{
		if (EnumHasAnyFlags(Usage, GpuBufferUsage::Dynamic))
		{
			CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(ByteSize, D3D12_HEAP_TYPE_UPLOAD, InitState);
			return new Dx12Buffer{ Usage, AllocationData, IsDeferred };
		}
		else if (EnumHasAnyFlags(Usage, GpuBufferUsage::Static))
		{
			CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(ByteSize, D3D12_HEAP_TYPE_DEFAULT, InitState);
			return new Dx12Buffer{ Usage, AllocationData, IsDeferred };
		}
		else if (EnumHasAnyFlags(Usage, GpuBufferUsage::Staging))
		{
			CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(ByteSize, D3D12_HEAP_TYPE_READBACK, InitState);
			return new Dx12Buffer{ Usage, AllocationData, IsDeferred };
		}

		checkf(false, TEXT("Invalid GpuBufferUsage"));
		return TRefCountPtr<Dx12Buffer>{};
	}

	TRefCountPtr<Dx12Buffer> CreateDx12ConstantBuffer(uint32 ByteSize, GpuBufferUsage Usage, bool IsDeferred)
	{
		if (EnumHasAllFlags(Usage, GpuBufferUsage::PersistentUniform))
		{
			BuddyAllocationData AllocationData = GPersistantUniformBufferAllocator->Alloc(ByteSize);
			return new Dx12Buffer{ Usage, AllocationData, IsDeferred };
		}
		else if (EnumHasAllFlags(Usage, GpuBufferUsage::TemporaryUniform))
		{
			BumpAllocationData AllocationData = GTempUniformBufferAllocator[GetCurFrameSourceIndex()]->Alloc(ByteSize);
			return new Dx12Buffer{ Usage, AllocationData, IsDeferred };
		}
		checkf(false, TEXT("Invalid GpuBufferUsage"));
		return TRefCountPtr<Dx12Buffer>{};
	}

}
