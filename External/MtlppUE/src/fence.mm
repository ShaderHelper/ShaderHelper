/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLFence.h>
#include "fence.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    ns::AutoReleased<Device> Fence::GetDevice() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<Device>(m_table->Device(m_ptr));
#else
		if(@available(macOS 10.13, iOS 10.0, *))
			return ns::AutoReleased<Device>([(id<MTLFence>)m_ptr device]);
		else
			return ns::AutoReleased<Device>();
#endif
#else
		return ns::AutoReleased<Device>();
#endif
    }

    ns::AutoReleased<ns::String> Fence::GetLabel() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::String>(m_table->Label(m_ptr));
#else
		if(@available(macOS 10.13, iOS 10.0, *))
			return ns::AutoReleased<ns::String>([(id<MTLFence>)m_ptr label]);
		else
			return ns::AutoReleased<ns::String>();
#endif
#else
		return ns::AutoReleased<ns::String>();
#endif
    }

    void Fence::SetLabel(const ns::String& label)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetLabel(m_ptr, label.GetPtr());
#else
		if(@available(macOS 10.13, iOS 10.0, *))
			[(id<MTLFence>)m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
#endif
	}
}

MTLPP_END
