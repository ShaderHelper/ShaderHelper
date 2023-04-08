/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLSampler.h>
#include "sampler.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    SamplerDescriptor::SamplerDescriptor() :
        ns::Object<MTLSamplerDescriptor*>([[MTLSamplerDescriptor alloc] init], ns::Ownership::Assign)
    {
    }

    SamplerMinMagFilter SamplerDescriptor::GetMinFilter() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return SamplerMinMagFilter(m_table->minFilter(m_ptr));
#else
        return SamplerMinMagFilter([(MTLSamplerDescriptor*)m_ptr minFilter]);
#endif
    }

    SamplerMinMagFilter SamplerDescriptor::GetMagFilter() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return SamplerMinMagFilter(m_table->magFilter(m_ptr));
#else
        return SamplerMinMagFilter([(MTLSamplerDescriptor*)m_ptr magFilter]);
#endif
    }

    SamplerMipFilter SamplerDescriptor::GetMipFilter() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return SamplerMipFilter(m_table->mipFilter(m_ptr));
#else
        return SamplerMipFilter([(MTLSamplerDescriptor*)m_ptr mipFilter]);
#endif
    }

    NSUInteger SamplerDescriptor::GetMaxAnisotropy() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(m_table->maxAnisotropy(m_ptr));
#else
        return NSUInteger([(MTLSamplerDescriptor*)m_ptr maxAnisotropy]);
#endif
    }

    SamplerAddressMode SamplerDescriptor::GetSAddressMode() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return SamplerAddressMode(m_table->sAddressMode(m_ptr));
#else
        return SamplerAddressMode([(MTLSamplerDescriptor*)m_ptr sAddressMode]);
#endif
    }

    SamplerAddressMode SamplerDescriptor::GetTAddressMode() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return SamplerAddressMode(m_table->tAddressMode(m_ptr));
#else
        return SamplerAddressMode([(MTLSamplerDescriptor*)m_ptr tAddressMode]);
#endif
    }

    SamplerAddressMode SamplerDescriptor::GetRAddressMode() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return SamplerAddressMode(m_table->rAddressMode(m_ptr));
#else
        return SamplerAddressMode([(MTLSamplerDescriptor*)m_ptr rAddressMode]);
#endif
    }

    SamplerBorderColor SamplerDescriptor::GetBorderColor() const
    {
#if MTLPP_IS_AVAILABLE_MAC(10_12)
#if MTLPP_CONFIG_IMP_CACHE
		return SamplerBorderColor(m_table->borderColor(m_ptr));
#else
#if MTLPP_PLATFORM_MAC
		if (@available(macOS 10.12, *))
			return SamplerBorderColor([(MTLSamplerDescriptor*)m_ptr borderColor]);
		else
			return SamplerBorderColor(0);
#endif
#endif
#else
		return SamplerBorderColor(0);
#endif
    }

    bool SamplerDescriptor::IsNormalizedCoordinates() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->normalizedCoordinates(m_ptr);
#else
        return [(MTLSamplerDescriptor*)m_ptr normalizedCoordinates];
#endif
    }

    float SamplerDescriptor::GetLodMinClamp() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->lodMinClamp(m_ptr);
#else
        return [(MTLSamplerDescriptor*)m_ptr lodMinClamp];
#endif
    }

    float SamplerDescriptor::GetLodMaxClamp() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->lodMaxClamp(m_ptr);
#else
        return [(MTLSamplerDescriptor*)m_ptr lodMaxClamp];
#endif
    }

    CompareFunction SamplerDescriptor::GetCompareFunction() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
        return CompareFunction(m_table->compareFunction(m_ptr));
#else
        return CompareFunction([(MTLSamplerDescriptor*)m_ptr compareFunction]);
#endif
#else
        return CompareFunction(0);
#endif
    }

    ns::AutoReleased<ns::String> SamplerDescriptor::GetLabel() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<ns::String>(m_table->label(m_ptr));
#else
        return ns::AutoReleased<ns::String>([(MTLSamplerDescriptor*)m_ptr label]);
#endif
    }
	
	bool SamplerDescriptor::SupportArgumentBuffers() const
	{
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->supportArgumentBuffers(m_ptr);
#else
		return [(MTLSamplerDescriptor*)m_ptr supportArgumentBuffers];
#endif
#else
		return false;
#endif
	}

    void SamplerDescriptor::SetMinFilter(SamplerMinMagFilter minFilter)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setminFilter(m_ptr, MTLSamplerMinMagFilter(minFilter));
#else
        [(MTLSamplerDescriptor*)m_ptr setMinFilter:MTLSamplerMinMagFilter(minFilter)];
#endif
    }

    void SamplerDescriptor::SetMagFilter(SamplerMinMagFilter magFilter)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setmagFilter(m_ptr, MTLSamplerMinMagFilter(magFilter));
#else
        [(MTLSamplerDescriptor*)m_ptr setMagFilter:MTLSamplerMinMagFilter(magFilter)];
#endif
    }

    void SamplerDescriptor::SetMipFilter(SamplerMipFilter mipFilter)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setmipFilter(m_ptr, MTLSamplerMipFilter(mipFilter));
#else
        [(MTLSamplerDescriptor*)m_ptr setMipFilter:MTLSamplerMipFilter(mipFilter)];
#endif
    }

    void SamplerDescriptor::SetMaxAnisotropy(NSUInteger maxAnisotropy)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setmaxAnisotropy(m_ptr, maxAnisotropy);
#else
        [(MTLSamplerDescriptor*)m_ptr setMaxAnisotropy:maxAnisotropy];
#endif
    }

    void SamplerDescriptor::SetSAddressMode(SamplerAddressMode sAddressMode)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setsAddressMode(m_ptr, MTLSamplerAddressMode(sAddressMode));
#else
        [(MTLSamplerDescriptor*)m_ptr setSAddressMode:MTLSamplerAddressMode(sAddressMode)];
#endif
    }

    void SamplerDescriptor::SetTAddressMode(SamplerAddressMode tAddressMode)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->settAddressMode(m_ptr, MTLSamplerAddressMode(tAddressMode));
#else
        [(MTLSamplerDescriptor*)m_ptr setTAddressMode:MTLSamplerAddressMode(tAddressMode)];
#endif
    }

    void SamplerDescriptor::SetRAddressMode(SamplerAddressMode rAddressMode)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setrAddressMode(m_ptr, MTLSamplerAddressMode(rAddressMode));
#else
        [(MTLSamplerDescriptor*)m_ptr setRAddressMode:MTLSamplerAddressMode(rAddressMode)];
#endif
    }

    void SamplerDescriptor::SetBorderColor(SamplerBorderColor borderColor)
    {
#if MTLPP_IS_AVAILABLE_MAC(10_12)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setborderColor(m_ptr, MTLSamplerBorderColor(borderColor));
#else
		if (@available(macOS 10.12, *))
			[(MTLSamplerDescriptor*)m_ptr setBorderColor:MTLSamplerBorderColor(borderColor)];
#endif
#endif
    }

    void SamplerDescriptor::SetNormalizedCoordinates(bool normalizedCoordinates)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setnormalizedCoordinates(m_ptr, normalizedCoordinates);
#else
        [(MTLSamplerDescriptor*)m_ptr setNormalizedCoordinates:normalizedCoordinates];
#endif
    }

    void SamplerDescriptor::SetLodMinClamp(float lodMinClamp)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setlodMinClamp(m_ptr, lodMinClamp);
#else
        [(MTLSamplerDescriptor*)m_ptr setLodMinClamp:lodMinClamp];
#endif
    }

    void SamplerDescriptor::SetLodMaxClamp(float lodMaxClamp)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setlodMaxClamp(m_ptr, lodMaxClamp);
#else
        [(MTLSamplerDescriptor*)m_ptr setLodMaxClamp:lodMaxClamp];
#endif
    }

    void SamplerDescriptor::SetCompareFunction(CompareFunction compareFunction)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setcompareFunction(m_ptr, MTLCompareFunction(compareFunction));
#else
        [(MTLSamplerDescriptor*)m_ptr setCompareFunction:MTLCompareFunction(compareFunction)];
#endif
#endif
    }

    void SamplerDescriptor::SetLabel(const ns::String& label)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setlabel(m_ptr, label.GetPtr());
#else
        [(MTLSamplerDescriptor*)m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
    }
	
	void SamplerDescriptor::SetSupportArgumentBuffers(bool flag)
	{
		Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setsupportArgumentBuffers(m_ptr, flag);
#else
		[(MTLSamplerDescriptor*)m_ptr setSupportArgumentBuffers:flag];
#endif
	}

    ns::AutoReleased<ns::String> SamplerState::GetLabel() const
    {
#if MTLPP_CONFIG_IMP_CACHE
        Validate();
		return ns::AutoReleased<ns::String>(m_table->Label(m_ptr));
#else
        return ns::AutoReleased<ns::String>([(id<MTLSamplerState>)m_ptr label]);
#endif
    }

    ns::AutoReleased<Device> SamplerState::GetDevice() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<Device>(m_table->Device(m_ptr));
#else
        return ns::AutoReleased<Device>([(id<MTLSamplerState>)m_ptr device]);
#endif
    }
}

MTLPP_END

