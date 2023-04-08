// Copyright Epic Games, Inc. All Rights Reserved.
#ifndef imp_CaptureManager_hpp
#define imp_CaptureManager_hpp

#include "imp_Object.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<MTLCaptureManager*, void> : public IMPTableBase<MTLCaptureManager*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLCaptureManager*>(C)
	, INTERPOSE_CONSTRUCTOR(newCaptureScopeWithDevice, C)
	, INTERPOSE_CONSTRUCTOR(newCaptureScopeWithCommandQueue, C)
	, INTERPOSE_CONSTRUCTOR(startCaptureWithDevice, C)
	, INTERPOSE_CONSTRUCTOR(startCaptureWithCommandQueue, C)
	, INTERPOSE_CONSTRUCTOR(startCaptureWithScope, C)
	, INTERPOSE_CONSTRUCTOR(stopCapture, C)
	, INTERPOSE_CONSTRUCTOR(defaultCaptureScope, C)
	, INTERPOSE_CONSTRUCTOR(SetdefaultCaptureScope, C)
	, INTERPOSE_CONSTRUCTOR(isCapturing, C)
	{
	}
	
	INTERPOSE_SELECTOR(MTLCaptureManager*, newCaptureScopeWithDevice:, newCaptureScopeWithDevice, id<MTLCaptureScope>, id<MTLDevice>);
	INTERPOSE_SELECTOR(MTLCaptureManager*, newCaptureScopeWithCommandQueue:, newCaptureScopeWithCommandQueue, id<MTLCaptureScope>, id<MTLCommandQueue>);
	INTERPOSE_SELECTOR(MTLCaptureManager*, startCaptureWithDevice:, startCaptureWithDevice, void, id<MTLDevice>);
	INTERPOSE_SELECTOR(MTLCaptureManager*, startCaptureWithCommandQueue:, startCaptureWithCommandQueue, void, id<MTLCommandQueue>);
	INTERPOSE_SELECTOR(MTLCaptureManager*, startCaptureWithScope:, startCaptureWithScope, void, id<MTLCaptureScope>);
	INTERPOSE_SELECTOR(MTLCaptureManager*, stopCapture, stopCapture, void);
	INTERPOSE_SELECTOR(MTLCaptureManager*, defaultCaptureScope, defaultCaptureScope, id<MTLCaptureScope>);
	INTERPOSE_SELECTOR(MTLCaptureManager*, setDefaultCaptureScope:, SetdefaultCaptureScope, void, id<MTLCaptureScope>);
	INTERPOSE_SELECTOR(MTLCaptureManager*, isCapturing, isCapturing, BOOL);
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<MTLCaptureManager*, InterposeClass> : public IMPTable<MTLCaptureManager*, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<MTLCaptureManager*, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		INTERPOSE_REGISTRATION(newCaptureScopeWithDevice, C);
		INTERPOSE_REGISTRATION(newCaptureScopeWithCommandQueue, C);
		INTERPOSE_REGISTRATION(startCaptureWithDevice, C);
		INTERPOSE_REGISTRATION(startCaptureWithCommandQueue, C);
		INTERPOSE_REGISTRATION(startCaptureWithScope, C);
		INTERPOSE_REGISTRATION(stopCapture, C);
		INTERPOSE_REGISTRATION(SetdefaultCaptureScope, C);
	}
};

MTLPP_END

#endif /* imp_CaptureManager_hpp */
