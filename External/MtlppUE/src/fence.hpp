/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Fence.hpp"
#include "ns.hpp"
#include "device.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	struct ITable<id<MTLFence>, void> : public IMPTable<id<MTLFence>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLFence>, void>(C)
		{
		}
	};
}

namespace mtlpp
{
    class MTLPP_EXPORT Fence : public ns::Object<ns::Protocol<id<MTLFence>>::type>
    {
    public:
        Fence() { }
		Fence(ns::Protocol<id<MTLFence>>::type handle, UE::ITableCache* cache = nullptr, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<ns::Protocol<id<MTLFence>>::type>(handle, retain, UE::ITableCacheRef(cache).GetFence(handle)) { }

        ns::AutoReleased<Device>    GetDevice() const;
        ns::AutoReleased<ns::String> GetLabel() const;

        void SetLabel(const ns::String& label);
    }
    MTLPP_AVAILABLE(10_13, 10_0);
}

MTLPP_END
