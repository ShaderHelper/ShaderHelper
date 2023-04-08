/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLDepthStencil.h>
#include "depth_stencil.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    StencilDescriptor::StencilDescriptor() :
        ns::Object<MTLStencilDescriptor*>([[MTLStencilDescriptor alloc] init], ns::Ownership::Assign)
    {
    }

    CompareFunction StencilDescriptor::GetStencilCompareFunction() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return CompareFunction(m_table->stencilCompareFunction(m_ptr));
#else
        return CompareFunction([(MTLStencilDescriptor*)m_ptr stencilCompareFunction]);
#endif
    }

    StencilOperation StencilDescriptor::GetStencilFailureOperation() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return StencilOperation(m_table->stencilFailureOperation(m_ptr));
#else
        return StencilOperation([(MTLStencilDescriptor*)m_ptr stencilFailureOperation]);
#endif
    }

    StencilOperation StencilDescriptor::GetDepthFailureOperation() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return StencilOperation(m_table->depthFailureOperation(m_ptr));
#else
        return StencilOperation([(MTLStencilDescriptor*)m_ptr depthFailureOperation]);
#endif
    }

    StencilOperation StencilDescriptor::GetDepthStencilPassOperation() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return StencilOperation(m_table->depthStencilPassOperation(m_ptr));
#else
        return StencilOperation([(MTLStencilDescriptor*)m_ptr depthStencilPassOperation]);
#endif
    }

    uint32_t StencilDescriptor::GetReadMask() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->readMask(m_ptr);
#else
        return uint32_t([(MTLStencilDescriptor*)m_ptr readMask]);
#endif
    }

    uint32_t StencilDescriptor::GetWriteMask() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->writeMask(m_ptr);
#else
        return uint32_t([(MTLStencilDescriptor*)m_ptr writeMask]);
#endif
    }

    void StencilDescriptor::SetStencilCompareFunction(CompareFunction stencilCompareFunction)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setStencilCompareFunction(m_ptr, MTLCompareFunction(stencilCompareFunction));
#else
        [(MTLStencilDescriptor*)m_ptr setStencilCompareFunction:MTLCompareFunction(stencilCompareFunction)];
#endif
    }

    void StencilDescriptor::SetStencilFailureOperation(StencilOperation stencilFailureOperation)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setStencilFailureOperation(m_ptr, MTLStencilOperation(stencilFailureOperation));
#else
        [(MTLStencilDescriptor*)m_ptr setStencilFailureOperation:MTLStencilOperation(stencilFailureOperation)];
#endif
    }

    void StencilDescriptor::SetDepthFailureOperation(StencilOperation depthFailureOperation)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setDepthFailureOperation(m_ptr, MTLStencilOperation(depthFailureOperation));
#else
        [(MTLStencilDescriptor*)m_ptr setDepthFailureOperation:MTLStencilOperation(depthFailureOperation)];
#endif
    }

    void StencilDescriptor::SetDepthStencilPassOperation(StencilOperation depthStencilPassOperation)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setDepthStencilPassOperation(m_ptr, MTLStencilOperation(depthStencilPassOperation));
#else
        [(MTLStencilDescriptor*)m_ptr setDepthStencilPassOperation:MTLStencilOperation(depthStencilPassOperation)];
#endif
    }

    void StencilDescriptor::SetReadMask(uint32_t readMask)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setReadMask(m_ptr, readMask);
#else
        [(MTLStencilDescriptor*)m_ptr setReadMask:readMask];
#endif
    }

    void StencilDescriptor::SetWriteMask(uint32_t writeMask)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setWriteMask(m_ptr, writeMask);
#else
        [(MTLStencilDescriptor*)m_ptr setWriteMask:writeMask];
#endif
    }

    DepthStencilDescriptor::DepthStencilDescriptor() :
        ns::Object<MTLDepthStencilDescriptor*>([[MTLDepthStencilDescriptor alloc] init], ns::Ownership::Assign)
    {
    }

    CompareFunction DepthStencilDescriptor::GetDepthCompareFunction() const
    {
        Validate();
		
#if MTLPP_CONFIG_IMP_CACHE
        return CompareFunction(m_table->depthCompareFunction(m_ptr));
#else
        return CompareFunction([(MTLDepthStencilDescriptor*)m_ptr depthCompareFunction]);
#endif
    }

    bool DepthStencilDescriptor::IsDepthWriteEnabled() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->isDepthWriteEnabled(m_ptr);
#else
        return [(MTLDepthStencilDescriptor*)m_ptr isDepthWriteEnabled];
#endif
    }

    ns::AutoReleased<StencilDescriptor> DepthStencilDescriptor::GetFrontFaceStencil() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<StencilDescriptor>(m_table->frontFaceStencil(m_ptr));
#else
        return ns::AutoReleased<StencilDescriptor>([(MTLDepthStencilDescriptor*)m_ptr frontFaceStencil]);
#endif
    }

    ns::AutoReleased<StencilDescriptor> DepthStencilDescriptor::GetBackFaceStencil() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<StencilDescriptor>(m_table->backFaceStencil(m_ptr));
#else
        return ns::AutoReleased<StencilDescriptor>([(MTLDepthStencilDescriptor*)m_ptr backFaceStencil]);
#endif
    }

    ns::AutoReleased<ns::String> DepthStencilDescriptor::GetLabel() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<ns::String>(m_table->label(m_ptr));
#else
        return ns::AutoReleased<ns::String>([(MTLDepthStencilDescriptor*)m_ptr label]);
#endif
    }

    void DepthStencilDescriptor::SetDepthCompareFunction(CompareFunction depthCompareFunction) const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setDepthCompareFunction(m_ptr, MTLCompareFunction(depthCompareFunction));
#else
        [(MTLDepthStencilDescriptor*)m_ptr setDepthCompareFunction:MTLCompareFunction(depthCompareFunction)];
#endif
    }

    void DepthStencilDescriptor::SetDepthWriteEnabled(bool depthWriteEnabled) const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setDepthWriteEnabled(m_ptr, depthWriteEnabled);
#else
        [(MTLDepthStencilDescriptor*)m_ptr setDepthWriteEnabled:depthWriteEnabled];
#endif
    }

    void DepthStencilDescriptor::SetFrontFaceStencil(const StencilDescriptor& frontFaceStencil) const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setFrontFaceStencil(m_ptr, (MTLStencilDescriptor*)frontFaceStencil.GetPtr());
#else
        [(MTLDepthStencilDescriptor*)m_ptr setFrontFaceStencil:(MTLStencilDescriptor*)frontFaceStencil.GetPtr()];
#endif
    }

    void DepthStencilDescriptor::SetBackFaceStencil(const StencilDescriptor& backFaceStencil) const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setBackFaceStencil(m_ptr, (MTLStencilDescriptor*)backFaceStencil.GetPtr());
#else
        [(MTLDepthStencilDescriptor*)m_ptr setBackFaceStencil:(MTLStencilDescriptor*)backFaceStencil.GetPtr()];
#endif
    }

    void DepthStencilDescriptor::SetLabel(const ns::String& label) const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setLabel(m_ptr, (NSString*)label.GetPtr());
#else
        [(MTLDepthStencilDescriptor*)m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
    }

    ns::AutoReleased<ns::String> DepthStencilState::GetLabel() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::String>(m_table->Label(m_ptr));
#else
        return ns::AutoReleased<ns::String>([(id<MTLDepthStencilState>)m_ptr label]);
#endif
    }

    ns::AutoReleased<Device> DepthStencilState::GetDevice() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<Device>(m_table->Device(m_ptr));
#else
        return ns::AutoReleased<Device>([(id<MTLDepthStencilState>)m_ptr device]);
#endif
    }
}

MTLPP_END
