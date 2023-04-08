/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_StageInputOutputDescriptor.hpp"
#include "device.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	inline ITable<MTLBufferLayoutDescriptor*, void>* CreateIMPTable(MTLBufferLayoutDescriptor* handle)
	{
		static ITable<MTLBufferLayoutDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLAttributeDescriptor*, void>* CreateIMPTable(MTLAttributeDescriptor* handle)
	{
		static ITable<MTLAttributeDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLStageInputOutputDescriptor*, void>* CreateIMPTable(MTLStageInputOutputDescriptor* handle)
	{
		static ITable<MTLStageInputOutputDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
}

namespace mtlpp
{
    enum class AttributeFormat
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
    MTLPP_AVAILABLE(10_12, 10_0);

    enum class IndexType
    {
        UInt16 = 0,
        UInt32 = 1,
    }
    MTLPP_AVAILABLE(10_11, 8_0);


    enum class StepFunction
    {
        Constant                                                  = 0,

        PerVertex                                                 = 1,
        PerInstance                                               = 2,
        PerPatch                     MTLPP_AVAILABLE(10_12, 10_0) = 3,
        PerPatchControlPoint         MTLPP_AVAILABLE(10_12, 10_0) = 4,

        ThreadPositionInGridX                                     = 5,
        ThreadPositionInGridY                                     = 6,
        ThreadPositionInGridXIndexed                              = 7,
        ThreadPositionInGridYIndexed                              = 8,
    }
    MTLPP_AVAILABLE(10_12, 10_0);

    class MTLPP_EXPORT BufferLayoutDescriptor : public ns::Object<MTLBufferLayoutDescriptor*>
    {
    public:
        BufferLayoutDescriptor();
		BufferLayoutDescriptor(ns::Ownership const retain) : ns::Object<MTLBufferLayoutDescriptor*>(retain) {}
        BufferLayoutDescriptor(MTLBufferLayoutDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLBufferLayoutDescriptor*>(handle, retain) { }

        NSUInteger     GetStride() const;
        StepFunction GetStepFunction() const;
        NSUInteger     GetStepRate() const;

        void SetStride(NSUInteger stride);
        void SetStepFunction(StepFunction stepFunction);
        void SetStepRate(NSUInteger stepRate);
    }
    MTLPP_AVAILABLE(10_12, 10_0);

    class MTLPP_EXPORT AttributeDescriptor : public ns::Object<MTLAttributeDescriptor*>
    {
    public:
        AttributeDescriptor();
		AttributeDescriptor(ns::Ownership const retain) : ns::Object<MTLAttributeDescriptor*>(retain) {}
        AttributeDescriptor(MTLAttributeDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLAttributeDescriptor*>(handle, retain) { }

        AttributeFormat GetFormat() const;
        NSUInteger        GetOffset() const;
        NSUInteger        GetBufferIndex() const;

        void SetFormat(AttributeFormat format);
        void SetOffset(NSUInteger offset);
        void SetBufferIndex(NSUInteger bufferIndex);
    }
    MTLPP_AVAILABLE(10_12, 10_0);

    class MTLPP_EXPORT StageInputOutputDescriptor : public ns::Object<MTLStageInputOutputDescriptor*>
    {
    public:
        StageInputOutputDescriptor();
		StageInputOutputDescriptor(ns::Ownership const retain)  : ns::Object<MTLStageInputOutputDescriptor*>(retain) {}
        StageInputOutputDescriptor(MTLStageInputOutputDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLStageInputOutputDescriptor*>(handle, retain) { }


        ns::AutoReleased<ns::Array<BufferLayoutDescriptor>> GetLayouts() const;
        ns::AutoReleased<ns::Array<AttributeDescriptor>>    GetAttributes() const;
        IndexType                         GetIndexType() const;
        NSUInteger                          GetIndexBufferIndex() const;

        void SetIndexType(IndexType indexType);
        void SetIndexBufferIndex(NSUInteger indexBufferIndex);

        void Reset();
    }
    MTLPP_AVAILABLE(10_12, 10_0);
}

MTLPP_END
