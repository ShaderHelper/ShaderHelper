#pragma once
#include "D3D12Common.h"
namespace FRAMEWORK
{
	enum class DescriptorType
	{
		CBV = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		SRV = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		UAV = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		SAMPLER = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		RTV = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		DSV = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
	};

	enum class Descriptorvisibility
	{
		CpuGpuVisible = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		CpuVisible = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	};
	
	template<uint32 MaxSize, Descriptorvisibility Visibility, DescriptorType DescType> class DescriptorAllocator;

	template<uint32 MaxSize, DescriptorType DescType>
	using CpuDescriptorAllocator = DescriptorAllocator<MaxSize, Descriptorvisibility::CpuVisible, DescType>;

	template<uint32 MaxSize, DescriptorType DescType>
	using GpuDescriptorAllocator = DescriptorAllocator<MaxSize, Descriptorvisibility::CpuGpuVisible, DescType>;


	template<Descriptorvisibility Visibility>
	struct DescriptorHandle;

	template<>
	struct DescriptorHandle<Descriptorvisibility::CpuVisible>
	{
		DescriptorHandle(uint32 Index, ID3D12DescriptorHeap* DescriptorHeap, uint32 DescriptorSize) {
			Init(Index, DescriptorHeap, DescriptorSize);
		}

		void Init(uint32 Index, ID3D12DescriptorHeap* DescriptorHeap, uint32 DescriptorSize) {
			if (!IsValid()) {
				CpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
				CpuHandle.Offset(Index, DescriptorSize);
			}
		}

		bool IsValid() const {
			return CpuHandle.ptr != 0;
		}

		void Reset() {
			CpuHandle = {};
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle;
	};

	template<>
	struct DescriptorHandle<Descriptorvisibility::CpuGpuVisible>
	{
		DescriptorHandle(uint32 Index, ID3D12DescriptorHeap* DescriptorHeap, uint32 DescriptorSize)
		{
			Init(Index, DescriptorHeap, DescriptorSize);
		}

		void Init(uint32 Index, ID3D12DescriptorHeap* DescriptorHeap, uint32 DescriptorSize) {
			if (!IsValid()) {
				CpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
				GpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
				CpuHandle.Offset(Index, DescriptorSize);
				GpuHandle.Offset(Index, DescriptorSize);
			}
		}

		bool IsValid() const {
			return CpuHandle.ptr != 0 && GpuHandle.ptr != 0;
		}

		void Reset() {
			CpuHandle = {};
			GpuHandle = {};
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE GpuHandle;
	};
	
	template<uint32 MaxSize, Descriptorvisibility Visibility, DescriptorType DescType>
	class DescriptorAllocator : public FNoncopyable
	{
	public:
		DescriptorAllocator();
		DescriptorHandle<Visibility> Allocate();
		void Free(DescriptorHandle<Visibility>& Handle);
		void Reset();
	private:
		TRefCountPtr<ID3D12DescriptorHeap> DescriptorHeap;
		uint32 DescriptorSize;
		TQueue<uint32> AllocateIndexs;
	};

}

#include "D3D12Descriptor.hpp"