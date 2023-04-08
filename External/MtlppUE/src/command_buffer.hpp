/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_CommandBuffer.hpp"
#include "ns.hpp"
#include "command_buffer_fence.hpp"
#include "validation.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	struct MTLPP_EXPORT ITable<id<MTLCommandBuffer>, void> : public IMPTable<id<MTLCommandBuffer>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLCommandBuffer>, void>(C)
		{
		}
	};
}

namespace mtlpp
{
    class Device;
    class CommandQueue;
    class BlitCommandEncoder;
    class RenderCommandEncoder;
    class ParallelRenderCommandEncoder;
    class ComputeCommandEncoder;
    class CommandQueue;
    class Drawable;
    class RenderPassDescriptor;
	class CommandBuffer;

    enum class CommandBufferStatus
    {
        NotEnqueued = 0,
        Enqueued    = 1,
        Committed   = 2,
        Scheduled   = 3,
        Completed   = 4,
        Error       = 5,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class CommandBufferError
    {
        None                                      = 0,
        Internal                                  = 1,
        Timeout                                   = 2,
        PageFault                                 = 3,
        Blacklisted                               = 4,
        NotPermitted                              = 7,
        OutOfMemory                               = 8,
        InvalidResource                           = 9,
        Memoryless      MTLPP_AVAILABLE_AX(10_0)  = 10,
		DeviceRemoved  MTLPP_AVAILABLE_MAC(10_13) = 11,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

	enum class DispatchType
	{
		Serial,
		Concurrent,
	}
	/* MTLPP_AVAILABLE(10_14, 12_0) */;
	
	MTLPP_CLOSURE(CommandBufferHandler, void, const CommandBuffer&);
	
    class MTLPP_EXPORT CommandBuffer : public ns::Object<ns::Protocol<id<MTLCommandBuffer>>::type>
    {
		mutable CommandBufferFence CmdBufferFence;
    public:
		CommandBuffer(ns::Ownership const retain = ns::Ownership::Retain);
		CommandBuffer(ns::Protocol<id<MTLCommandBuffer>>::type handle, UE::ITableCache* cache = nullptr);
		
		CommandBuffer(const CommandBuffer& rhs);
		CommandBuffer& operator=(const CommandBuffer& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
		CommandBuffer(CommandBuffer&& rhs);
		CommandBuffer& operator=(CommandBuffer&& rhs);
#endif
		CommandBufferFence& GetCompletionFence(void);
		void InsertCompletionFence(CommandBufferFence& Fence);

		operator ns::Protocol<id<MTLCommandBuffer>>::type() const = delete;
		
        ns::AutoReleased<Device>              GetDevice() const;
        ns::AutoReleased<CommandQueue>        GetCommandQueue() const;
        bool                GetRetainedReferences() const;
        ns::AutoReleased<ns::String>          GetLabel() const;
        CommandBufferStatus GetStatus() const;
        ns::AutoReleased<ns::Error>           GetError() const;
        double              GetKernelStartTime() const MTLPP_AVAILABLE(10_13, 10_3);
        double              GetKernelEndTime() const MTLPP_AVAILABLE(10_13, 10_3);
        double              GetGpuStartTime() const MTLPP_AVAILABLE(10_13, 10_3);
        double              GetGpuEndTime() const MTLPP_AVAILABLE(10_13, 10_3);

        void SetLabel(const ns::String& label);

        MTLPP_VALIDATED void Enqueue();
        MTLPP_VALIDATED void Commit();
        void AddScheduledHandler(CommandBufferHandler handler);
        void AddCompletedHandler(CommandBufferHandler handler);
        void Present(const Drawable& drawable);
        void PresentAtTime(const Drawable& drawable, double presentationTime);
        void PresentAfterMinimumDuration(const Drawable& drawable, double duration) MTLPP_AVAILABLE_AX(10_3);
        void WaitUntilScheduled();
        void WaitUntilCompleted();
        MTLPP_VALIDATED BlitCommandEncoder BlitCommandEncoder();
        MTLPP_VALIDATED RenderCommandEncoder RenderCommandEncoder(const RenderPassDescriptor& renderPassDescriptor);
        MTLPP_VALIDATED class ComputeCommandEncoder ComputeCommandEncoder();
		MTLPP_VALIDATED class ComputeCommandEncoder ComputeCommandEncoder(DispatchType Type) /* MTLPP_AVAILABLE(10_14, 12_0) */;
        MTLPP_VALIDATED ParallelRenderCommandEncoder ParallelRenderCommandEncoder(const RenderPassDescriptor& renderPassDescriptor);
		
		void PushDebugGroup(const ns::String& string) MTLPP_AVAILABLE(10_13, 11_0);
		void PopDebugGroup() MTLPP_AVAILABLE(10_13, 11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
	
#if MTLPP_CONFIG_VALIDATE
	class MTLPP_EXPORT ValidatedCommandBuffer : public ns::AutoReleased<CommandBuffer>
	{
		CommandBufferValidationTable Validator;
		
	public:
		ValidatedCommandBuffer()
		: Validator(nullptr)
		{
		}
		
		ValidatedCommandBuffer(CommandBuffer& Wrapped)
		: ns::AutoReleased<CommandBuffer>(Wrapped)
		, Validator(Wrapped.GetAssociatedObject<CommandBufferValidationTable>(CommandBufferValidationTable::kTableAssociationKey).GetPtr())
		{
		}
		
		MTLPP_VALIDATED class BlitCommandEncoder BlitCommandEncoder();
		MTLPP_VALIDATED class RenderCommandEncoder RenderCommandEncoder(const RenderPassDescriptor& renderPassDescriptor);
		MTLPP_VALIDATED class ComputeCommandEncoder ComputeCommandEncoder();
		MTLPP_VALIDATED class ParallelRenderCommandEncoder ParallelRenderCommandEncoder(const RenderPassDescriptor& renderPassDescriptor);
		
		MTLPP_VALIDATED void Enqueue();
		MTLPP_VALIDATED void Commit();
	};
	
	template <>
	class MTLPP_EXPORT Validator<CommandBuffer>
	{
		public:
		Validator(CommandBuffer& Val, bool bEnable)
		: Resource(Val)
		{
			if (bEnable)
			{
				Validation = ValidatedCommandBuffer(Val);
			}
		}
		
		ValidatedCommandBuffer& operator*()
		{
			assert(Validation.GetPtr() != nullptr);
			return Validation;
		}
		
		CommandBuffer* operator->()
		{
			return Validation.GetPtr() == nullptr ? &Resource : &Validation;
		}
		
		private:
		CommandBuffer& Resource;
		ValidatedCommandBuffer Validation;
	};
#endif
}

MTLPP_END
