/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_DepthStencil.hpp"
#include "ns.hpp"
#include "device.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	struct ITable<id<MTLDepthStencilState>, void> : public IMPTable<id<MTLDepthStencilState>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLDepthStencilState>, void>(C)
		{
		}
	};
	
	template<>
	inline ITable<MTLStencilDescriptor*, void>* CreateIMPTable(MTLStencilDescriptor* handle)
	{
		static ITable<MTLStencilDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
	template<>
	inline ITable<MTLDepthStencilDescriptor*, void>* CreateIMPTable(MTLDepthStencilDescriptor* handle)
	{
		static ITable<MTLDepthStencilDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
}

namespace mtlpp
{
    enum class CompareFunction
    {
        Never        = 0,
        Less         = 1,
        Equal        = 2,
        LessEqual    = 3,
        Greater      = 4,
        NotEqual     = 5,
        GreaterEqual = 6,
        Always       = 7,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class StencilOperation
    {
        Keep           = 0,
        Zero           = 1,
        Replace        = 2,
        IncrementClamp = 3,
        DecrementClamp = 4,
        Invert         = 5,
        IncrementWrap  = 6,
        DecrementWrap  = 7,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class MTLPP_EXPORT StencilDescriptor : public ns::Object<MTLStencilDescriptor*>
    {
    public:
        StencilDescriptor();
		StencilDescriptor(ns::Ownership const retain) : ns::Object<MTLStencilDescriptor*>(retain) {}
        StencilDescriptor(MTLStencilDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLStencilDescriptor*>(handle, retain) { }

        CompareFunction  GetStencilCompareFunction() const;
        StencilOperation GetStencilFailureOperation() const;
        StencilOperation GetDepthFailureOperation() const;
        StencilOperation GetDepthStencilPassOperation() const;
        uint32_t         GetReadMask() const;
        uint32_t         GetWriteMask() const;

        void SetStencilCompareFunction(CompareFunction stencilCompareFunction);
        void SetStencilFailureOperation(StencilOperation stencilFailureOperation);
        void SetDepthFailureOperation(StencilOperation depthFailureOperation);
        void SetDepthStencilPassOperation(StencilOperation depthStencilPassOperation);
        void SetReadMask(uint32_t readMask);
        void SetWriteMask(uint32_t writeMask);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class MTLPP_EXPORT DepthStencilDescriptor : public ns::Object<MTLDepthStencilDescriptor*>
    {
    public:
        DepthStencilDescriptor();
        DepthStencilDescriptor(MTLDepthStencilDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLDepthStencilDescriptor*>(handle, retain) { }

        CompareFunction   GetDepthCompareFunction() const;
        bool              IsDepthWriteEnabled() const;
        ns::AutoReleased<StencilDescriptor> GetFrontFaceStencil() const;
        ns::AutoReleased<StencilDescriptor> GetBackFaceStencil() const;
        ns::AutoReleased<ns::String>        GetLabel() const;

        void SetDepthCompareFunction(CompareFunction depthCompareFunction) const;
        void SetDepthWriteEnabled(bool depthWriteEnabled) const;
        void SetFrontFaceStencil(const StencilDescriptor& frontFaceStencil) const;
        void SetBackFaceStencil(const StencilDescriptor& backFaceStencil) const;
        void SetLabel(const ns::String& label) const;
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class MTLPP_EXPORT DepthStencilState : public ns::Object<ns::Protocol<id<MTLDepthStencilState>>::type>
    {
    public:
        DepthStencilState() { }
		DepthStencilState(ns::Protocol<id<MTLDepthStencilState>>::type handle, UE::ITableCache* cache = nullptr, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<ns::Protocol<id<MTLDepthStencilState>>::type>(handle, retain, UE::ITableCacheRef(cache).GetDepthStencilState(handle)) { }

        ns::AutoReleased<ns::String> GetLabel() const;
        ns::AutoReleased<Device>     GetDevice() const;
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

MTLPP_END
