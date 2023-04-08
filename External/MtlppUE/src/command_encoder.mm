/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once

#include "command_encoder.hpp"
#include "device.hpp"

#ifdef __OBJC__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wduplicate-protocol"
NS_AVAILABLE(10_11, 8_0)
@protocol MTLCommandEncoder <NSObject>
@property (readonly) id <MTLDevice> device;
@property (nullable, copy, atomic) NSString *label;
- (void)endEncoding;
- (void)insertDebugSignpost:(NSString *)string;
- (void)pushDebugGroup:(NSString *)string;
- (void)popDebugGroup;
@end
#pragma clang diagnostic pop
#endif

MTLPP_BEGIN

namespace mtlpp
{
	template<typename T>
	CommandEncoder<T>::CommandEncoder(ns::Ownership const retain)
	: ns::Object<T>(retain)
	{
		
	}
	template<typename T>
	CommandEncoder<T>::CommandEncoder(T handle, ns::Ownership const retain, typename ns::Object<T>::ITable* cache)
	: ns::Object<T>(handle, retain, cache)
	{
		
	}
	
#if MTLPP_CONFIG_VALIDATE
	template<typename T>
	CommandEncoder<T>::CommandEncoder(const CommandEncoder<T>& rhs)
	: ns::Object<T>(rhs)
	, CmdBufferFence(rhs.CmdBufferFence)
	{
		
	}
	template<typename T>
	CommandEncoder<T>& CommandEncoder<T>::operator=(const CommandEncoder<T>& rhs)
	{
		if (this != &rhs)
		{
			ns::Object<T>::operator=(rhs);
			CmdBufferFence = rhs.CmdBufferFence;
		}
		return *this;
	}
#if MTLPP_CONFIG_RVALUE_REFERENCES
	template<typename T>
	CommandEncoder<T>::CommandEncoder(CommandEncoder<T>&& rhs)
	: ns::Object<T>((CommandEncoder<T>&&)rhs)
	, CmdBufferFence(rhs.CmdBufferFence)
	{
		
	}
	template<typename T>
	CommandEncoder<T>& CommandEncoder<T>::operator=(CommandEncoder<T>&& rhs)
	{
		ns::Object<T>::operator=((CommandEncoder<T>&&)rhs);
		CmdBufferFence = rhs.CmdBufferFence;
		return *this;
	}
#endif
	template<typename T>
	void CommandEncoder<T>::SetCommandBufferFence(CommandBufferFence& Fence)
	{
		assert(CmdBufferFence == nullptr);
		assert(Fence != nullptr);
		CmdBufferFence = Fence;
	}
	template<typename T>
	CommandBufferFence& CommandEncoder<T>::GetCommandBufferFence(void)
	{
		assert(CmdBufferFence != nullptr);
		return CmdBufferFence;
	}
	template<typename T>
	CommandBufferFence const& CommandEncoder<T>::GetCommandBufferFence(void) const
	{
		assert(CmdBufferFence != nullptr);
		return CmdBufferFence;
	}
#endif
	
	template<typename T>
    ns::AutoReleased<Device> CommandEncoder<T>::GetDevice() const
    {
#if MTLPP_CONFIG_VALIDATE
        this->Validate();
#endif
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<Device>(this->m_table->Device((id<MTLCommandEncoder>)this->m_ptr));
#else
#ifdef __OBJC__
        return ns::AutoReleased<Device>([(id<MTLCommandEncoder>)this->m_ptr device]);
#else
		return Device();
#endif
#endif
    }

	template<typename T>
    ns::AutoReleased<ns::String> CommandEncoder<T>::GetLabel() const
    {
#if MTLPP_CONFIG_VALIDATE
        this->Validate();
#endif
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::String>(this->m_table->Label((id<MTLCommandEncoder>)this->m_ptr));
#else
#ifdef __OBJC__
        return ns::AutoReleased<ns::String>([(id<MTLCommandEncoder>)this->m_ptr label]);
#else
		return ns::String();
#endif
#endif
    }

	template<typename T>
    void CommandEncoder<T>::SetLabel(const ns::String& label)
    {
#if MTLPP_CONFIG_VALIDATE
        this->Validate();
#endif
#if MTLPP_CONFIG_IMP_CACHE
		this->m_table->SetLabel((id<MTLCommandEncoder>)this->m_ptr, label.GetPtr());
#else
#ifdef __OBJC__
        [(id<MTLCommandEncoder>)this->m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
#endif
    }

	template<typename T>
    void CommandEncoder<T>::EndEncoding()
    {
#if MTLPP_CONFIG_VALIDATE
        this->Validate();
#endif
#if MTLPP_CONFIG_IMP_CACHE
		this->m_table->EndEncoding((id<MTLCommandEncoder>)this->m_ptr);
#else
#ifdef __OBJC__
        [(id<MTLCommandEncoder>)this->m_ptr endEncoding];
#endif
#endif
    }

	template<typename T>
    void CommandEncoder<T>::InsertDebugSignpost(const ns::String& string)
    {
#if MTLPP_CONFIG_VALIDATE
    	this->Validate();
#endif
#if MTLPP_CONFIG_IMP_CACHE
		this->m_table->InsertDebugSignpost((id<MTLCommandEncoder>)this->m_ptr, string.GetPtr());
#else
       this->Validate();
#ifdef __OBJC__
        [(id<MTLCommandEncoder>)this->m_ptr insertDebugSignpost:(NSString*)string.GetPtr()];
#endif
#endif
    }

	template<typename T>
    void CommandEncoder<T>::PushDebugGroup(const ns::String& string)
    {
#if MTLPP_CONFIG_VALIDATE
        this->Validate();
#endif
#if MTLPP_CONFIG_IMP_CACHE
		this->m_table->PushDebugGroup((id<MTLCommandEncoder>)this->m_ptr, string.GetPtr());
#else
#ifdef __OBJC__
        [(id<MTLCommandEncoder>)this->m_ptr pushDebugGroup:(NSString*)string.GetPtr()];
#endif
#endif
    }

	template<typename T>
    void CommandEncoder<T>::PopDebugGroup()
    {
#if MTLPP_CONFIG_VALIDATE
        this->Validate();
#endif
#if MTLPP_CONFIG_IMP_CACHE
		this->m_table->PopDebugGroup((id<MTLCommandEncoder>)this->m_ptr);
#else
#ifdef __OBJC__
        [(id<MTLCommandEncoder>)this->m_ptr popDebugGroup];
#endif
#endif
    }
}

MTLPP_END
