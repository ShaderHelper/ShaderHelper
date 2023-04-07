#pragma once

namespace FRAMEWORK
{
	template<uint32 MaxSize, Descriptorvisibility Visibility, DescriptorType DescType>
	DescriptorAllocator<MaxSize, Visibility, DescType>::DescriptorAllocator()
	{
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc{};
		HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE(DescType);
		HeapDesc.NumDescriptors = MaxSize;
		HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS(Visibility);
		DxCheck(GDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(DescriptorHeap.GetInitReference())));
		DescriptorSize = GDevice->GetDescriptorHandleIncrementSize(HeapDesc.Type);

		for (int i = 0; i < MaxSize; i++) {
			AllocateIndexs.Enqueue(i);
		}
	}

	template<uint32 MaxSize, Descriptorvisibility Visibility, DescriptorType DescType>
	DescriptorHandle<Visibility> DescriptorAllocator<MaxSize, Visibility, DescType>::Allocate()
	{
		uint32 Index;
		checkf(AllocateIndexs.Dequeue(Index), TEXT("No space left in the DescriptorAllocator."));
		return { Index, DescriptorHeap, DescriptorSize };
	}

	template<uint32 MaxSize, Descriptorvisibility Visibility, DescriptorType DescType>
	void DescriptorAllocator<MaxSize, Visibility, DescType>::Free(DescriptorHandle<Visibility>& Handle)
	{
		if (Handle.IsValid())
		{
			const size_t HeapAddr = DescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
			const uint32 FreeIndex = (Handle.CpuHandle.ptr - HeapAddr) / DescriptorSize;
			AllocateIndexs.Enqueue(FreeIndex);
			Handle.Reset();
		}
	}

	template<uint32 MaxSize, Descriptorvisibility Visibility, DescriptorType DescType>
	void DescriptorAllocator<MaxSize, Visibility, DescType>::Reset()
	{
		AllocateIndexs.Empty();
		for (int i = 0; i < MaxSize; i++) {
			AllocateIndexs.Enqueue(i);
		}
	}
}
