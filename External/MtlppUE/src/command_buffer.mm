/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma clang visibility push(default)
#import <Foundation/NSDictionary.h>
#import <Foundation/NSError.h>
#pragma clang visibility pop

#include <Metal/MTLCommandBuffer.h>
#include <Metal/MTLRenderPass.h>
#include <Metal/MTLSampler.h>
#include <Metal/MTLStageInputOutputDescriptor.h>

#include "command_buffer.hpp"
#include "command_queue.hpp"
#include "drawable.hpp"
#include "blit_command_encoder.hpp"
#include "render_command_encoder.hpp"
#include "compute_command_encoder.hpp"
#include "parallel_render_command_encoder.hpp"
#include "render_pass.hpp"

MTLPP_BEGIN


NS_AVAILABLE(10_11, 8_0)
@protocol MTLCommandBufferAvailability <NSObject>
@property (readonly) CFTimeInterval kernelStartTime NS_AVAILABLE(10_13, 10_3);
@property (readonly) CFTimeInterval kernelEndTime NS_AVAILABLE(10_13, 10_3);
@property (readonly) CFTimeInterval GPUStartTime NS_AVAILABLE(10_13, 10_3);
@property (readonly) CFTimeInterval GPUEndTime NS_AVAILABLE(10_13, 10_3);
@end


namespace mtlpp
{
	CommandBuffer::CommandBuffer(ns::Ownership const retain)
	: ns::Object<ns::Protocol<id<MTLCommandBuffer>>::type>(retain)
	{
		
	}
	
	CommandBuffer::CommandBuffer(ns::Protocol<id<MTLCommandBuffer>>::type handle, UE::ITableCache* cache)
	: ns::Object<ns::Protocol<id<MTLCommandBuffer>>::type>(handle, ns::Ownership::Retain, UE::ITableCacheRef(cache).GetCommandBuffer(handle))
	{
	}
	
	CommandBuffer::CommandBuffer(const CommandBuffer& rhs)
	: ns::Object<ns::Protocol<id<MTLCommandBuffer>>::type>(rhs)
	, CmdBufferFence(rhs.CmdBufferFence)
	{
		
	}
	CommandBuffer& CommandBuffer::operator=(const CommandBuffer& rhs)
	{
		if (this != &rhs)
		{
			ns::Object<ns::Protocol<id<MTLCommandBuffer>>::type>::operator=(rhs);
			CmdBufferFence = rhs.CmdBufferFence;
		}
		return *this;
	}
#if MTLPP_CONFIG_RVALUE_REFERENCES
	CommandBuffer::CommandBuffer(CommandBuffer&& rhs)
	: ns::Object<ns::Protocol<id<MTLCommandBuffer>>::type>((ns::Object<ns::Protocol<id<MTLCommandBuffer>>::type>&&)rhs)
	, CmdBufferFence(rhs.CmdBufferFence)
	{
		
	}
	CommandBuffer& CommandBuffer::operator=(CommandBuffer&& rhs)
	{
		ns::Object<ns::Protocol<id<MTLCommandBuffer>>::type>::operator=((ns::Object<ns::Protocol<id<MTLCommandBuffer>>::type>&&)rhs);
		CmdBufferFence = rhs.CmdBufferFence;
		return *this;
	}
#endif
	CommandBufferFence& CommandBuffer::GetCompletionFence(void)
	{
		if(CmdBufferFence.GetPtr() == nullptr)
		{
			CmdBufferFence.Init();
			InsertCompletionFence(CmdBufferFence);
		}
		assert(CmdBufferFence.GetPtr() != nullptr);
		return CmdBufferFence;
	}
	
	void CommandBuffer::InsertCompletionFence(CommandBufferFence& Fence)
	{
		Fence.Insert(*this);
	}
	
	ns::AutoReleased<Device> CommandBuffer::GetDevice() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<Device>(m_table->Device(m_ptr));
#else
        return ns::AutoReleased<Device>([(id<MTLCommandBuffer>)m_ptr device]);
#endif
    }

    ns::AutoReleased<CommandQueue> CommandBuffer::GetCommandQueue() const
    {
#if MTLPP_CONFIG_IMP_CACHE
        Validate();
		id<MTLCommandQueue> handle = m_table->CommandQueue(m_ptr);
		return ns::AutoReleased<CommandQueue>(handle, m_table->TableCache->GetCommandQueue(handle));
#else
        return ns::AutoReleased<CommandQueue>([(id<MTLCommandBuffer>)m_ptr commandQueue]);
#endif
    }

    bool CommandBuffer::GetRetainedReferences() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->RetainedReferences(m_ptr);
#else
        return [(id<MTLCommandBuffer>)m_ptr retainedReferences];
#endif
    }

    ns::AutoReleased<ns::String> CommandBuffer::GetLabel() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::String>(m_table->Label(m_ptr));
#else
        return ns::AutoReleased<ns::String>([(id<MTLCommandBuffer>)m_ptr label]);
#endif
    }

    CommandBufferStatus CommandBuffer::GetStatus() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return (CommandBufferStatus)m_table->Status(m_ptr);
#else
        return CommandBufferStatus([(id<MTLCommandBuffer>)m_ptr status]);
#endif
    }

    ns::AutoReleased<ns::Error> CommandBuffer::GetError() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::Error>(m_table->Error(m_ptr));
#else
        return ns::AutoReleased<ns::Error>([(id<MTLCommandBuffer>)m_ptr error]);
#endif
    }

    double CommandBuffer::GetKernelStartTime() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_3)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->KernelStartTime(m_ptr);
#else
		if(@available(macOS 10.13, iOS 10.3, *))
			return [(id<MTLCommandBufferAvailability>)m_ptr kernelStartTime];
		else
			return 0.0;
#endif
#else
		return 0.0;
#endif
	}

    double CommandBuffer::GetKernelEndTime() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_3)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->KernelEndTime(m_ptr);
#else
		if(@available(macOS 10.13, iOS 10.3, *))
			return [(id<MTLCommandBufferAvailability>)m_ptr kernelEndTime];
		else
			return 0.0;
#endif
#else
        return 0.0;
#endif
    }

    double CommandBuffer::GetGpuStartTime() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_3)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->GPUStartTime(m_ptr);
#else
		if(@available(macOS 10.13, iOS 10.3, *))
			return [(id<MTLCommandBufferAvailability>)m_ptr GPUStartTime];
		else
			return 0.0;
#endif
#else
        return 0.0;
#endif
    }

    double CommandBuffer::GetGpuEndTime() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_3)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->GPUEndTime(m_ptr);
#else
		if(@available(macOS 10.13, iOS 10.3, *))
			return [(id<MTLCommandBufferAvailability>)m_ptr GPUEndTime];
		else
			return 0.0;
#endif
#else
        return 0.0;
#endif
    }

    void CommandBuffer::SetLabel(const ns::String& label)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetLabel(m_ptr, (NSString*)label.GetPtr());
#else
        [(id<MTLCommandBuffer>)m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
    }

    void CommandBuffer::Enqueue()
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Enqueue(m_ptr);
#else
        [(id<MTLCommandBuffer>)m_ptr enqueue];
#endif
    }

    void CommandBuffer::Commit()
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Commit(m_ptr);
#else
        [(id<MTLCommandBuffer>)m_ptr commit];
#endif
    }

    void CommandBuffer::AddScheduledHandler(CommandBufferHandler handler)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		UE::ITableCache* cache = m_table->TableCache;
		m_table->AddScheduledHandler(m_ptr, ^(id <MTLCommandBuffer> mtlCommandBuffer){
			CommandBuffer commandBuffer(mtlCommandBuffer, cache);
			handler(commandBuffer);
		});
#else
        [(id<MTLCommandBuffer>)m_ptr addScheduledHandler:^(id <MTLCommandBuffer> mtlCommandBuffer){
            CommandBuffer commandBuffer(mtlCommandBuffer);
            handler(commandBuffer);
        }];
#endif
    }

    void CommandBuffer::AddCompletedHandler(CommandBufferHandler handler)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		UE::ITableCache* cache = m_table->TableCache;
		m_table->AddCompletedHandler(m_ptr, ^(id <MTLCommandBuffer> mtlCommandBuffer){
			CommandBuffer commandBuffer(mtlCommandBuffer, cache);
			handler(commandBuffer);
		});
#else
        [(id<MTLCommandBuffer>)m_ptr addCompletedHandler:^(id <MTLCommandBuffer> mtlCommandBuffer){
            CommandBuffer commandBuffer(mtlCommandBuffer);
            handler(commandBuffer);
        }];
#endif
    }

    void CommandBuffer::Present(const Drawable& drawable)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->PresentDrawable(m_ptr, (id<MTLDrawable>)drawable.GetPtr());
#else
        [(id<MTLCommandBuffer>)m_ptr presentDrawable:(id<MTLDrawable>)drawable.GetPtr()];
#endif
    }

    void CommandBuffer::PresentAtTime(const Drawable& drawable, double presentationTime)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->PresentDrawableAtTime(m_ptr, (id<MTLDrawable>)drawable.GetPtr(), presentationTime);
#else
        [(id<MTLCommandBuffer>)m_ptr presentDrawable:(id<MTLDrawable>)drawable.GetPtr() atTime:presentationTime];
#endif
    }

    void CommandBuffer::PresentAfterMinimumDuration(const Drawable& drawable, double duration)
    {
        Validate();
#if MTLPP_PLATFORM_IOS
        [(id<MTLCommandBuffer>)m_ptr presentDrawable:(id<MTLDrawable>)drawable.GetPtr() afterMinimumDuration:duration];
#endif
    }

    void CommandBuffer::WaitUntilScheduled()
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->WaitUntilScheduled(m_ptr);
#else
        [(id<MTLCommandBuffer>)m_ptr waitUntilScheduled];
#endif
    }

    void CommandBuffer::WaitUntilCompleted()
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->WaitUntilCompleted(m_ptr);
#else
        [(id<MTLCommandBuffer>)m_ptr waitUntilCompleted];
#endif
    }

    BlitCommandEncoder CommandBuffer::BlitCommandEncoder()
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		class BlitCommandEncoder Encoder = mtlpp::BlitCommandEncoder(m_table->BlitCommandEncoder(m_ptr), m_table->TableCache);
#else
        class BlitCommandEncoder Encoder = [(id<MTLCommandBuffer>)m_ptr blitCommandEncoder];
#endif
#if MTLPP_CONFIG_VALIDATE
		Encoder.SetCommandBufferFence(GetCompletionFence());
#endif
		return Encoder;
    }

    RenderCommandEncoder CommandBuffer::RenderCommandEncoder(const RenderPassDescriptor& renderPassDescriptor)
    {
        Validate();
		MTLRenderPassDescriptor* mtlRenderPassDescriptor = (MTLRenderPassDescriptor*)renderPassDescriptor.GetPtr();
#if MTLPP_CONFIG_IMP_CACHE
		class RenderCommandEncoder Encoder = mtlpp::RenderCommandEncoder(m_table->RenderCommandEncoderWithDescriptor(m_ptr, mtlRenderPassDescriptor), m_table->TableCache);
#else
        class RenderCommandEncoder Encoder = [(id<MTLCommandBuffer>)m_ptr renderCommandEncoderWithDescriptor:mtlRenderPassDescriptor];
#endif
#if MTLPP_CONFIG_VALIDATE
		Encoder.SetCommandBufferFence(GetCompletionFence());
#endif
		return Encoder;
    }

    ComputeCommandEncoder CommandBuffer::ComputeCommandEncoder()
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		class ComputeCommandEncoder Encoder = mtlpp::ComputeCommandEncoder(m_table->ComputeCommandEncoder(m_ptr), m_table->TableCache);
#else
        class ComputeCommandEncoder Encoder = [(id<MTLCommandBuffer>)m_ptr computeCommandEncoder];
#endif
#if MTLPP_CONFIG_VALIDATE
		Encoder.SetCommandBufferFence(GetCompletionFence());
#endif
		return Encoder;
    }
	
	
	ComputeCommandEncoder CommandBuffer::ComputeCommandEncoder(DispatchType Type)
	{
		Validate();
		class ComputeCommandEncoder Encoder;
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		Encoder = mtlpp::ComputeCommandEncoder(m_table->ComputeCommandEncoderWithType(m_ptr, (MTLDispatchType)Type), m_table->TableCache);
#else
		Encoder = mtlpp::ComputeCommandEncoder([(id<MTLCommandBuffer>)m_ptr computeCommandEncoderWithDispatchType:(MTLDispatchType)Type], m_table->TableCache);
#endif
#if MTLPP_CONFIG_VALIDATE
		Encoder.SetCommandBufferFence(GetCompletionFence());
#endif
#endif
		return Encoder;
	}

    ParallelRenderCommandEncoder CommandBuffer::ParallelRenderCommandEncoder(const RenderPassDescriptor& renderPassDescriptor)
    {
        Validate();
		MTLRenderPassDescriptor* mtlRenderPassDescriptor = (MTLRenderPassDescriptor*)renderPassDescriptor.GetPtr();
#if MTLPP_CONFIG_IMP_CACHE
		class ParallelRenderCommandEncoder Encoder = mtlpp::ParallelRenderCommandEncoder(m_table->ParallelRenderCommandEncoderWithDescriptor(m_ptr, mtlRenderPassDescriptor), m_table->TableCache);
#else
        class ParallelRenderCommandEncoder Encoder = [(id<MTLCommandBuffer>)m_ptr parallelRenderCommandEncoderWithDescriptor:mtlRenderPassDescriptor];
#endif
#if MTLPP_CONFIG_VALIDATE
		Encoder.SetCommandBufferFence(GetCompletionFence());
#endif
		return Encoder;
    }
	
	void CommandBuffer::PushDebugGroup(const ns::String& string)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->PushDebugGroup(m_ptr, string.GetPtr());
#else
		[(id<MTLCommandBuffer>)m_ptr pushDebugGroup:(NSString *)string.GetPtr()];
#endif
#endif
	}
	
	void CommandBuffer::PopDebugGroup()
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->PopDebugGroup(m_ptr);
#else
		[(id<MTLCommandBuffer>)m_ptr popDebugGroup];
#endif
#endif
	}
	
#if MTLPP_CONFIG_VALIDATE
	BlitCommandEncoder ValidatedCommandBuffer::BlitCommandEncoder()
	{
		class BlitCommandEncoder Enc = CommandBuffer::BlitCommandEncoder();
		Validator.AddEncoderValidator(Enc);
		return Enc;
	}
	
	RenderCommandEncoder ValidatedCommandBuffer::RenderCommandEncoder(const RenderPassDescriptor& renderPassDescriptor)
	{
		class RenderCommandEncoder Enc = CommandBuffer::RenderCommandEncoder(renderPassDescriptor);
		CommandEncoderValidationTable RenderValidator = Validator.AddEncoderValidator(Enc);
		
		RenderPassDepthAttachmentDescriptor Depth = renderPassDescriptor.GetDepthAttachment();
		if (Depth)
		{
			if (Depth.GetTexture())
			{
				RenderValidator.UseResource(Depth.GetTexture(), mtlpp::ResourceUsage::Read);
			}
			if (Depth.GetResolveTexture())
			{
				RenderValidator.UseResource(Depth.GetResolveTexture(), mtlpp::ResourceUsage::Write);
			}
		}
		
		RenderPassStencilAttachmentDescriptor Stencil = renderPassDescriptor.GetStencilAttachment();
		if (Stencil)
		{
			if (Stencil.GetTexture())
			{
				RenderValidator.UseResource(Stencil.GetTexture(), mtlpp::ResourceUsage::Read);
			}
			if (Stencil.GetResolveTexture())
			{
				RenderValidator.UseResource(Stencil.GetResolveTexture(), mtlpp::ResourceUsage::Write);
			}
		}
		
		ns::Array<RenderPassColorAttachmentDescriptor> ColorAttachments = renderPassDescriptor.GetColorAttachments();
		for (NSUInteger i = 0; i < 8; i++)
		{
			RenderPassColorAttachmentDescriptor ColorAttachment = ColorAttachments[i];
			if (ColorAttachment)
			{
				if (ColorAttachment.GetTexture())
				{
					RenderValidator.UseResource(ColorAttachment.GetTexture(), mtlpp::ResourceUsage::Write);
				}
				if (ColorAttachment.GetResolveTexture())
				{
					RenderValidator.UseResource(ColorAttachment.GetResolveTexture(), mtlpp::ResourceUsage::Write);
				}
			}
		}
		
		if (renderPassDescriptor.GetVisibilityResultBuffer())
		{
			RenderValidator.UseResource(renderPassDescriptor.GetVisibilityResultBuffer(), mtlpp::ResourceUsage::Write);
		}
		
		return Enc;
	}
	
	ComputeCommandEncoder ValidatedCommandBuffer::ComputeCommandEncoder()
	{
		class ComputeCommandEncoder Enc = CommandBuffer::ComputeCommandEncoder();
		Validator.AddEncoderValidator(Enc);
		return Enc;
	}
	
	ParallelRenderCommandEncoder ValidatedCommandBuffer::ParallelRenderCommandEncoder(const RenderPassDescriptor& renderPassDescriptor)
	{
		class ParallelRenderCommandEncoder Enc = CommandBuffer::ParallelRenderCommandEncoder(renderPassDescriptor);
		CommandEncoderValidationTable RenderValidator = Validator.AddEncoderValidator(Enc);
		
		RenderPassDepthAttachmentDescriptor Depth = renderPassDescriptor.GetDepthAttachment();
		if (Depth)
		{
			if (Depth.GetTexture())
			{
				RenderValidator.UseResource(Depth.GetTexture(), mtlpp::ResourceUsage::Read);
			}
			if (Depth.GetResolveTexture())
			{
				RenderValidator.UseResource(Depth.GetResolveTexture(), mtlpp::ResourceUsage::Write);
			}
		}
		
		RenderPassStencilAttachmentDescriptor Stencil = renderPassDescriptor.GetStencilAttachment();
		if (Stencil)
		{
			if (Stencil.GetTexture())
			{
				RenderValidator.UseResource(Stencil.GetTexture(), mtlpp::ResourceUsage::Read);
			}
			if (Stencil.GetResolveTexture())
			{
				RenderValidator.UseResource(Stencil.GetResolveTexture(), mtlpp::ResourceUsage::Write);
			}
		}
		
		ns::Array<RenderPassColorAttachmentDescriptor> ColorAttachments = renderPassDescriptor.GetColorAttachments();
		for (NSUInteger i = 0; i < 8; i++)
		{
			RenderPassColorAttachmentDescriptor ColorAttachment = ColorAttachments[i];
			if (ColorAttachment)
			{
				if (ColorAttachment.GetTexture())
				{
					RenderValidator.UseResource(ColorAttachment.GetTexture(), mtlpp::ResourceUsage::Write);
				}
				if (ColorAttachment.GetResolveTexture())
				{
					RenderValidator.UseResource(ColorAttachment.GetResolveTexture(), mtlpp::ResourceUsage::Write);
				}
			}
		}
		
		if (renderPassDescriptor.GetVisibilityResultBuffer())
		{
			RenderValidator.UseResource(renderPassDescriptor.GetVisibilityResultBuffer(), mtlpp::ResourceUsage::Write);
		}
		return Enc;
	}
	
	void ValidatedCommandBuffer::Enqueue()
	{
		Validator.Enqueue(*this);
		CommandBuffer::Enqueue();
	}
	
	void ValidatedCommandBuffer::Commit()
	{
		Validator.Enqueue(*this);
		CommandBuffer::Commit();
	}
#endif
}

MTLPP_END
