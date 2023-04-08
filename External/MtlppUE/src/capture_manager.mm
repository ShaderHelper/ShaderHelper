// Copyright Epic Games, Inc. All Rights Reserved.

#include <Metal/MTLCaptureManager.h>
#include <Metal/MTLCaptureScope.h>
#include <Metal/MTLDevice.h>
#include <Metal/MTLCommandQueue.h>
#include "capture_manager.hpp"
#include "capture_scope.hpp"
#include "device.hpp"
#include "command_queue.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	CaptureManager& CaptureManager::SharedCaptureManager()
	{
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		static CaptureManager CaptureMan([MTLCaptureManager sharedCaptureManager]);
#else
		static CaptureManager;
#endif
		return CaptureMan;
	}
	
	CaptureScope CaptureManager::NewCaptureScopeWithDevice(Device Device)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return CaptureScope(m_table->newCaptureScopeWithDevice(m_ptr, Device.GetPtr()), Device.GetTable()->TableCache, ns::Ownership::Assign);
#else
		return CaptureScope([(MTLCaptureManager*) m_ptr newCaptureScopeWithDevice:(id<MTLDevice>)Device.GetPtr()], nullptr, ns::Ownership::Assign);
#endif
#else
		return CaptureScope();
#endif
	}
	
	CaptureScope CaptureManager::NewCaptureScopeWithCommandQueue(CommandQueue Queue)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return CaptureScope(m_table->newCaptureScopeWithCommandQueue(m_ptr, Queue.GetPtr()), Queue.GetTable()->TableCache, ns::Ownership::Assign);
#else
		return CaptureScope([(MTLCaptureManager*) m_ptr newCaptureScopeWithCommandQueue:(id<MTLCommandQueue>)Queue.GetPtr()], nullptr, ns::Ownership::Assign);
#endif
#else
		return CaptureScope();
#endif
	}
	
	void CaptureManager::StartCaptureWithDevice(Device device)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->startCaptureWithDevice(m_ptr, device.GetPtr());
#else
		[(MTLCaptureManager*) m_ptr startCaptureWithDevice:(id<MTLDevice>)device.GetPtr()];
#endif
#endif
	}
	
	void CaptureManager::StartCaptureWithCommandQueue(CommandQueue queue)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->startCaptureWithCommandQueue(m_ptr, queue.GetPtr());
#else
		[(MTLCaptureManager*) m_ptr startCaptureWithCommandQueue:(id<MTLCommandQueue>)queue.GetPtr()];
#endif
#endif
	}
	
	void CaptureManager::StartCaptureWithScope(CaptureScope scope)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->startCaptureWithScope(m_ptr, scope.GetPtr());
#else
		[(MTLCaptureManager*) m_ptr startCaptureWithScope:(id<MTLCaptureScope>)scope.GetPtr()];
#endif
#endif
	}
	
	void CaptureManager::StopCapture()
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->stopCapture(m_ptr);
#else
		[(MTLCaptureManager*) m_ptr stopCapture];
#endif
#endif
	}
	
	ns::AutoReleased<CaptureScope> CaptureManager::GetDefaultCaptureScope() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<CaptureScope>(m_table->defaultCaptureScope(m_ptr));
#else
		return ns::AutoReleased<CaptureScope>([(MTLCaptureManager*) m_ptr defaultCaptureScope]);
#endif
#else
		return ns::AutoReleased<CaptureScope>();
#endif
	}
	
	void CaptureManager::SetDefaultCaptureScope(CaptureScope scope)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetdefaultCaptureScope(m_ptr, scope.GetPtr());
#else
		[(MTLCaptureManager*) m_ptr setDefaultCaptureScope: (id<MTLCaptureScope>)scope.GetPtr()];
#endif
#endif
	}
	
	bool CaptureManager::IsCapturing() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->isCapturing(m_ptr);
#else
		return [(MTLCaptureManager*) m_ptr isCapturing];
#endif
#else
		return false;
#endif
	}
}

MTLPP_END

