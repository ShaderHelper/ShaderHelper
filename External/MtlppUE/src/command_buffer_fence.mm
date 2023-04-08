/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLCommandBuffer.h>
#include <Metal/MTLBuffer.h>
#include <Metal/MTLTexture.h>
#include <Metal/MTLRenderPass.h>
#include <Metal/MTLSampler.h>
#include <Metal/MTLStageInputOutputDescriptor.h>

#include "command_buffer_fence.hpp"
#include "command_buffer.hpp"
#include "command_encoder.hpp"
#include "buffer.hpp"
#include "texture.hpp"

MTLPP_BEGIN

@interface CommandBufferFenceImpl : NSObject
{
@public
	ns::Condition Condition;
	mtlpp::CommandBuffer::Type CmdBuffer;
	uint32_t bPassed;
}
@end

@implementation CommandBufferFenceImpl
-(instancetype)init
{
	id Self = [super init];
	if (Self)
	{
		Condition = ns::Condition([NSCondition new], ns::Ownership::Assign);
		CmdBuffer = nullptr;
		bPassed = 0;
	}
	return Self;
}
@end

namespace mtlpp
{
	CommandBufferFence::CommandBufferFence()
	: ns::Object<CommandBufferFenceImpl*, ns::CallingConvention::ObjectiveC>(ns::Ownership::Assign)
	{
	}
	
	CommandBufferFence::~CommandBufferFence()
	{
	}
		
	CommandBufferFence::CommandBufferFence(const CommandBufferFence& rhs)
	: ns::Object<CommandBufferFenceImpl*, ns::CallingConvention::ObjectiveC>(rhs)
	{
	}
	
#if MTLPP_CONFIG_RVALUE_REFERENCES
	CommandBufferFence::CommandBufferFence(CommandBufferFence&& rhs)
	: ns::Object<CommandBufferFenceImpl*, ns::CallingConvention::ObjectiveC>((ns::Object<CommandBufferFenceImpl*, ns::CallingConvention::ObjectiveC>&&)rhs)
	{
	}
#endif
	
	CommandBufferFence& CommandBufferFence::operator=(const CommandBufferFence& rhs)
	{
		if (this != &rhs)
		{
			ns::Object<CommandBufferFenceImpl*, ns::CallingConvention::ObjectiveC>::operator=(rhs);
		}
		return *this;
	}
	
#if MTLPP_CONFIG_RVALUE_REFERENCES
	CommandBufferFence& CommandBufferFence::operator=(CommandBufferFence&& rhs)
	{
		ns::Object<CommandBufferFenceImpl*, ns::CallingConvention::ObjectiveC>::operator=((ns::Object<CommandBufferFenceImpl*, ns::CallingConvention::ObjectiveC>&&)rhs);
		return *this;
	}
#endif
	
	void CommandBufferFence::Insert(CommandBuffer& CommandBuffer)
	{
		assert(CommandBuffer.GetPtr());
		assert(m_ptr->bPassed == false);
		assert(m_ptr->CmdBuffer == nil);
		m_ptr->CmdBuffer = CommandBuffer.GetPtr();
		
		CommandBufferFenceImpl* Ptr = m_ptr;
		[CommandBuffer.GetPtr() addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull Buffer) {
			assert(Buffer);
		
			NSCondition* Condition = Ptr->Condition.GetPtr();
			uint32_t* Passed = &Ptr->bPassed;
			id<MTLCommandBuffer> CmdBuf = Ptr->CmdBuffer;
			
			assert(*Passed == false);
			assert(CmdBuf == Buffer);
			__sync_fetch_and_add(Passed, 1);
			[Condition lock];
			[Condition broadcast];
			[Condition unlock];
		}];
	}
	
	void CommandBufferFence::Init()
	{
		assert(!m_ptr);
		m_ptr = [[CommandBufferFenceImpl alloc] init];
	}
	
	bool CommandBufferFence::Wait(NSUInteger TimeInterval) const
	{
		if(m_ptr && m_ptr->CmdBuffer != nil)
		{
			NSCondition* Condition = m_ptr->Condition.GetPtr();
			[Condition lock];
			__sync_synchronize();
			uint32_t bFinished = m_ptr->bPassed;
			if(!bFinished)
			{
				@autoreleasepool
				{
					
					switch (TimeInterval)
					{
						case NSUIntegerMax:
						{
							[Condition wait];
							bFinished = true;
							break;
						}
						case 0:
						{
							bFinished = [Condition waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.0]];
							break;
						}
						default:
						{
							bFinished = [Condition waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:TimeInterval / 1000.0]];
							break;
						}
					}
				}
				__sync_fetch_and_add(&m_ptr->bPassed, bFinished);
			}
			[Condition unlock];
			
			__sync_synchronize();
			return (m_ptr->bPassed != 0);
		}
		else
		{
			return true;
		}
	}
}

MTLPP_END
