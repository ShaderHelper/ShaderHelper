#include "CommonHeader.h"
#include "Dx12Buffer.h"
#include "Dx12Map.h"

namespace FW
{
	Dx12Buffer::Dx12Buffer(GpuBufferUsage InUsage, ResourceAllocation InAllocation, bool IsDeferred)
		: GpuBuffer(InUsage)
		, Dx12DeferredDeleteObject(IsDeferred)
		, Allocation(MoveTemp(InAllocation))
	{
		Allocation.SetOwner(this);
		if (EnumHasAllFlags(InUsage, GpuBufferUsage::RWStorage))
		{
			UAV = AllocCpuCbvSrvUav();
			if (Allocation.GetPolicy() == AllocationPolicy::Placed || Allocation.GetPolicy() == AllocationPolicy::Committed)
			{
				GDevice->CreateUnorderedAccessView(Allocation.GetResource(), nullptr, nullptr, UAV->GetHandle());
			}
		}
	}

	TRefCountPtr<Dx12Buffer> CreateDx12ConstantBuffer(uint32 ByteSize, GpuBufferUsage Usage, bool IsDeferred)
	{
		if (EnumHasAnyFlags(Usage, GpuBufferUsage::Temporary))
		{
			BumpAllocationData AllocationData = GTempUniformBufferAllocator[GetCurFrameSourceIndex()]->Alloc(ByteSize);
			return new Dx12Buffer{ Usage, AllocationData, IsDeferred };
		}
		else
		{
			BuddyAllocationData AllocationData = GPersistantUniformBufferAllocator->Alloc(ByteSize);
			return new Dx12Buffer{ Usage, AllocationData, IsDeferred };
		}
	}

	static D3D12_RESOURCE_STATES GetBufferState(GpuResourceState InitState, GpuBufferUsage Usage)
	{
		if (EnumHasAllFlags(Usage, GpuBufferUsage::RWStorage))
		{
			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		check(false);
		return D3D12_RESOURCE_STATE_COMMON;
	}

	TRefCountPtr<Dx12Buffer> CreateDx12Buffer(GpuResourceState InInitState, uint32 ByteSize, GpuBufferUsage Usage, bool IsDeferred)
	{
		if (EnumHasAllFlags(Usage, GpuBufferUsage::Uniform))
		{
			return CreateDx12ConstantBuffer(ByteSize, Usage, IsDeferred);
		}

		CD3DX12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(ByteSize);
		D3D12_RESOURCE_STATES InitState = InInitState == GpuResourceState::Unknown ? GetBufferState(InInitState, Usage) : MapResourceState(InInitState);
		if (EnumHasAllFlags(Usage, GpuBufferUsage::RWStorage))
		{
			BufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		if (EnumHasAnyFlags(Usage, GpuBufferUsage::Dynamic))
		{
			CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(ByteSize, D3D12_HEAP_TYPE_UPLOAD, BufferDesc, InitState);
			return new Dx12Buffer{ Usage, AllocationData, IsDeferred };
		}
		else if (EnumHasAnyFlags(Usage, GpuBufferUsage::Static))
		{
			CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(ByteSize, D3D12_HEAP_TYPE_DEFAULT, BufferDesc, InitState);
			return new Dx12Buffer{ Usage, AllocationData, IsDeferred };
		}
		else if (EnumHasAnyFlags(Usage, GpuBufferUsage::Staging))
		{
			CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(ByteSize, D3D12_HEAP_TYPE_READBACK, BufferDesc, InitState);
			return new Dx12Buffer{ Usage, AllocationData, IsDeferred };
		}

		checkf(false, TEXT("Invalid GpuBufferUsage"));
		return TRefCountPtr<Dx12Buffer>{};
	}

}
