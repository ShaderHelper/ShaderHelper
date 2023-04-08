/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_CommandEncoder.hpp"
#include "ns.hpp"
#include "command_buffer_fence.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    class Device;
	
	enum ResourceUsage : NSUInteger
	{
		Read   = 1 << 0,
		Write  = 1 << 1,
		Sample = 1 << 2
	}
	MTLPP_AVAILABLE(10_13, 11_0);

	template<typename T>
	class MTLPP_EXPORT CommandEncoder : public ns::Object<T>
    {
#if MTLPP_CONFIG_VALIDATE
		CommandBufferFence CmdBufferFence;
#endif
    public:
		CommandEncoder(ns::Ownership const retain = ns::Ownership::Retain);
		CommandEncoder(T handle, ns::Ownership const retain = ns::Ownership::Retain, typename ns::Object<T>::ITable* cache = nullptr);
		
#if MTLPP_CONFIG_VALIDATE
		CommandEncoder(const CommandEncoder& rhs);
		CommandEncoder& operator=(const CommandEncoder& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
		CommandEncoder(CommandEncoder&& rhs);
		CommandEncoder& operator=(CommandEncoder&& rhs);
#endif
		void SetCommandBufferFence(CommandBufferFence& Fence);
		CommandBufferFence& GetCommandBufferFence(void);
		CommandBufferFence const& GetCommandBufferFence(void) const;
#endif

        ns::AutoReleased<Device>     GetDevice() const;
        ns::AutoReleased<ns::String> GetLabel() const;

        void SetLabel(const ns::String& label);

        void EndEncoding();
        void InsertDebugSignpost(const ns::String& string);
        void PushDebugGroup(const ns::String& string);
        void PopDebugGroup();
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

#include "command_encoder.mm"

MTLPP_END
