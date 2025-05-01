#pragma once
#include "Dx12Common.h"
#include "Dx12Device.h"
#include "GpuApi/GpuResource.h"
namespace FW
{
	enum class AllocationPolicy
	{
		Committed,
		SubAllocate,
		Placed,
	};

	struct CommonAllocationData
	{
		TRefCountPtr<ID3D12Resource> UnderlyResource;
		D3D12_GPU_VIRTUAL_ADDRESS ResourceBaseGpuAddr;
		void* ResourceBaseCpuAddr;

		ID3D12Resource* GetResource() const
		{
			return UnderlyResource;
		}
		
		D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddr() const
		{
			return ResourceBaseGpuAddr;
		}

		void* GetCpuAddr() const
		{
			check(ResourceBaseCpuAddr);
			return ResourceBaseCpuAddr;
		}
	};

	struct BumpAllocationData
	{
		ID3D12Resource* UnderlyResource;
		D3D12_GPU_VIRTUAL_ADDRESS ResourceBaseGpuAddr;
		void* ResourceBaseCpuAddr;
		uint64 Offset;

		bool IsPlaced = false;

		ID3D12Resource* GetResource() const
		{
			return UnderlyResource;
		}

		D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddr() const
		{
			return ResourceBaseGpuAddr + Offset;
		}

		void* GetCpuAddr() const
		{
			check(ResourceBaseCpuAddr);
			return static_cast<void*>((uint8*)ResourceBaseCpuAddr + Offset);
		}
	};

	class BufferBuddyAllocator;

	struct BuddySubAllocSection : public FNoncopyable
	{
		BuddySubAllocSection(ID3D12Resource* InUnderlyResource, D3D12_GPU_VIRTUAL_ADDRESS InResourceBaseGpuAddr,
			void* InResourceBaseCpuAddr, BufferBuddyAllocator* InFromAllocator,
			uint32 InOffset, uint32 InSize)
			: UnderlyResource(InUnderlyResource)
			, ResourceBaseGpuAddr(InResourceBaseGpuAddr)
			, ResourceBaseCpuAddr(InResourceBaseCpuAddr)
			, FromAllocator(InFromAllocator)
			, Offset(InOffset)
			, Size(InSize)
		{}

		ID3D12Resource* UnderlyResource;
		D3D12_GPU_VIRTUAL_ADDRESS ResourceBaseGpuAddr;
		void* ResourceBaseCpuAddr;
		BufferBuddyAllocator* FromAllocator;
		uint32 Offset;
		uint32 Size;

		~BuddySubAllocSection();
	};

	struct BuddyAllocationData
	{
		TSharedPtr<BuddySubAllocSection> Section;
		//Just for Upload heap resource(eg. constant buffer)
		bool IsFrameSource = false;
		//Allow to update resource on demand with frame resource strategy.
		mutable uint32 LastFrameBlockIndexWritten = 0;

		bool IsPlaced = false;

		ID3D12Resource* GetResource() const
		{
			return Section->UnderlyResource;
		}

		D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddr() const
		{
			if (IsFrameSource) {
				check(Section->Size % FrameSourceNum == 0);
				uint32 FrameBlockSize = Section->Size / FrameSourceNum;
				return Section->ResourceBaseGpuAddr + Section->Offset + FrameBlockSize * LastFrameBlockIndexWritten;
			}
			return Section->ResourceBaseGpuAddr + Section->Offset;
		}

		void* GetCpuAddr() const
		{
			check(Section->ResourceBaseCpuAddr);
			if (IsFrameSource) {
				check(Section->Size % FrameSourceNum == 0);
				uint32 FrameBlockSize = Section->Size / FrameSourceNum;
				void* CurAddress = (uint8*)Section->ResourceBaseCpuAddr + Section->Offset + FrameBlockSize * GetCurFrameSourceIndex();
				if (LastFrameBlockIndexWritten != GetCurFrameSourceIndex())
				{
					//Inherit the result last written
					void* LastAddress = (uint8*)Section->ResourceBaseCpuAddr + Section->Offset + FrameBlockSize * LastFrameBlockIndexWritten;
                    //TODO: Avoid reading data from write-combined memory
					FMemory::Memcpy(CurAddress, LastAddress, FrameBlockSize);
					LastFrameBlockIndexWritten = GetCurFrameSourceIndex();
				}
				return CurAddress;
			}
			return (uint8*)Section->ResourceBaseCpuAddr + Section->Offset;
		}
	};

	class ResourceAllocation
	{
	public:
		using AllocationDataVariant = TVariant<CommonAllocationData, BumpAllocationData, BuddyAllocationData>;

		ResourceAllocation(const CommonAllocationData& InData)
		{
			AllocationData.Set<CommonAllocationData>(InData);
			Policy = AllocationPolicy::Committed;
		}

		ResourceAllocation(const BumpAllocationData& InData)
		{
			AllocationData.Set<BumpAllocationData>(InData);
			Policy = InData.IsPlaced ? AllocationPolicy::Placed : AllocationPolicy::SubAllocate;
		}

		ResourceAllocation(const BuddyAllocationData& InData)
		{
			AllocationData.Set<BuddyAllocationData>(InData);
			Policy = InData.IsPlaced ? AllocationPolicy::Placed : AllocationPolicy::SubAllocate;
		}

	public:
		void SetOwner(GpuResource* InOwner) { Owner = InOwner; }

		AllocationPolicy GetPolicy() const { return Policy;  }
		const AllocationDataVariant& GetAllocationData() const
		{
			return AllocationData;
		}

		ID3D12Resource* GetResource() const {
			return Visit(
				[](auto&& Arg)
				{
					return Arg.GetResource();
				}, AllocationData);
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
		AllocationPolicy Policy;
	};

	class BuddyAllocator
	{
	public:
		BuddyAllocator(uint32 InMinBlockSize, uint32 InMaxBlockSize);

		uint32 Allocate(uint32 InSize, uint32 Alignment);
		uint32 AllocateBlock(uint32 Order);
		void Deallocate(uint32 Offset, uint32 Size);
		void DeallocateBlock(uint32 UnitSizeOffset, uint32 BlockOrder);
		bool CanAllocate(uint32 InSize, uint32 Alignment) const;
		void Reset();

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
		uint32 MaxOrder;
		TArray<TSet<uint32>> FreeBlocks;
	};

	class BufferBuddyAllocator
	{
	public:
		BufferBuddyAllocator(uint32 InMinBlockSize, uint32 InMaxBlockSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState);
		BuddyAllocationData Allocate(uint32 InSize, uint32 Alignment);
		void Deallocate(uint32 Offset, uint32 Size);
		bool CanAllocate(uint32 InSize, uint32 Alignment) const;

	private:
		BuddyAllocator InternalAllocator;
		TRefCountPtr<ID3D12Resource> Resource;
		void* ResourceBaseCpuAddr = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS ResourceBaseGpuAddr;
	};


	class BufferBumpAllocator
	{
	public:
		BufferBumpAllocator(uint32 InSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState);
		bool CanAllocate(uint32 InSize, uint32 Alignment) const
		{
			return Align(ByteOffset, Alignment) + InSize <= TotalSize;
		}
		BumpAllocationData Allocate(uint32 InSize, uint32 Alignment)
		{
			uint32 AlignOffset = Align(ByteOffset, Alignment);
			ByteOffset = AlignOffset + InSize;
			return { Resource.GetReference(), ResourceBaseGpuAddr, ResourceBaseCpuAddr, AlignOffset};
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
		TArray<BufferBumpAllocator> AllocatorImpls;
	};

	class PersistantUniformBufferAllocator
	{
	public:
		PersistantUniformBufferAllocator(uint32 InMinBlockSize, uint32 InMaxBlockSize);
		BuddyAllocationData Alloc(uint32 BufferSize);

	private:
		uint32 MinBlockSize, MaxBlockSize;
		TArray<BufferBuddyAllocator> AllocatorImpls;
	};

	class CommonBufferAllocator
	{
	public:
		CommonAllocationData Alloc(uint32 ByteSize, 
			D3D12_HEAP_TYPE InHeapType, const CD3DX12_RESOURCE_DESC& BufferDesc, D3D12_RESOURCE_STATES InInitialState);

	};

	inline TempUniformBufferAllocator* GTempUniformBufferAllocator[FrameSourceNum]; //Scope = frame
	inline PersistantUniformBufferAllocator* GPersistantUniformBufferAllocator;

	inline CommonBufferAllocator* GCommonBufferAllocator;

	extern void InitBufferAllocator();
}
