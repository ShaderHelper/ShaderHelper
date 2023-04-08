/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "ns.hpp"

MTLPP_BEGIN

MTLPP_CLASS(CommandBufferFenceImpl);

namespace mtlpp
{
    class CommandBuffer;
	
	class MTLPP_EXPORT CommandBufferFence : public ns::Object<CommandBufferFenceImpl*, ns::CallingConvention::ObjectiveC>
	{
		friend class CommandBuffer;
	public:
		CommandBufferFence();
		~CommandBufferFence();
		CommandBufferFence(const CommandBufferFence& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
		CommandBufferFence(CommandBufferFence&& rhs);
#endif
		
		CommandBufferFence& operator=(const CommandBufferFence& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
		CommandBufferFence& operator=(CommandBufferFence&& rhs);
#endif

	public:
		void Init();
		bool Wait(NSUInteger TimeInterval) const;
		
	private:
		void Insert(CommandBuffer& CmdBuffer);
		void Signal(CommandBuffer const& CmdBuffer);
	};
}

MTLPP_END
