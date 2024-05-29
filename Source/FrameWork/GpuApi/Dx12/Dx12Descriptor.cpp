#include "CommonHeader.h"
#include "Dx12Descriptor.h"

namespace FRAMEWORK
{

	TUniquePtr<CpuDescriptorAllocator> RtvAllocator;
	TUniquePtr<CpuDescriptorAllocator> Cpu_CbvSrvUavAllocator;
	TUniquePtr<GpuDescriptorAllocator> Gpu_CbvSrvUavAllocator;
	TUniquePtr<CpuDescriptorAllocator> Cpu_SamplerAllocator;
	TUniquePtr<GpuDescriptorAllocator> Gpu_SamplerAllocator;

	void InitDescriptorAllocator()
	{
		RtvAllocator = MakeUnique<CpuDescriptorAllocator>(256, DescriptorType::Rtv);
		Cpu_CbvSrvUavAllocator = MakeUnique<CpuDescriptorAllocator>(1024, DescriptorType::CbvSrvUav);
		Gpu_CbvSrvUavAllocator = MakeUnique<GpuDescriptorAllocator>(1024, DescriptorType::CbvSrvUav);
		Cpu_SamplerAllocator = MakeUnique<CpuDescriptorAllocator>(256, DescriptorType::Sampler);
		Gpu_SamplerAllocator = MakeUnique<GpuDescriptorAllocator>(256, DescriptorType::Sampler);
	}

	TUniquePtr<CpuDescriptor> AllocRtv()
	{
		return RtvAllocator->Allocate();
	}

	TUniquePtr<CpuDescriptor> AllocSampler()
	{
		return Cpu_SamplerAllocator->Allocate();
	}

	TUniquePtr<CpuDescriptor> AllocCpuCbvSrvUav()
	{
		return Cpu_CbvSrvUavAllocator->Allocate();
	}

	TUniquePtr<GpuDescriptorRange> AllocGpuCbvSrvUavRange(uint32 InDescriptorNum)
	{
		return Gpu_CbvSrvUavAllocator->Allocate(InDescriptorNum);
	}

	TUniquePtr<GpuDescriptorRange> AllocGpuSamplerRange(uint32 InDescriptorNum)
	{
		return Gpu_SamplerAllocator->Allocate(InDescriptorNum);
	}

	CpuDescriptorAllocator::CpuDescriptorAllocator(uint32 MaxSize, DescriptorType DescType)
	{
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc{};
		HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE(DescType);
		HeapDesc.NumDescriptors = MaxSize;
		HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		DxCheck(GDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(DescriptorHeap.GetInitReference())));
		DescriptorSize = GDevice->GetDescriptorHandleIncrementSize(HeapDesc.Type);

		for (uint32 i = 0; i < MaxSize; i++) {
			AllocateIndexs.Enqueue(i);
		}
	}

	TUniquePtr<CpuDescriptor> CpuDescriptorAllocator::Allocate()
	{
		uint32 Index;
		checkf(AllocateIndexs.Dequeue(Index), TEXT("No space left in the DescriptorAllocator."));
		CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		CpuHandle.Offset(Index, DescriptorSize);
		return MakeUnique<CpuDescriptor>(CpuHandle, this);
	}

	void CpuDescriptorAllocator::Free(CpuDescriptor& Descriptor)
	{
		if (Descriptor.IsValid())
		{
			const size_t HeapAddr = DescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
			const uint32 FreeIndex = (Descriptor.GetHandle().ptr - HeapAddr) / DescriptorSize;
			AllocateIndexs.Enqueue(FreeIndex);
			Descriptor.Reset();
		}
	}

	GpuDescriptorAllocator::GpuDescriptorAllocator(uint32 MaxSize, DescriptorType DescType)
		: InternalAllocator(1, MaxSize)
	{
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc{};
		HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE(DescType);
		HeapDesc.NumDescriptors = MaxSize;
		HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		DxCheck(GDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(DescriptorHeap.GetInitReference())));
		DescriptorSize = GDevice->GetDescriptorHandleIncrementSize(HeapDesc.Type);

	}

	TUniquePtr<GpuDescriptorRange> GpuDescriptorAllocator::Allocate(uint32 InDescriptorNum)
	{
		uint32 FreeBlockByteSizeOffset = InternalAllocator.Allocate(InDescriptorNum, 1);
		CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_GPU_DESCRIPTOR_HANDLE GpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		CpuHandle.Offset(FreeBlockByteSizeOffset, DescriptorSize);
		GpuHandle.Offset(FreeBlockByteSizeOffset, DescriptorSize);
		return MakeUnique<GpuDescriptorRange>(InDescriptorNum, FreeBlockByteSizeOffset, CpuHandle, GpuHandle, this );
	}

	void GpuDescriptorAllocator::Deallocate(uint32 OffsetInHeap, uint32 DescriptorNum)
	{
		InternalAllocator.Deallocate(OffsetInHeap, DescriptorNum);
	}

	CpuDescriptor::CpuDescriptor(CD3DX12_CPU_DESCRIPTOR_HANDLE InHandle, CpuDescriptorAllocator* InAllocator)
		: FromAllocator(InAllocator)
		, CpuHandle(InHandle)
	{
		
	}

	CpuDescriptor::~CpuDescriptor()
	{
		FromAllocator->Free(*this);
	}

	GpuDescriptorRange::GpuDescriptorRange(uint32 InDescriptorNum, uint32 InOffsetInHeap, 
		CD3DX12_CPU_DESCRIPTOR_HANDLE InCpuHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE InGpuHandle, 
		GpuDescriptorAllocator* InAllocator)
		: DescriptorNum(InDescriptorNum)
		, OffsetInHeap(InOffsetInHeap)
		, CpuHandle(InCpuHandle)
		, GpuHandle(InGpuHandle)
		, FromAllocator(InAllocator)
	{

	}

	GpuDescriptorRange::~GpuDescriptorRange()
	{
		FromAllocator->Deallocate(OffsetInHeap, DescriptorNum);
	}

}
