// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


#include "declare.hpp"
#include "imp_CaptureManager.hpp"
#include "ns.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	inline ITable<MTLCaptureManager*, void>* CreateIMPTable(MTLCaptureManager* handle)
	{
		static ITable<MTLCaptureManager*, void> Table(object_getClass(handle));
		return &Table;
	}
}

namespace mtlpp
{
	class MTLPP_EXPORT Device;
	class MTLPP_EXPORT CaptureScope;
	class MTLPP_EXPORT CommandQueue;
	
	class MTLPP_EXPORT CaptureManager : public ns::Object<MTLCaptureManager*>
	{
		CaptureManager() { }
		CaptureManager(MTLCaptureManager* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLCaptureManager*>(handle, retain) { }
	public:
		static CaptureManager& SharedCaptureManager();
		
		CaptureScope NewCaptureScopeWithDevice(Device Device);
		CaptureScope NewCaptureScopeWithCommandQueue(CommandQueue Queue);
		
		void StartCaptureWithDevice(Device device);
		void StartCaptureWithCommandQueue(CommandQueue queue);
		void StartCaptureWithScope(CaptureScope scope);
		
		void StopCapture();
		
		ns::AutoReleased<CaptureScope> GetDefaultCaptureScope() const;
		void SetDefaultCaptureScope(CaptureScope scope);
		
		bool IsCapturing() const;
	} MTLPP_AVAILABLE(10_13, 11_0);
	
}

MTLPP_END
