/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLResource.h>
#include "resource.hpp"
#include "device.hpp"
#include "heap.hpp"
#include "command_encoder.hpp"
#include "debugger.hpp"
#include <objc/runtime.h>

MTLPP_BEGIN

namespace mtlpp
{
	Resource::Resource(ns::Protocol<id<MTLResource>>::type handle, ns::Ownership const retain, ITable* table)
	: ns::Object<ns::Protocol<id<MTLResource>>::type, ns::CallingConvention::Mixed>(handle, retain, table)
	{
	}
	
	Resource::Resource(const Resource& rhs)
	: ns::Object<ns::Protocol<id<MTLResource>>::type, ns::CallingConvention::Mixed>(rhs)
	{
	}
	
#if MTLPP_CONFIG_RVALUE_REFERENCES
	Resource::Resource(Resource&& rhs)
	: ns::Object<ns::Protocol<id<MTLResource>>::type, ns::CallingConvention::Mixed>((ns::Object<ns::Protocol<id<MTLResource>>::type, ns::CallingConvention::Mixed>&&)rhs)
	{
		
	}
#endif
	
	Resource& Resource::operator=(const Resource& rhs)
	{
		if (this != &rhs)
		{
			ns::Object<ns::Protocol<id<MTLResource>>::type, ns::CallingConvention::Mixed>::operator=(rhs);
		}
		return *this;
	}
	
#if MTLPP_CONFIG_RVALUE_REFERENCES
	Resource& Resource::operator=(Resource&& rhs)
	{
		ns::Object<ns::Protocol<id<MTLResource>>::type, ns::CallingConvention::Mixed>::operator=((ns::Object<ns::Protocol<id<MTLResource>>::type, ns::CallingConvention::Mixed>&&)rhs);
		return *this;
	}
#endif
	
	ns::AutoReleased<Device> Resource::GetDevice() const
	{
		Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
		{
			return ns::AutoReleased<Device>(((IMPTableResource<Resource::Type>*)m_table)->Device(m_ptr));
		}
		else
#endif
		{
			return ns::AutoReleased<Device>([(id<MTLResource>)m_ptr device]);
		}
	}
	
    ns::AutoReleased<ns::String> Resource::GetLabel() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ns::AutoReleased<ns::String>(((IMPTableResource<Resource::Type>*)m_table)->Label(m_ptr));
		else
#endif
        	return ns::AutoReleased<ns::String>([(id<MTLResource>)m_ptr label]);
    }

    CpuCacheMode Resource::GetCpuCacheMode() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return CpuCacheMode(((IMPTableResource<Resource::Type>*)m_table)->CpuCacheMode(m_ptr));
		else
#endif
        	return CpuCacheMode([(id<MTLResource>)m_ptr cpuCacheMode]);
    }

    StorageMode Resource::GetStorageMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return StorageMode(((IMPTableResource<Resource::Type>*)m_table)->StorageMode(m_ptr));
		else
#endif
        	return StorageMode([(id<MTLResource>)m_ptr storageMode]);
#else
        return StorageMode(0);
#endif
    }

    ns::AutoReleased<Heap> Resource::GetHeap() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ns::AutoReleased<Heap>(((IMPTableResource<Resource::Type>*)m_table)->Heap(m_ptr));
		else
#endif
		if(@available(macOS 10.13, iOS 10.0, *))
			return ns::AutoReleased<Heap>([(id<MTLResource>)m_ptr heap]);
		else
			return ns::AutoReleased<Heap>();
#else
		return ns::AutoReleased<Heap>();
#endif
    }

    bool Resource::IsAliasable() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ((IMPTableResource<Resource::Type>*)m_table)->IsAliasable(m_ptr);
		else
#endif
		if(@available(macOS 10.13, iOS 10.0, *))
			return [(id<MTLResource>)m_ptr isAliasable];
		else
			return false;
#else
		return false;
#endif
    }
	
	NSUInteger Resource::GetAllocatedSize() const
	{
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ((IMPTableResource<Resource::Type>*)m_table)->AllocatedSize(m_ptr);
		else
#endif
			return [(id<MTLResource>)m_ptr allocatedSize];
#else
		return 0;
#endif
	}

    void Resource::SetLabel(const ns::String& label)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			((IMPTableResource<Resource::Type>*)m_table)->SetLabel(m_ptr, label.GetPtr());
		else
#endif
        	[(id<MTLResource>)m_ptr setLabel:(NSString*)label.GetPtr()];
    }

    PurgeableState Resource::SetPurgeableState(PurgeableState state)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return (PurgeableState)((IMPTableResource<Resource::Type>*)m_table)->SetPurgeableState(m_ptr, MTLPurgeableState(state));
		else
#endif
        	return PurgeableState([(id<MTLResource>)m_ptr setPurgeableState:MTLPurgeableState(state)]);
    }

    void Resource::MakeAliasable() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
		{
			((IMPTableResource<Resource::Type>*)m_table)->MakeAliasable(m_ptr);
		}
		else
#endif
		if(@available(macOS 10.13, iOS 10.0, *))
			[(id<MTLResource>)m_ptr makeAliasable];
#endif
    }
}

MTLPP_END
