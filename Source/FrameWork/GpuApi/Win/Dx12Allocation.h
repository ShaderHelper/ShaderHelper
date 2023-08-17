#pragma once
#include "Dx12Common.h"
#include "Dx12Device.h"
#include "GpuApi/GpuResource.h"
namespace FRAMEWORK
{
	struct CommonAllocationData


	{
		ID3D12Resource* UnderlyResource;
		D3D12_GPU_VIRTUAL_ADDRESS ResourceBaseGpuAddr;
		void* ResourceBaseCpuAddr;
		
		D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddr() const;
		void* GetCpuAddr() const;
	};

	struct BumpAllocationData
	{
		ID3D12Resource* UnderlyResource;
		D3D12_GPU_VIRTUAL_ADDRESS ResourceBaseGpuAddr;
		void* ResourceBaseCpuAddr;
		uint32 Offset;

		D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddr() const;
		void* GetCpuAddr() const;
	};

	struct BuddyAllocationData
	{
		ID3D12Resource* UnderlyResource;
		D3D12_GPU_VIRTUAL_ADDRESS ResourceBaseGpuAddr;
		void* ResourceBaseCpuAddr;
		BufferBuddyAllocator* FromAllocator;
		uint32 Offset;
		uint32 Order;
		bool FrameSource;

		~BuddyAllocationData();
		D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddr() const;
		void* GetCpuAddr() const;
	};

	class ResourceAllocation
	{
	public:
		using AllocationDataVariant = TVariant<CommonAllocationData, BumpAllocationData, BuddyAllocationData>;

		ResourceAllocation(const CommonAllocationData& InData)
		{
			AllocationData.Set<CommonAllocationData>(InData);
		}

		ResourceAllocation(const BumpAllocationData& InData)
		{
			AllocationData.Set<BumpAllocationData>(InData);
		}

		ResourceAllocation(const BuddyAllocationData& InData)
		{
			AllocationData.Set<BuddyAllocationData>(InData);
		}

	public:
		void SetOwner(GpuResource* InOwner) { Owner = InOwner; }

		const AllocationDataVariant& GetAllocationData() const
		{
			return AllocationData;
		}

		D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddr() const {
			return Visit(
				[](auto&& Arg)
				{
					return Arg.GetGpuAddr();
				}, AllocationData);
		}

		void* GetCpuAddr() const {
			return Visit(
				[](auto&& Arg) 
				{
					return Arg.GetCpuAddr();
				}, AllocationData);
		}

	private:
		GpuResource* Owner = nullptr;
		AllocationDataVariant AllocationData;

	};

	class BufferBuddyAllocator
	{
	public:

		void Deallocate(uint32 Offset, uint32 Order);

	private:
		uint32 SizeToUnitSize(uint32 InSize) const
		{
			return (InSize + MinBlockSize - 1) / MinBlockSize;
		}

		uint32 UnitSizeToOrder(uint32 InSize) const 
		{
			return FMath::CeilLogTwo(InSize);
		}

		uint32 GetBuddyOffset(uint32 InOffset, uint32 InSize) const 
		{
			return InOffset ^ InSize;
		}

		uint32 OrderToUnitSize(uint32 Order) const
		{
			return 1 << Order;
		}

		uint32 MinBlockSize, MaxBlockSize;
	};


	class BufferBumpAllocator
	{
	public:
		BufferBumpAllocator(uint32 InSize, uint32 Alignment, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState);
		bool CanAllocate(uint32 InSize, uint32 Alignment) const
		{
			return Align(ByteOffset, Alignment) + InSize <= TotalSize;
		}
		auto Allocate(uint32 InSize, uint32 Alignment)
		{
			uint32 AlignOffset = Align(ByteOffset, Alignment);
			ByteOffset = AlignOffset + InSize;
			return MakeTuple(Resource.GetReference(), ResourceBaseCpuAddr, AlignOffset, ResourceBaseGpuAddr);
		}
		void Flush()
		{
			ByteOffset = 0;
		}

	private:
		TRefCountPtr<ID3D12Resource> Resource;
		void* ResourceBaseCpuAddr = nullptr;
		uint32 TotalSize;
		uint32 ByteOffset;
		D3D12_GPU_VIRTUAL_ADDRESS ResourceBaseGpuAddr;
	};

	class TempUniformBufferAllocator
	{
	public:
		TempUniformBufferAllocator(uint32 InPageSize);
		BumpAllocationData Alloc(uint32 BufferSize);
		void Flush();

	private:
		uint32 PageSize;
		int32 CurAllocatorIndex;
		TArray<TUniquePtr<BufferBumpAllocator>> AllocatorImpls;
	};

	class PersistantUniformBufferAllocator
	{
	public:
		PersistantUniformBufferAllocator(uint32 InMinBlockSize, uint32 InMaxBlockSize);
		BuddyAllocationData Alloc(uint32 BufferSize);

	private:
		uint32 MinBlockSize, MaxBlockSize;
		TArray<TUniquePtr<BufferBuddyAllocator>> AllocatorImpls;
	};

	class CommonBufferAllocator
	{
	public:
		CommonAllocationData Alloc(uint32 ByteSize, uint32 Alignment,
			D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState = D3D12_RESOURCE_STATE_COMMON);

	};

	inline TUniquePtr<TempUniformBufferAllocator> GTempUniformBufferAllocator[FrameSourceNum]; //Scope = frame
	inline TUniquePtr<PersistantUniformBufferAllocator> GPersistantUniformBufferAllocator;

	inline TUniquePtr<CommonBufferAllocator> GCommonBufferAllocator;

	extern void InitBufferAllocator();
}