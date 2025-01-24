#pragma once
#include "Dx12Common.h"
#include "Dx12Allocation.h"

namespace FW
{
	enum class DescriptorType
	{
		CbvSrvUav = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		Sampler = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		Rtv = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		Dsv = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
	};

	class CpuDescriptorAllocator;

	class CpuDescriptor : public FNoncopyable
	{
	public:
		CpuDescriptor(CD3DX12_CPU_DESCRIPTOR_HANDLE InHandle, CpuDescriptorAllocator* InAllocator);
		~CpuDescriptor();

		bool IsValid() const {
			return CpuHandle.ptr != 0;
		}

		void Reset() {
			CpuHandle.ptr = 0;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() const
		{
			check(IsValid());
			return CpuHandle;
		}

	private:
		CpuDescriptorAllocator* FromAllocator;
		CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle;
	};

	class CpuDescriptorAllocator
	{
	public:
		CpuDescriptorAllocator(uint32 MaxSize, DescriptorType DescType);
		TUniquePtr<CpuDescriptor> Allocate();
		void Free(CpuDescriptor& Handle);
		ID3D12DescriptorHeap* GetDescriptorHeap() const {
			return DescriptorHeap;
		}

	private:
		TRefCountPtr<ID3D12DescriptorHeap> DescriptorHeap;
		uint32 DescriptorSize;
		TQueue<uint32> AllocateIndexs;
	};

	class GpuDescriptorAllocator;

	class GpuDescriptorRange : public FNoncopyable
	{
	public:
		GpuDescriptorRange(uint32 InDescriptorNum, uint32 InOffsetInHeap,
			CD3DX12_CPU_DESCRIPTOR_HANDLE InCpuHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE InGpuHandle,
			GpuDescriptorAllocator* InAllocator);
		~GpuDescriptorRange();

		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const
		{
			return CpuHandle;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const
		{
			return GpuHandle;
		}

		uint32 GetDescriptorNum() const { return DescriptorNum; }

	private:
		uint32 DescriptorNum;
		uint32 OffsetInHeap;
		CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE GpuHandle;
		GpuDescriptorAllocator* FromAllocator;
	};


	class GpuDescriptorAllocator
	{
	public:
		GpuDescriptorAllocator(uint32 MaxSize, DescriptorType DescType);
		TUniquePtr<GpuDescriptorRange> Allocate(uint32 InDescriptorNum);
		void Deallocate(uint32 OffsetInHeap, uint32 DescriptorNum);
		ID3D12DescriptorHeap* GetDescriptorHeap() const {
			return DescriptorHeap;
		}

	private:
		BuddyAllocator InternalAllocator;
		TRefCountPtr<ID3D12DescriptorHeap> DescriptorHeap;
		uint32 DescriptorSize;
	};

	extern TUniquePtr<CpuDescriptorAllocator> RtvAllocator;
	extern TUniquePtr<CpuDescriptorAllocator> Cpu_CbvSrvUavAllocator;
	extern TUniquePtr<GpuDescriptorAllocator> Gpu_CbvSrvUavAllocator;
	extern TUniquePtr<CpuDescriptorAllocator> Cpu_SamplerAllocator;
	extern TUniquePtr<GpuDescriptorAllocator> Gpu_SamplerAllocator;

	void InitDescriptorAllocator();
	TUniquePtr<CpuDescriptor> AllocRtv();
	TUniquePtr<CpuDescriptor> AllocSampler();
	TUniquePtr<CpuDescriptor> AllocCpuCbvSrvUav();
	TUniquePtr<GpuDescriptorRange> AllocGpuCbvSrvUavRange(uint32 InDescriptorNum);
	TUniquePtr<GpuDescriptorRange> AllocGpuSamplerRange(uint32 InDescriptorNum);

}
