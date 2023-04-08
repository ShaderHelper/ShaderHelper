/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLCommandQueue.h>
#include "command_queue.hpp"
#include "command_buffer.hpp"
#include "device.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    ns::AutoReleased<ns::String> CommandQueue::GetLabel() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::String>(m_table->Label(m_ptr));
#else
        return ns::AutoReleased<ns::String>([(id<MTLCommandQueue>)m_ptr label]);
#endif
    }

    ns::AutoReleased<Device> CommandQueue::GetDevice() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<Device>(m_table->Device(m_ptr));
#else
        return ns::AutoReleased<Device>([(id<MTLCommandQueue>)m_ptr device]);
#endif
    }

    void CommandQueue::SetLabel(const ns::String& label)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetLabel(m_ptr, label.GetPtr());
#else
        [(id<MTLCommandQueue>)m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
    }

    CommandBuffer CommandQueue::CommandBufferWithUnretainedReferences()
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return mtlpp::CommandBuffer(m_table->CommandBufferWithUnretainedReferences(m_ptr), m_table->TableCache);
#else
        return [(id<MTLCommandQueue>)m_ptr commandBufferWithUnretainedReferences];
#endif
    }

    CommandBuffer CommandQueue::CommandBuffer()
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return mtlpp::CommandBuffer(m_table->CommandBuffer(m_ptr), m_table->TableCache);
#else
        return [(id<MTLCommandQueue>)m_ptr commandBuffer];
#endif
    }

    void CommandQueue::InsertDebugCaptureBoundary()
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->InsertDebugCaptureBoundary(m_ptr);
#else
        [(id<MTLCommandQueue>)m_ptr insertDebugCaptureBoundary];
#endif
    }
	
#if MTLPP_CONFIG_VALIDATE
	class CommandBuffer ValidatedCommandQueue::CommandBufferWithUnretainedReferences()
	{
		class CommandBuffer Buf = CommandQueue::CommandBufferWithUnretainedReferences();
		CommandBufferValidationTable Validator(Buf);
		return Buf;
	}
	
	class CommandBuffer ValidatedCommandQueue::CommandBuffer()
	{
		class CommandBuffer Buf = CommandQueue::CommandBuffer();
		CommandBufferValidationTable Validator(Buf);
		return Buf;
	}
#endif
}

MTLPP_END
