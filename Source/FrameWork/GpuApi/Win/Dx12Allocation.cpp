#include "CommonHeader.h"
#include "Dx12Allocation.h"
#include "Dx12Device.h"

namespace FRAMEWORK
{
	constexpr uint32 PersistantUniformBufferMinBlockSize = 256 * FrameSourceNum;
	constexpr uint32 PersistantUniformBufferMaxBlockSize = 64 * 1024 * FrameSourceNum;
	constexpr uint32 TempUniformBufferPageSize = 64 * 1024; //64kb - cbuffer limit.

	void InitBufferAllocator()
	{
		GCommonBufferAllocator = MakeUnique<CommonBufferAllocator>();
		GPersistantUniformBufferAllocator = MakeUnique<PersistantUniformBufferAllocator>(PersistantUniformBufferMinBlockSize, PersistantUniformBufferMaxBlockSize);
		for (int32 i = 0; i < FrameSourceNum; i++)
		{
			GTempUniformBufferAllocator[i] = MakeUnique<TempUniformBufferAllocator>(TempUniformBufferPageSize);
		}
	}

	CommonAllocationData CommonBufferAllocator::Alloc(uint32 ByteSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState)
	{
		CommonAllocationData Data{};

		CD3DX12_HEAP_PROPERTIES HeapType{ InHeapType };
		D3D12_RESOURCE_STATES InitialState = InInitialState;
		CD3DX12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(ByteSize);

		//TODO : d3d12ma
		DxCheck(GDevice->CreateCommittedResource(&HeapType, D3D12_HEAP_FLAG_NONE,
			&BufferDesc, InitialState, nullptr, IID_PPV_ARGS(&Data.UnderlyResource)));

		Data.ResourceBaseGpuAddr = Data.UnderlyResource->GetGPUVirtualAddress();
		if (InHeapType == D3D12_HEAP_TYPE_UPLOAD || InHeapType == D3D12_HEAP_TYPE_READBACK)
		{
			Data.UnderlyResource->Map(0, nullptr, static_cast<void**>(&Data.ResourceBaseCpuAddr));
		}

		return Data;
	}

	PersistantUniformBufferAllocator::PersistantUniformBufferAllocator(uint32 InMinBlockSize, uint32 InMaxBlockSize)
		: MinBlockSize(InMinBlockSize)
		, MaxBlockSize(InMaxBlockSize)
	{
		AllocatorImpls.Emplace(MinBlockSize,MaxBlockSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}

	BuddyAllocationData PersistantUniformBufferAllocator::Alloc(uint32 BufferSize)
	{
		uint32 FrameSourceBufferSize = Align(BufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) * FrameSourceNum;
		check(FrameSourceBufferSize <= MaxBlockSize);
		BuddyAllocationData Data;

		for (int32 i = 0; i < AllocatorImpls.Num(); i++)
		{
			if (AllocatorImpls[i].CanAllocate(FrameSourceBufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT))
			{
				Data = AllocatorImpls[i].Allocate(FrameSourceBufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
				Data.IsFrameSource = true;
				return Data;
			}
		}

		AllocatorImpls.Emplace(MinBlockSize, MaxBlockSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		Data =  AllocatorImpls.Last().Allocate(FrameSourceBufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		Data.IsFrameSource = true;
		return Data;
	}

	BufferBumpAllocator::BufferBumpAllocator(uint32 InSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState)
	{
		CD3DX12_HEAP_PROPERTIES HeapType{ InHeapType };
		D3D12_RESOURCE_STATES InitialState = InInitialState;
		CD3DX12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(InSize);

		//TODO: CreatePlacedResource
		DxCheck(GDevice->CreateCommittedResource(&HeapType, D3D12_HEAP_FLAG_NONE,
			&BufferDesc, InitialState, nullptr, IID_PPV_ARGS(Resource.GetInitReference())));

		if (HeapType.Type == D3D12_HEAP_TYPE_UPLOAD)
		{
			Resource->Map(0, nullptr, static_cast<void**>(&ResourceBaseCpuAddr));
		}
		ResourceBaseGpuAddr = Resource->GetGPUVirtualAddress();
		TotalSize = InSize;
		ByteOffset = 0;
	}

	TempUniformBufferAllocator::TempUniformBufferAllocator(uint32 InPageSize)
		: PageSize(InPageSize)
	{
		AllocatorImpls.Emplace(PageSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		CurAllocatorIndex = 0;
	}

	BumpAllocationData TempUniformBufferAllocator::Alloc(uint32 BufferSize)
	{
		check(Align(BufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) <= PageSize);

		if (!AllocatorImpls[CurAllocatorIndex].CanAllocate(BufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT))
		{
			if (!AllocatorImpls.IsValidIndex(CurAllocatorIndex + 1))
			{
				AllocatorImpls.Emplace(PageSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			}
			CurAllocatorIndex++;
		}
		
		return AllocatorImpls[CurAllocatorIndex].Allocate(BufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	void TempUniformBufferAllocator::Flush()
	{
		CurAllocatorIndex = 0;
		for (auto& AllocatorImpl : AllocatorImpls)
		{
			AllocatorImpl.Flush();
		}
	}

	BufferBuddyAllocator::BufferBuddyAllocator(uint32 InMinBlockSize, uint32 InMaxBlockSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState)
		: MinBlockSize(InMinBlockSize)
		, MaxBlockSize(InMaxBlockSize)
	{
		check(MaxBlockSize % MinBlockSize == 0);
		check(FMath::IsPowerOfTwo(MaxBlockSize / MinBlockSize));
		MaxOrder = UnitSizeToOrder(SizeToUnitSize(MaxBlockSize));
		Reset();

		CD3DX12_HEAP_PROPERTIES HeapType{ InHeapType };
		D3D12_RESOURCE_STATES InitialState = InInitialState;
		CD3DX12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(MaxBlockSize);

		DxCheck(GDevice->CreateCommittedResource(&HeapType, D3D12_HEAP_FLAG_NONE,
			&BufferDesc, InitialState, nullptr, IID_PPV_ARGS(Resource.GetInitReference())));

		if (HeapType.Type == D3D12_HEAP_TYPE_UPLOAD)
		{
			Resource->Map(0, nullptr, static_cast<void**>(&ResourceBaseCpuAddr));
		}
		ResourceBaseGpuAddr = Resource->GetGPUVirtualAddress();
	}

	BuddyAllocationData BufferBuddyAllocator::Allocate(uint32 InSize, uint32 Alignment)
	{
		uint32 AlignSize = Align(InSize, Alignment);
		uint32 Order = UnitSizeToOrder(SizeToUnitSize(AlignSize));
		uint32 FreeBlockUnitSizeOffset = AllocateBlock(Order);

		uint32 FreeBlockByteSizeOffset = FreeBlockUnitSizeOffset * MinBlockSize;
		return {Resource.GetReference(), ResourceBaseGpuAddr, ResourceBaseCpuAddr, this, FreeBlockByteSizeOffset, AlignSize};
	}

	uint32 BufferBuddyAllocator::AllocateBlock(uint32 Order)
	{
		uint32 UnitSizeOffset;

		if (Order > MaxOrder)
		{
			checkf(false, TEXT("Can't allocate a block that large  "));
		}

		if (FreeBlocks[Order].Num() == 0)
		{
			uint32 LeftBlockUnitSizeOffset = AllocateBlock(Order + 1);
			uint32 CurOrderBlockUnitSize = OrderToUnitSize(Order);
			uint32 RightBlockUnitSizeOffset = LeftBlockUnitSizeOffset + CurOrderBlockUnitSize;
			FreeBlocks[Order].Add(RightBlockUnitSizeOffset);
			UnitSizeOffset = LeftBlockUnitSizeOffset;
		}
		else
		{
			UnitSizeOffset = *FreeBlocks[Order].begin();
			FreeBlocks[Order].Remove(UnitSizeOffset);
		}

		return UnitSizeOffset;
	}

	void BufferBuddyAllocator::Deallocate(uint32 Offset, uint32 Size)
	{
		uint32 UnitSizeOffset = SizeToUnitSize(Offset);
		uint32 Order = UnitSizeToOrder(SizeToUnitSize(Size));
		DeallocateBlock(UnitSizeOffset, Order);
	}

	void BufferBuddyAllocator::DeallocateBlock(uint32 UnitSizeOffset, uint32 BlockOrder)
	{
		uint32 CurOrderBlockUnitSize = OrderToUnitSize(BlockOrder);
		uint32 BuddyBlockUnitSizeOffset = GetBuddyOffset(UnitSizeOffset, CurOrderBlockUnitSize);
		if (FreeBlocks[BlockOrder].Contains(BuddyBlockUnitSizeOffset))
		{
			DeallocateBlock(FMath::Min(UnitSizeOffset, BuddyBlockUnitSizeOffset), BlockOrder + 1);
			FreeBlocks[BlockOrder].Remove(BuddyBlockUnitSizeOffset);
		}
		else
		{
			FreeBlocks[BlockOrder].Add(UnitSizeOffset);
		}
	}

	bool BufferBuddyAllocator::CanAllocate(uint32 InSize, uint32 Alignment) const
	{
		uint32 AlignSize = Align(InSize, Alignment);
		for (int32 i = MaxOrder; i >= 0; i--)
		{
			uint32 CurOrderBlockByteSize = OrderToUnitSize(i) * MinBlockSize;
			if (FreeBlocks[i].Num() && CurOrderBlockByteSize >= AlignSize) {
				return true;
			}
		}
		return false;
	}

	void BufferBuddyAllocator::Reset()
	{
		FreeBlocks.Empty();
		FreeBlocks.SetNum(MaxOrder + 1);
		FreeBlocks[MaxOrder].Add(0);
	}

	void BuddyAllocationData::Release()
	{
		FromAllocator->Deallocate(Offset, Size);
	}

}

