/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLVertexDescriptor.h>
#include "vertex_descriptor.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    VertexBufferLayoutDescriptor::VertexBufferLayoutDescriptor() :
        ns::Object<MTLVertexBufferLayoutDescriptor*>([[MTLVertexBufferLayoutDescriptor alloc] init], ns::Ownership::Assign)
    {
    }

    NSUInteger VertexBufferLayoutDescriptor::GetStride() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->stride(m_ptr);
#else
        return NSUInteger([(MTLVertexBufferLayoutDescriptor*)m_ptr stride]);
#endif
    }

    NSUInteger VertexBufferLayoutDescriptor::GetStepRate() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->stepRate(m_ptr);
#else
        return NSUInteger([(MTLVertexBufferLayoutDescriptor*)m_ptr stepRate]);
#endif
    }

    VertexStepFunction VertexBufferLayoutDescriptor::GetStepFunction() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return VertexStepFunction(m_table->stepFunction(m_ptr));
#else
        return VertexStepFunction([(MTLVertexBufferLayoutDescriptor*)m_ptr stepFunction]);
#endif
    }

    void VertexBufferLayoutDescriptor::SetStride(NSUInteger stride)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setStride(m_ptr, stride);
#else
        [(MTLVertexBufferLayoutDescriptor*)m_ptr setStride:stride];
#endif
    }

    void VertexBufferLayoutDescriptor::SetStepRate(NSUInteger stepRate)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setStepRate(m_ptr, stepRate);
#else
        [(MTLVertexBufferLayoutDescriptor*)m_ptr setStepRate:stepRate];
#endif
    }

    void VertexBufferLayoutDescriptor::SetStepFunction(VertexStepFunction stepFunction)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setStepFunction(m_ptr, MTLVertexStepFunction(stepFunction));
#else
        [(MTLVertexBufferLayoutDescriptor*)m_ptr setStepFunction:MTLVertexStepFunction(stepFunction)];
#endif
    }

    VertexAttributeDescriptor::VertexAttributeDescriptor() :
        ns::Object<MTLVertexAttributeDescriptor*>([[MTLVertexAttributeDescriptor alloc] init], ns::Ownership::Assign)
    {
    }

    VertexFormat VertexAttributeDescriptor::GetFormat() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return VertexFormat(m_table->format(m_ptr));
#else
        return VertexFormat([(MTLVertexAttributeDescriptor*)m_ptr format]);
#endif
    }

    NSUInteger VertexAttributeDescriptor::GetOffset() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(m_table->offset(m_ptr));
#else
        return NSUInteger([(MTLVertexAttributeDescriptor*)m_ptr offset]);
#endif
    }

    NSUInteger VertexAttributeDescriptor::GetBufferIndex() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(m_table->bufferIndex(m_ptr));
#else
        return NSUInteger([(MTLVertexAttributeDescriptor*)m_ptr bufferIndex]);
#endif
    }

    void VertexAttributeDescriptor::SetFormat(VertexFormat format)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setFormat(m_ptr, MTLVertexFormat(format));
#else
        [(MTLVertexAttributeDescriptor*)m_ptr setFormat:MTLVertexFormat(format)];
#endif
    }

    void VertexAttributeDescriptor::SetOffset(NSUInteger offset)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setOffset(m_ptr, offset);
#else
        [(MTLVertexAttributeDescriptor*)m_ptr setOffset:offset];
#endif
    }

    void VertexAttributeDescriptor::SetBufferIndex(NSUInteger bufferIndex)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setBufferIndex(m_ptr, bufferIndex);
#else
        [(MTLVertexAttributeDescriptor*)m_ptr setBufferIndex:bufferIndex];
#endif
    }

    VertexDescriptor::VertexDescriptor() :
        ns::Object<MTLVertexDescriptor*>([[MTLVertexDescriptor alloc] init], ns::Ownership::Assign)
    {
    }

    ns::AutoReleased<ns::Array<VertexBufferLayoutDescriptor>> VertexDescriptor::GetLayouts() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::Array<VertexBufferLayoutDescriptor>>((NSArray*)m_table->layouts(m_ptr));
#else
        return ns::AutoReleased<ns::Array<VertexBufferLayoutDescriptor>>((NSArray*)[(MTLVertexDescriptor*)m_ptr layouts]);
#endif
    }

    ns::AutoReleased<ns::Array<VertexAttributeDescriptor>> VertexDescriptor::GetAttributes() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::Array<VertexAttributeDescriptor>>((NSArray*)m_table->attributes(m_ptr));
#else
        return ns::AutoReleased<ns::Array<VertexAttributeDescriptor>>((NSArray*)[(MTLVertexDescriptor*)m_ptr attributes]);
#endif
    }

    void VertexDescriptor::Reset()
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->reset(m_ptr);
#else
        [(MTLVertexDescriptor*)m_ptr reset];
#endif
    }
}

MTLPP_END
