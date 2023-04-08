/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLComputePipeline.h>
#include "compute_pipeline.hpp"

MTLPP_BEGIN

namespace mtlpp
{
   ComputePipelineReflection::ComputePipelineReflection() :
	ns::Object<MTLComputePipelineReflection*>([[MTLComputePipelineReflection alloc] init], ns::Ownership::Assign)
	{
	}
	
	ns::AutoReleased<ns::Array<Argument>> ComputePipelineReflection::GetArguments() const
	{
		Validate();
		return ns::AutoReleased<ns::Array<Argument>>([m_ptr arguments]);
	}

    ComputePipelineDescriptor::ComputePipelineDescriptor() :
        ns::Object<MTLComputePipelineDescriptor*>([[MTLComputePipelineDescriptor alloc] init], ns::Ownership::Assign)
    {
    }

    ns::AutoReleased<ns::String> ComputePipelineDescriptor::GetLabel() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<ns::String>(m_table->label(m_ptr));
#else
        return ns::AutoReleased<ns::String>([(MTLComputePipelineDescriptor*)m_ptr label]);
#endif
    }

    ns::AutoReleased<Function> ComputePipelineDescriptor::GetComputeFunction() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<Function>(m_table->computeFunction(m_ptr));
#else
        return ns::AutoReleased<Function>([(MTLComputePipelineDescriptor*)m_ptr computeFunction]);
#endif
    }

    bool ComputePipelineDescriptor::GetThreadGroupSizeIsMultipleOfThreadExecutionWidth() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->threadGroupSizeIsMultipleOfThreadExecutionWidth(m_ptr);
#else
        return [(MTLComputePipelineDescriptor*)m_ptr threadGroupSizeIsMultipleOfThreadExecutionWidth];
#endif
    }

    ns::AutoReleased<StageInputOutputDescriptor> ComputePipelineDescriptor::GetStageInputDescriptor() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<StageInputOutputDescriptor>(m_table->stageInputDescriptor(m_ptr));
#else
        return ns::AutoReleased<StageInputOutputDescriptor>([(MTLComputePipelineDescriptor*)m_ptr stageInputDescriptor]);
#endif
#else
        return ns::AutoReleased<StageInputOutputDescriptor>();
#endif
    }
	
	ns::AutoReleased<ns::Array<PipelineBufferDescriptor>> ComputePipelineDescriptor::GetBuffers() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::Array<PipelineBufferDescriptor>>(ns::Array<PipelineBufferDescriptor>((NSArray<MTLPipelineBufferDescriptor*>*)m_table->buffers(m_ptr)));
#else
		return ns::AutoReleased<ns::Array<PipelineBufferDescriptor>>(ns::Array<PipelineBufferDescriptor>((NSArray*)[(MTLComputePipelineDescriptor*)m_ptr buffers]));
#endif
#else
		return ns::AutoReleased<ns::Array<PipelineBufferDescriptor>>();
#endif
	}
	
	NSUInteger ComputePipelineDescriptor::GetMaxTotalThreadsPerThreadgroup() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_14, 12_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->maxTotalThreadsPerThreadgroup(m_ptr);
#else
		return [(MTLComputePipelineDescriptor*)m_ptr maxTotalThreadsPerThreadgroup];
#endif
#else
		return 0;
#endif
	}
	
	void ComputePipelineDescriptor::SetMaxTotalThreadsPerThreadgroup(NSUInteger ThreadCount)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_14, 12_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->setMaxTotalThreadsPerThreadgroup(m_ptr, ThreadCount);
#else
		return [(MTLComputePipelineDescriptor*)m_ptr setMaxTotalThreadsPerThreadgroup:ThreadCount];
#endif
#endif
	}

    void ComputePipelineDescriptor::SetLabel(const ns::String& label)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setLabel(m_ptr, (NSString*)label.GetPtr());
#else
        [(MTLComputePipelineDescriptor*)m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
    }

    void ComputePipelineDescriptor::SetComputeFunction(const Function& function)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setComputeFunction(m_ptr, (id<MTLFunction>)function.GetPtr());
#else
        [(MTLComputePipelineDescriptor*)m_ptr setComputeFunction:(id<MTLFunction>)function.GetPtr()];
#endif
    }

    void ComputePipelineDescriptor::SetThreadGroupSizeIsMultipleOfThreadExecutionWidth(bool value)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setThreadGroupSizeIsMultipleOfThreadExecutionWidth(m_ptr, value);
#else
        [(MTLComputePipelineDescriptor*)m_ptr setThreadGroupSizeIsMultipleOfThreadExecutionWidth:value];
#endif
    }

    void ComputePipelineDescriptor::SetStageInputDescriptor(const StageInputOutputDescriptor& stageInputDescriptor) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setStageInputDescriptor(m_ptr, (MTLStageInputOutputDescriptor*)stageInputDescriptor.GetPtr());
#else
        [(MTLComputePipelineDescriptor*)m_ptr setStageInputDescriptor:(MTLStageInputOutputDescriptor*)stageInputDescriptor.GetPtr()];
#endif
#endif
    }
	
	void ComputePipelineDescriptor::Reset()
	{
		Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->reset(m_ptr);
#else
		[(MTLComputePipelineDescriptor*)m_ptr reset];
#endif
	}
	
	ns::AutoReleased<ns::String> ComputePipelineState::GetLabel() const
	{
		Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::String>(m_table->Label(m_ptr));
#else
		return ns::AutoReleased<ns::String>([(id<MTLComputePipelineState>)m_ptr label]);
#endif
	}

    ns::AutoReleased<Device> ComputePipelineState::GetDevice() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<Device>(m_table->Device(m_ptr));
#else
        return ns::AutoReleased<Device>([(id<MTLComputePipelineState>)m_ptr device]);
#endif
    }

    NSUInteger ComputePipelineState::GetMaxTotalThreadsPerThreadgroup() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->maxTotalThreadsPerThreadgroup(m_ptr);
#else
        return NSUInteger([(id<MTLComputePipelineState>)m_ptr maxTotalThreadsPerThreadgroup]);
#endif
    }

    NSUInteger ComputePipelineState::GetThreadExecutionWidth() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->threadExecutionWidth(m_ptr);
#else
        return NSUInteger([(id<MTLComputePipelineState>)m_ptr threadExecutionWidth]);
#endif
    }
	
	NSUInteger ComputePipelineState::GetStaticThreadgroupMemoryLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->staticThreadgroupMemoryLength(m_ptr);
#else
		return NSUInteger([(id<MTLComputePipelineState>)m_ptr staticThreadgroupMemoryLength]);
#endif
#else
		return 0;
#endif
	}
	
	NSUInteger ComputePipelineState::GetImageblockMemoryLengthForDimensions(Size const& imageblockDimensions)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->imageblockMemoryLengthForDimensions(m_ptr, imageblockDimensions);
#else
		return NSUInteger([(id<MTLComputePipelineState>)m_ptr imageblockMemoryLengthForDimensions:MTLSizeMake(imageblockDimensions.width, imageblockDimensions.height, imageblockDimensions.depth)]);
#endif
#else
		return 0;
#endif
	}
}

MTLPP_END
