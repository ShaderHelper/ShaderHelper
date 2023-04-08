/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLStageInputOutputDescriptor.h>
#include "stage_input_output_descriptor.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    BufferLayoutDescriptor::BufferLayoutDescriptor() :
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        ns::Object<MTLBufferLayoutDescriptor*>([[MTLBufferLayoutDescriptor alloc] init], ns::Ownership::Assign)
#else
        ns::Object<MTLBufferLayoutDescriptor*>(nullptr)
#endif
    {
    }

    NSUInteger BufferLayoutDescriptor::GetStride() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->stride(m_ptr);
#else
        return NSUInteger([(MTLBufferLayoutDescriptor*)m_ptr stride]);
#endif
#else
        return 0;
#endif
    }

    StepFunction BufferLayoutDescriptor::GetStepFunction() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return StepFunction(m_table->stepFunction(m_ptr));
#else
        return StepFunction([(MTLBufferLayoutDescriptor*)m_ptr stepFunction]);
#endif
#else
        return StepFunction(0);
#endif
    }

    NSUInteger BufferLayoutDescriptor::GetStepRate() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(m_table->stepRate(m_ptr));
#else
        return NSUInteger([(MTLBufferLayoutDescriptor*)m_ptr stepRate]);
#endif
#else
        return 0;
#endif
    }

    void BufferLayoutDescriptor::SetStride(NSUInteger stride)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setStride(m_ptr, stride);
#else
        [(MTLBufferLayoutDescriptor*)m_ptr setStride:stride];
#endif
#endif
    }

    void BufferLayoutDescriptor::SetStepFunction(StepFunction stepFunction)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setStepFunction(m_ptr, MTLStepFunction(stepFunction));
#else
        [(MTLBufferLayoutDescriptor*)m_ptr setStepFunction:MTLStepFunction(stepFunction)];
#endif
#endif
    }

    void BufferLayoutDescriptor::SetStepRate(NSUInteger stepRate)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setStepRate(m_ptr, stepRate);
#else
        [(MTLBufferLayoutDescriptor*)m_ptr setStepRate:stepRate];
#endif
#endif
    }

    AttributeDescriptor::AttributeDescriptor() :
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        ns::Object<MTLAttributeDescriptor*>([[MTLAttributeDescriptor alloc] init], ns::Ownership::Assign)
#else
        ns::Object<MTLAttributeDescriptor*>(nullptr)
#endif
    {
    }

    AttributeFormat AttributeDescriptor::GetFormat() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return AttributeFormat(m_table->format(m_ptr));
#else
        return AttributeFormat([(MTLAttributeDescriptor*)m_ptr format]);
#endif
#else
        return AttributeFormat(0);
#endif
    }

    NSUInteger AttributeDescriptor::GetOffset() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(m_table->offset(m_ptr));
#else
        return NSUInteger([(MTLAttributeDescriptor*)m_ptr offset]);
#endif
#else
        return 0;
#endif
    }

    NSUInteger AttributeDescriptor::GetBufferIndex() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(m_table->bufferIndex(m_ptr));
#else
        return NSUInteger([(MTLAttributeDescriptor*)m_ptr bufferIndex]);
#endif
#else
        return 0;
#endif
    }

    void AttributeDescriptor::SetFormat(AttributeFormat format)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setFormat(m_ptr, MTLAttributeFormat(format));
#else
        [(MTLAttributeDescriptor*)m_ptr setFormat:MTLAttributeFormat(format)];
#endif
#endif
    }

    void AttributeDescriptor::SetOffset(NSUInteger offset)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setOffset(m_ptr, offset);
#else
        [(MTLAttributeDescriptor*)m_ptr setOffset:offset];
#endif
#endif
    }

    void AttributeDescriptor::SetBufferIndex(NSUInteger bufferIndex)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setBufferIndex(m_ptr, bufferIndex);
#else
        [(MTLAttributeDescriptor*)m_ptr setBufferIndex:bufferIndex];
#endif
#endif
    }

    StageInputOutputDescriptor::StageInputOutputDescriptor() :
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        ns::Object<MTLStageInputOutputDescriptor*>([[MTLStageInputOutputDescriptor alloc] init], ns::Ownership::Assign)
#else
        ns::Object<MTLStageInputOutputDescriptor*>(nullptr)
#endif
    {
    }

    ns::AutoReleased<ns::Array<BufferLayoutDescriptor>> StageInputOutputDescriptor::GetLayouts() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::Array<BufferLayoutDescriptor>>((NSArray<MTLBufferLayoutDescriptor*>*)m_table->layouts(m_ptr));
#else
        return ns::AutoReleased<ns::Array<BufferLayoutDescriptor>>((NSArray*)[(MTLStageInputOutputDescriptor*)m_ptr layouts]);
#endif
#else
        return nullptr;
#endif
    }

    ns::AutoReleased<ns::Array<AttributeDescriptor>> StageInputOutputDescriptor::GetAttributes() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::Array<AttributeDescriptor>>((NSArray<MTLAttributeDescriptor*>*)m_table->attributes(m_ptr));
#else
        return ns::AutoReleased<ns::Array<AttributeDescriptor>>((NSArray*)[(MTLStageInputOutputDescriptor*)m_ptr attributes]);
#endif
#else
        return nullptr;
#endif
    }

    IndexType StageInputOutputDescriptor::GetIndexType() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return IndexType(m_table->indexType(m_ptr));
#else
        return IndexType([(MTLStageInputOutputDescriptor*)m_ptr indexType]);
#endif
#else
        return IndexType(0);
#endif
    }

    NSUInteger StageInputOutputDescriptor::GetIndexBufferIndex() const
   {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
	   return m_table->indexBufferIndex(m_ptr);
#else
        return NSUInteger([(MTLStageInputOutputDescriptor*)m_ptr indexBufferIndex]);
#endif
#else
        return 0;
#endif
    }

   void StageInputOutputDescriptor::SetIndexType(IndexType indexType)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setIndexType(m_ptr, MTLIndexType(indexType));
#else
        [(MTLStageInputOutputDescriptor*)m_ptr setIndexType:MTLIndexType(indexType)];
#endif
#endif
    }

    void StageInputOutputDescriptor::SetIndexBufferIndex(NSUInteger indexBufferIndex)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setIndexBufferIndex(m_ptr, indexBufferIndex);
#else
        [(MTLStageInputOutputDescriptor*)m_ptr setIndexBufferIndex:indexBufferIndex];
#endif
#endif
    }

    void StageInputOutputDescriptor::Reset()
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->reset(m_ptr);
#else
        [(MTLStageInputOutputDescriptor*)m_ptr reset];
#endif
#endif
    }
}


MTLPP_END
