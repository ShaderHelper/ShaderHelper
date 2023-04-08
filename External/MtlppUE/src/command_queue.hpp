/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_CommandQueue.hpp"
#include "ns.hpp"
#include "validation.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	struct ITable<id<MTLCommandQueue>, void> : public IMPTable<id<MTLCommandQueue>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLCommandQueue>, void>(C)
		{
		}
	};
}

namespace mtlpp
{
    class Device;
    class CommandBuffer;

    class MTLPP_EXPORT CommandQueue : public ns::Object<ns::Protocol<id<MTLCommandQueue>>::type>
    {
    public:
		CommandQueue(ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<ns::Protocol<id<MTLCommandQueue>>::type>(retain) { }
		CommandQueue(ns::Protocol<id<MTLCommandQueue>>::type handle, UE::ITableCache* cache = nullptr, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<ns::Protocol<id<MTLCommandQueue>>::type>(handle, retain, UE::ITableCacheRef(cache).GetCommandQueue(handle)) { }

        ns::AutoReleased<ns::String> GetLabel() const;
        ns::AutoReleased<Device>     GetDevice() const;

        void SetLabel(const ns::String& label);

        MTLPP_VALIDATED class CommandBuffer CommandBufferWithUnretainedReferences();
        MTLPP_VALIDATED class CommandBuffer CommandBuffer();
        void                InsertDebugCaptureBoundary() MTLPP_DEPRECATED(10_11, 10_13, 8_0, 11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
	
#if MTLPP_CONFIG_VALIDATE
	class MTLPP_EXPORT ValidatedCommandQueue : public ns::AutoReleased<CommandQueue>
	{
	public:
		ValidatedCommandQueue()
		{
		}
		
		ValidatedCommandQueue(CommandQueue& Wrapped)
		: ns::AutoReleased<CommandQueue>(Wrapped)
		{
		}
		
		MTLPP_VALIDATED class CommandBuffer CommandBufferWithUnretainedReferences();
		MTLPP_VALIDATED class CommandBuffer CommandBuffer();
	};
	
	template <>
	class MTLPP_EXPORT Validator<CommandQueue>
	{
		public:
		Validator(CommandQueue& Val, bool bEnable)
		: Resource(Val)
		{
			if (bEnable)
			{
				Validation = ValidatedCommandQueue(Val);
			}
		}
		
		ValidatedCommandQueue& operator*()
		{
			assert(Validation.GetPtr() != nullptr);
			return Validation;
		}
		
		CommandQueue* operator->()
		{
			return Validation.GetPtr() == nullptr ? &Resource : &Validation;
		}
		
		private:
		CommandQueue& Resource;
		ValidatedCommandQueue Validation;
	};
#endif
}

MTLPP_END
