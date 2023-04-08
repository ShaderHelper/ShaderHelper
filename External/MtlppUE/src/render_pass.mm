/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLRenderPass.h>
#include <Metal/MTLDepthStencil.h>
#include "render_pass.hpp"
#include "texture.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	RenderPassColorAttachmentDescriptor::RenderPassColorAttachmentDescriptor(ns::Ownership const retain) :
	RenderPassAttachmentDescriptor(nil, retain)
	{
	}
	
	RenderPassColorAttachmentDescriptor::RenderPassColorAttachmentDescriptor() :
        RenderPassAttachmentDescriptor([[MTLRenderPassColorAttachmentDescriptor alloc] init], ns::Ownership::Assign)
    {
    }

    ClearColor RenderPassColorAttachmentDescriptor::GetClearColor() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        MTLPPClearColor mtlClearColor = m_table->clearColor(m_ptr);
#else
        MTLClearColor mtlClearColor = [(MTLRenderPassColorAttachmentDescriptor*)m_ptr clearColor];
#endif
        return ClearColor(mtlClearColor.red, mtlClearColor.green, mtlClearColor.blue, mtlClearColor.alpha);
    }

    void RenderPassColorAttachmentDescriptor::SetClearColor(const ClearColor& clearColor)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        MTLPPClearColor mtlClearColor = { clearColor.Red, clearColor.Green, clearColor.Blue, clearColor.Alpha };
		m_table->setClearColor(m_ptr, mtlClearColor);
#else
        MTLClearColor mtlClearColor = { clearColor.Red, clearColor.Green, clearColor.Blue, clearColor.Alpha };
        [(MTLRenderPassColorAttachmentDescriptor*)m_ptr setClearColor:mtlClearColor];
#endif
    }

    RenderPassDepthAttachmentDescriptor::RenderPassDepthAttachmentDescriptor(ns::Ownership const retain) :
        RenderPassAttachmentDescriptor(nil, retain)
    {
    }
	
	RenderPassDepthAttachmentDescriptor::RenderPassDepthAttachmentDescriptor() :
	RenderPassAttachmentDescriptor([[MTLRenderPassDepthAttachmentDescriptor alloc] init], ns::Ownership::Assign)
	{
	}

    double RenderPassDepthAttachmentDescriptor::GetClearDepth() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->clearDepth(m_ptr);
#else
        return [(MTLRenderPassDepthAttachmentDescriptor*)m_ptr clearDepth];
#endif
    }

    MultisampleDepthResolveFilter RenderPassDepthAttachmentDescriptor::GetDepthResolveFilter() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE_AX(9_0)
#if MTLPP_CONFIG_IMP_CACHE
        return MultisampleDepthResolveFilter(m_table->depthResolveFilter(m_ptr));
#else
        return MultisampleDepthResolveFilter([(MTLRenderPassDepthAttachmentDescriptor*)m_ptr depthResolveFilter]);
#endif
#else
        return MultisampleDepthResolveFilter(0);
#endif
    }

    void RenderPassDepthAttachmentDescriptor::SetClearDepth(double clearDepth)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setClearDepth(m_ptr, clearDepth);
#else
        [(MTLRenderPassDepthAttachmentDescriptor*)m_ptr setClearDepth:clearDepth];
#endif
    }

    void RenderPassDepthAttachmentDescriptor::SetDepthResolveFilter(MultisampleDepthResolveFilter depthResolveFilter)
    {
        Validate();
#if MTLPP_IS_AVAILABLE_AX(9_0)
        [(MTLRenderPassDepthAttachmentDescriptor*)m_ptr setDepthResolveFilter:MTLMultisampleDepthResolveFilter(depthResolveFilter)];
#endif
    }

    RenderPassStencilAttachmentDescriptor::RenderPassStencilAttachmentDescriptor(ns::Ownership const retain) :
        RenderPassAttachmentDescriptor(nil, retain)
    {
    }
	
	RenderPassStencilAttachmentDescriptor::RenderPassStencilAttachmentDescriptor() :
	RenderPassAttachmentDescriptor([[MTLRenderPassStencilAttachmentDescriptor alloc] init], ns::Ownership::Assign)
	{
	}

    NSUInteger RenderPassStencilAttachmentDescriptor::GetClearStencil() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->clearStencil(m_ptr);
#else
        return NSUInteger([(MTLRenderPassStencilAttachmentDescriptor*)m_ptr clearStencil]);
#endif
    }

    void RenderPassStencilAttachmentDescriptor::SetClearStencil(uint32_t clearStencil)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setClearStencil(m_ptr, clearStencil);
#else
        [(MTLRenderPassStencilAttachmentDescriptor*)m_ptr setClearStencil:clearStencil];
#endif
    }

    RenderPassDescriptor::RenderPassDescriptor() :
        ns::Object<MTLRenderPassDescriptor*>([[MTLRenderPassDescriptor alloc] init], ns::Ownership::Assign)
    {
    }

    ns::AutoReleased<ns::Array<RenderPassColorAttachmentDescriptor>> RenderPassDescriptor::GetColorAttachments() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::Array<RenderPassColorAttachmentDescriptor>>((NSArray<RenderPassColorAttachmentDescriptor::Type>*)m_table->colorAttachments(m_ptr));
#else
        return ns::AutoReleased<ns::Array<RenderPassColorAttachmentDescriptor>>((NSArray<RenderPassColorAttachmentDescriptor::Type>*)[(MTLRenderPassDescriptor*)m_ptr colorAttachments]);
#endif
    }

    ns::AutoReleased<RenderPassDepthAttachmentDescriptor> RenderPassDescriptor::GetDepthAttachment() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased< RenderPassDepthAttachmentDescriptor>(m_table->depthAttachment(m_ptr));
#else
        return ns::AutoReleased<RenderPassDepthAttachmentDescriptor>([(MTLRenderPassDescriptor*)m_ptr depthAttachment]);
#endif
    }

    ns::AutoReleased<RenderPassStencilAttachmentDescriptor> RenderPassDescriptor::GetStencilAttachment() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<RenderPassStencilAttachmentDescriptor>(m_table->stencilAttachment(m_ptr));
#else
        return ns::AutoReleased<RenderPassStencilAttachmentDescriptor>([(MTLRenderPassDescriptor*)m_ptr stencilAttachment]);
#endif
    }

    ns::AutoReleased<Buffer> RenderPassDescriptor::GetVisibilityResultBuffer() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<Buffer>(m_table->visibilityResultBuffer(m_ptr));
#else
        return ns::AutoReleased<Buffer>([(MTLRenderPassDescriptor*)m_ptr visibilityResultBuffer]);
#endif
    }

    NSUInteger RenderPassDescriptor::GetRenderTargetArrayLength() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->renderTargetArrayLength(m_ptr);
#else
        return NSUInteger([(MTLRenderPassDescriptor*)m_ptr renderTargetArrayLength]);
#endif
#else
        return 0;
#endif
    }

    void RenderPassDescriptor::SetDepthAttachment(const RenderPassDepthAttachmentDescriptor& depthAttachment)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setDepthAttachment(m_ptr, depthAttachment.GetPtr());
#else
        [(MTLRenderPassDescriptor*)m_ptr setDepthAttachment:(MTLRenderPassDepthAttachmentDescriptor*)depthAttachment.GetPtr()];
#endif
    }

    void RenderPassDescriptor::SetStencilAttachment(const RenderPassStencilAttachmentDescriptor& stencilAttachment)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setStencilAttachment(m_ptr, stencilAttachment.GetPtr());
#else
        [(MTLRenderPassDescriptor*)m_ptr setStencilAttachment:(MTLRenderPassStencilAttachmentDescriptor*)stencilAttachment.GetPtr()];
#endif
    }

    void RenderPassDescriptor::SetVisibilityResultBuffer(const Buffer& visibilityResultBuffer)
    {
        Validate();
		assert(visibilityResultBuffer.GetPtr() == nil || visibilityResultBuffer.GetParentBuffer().GetPtr() == nil /* We can't support sub-range buffers for visibility results without overcomplicating mtlpp so you can't use them here. */);
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setVisibilityResultBuffer(m_ptr, visibilityResultBuffer.GetPtr());
#else
        [(MTLRenderPassDescriptor*)m_ptr setVisibilityResultBuffer:(id<MTLBuffer>)visibilityResultBuffer.GetPtr()];
#endif
    }

    void RenderPassDescriptor::SetRenderTargetArrayLength(NSUInteger renderTargetArrayLength)
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setRenderTargetArrayLength(m_ptr, renderTargetArrayLength);
#else
        [(MTLRenderPassDescriptor*)m_ptr setRenderTargetArrayLength:renderTargetArrayLength];
#endif
#endif
    }
	
	void RenderPassDescriptor::SetSamplePositions(SamplePosition const* positions, NSUInteger count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setSamplePositionscount(m_ptr, (const MTLPPSamplePosition *)positions, count);
#else
		[(MTLRenderPassDescriptor*)m_ptr setSamplePositions:(const MTLSamplePosition *)positions count:count];
#endif
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetSamplePositions(SamplePosition* positions, NSUInteger count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->getSamplePositionscount(m_ptr, (MTLPPSamplePosition *)positions, count);
#else
		return [(MTLRenderPassDescriptor*)m_ptr getSamplePositions:(MTLSamplePosition *)positions count:count];
#endif
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetImageblockSampleLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr imageblockSampleLength];
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetThreadgroupMemoryLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr threadgroupMemoryLength];
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetTileWidth() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr tileWidth];
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetTileHeight() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr tileHeight];
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetDefaultRasterSampleCount() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr defaultRasterSampleCount];
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetRenderTargetWidth() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr renderTargetWidth];
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetRenderTargetHeight() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr renderTargetHeight];
#else
		return 0;
#endif
	}
	
	void RenderPassDescriptor::SetImageblockSampleLength(NSUInteger Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setImageblockSampleLength:Val];
#endif
	}
	
	void RenderPassDescriptor::SetThreadgroupMemoryLength(NSUInteger Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setThreadgroupMemoryLength:Val];
#endif
	}
	
	void RenderPassDescriptor::SetTileWidth(NSUInteger Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setTileWidth:Val];
#endif
	}
	
	void RenderPassDescriptor::SetTileHeight(NSUInteger Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setRenderTargetHeight:Val];
#endif
	}
	
	void RenderPassDescriptor::SetDefaultRasterSampleCount(NSUInteger Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setDefaultRasterSampleCount:Val];
#endif
	}
	
	void RenderPassDescriptor::SetRenderTargetWidth(NSUInteger Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setRenderTargetWidth:Val];
#endif
	}
	
	void RenderPassDescriptor::SetRenderTargetHeight(NSUInteger Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setRenderTargetHeight:Val];
#endif
	}
}

MTLPP_END
