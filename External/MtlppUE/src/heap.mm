/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLHeap.h>
#include "heap.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	HeapDescriptor::HeapDescriptor()
	: ns::Object<MTLHeapDescriptor*>([[MTLHeapDescriptor alloc] init], ns::Ownership::Assign)
	{
	}
	
    NSUInteger HeapDescriptor::GetSize() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->Size(m_ptr);
#else
        return NSUInteger([(MTLHeapDescriptor*)m_ptr size]);
#endif
#else
        return 0;
#endif

    }

    StorageMode HeapDescriptor::GetStorageMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return StorageMode(m_table->StorageMode(m_ptr));
#else
        return StorageMode([(MTLHeapDescriptor*)m_ptr storageMode]);
#endif
#else
        return StorageMode(0);
#endif

    }

    CpuCacheMode HeapDescriptor::GetCpuCacheMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return CpuCacheMode(m_table->CpuCacheMode(m_ptr));
#else
        return CpuCacheMode([(MTLHeapDescriptor*)m_ptr cpuCacheMode]);
#endif
#else
        return CpuCacheMode(0);
#endif

    }

    void HeapDescriptor::SetSize(NSUInteger size) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setSize(m_ptr, size);
#else
        [(MTLHeapDescriptor*)m_ptr setSize:size];
#endif
#endif

    }

    void HeapDescriptor::SetStorageMode(StorageMode storageMode) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setStorageMode(m_ptr, MTLStorageMode(storageMode));
#else
        [(MTLHeapDescriptor*)m_ptr setStorageMode:MTLStorageMode(storageMode)];
#endif
#endif

    }

    void HeapDescriptor::SetCpuCacheMode(CpuCacheMode cpuCacheMode) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setCpuCacheMode(m_ptr, MTLCPUCacheMode(cpuCacheMode));
#else
        [(MTLHeapDescriptor*)m_ptr setCpuCacheMode:MTLCPUCacheMode(cpuCacheMode)];
#endif
#endif

    }

    ns::AutoReleased<ns::String> Heap::GetLabel() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::String>(m_table->Label(m_ptr));
#else
        return ns::AutoReleased<ns::String>([(id<MTLHeap>)m_ptr label]);
#endif
#else
        return ns::AutoReleased<ns::String>();
#endif

    }

    ns::AutoReleased<Device> Heap::GetDevice() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<Device>(m_table->Device(m_ptr));
#else
        return ns::AutoReleased<Device>([(id<MTLHeap>)m_ptr device]);
#endif
#else
        return ns::AutoReleased<Device>();
#endif

    }

    StorageMode Heap::GetStorageMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return StorageMode(m_table->StorageMode(m_ptr));
#else
        return StorageMode([(id<MTLHeap>)m_ptr storageMode]);
#endif
#else
        return StorageMode(0);
#endif

    }

    CpuCacheMode Heap::GetCpuCacheMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return CpuCacheMode(m_table->CpuCacheMode(m_ptr));
#else
        return CpuCacheMode([(id<MTLHeap>)m_ptr cpuCacheMode]);
#endif
#else
        return CpuCacheMode(0);
#endif

    }

    NSUInteger Heap::GetSize() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->Size(m_ptr);
#else
        return NSUInteger([(id<MTLHeap>)m_ptr size]);
#endif
#else
        return 0;
#endif

    }

    NSUInteger Heap::GetUsedSize() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->UsedSize(m_ptr);
#else
        return NSUInteger([(id<MTLHeap>)m_ptr usedSize]);
#endif
#else
        return 0;
#endif

    }
	
	NSUInteger Heap::GetCurrentAllocatedSize() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->CurrentAllocatedSize(m_ptr);
#else
		return NSUInteger([(id<MTLHeap>)m_ptr currentAllocatedSize]);
#endif
#else
		return GetSize();
#endif
	}

    void Heap::SetLabel(const ns::String& label)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetLabel(m_ptr, label.GetPtr());
#else
        [(id<MTLHeap>)m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
#endif

    }

    NSUInteger Heap::MaxAvailableSizeWithAlignment(NSUInteger alignment)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->MaxAvailableSizeWithAlignment(m_ptr, alignment);
#else
        return NSUInteger([(id<MTLHeap>)m_ptr maxAvailableSizeWithAlignment:alignment]);
#endif
#else
        return 0;
#endif

    }

    Buffer Heap::NewBuffer(NSUInteger length, ResourceOptions options)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return Buffer(m_table->NewBufferWithLength(m_ptr, length, MTLResourceOptions(options)), m_table->TableCache, ns::Ownership::Assign);
#else
        return Buffer([(id<MTLHeap>)m_ptr newBufferWithLength:length options:MTLResourceOptions(options)], nullptr, ns::Ownership::Assign);
#endif
#else
        return nullptr;
#endif

    }

    Texture Heap::NewTexture(const TextureDescriptor& desc)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return Texture(m_table->NewTextureWithDescriptor(m_ptr, desc.GetPtr()), m_table->TableCache, ns::Ownership::Assign);
#else
        return Texture([(id<MTLHeap>)m_ptr newTextureWithDescriptor:(MTLTextureDescriptor*)desc.GetPtr()], nullptr, ns::Ownership::Assign);
#endif
#else
        return nullptr;
#endif

    }

    PurgeableState Heap::SetPurgeableState(PurgeableState state)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return PurgeableState(m_table->SetPurgeableState(m_ptr, MTLPurgeableState(state)));
#else
        return PurgeableState([(id<MTLHeap>)m_ptr setPurgeableState:MTLPurgeableState(state)]);
#endif
#else
        return PurgeableState(0);
#endif

    }
}

MTLPP_END
