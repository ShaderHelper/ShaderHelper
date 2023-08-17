#include "CommonHeader.h"
#include "Dx12Allocation.h"
#include "Dx12Device.h"

namespace FRAMEWORK
{
	constexpr uint32 PersistantUniformBufferMinBlockSize = 256 * FrameSourceNum;
	constexpr uint32 PersistantUniformBufferMaxBlockSize = 64 * 1024 * FrameSourceNum;
	constexpr uint32 TempUniformBufferPageSize = 64 * 1024;

	void InitBufferAllocator()
	{
		GCommonBufferAllocator = MakeUnique<CommonBufferAllocator>();
		for (int32 i = 0; i < FrameSourceNum; i++)
		{
			GTempUniformBufferAllocator[i] = MakeUnique<TempUniformBufferAllocator>(TempUniformBufferPageSize);
		}
		GPersistantUniformBufferAllocator = MakeUnique<PersistantUniformBufferAllocator>(PersistantUniformBufferMinBlockSize, PersistantUniformBufferMaxBlockSize);
	}

	CommonAllocationData CommonBufferAllocator::Alloc(uint32 ByteSize, uint32 Alignment, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState)
	{
		TRefCountPtr<ID3D12Resource> Resource;

		CD3DX12_HEAP_PROPERTIES HeapType{ InHeapType };
		D3D12_RESOURCE_STATES InitialState = InInitialState;
		uint32 Width = Align(ByteSize, Alignment);
		CD3DX12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(Width);

		DxCheck(GDevice->CreateCommittedResource(&HeapType, D3D12_HEAP_FLAG_NONE,
			&BufferDesc, InitialState, nullptr, IID_PPV_ARGS(Resource.GetInitReference())));

		CommonAllocationData Data{};
		Data.UnderlyResource = Resource;
		Data.ResourceBaseGpuAddr = Resource->GetGPUVirtualAddress();

		if (InHeapType == D3D12_HEAP_TYPE_UPLOAD || InHeapType == D3D12_HEAP_TYPE_READBACK)
		{
			Resource->Map(0, nullptr, static_cast<void**>(&Data.ResourceBaseCpuAddr));
		}

		return Data;
	}

	PersistantUniformBufferAllocator::PersistantUniformBufferAllocator(uint32 InMinBlockSize, uint32 InMaxBlockSize)
		: MinBlockSize(InMinBlockSize)
		, MaxBlockSize(InMaxBlockSize)
	{

	}

	BuddyAllocationData PersistantUniformBufferAllocator::Alloc(uint32 BufferSize)
	{

	}

	BufferBumpAllocator::BufferBumpAllocator(uint32 InSize, uint32 Alignment, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState)
	{
		CD3DX12_HEAP_PROPERTIES HeapType{ InHeapType };
		D3D12_RESOURCE_STATES InitialState = InInitialState;
		uint32 Width = Align(Width, Alignment);
		CD3DX12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(Width);

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
		AllocatorImpls.Add(MakeUnique<BufferBumpAllocator>(PageSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
		CurAllocatorIndex = 0;
	}

	BumpAllocationData TempUniformBufferAllocator::Alloc(uint32 BufferSize)
	{
		check(BufferSize <= PageSize);

		if (!AllocatorImpls[CurAllocatorIndex]->CanAllocate(BufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT))
		{
			if (!AllocatorImpls.IsValidIndex(CurAllocatorIndex + 1))
			{
				AllocatorImpls.Add(MakeUnique<BufferBumpAllocator>(PageSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
			}
			CurAllocatorIndex++;
		}
		
		auto [Resource, ResourceBaseCpuAddr, Offset, ResourceBaseGpuAddr] = AllocatorImpls[CurAllocatorIndex]->Allocate(BufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		return { Resource, ResourceBaseGpuAddr ,ResourceBaseCpuAddr ,Offset };
	}

	void TempUniformBufferAllocator::Flush()
	{
		CurAllocatorIndex = 0;
	}

	D3D12_GPU_VIRTUAL_ADDRESS CommonAllocationData::GetGpuAddr() const
	{
		return ResourceBaseGpuAddr;
	}

	void* CommonAllocationData::GetCpuAddr() const
	{
		check(ResourceBaseCpuAddr);
		return ResourceBaseCpuAddr;
	}

	D3D12_GPU_VIRTUAL_ADDRESS BumpAllocationData::GetGpuAddr() const
	{
		return ResourceBaseGpuAddr + Offset;
	}

	void* BumpAllocationData::GetCpuAddr() const
	{
		check(ResourceBaseCpuAddr);
		return static_cast<void*>((uint8*)ResourceBaseCpuAddr + Offset);
	}

	BuddyAllocationData::~BuddyAllocationData()
	{

	}

	D3D12_GPU_VIRTUAL_ADDRESS BuddyAllocationData::GetGpuAddr() const
	{

	}

	void* BuddyAllocationData::GetCpuAddr() const
	{

	}

}

