/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_VertexDesc.hpp"
#include "ns.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	inline ITable<MTLVertexBufferLayoutDescriptor*, void>* CreateIMPTable(MTLVertexBufferLayoutDescriptor* handle)
	{
		static ITable<MTLVertexBufferLayoutDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLVertexAttributeDescriptor*, void>* CreateIMPTable(MTLVertexAttributeDescriptor* handle)
	{
		static ITable<MTLVertexAttributeDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLVertexDescriptor*, void>* CreateIMPTable(MTLVertexDescriptor* handle)
	{
		static ITable<MTLVertexDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
}

namespace mtlpp
{
    enum class VertexFormat
    {
        Invalid               = 0,

        UChar2                = 1,
        UChar3                = 2,
        UChar4                = 3,

        Char2                 = 4,
        Char3                 = 5,
        Char4                 = 6,

        UChar2Normalized      = 7,
        UChar3Normalized      = 8,
        UChar4Normalized      = 9,

        Char2Normalized       = 10,
        Char3Normalized       = 11,
        Char4Normalized       = 12,

        UShort2               = 13,
        UShort3               = 14,
        UShort4               = 15,

        Short2                = 16,
        Short3                = 17,
        Short4                = 18,

        UShort2Normalized     = 19,
        UShort3Normalized     = 20,
        UShort4Normalized     = 21,

        Short2Normalized      = 22,
        Short3Normalized      = 23,
        Short4Normalized      = 24,

        Half2                 = 25,
        Half3                 = 26,
        Half4                 = 27,

        Float                 = 28,
        Float2                = 29,
        Float3                = 30,
        Float4                = 31,

        Int                   = 32,
        Int2                  = 33,
        Int3                  = 34,
        Int4                  = 35,

        UInt                  = 36,
        UInt2                 = 37,
        UInt3                 = 38,
        UInt4                 = 39,

        Int1010102Normalized  = 40,
        UInt1010102Normalized = 41,
		
		UChar4Normalized_BGRA MTLPP_AVAILABLE(10_13, 11_0) = 42,
		
		UChar MTLPP_AVAILABLE(10_13, 11_0) = 45,
		Char MTLPP_AVAILABLE(10_13, 11_0) = 46,
		UCharNormalized MTLPP_AVAILABLE(10_13, 11_0) = 47,
		CharNormalized MTLPP_AVAILABLE(10_13, 11_0) = 48,
		
		UShort MTLPP_AVAILABLE(10_13, 11_0) = 49,
		Short MTLPP_AVAILABLE(10_13, 11_0) = 50,
		UShortNormalized MTLPP_AVAILABLE(10_13, 11_0) = 51,
		ShortNormalized MTLPP_AVAILABLE(10_13, 11_0) = 52,
		
		Half MTLPP_AVAILABLE(10_13, 11_0) = 53,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class VertexStepFunction
    {
        Constant                                          = 0,
        PerVertex                                         = 1,
        PerInstance                                       = 2,
        PerPatch             MTLPP_AVAILABLE(10_12, 10_0) = 3,
        PerPatchControlPoint MTLPP_AVAILABLE(10_12, 10_0) = 4,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class MTLPP_EXPORT VertexBufferLayoutDescriptor : public ns::Object<MTLVertexBufferLayoutDescriptor*>
    {
    public:
        VertexBufferLayoutDescriptor();
		VertexBufferLayoutDescriptor(ns::Ownership const retain) : ns::Object<MTLVertexBufferLayoutDescriptor*>(retain) {}
        VertexBufferLayoutDescriptor(MTLVertexBufferLayoutDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLVertexBufferLayoutDescriptor*>(handle, retain) { }

        NSUInteger           GetStride() const;
        VertexStepFunction GetStepFunction() const;
        NSUInteger           GetStepRate() const;

        void SetStride(NSUInteger stride);
        void SetStepFunction(VertexStepFunction stepFunction);
        void SetStepRate(NSUInteger stepRate);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class MTLPP_EXPORT VertexAttributeDescriptor : public ns::Object<MTLVertexAttributeDescriptor*>
    {
    public:
        VertexAttributeDescriptor();
		VertexAttributeDescriptor(ns::Ownership const retain) : ns::Object<MTLVertexAttributeDescriptor*>(retain) {}
        VertexAttributeDescriptor(MTLVertexAttributeDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLVertexAttributeDescriptor*>(handle, retain) { }

        VertexFormat GetFormat() const;
        NSUInteger     GetOffset() const;
        NSUInteger     GetBufferIndex() const;

        void SetFormat(VertexFormat format);
        void SetOffset(NSUInteger offset);
        void SetBufferIndex(NSUInteger bufferIndex);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class MTLPP_EXPORT VertexDescriptor : public ns::Object<MTLVertexDescriptor*>
    {
    public:
        VertexDescriptor();
		VertexDescriptor(ns::Ownership const retain) : ns::Object<MTLVertexDescriptor*>(retain) {}
        VertexDescriptor(MTLVertexDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLVertexDescriptor*>(handle, retain) { }

        ns::AutoReleased<ns::Array<VertexBufferLayoutDescriptor>> GetLayouts() const;
        ns::AutoReleased<ns::Array<VertexAttributeDescriptor>>    GetAttributes() const;

        void Reset();
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

MTLPP_END
