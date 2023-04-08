// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_CaptureScope_hpp
#define imp_CaptureScope_hpp

#include "imp_Object.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<id<MTLCaptureScope>, void> : public IMPTableBase<id<MTLCaptureScope>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<id<MTLCaptureScope>>(C)
	, INTERPOSE_CONSTRUCTOR(beginScope, C)
	, INTERPOSE_CONSTRUCTOR(endScope, C)
	, INTERPOSE_CONSTRUCTOR(label, C)
	, INTERPOSE_CONSTRUCTOR(Setlabel, C)
	, INTERPOSE_CONSTRUCTOR(device, C)
	, INTERPOSE_CONSTRUCTOR(commandQueue, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLCaptureScope>, beginScope, beginScope, void);
	INTERPOSE_SELECTOR(id<MTLCaptureScope>, endScope, endScope, void);
	INTERPOSE_SELECTOR(id<MTLCaptureScope>, label, label, NSString*);
	INTERPOSE_SELECTOR(id<MTLCaptureScope>, setLabel:, Setlabel, void, NSString*);
	INTERPOSE_SELECTOR(id<MTLCaptureScope>, device, device, id<MTLDevice>);
	INTERPOSE_SELECTOR(id<MTLCaptureScope>, commandQueue, commandQueue, id<MTLCommandQueue>);
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<id<MTLCaptureScope>, InterposeClass> : public IMPTable<id<MTLCaptureScope>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLCaptureScope>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableBase<id<MTLCaptureScope>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(beginScope, C);
		INTERPOSE_REGISTRATION(endScope, C);
		INTERPOSE_REGISTRATION(Setlabel, C);
	}
};

MTLPP_END

#endif /* imp_CaptureScope_hpp */
