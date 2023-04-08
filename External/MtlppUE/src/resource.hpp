/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Resource.hpp"
#include "imp_Buffer.hpp"
#include "imp_Texture.hpp"
#include "ns.hpp"
#include "command_buffer_fence.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	struct ITable<id<MTLBuffer>, void> : public IMPTable<id<MTLBuffer>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLBuffer>, void>(C)
		{
		}
	};
	
	template<>
	struct ITable<id<MTLTexture>, void> : public IMPTable<id<MTLTexture>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLTexture>, void>(C)
		{
		}
	};
}

namespace mtlpp
{
    class Heap;
	class Device;

    static const NSUInteger ResourceCpuCacheModeShift        = 0;
    static const NSUInteger ResourceStorageModeShift         = 4;
    static const NSUInteger ResourceHazardTrackingModeShift  = 8;
	static const NSUInteger ResourceCpuCacheModeMask        = 0xfUL << ResourceCpuCacheModeShift;
	static const NSUInteger ResourceStorageModeMask         = 0xfUL << ResourceStorageModeShift;
	static const NSUInteger ResourceHazardTrackingModeMask  = 0x1UL << ResourceHazardTrackingModeShift;
	
    enum class PurgeableState
    {
        KeepCurrent = 1,
        NonVolatile = 2,
        Volatile    = 3,
        Empty       = 4,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class CpuCacheMode
    {
        DefaultCache  = 0,
        WriteCombined = 1,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class StorageMode
    {
        Shared                                = 0,
        Managed    MTLPP_AVAILABLE(10_11, NA) = 1,
        Private                               = 2,
        Memoryless MTLPP_AVAILABLE(NA, 10_0)  = 3,
    }
    MTLPP_AVAILABLE(10_11, 9_0);

	enum ResourceOptions : NSUInteger
    {
        CpuCacheModeDefaultCache                                = NSUInteger(CpuCacheMode::DefaultCache)  << ResourceCpuCacheModeShift,
        CpuCacheModeWriteCombined                               = NSUInteger(CpuCacheMode::WriteCombined) << ResourceCpuCacheModeShift,

        StorageModeShared           MTLPP_AVAILABLE(10_11, 9_0) = NSUInteger(StorageMode::Shared)     << ResourceStorageModeShift,
        StorageModeManaged          MTLPP_AVAILABLE(10_11, NA)  = NSUInteger(StorageMode::Managed)    << ResourceStorageModeShift,
        StorageModePrivate          MTLPP_AVAILABLE(10_11, 9_0) = NSUInteger(StorageMode::Private)    << ResourceStorageModeShift,
        StorageModeMemoryless       MTLPP_AVAILABLE(NA, 10_0)   = NSUInteger(StorageMode::Memoryless) << ResourceStorageModeShift,

        HazardTrackingModeUntracked MTLPP_AVAILABLE(10_13, 10_0)   = 0x1 << ResourceHazardTrackingModeShift,

        OptionCpuCacheModeDefault                               = CpuCacheModeDefaultCache,
        OptionCpuCacheModeWriteCombined                         = CpuCacheModeWriteCombined,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

	class MTLPP_EXPORT Resource : public ns::Object<ns::Protocol<id<MTLResource>>::type, ns::CallingConvention::Mixed>
    {
    public:
        Resource(ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<ns::Protocol<id<MTLResource>>::type, ns::CallingConvention::Mixed>(retain) { }
		Resource(ns::Protocol<id<MTLResource>>::type handle, ns::Ownership const retain = ns::Ownership::Retain, ITable* table = nullptr);
		
		Resource(const Resource& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
		Resource(Resource&& rhs);
#endif
		
		Resource& operator=(const Resource& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
		Resource& operator=(Resource&& rhs);
#endif

		ns::AutoReleased<Device>   	GetDevice() const;
        ns::AutoReleased<ns::String>   GetLabel() const;
        CpuCacheMode GetCpuCacheMode() const;
        StorageMode  GetStorageMode() const MTLPP_AVAILABLE(10_11, 9_0);
        ns::AutoReleased<Heap>         GetHeap() const MTLPP_AVAILABLE(10_13, 10_0);
        bool         IsAliasable() const MTLPP_AVAILABLE(10_13, 10_0);
		NSUInteger	 GetAllocatedSize() const MTLPP_AVAILABLE(10_13, 11_0);

        void SetLabel(const ns::String& label);

        PurgeableState SetPurgeableState(PurgeableState state);
        void MakeAliasable() const MTLPP_AVAILABLE(10_13, 10_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

MTLPP_END
