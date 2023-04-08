// Copyright Epic Games, Inc. All Rights Reserved.

#include <Metal/MTLCaptureScope.h>
#include <Metal/MTLDevice.h>
#include <Metal/MTLCommandQueue.h>
#include <Foundation/NSString.h>
#include "capture_scope.hpp"
#include "device.hpp"
#include "command_queue.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	void CaptureScope::BeginScope()
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->beginScope(m_ptr);
#else
		[(id<MTLCaptureScope>) m_ptr beginScope];
#endif
#endif
	}
	
	void CaptureScope::EndScope()
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->endScope(m_ptr);
#else
		[(id<MTLCaptureScope>) m_ptr endScope];
#endif
#endif
	}
	
	ns::AutoReleased<ns::String>   CaptureScope::GetLabel() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::String>(m_table->label(m_ptr));
#else
		return ns::AutoReleased<ns::String>([(id<MTLCaptureScope>) m_ptr label]);
#endif
#else
		return ns::AutoReleased<ns::String>();
#endif
	}
	
	void CaptureScope::SetLabel(const ns::String& label)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setlabel(m_ptr, label.GetPtr());
#else
		[(id<MTLCaptureScope>) m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
#endif
	}
	
	ns::AutoReleased<Device> CaptureScope::GetDevice() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<Device>(m_table->device(m_ptr));
#else
		return ns::AutoReleased<Device>([(id<MTLCaptureScope>) m_ptr device]);
#endif
#else
		return ns::AutoReleased<Device>();
#endif
	}
	
	ns::AutoReleased<CommandQueue> CaptureScope::GetCommandQueue() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		id<MTLCommandQueue> handle = m_table->commandQueue(m_ptr);
		return ns::AutoReleased<CommandQueue>(handle, m_table->TableCache->GetCommandQueue(handle));
#else
		return ns::AutoReleased<CommandQueue>([(id<MTLCaptureScope>) m_ptr commandQueue]);
#endif
#else
		return ns::AutoReleased<CommandQueue>();
#endif
	}
}

MTLPP_END
