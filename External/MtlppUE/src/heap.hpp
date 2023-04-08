/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Heap.hpp"
#include "ns.hpp"
#include "device.hpp"
#include "resource.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "types.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	struct ITable<id<MTLHeap>, void> : public IMPTable<id<MTLHeap>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLHeap>, void>(C)
		{
		}
	};
	
	template<>
	inline ITable<MTLHeapDescriptor*, void>* CreateIMPTable(MTLHeapDescriptor* handle)
	{
		static ITable<MTLHeapDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
}

namespace mtlpp
{
    class MTLPP_EXPORT HeapDescriptor : public ns::Object<MTLHeapDescriptor*>
    {
    public:
		HeapDescriptor();
		
		HeapDescriptor(MTLHeapDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLHeapDescriptor*>(handle, retain) { }

        NSUInteger     GetSize() const;
        StorageMode  GetStorageMode() const;
        CpuCacheMode GetCpuCacheMode() const;

        void SetSize(NSUInteger size) const;
        void SetStorageMode(StorageMode storageMode) const;
        void SetCpuCacheMode(CpuCacheMode cpuCacheMode) const;
    }
    MTLPP_AVAILABLE(10_13, 10_0);

    class MTLPP_EXPORT Heap : public ns::Object<ns::Protocol<id<MTLHeap>>::type>
    {
    public:
		Heap(ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<ns::Protocol<id<MTLHeap>>::type>(retain) { }
		Heap(ns::Protocol<id<MTLHeap>>::type handle, UE::ITableCache* cache = nullptr, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<ns::Protocol<id<MTLHeap>>::type>(handle, retain, UE::ITableCacheRef(cache).GetHeap(handle)) { }

        ns::AutoReleased<ns::String>   GetLabel() const;
        ns::AutoReleased<Device>       GetDevice() const;
        StorageMode  GetStorageMode() const;
        CpuCacheMode GetCpuCacheMode() const;
        NSUInteger     GetSize() const;
        NSUInteger     GetUsedSize() const;
		NSUInteger	 GetCurrentAllocatedSize() const MTLPP_AVAILABLE(10_13, 11_0);

        void SetLabel(const ns::String& label);

        NSUInteger MaxAvailableSizeWithAlignment(NSUInteger alignment);
        Buffer NewBuffer(NSUInteger length, ResourceOptions options);
        Texture NewTexture(const TextureDescriptor& desc);
        PurgeableState SetPurgeableState(PurgeableState state);
    }
    MTLPP_AVAILABLE(10_13, 10_0);
}

MTLPP_END
