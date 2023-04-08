// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_CommandEncoder_hpp
#define imp_CommandEncoder_hpp

#include "imp_Object.hpp"

MTLPP_BEGIN

template<typename ObjC>
struct MTLPP_EXPORT IMPTableCommandEncoder : public IMPTableBase<ObjC>
{
	IMPTableCommandEncoder()
	{
	}
	
	IMPTableCommandEncoder(Class C)
	: IMPTableBase<ObjC>(C)
	, INTERPOSE_CONSTRUCTOR(Device, C)
	, INTERPOSE_CONSTRUCTOR(Label, C)
	, INTERPOSE_CONSTRUCTOR(SetLabel, C)
	, INTERPOSE_CONSTRUCTOR(EndEncoding, C)
	, INTERPOSE_CONSTRUCTOR(InsertDebugSignpost, C)
	, INTERPOSE_CONSTRUCTOR(PushDebugGroup, C)
	, INTERPOSE_CONSTRUCTOR(PopDebugGroup, C)
	{
	}
	
	template<typename InterposeClass>
	void RegisterInterpose(Class C)
	{
		IMPTableBase<ObjC>::template RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(SetLabel, C);
		INTERPOSE_REGISTRATION(EndEncoding, C);
		INTERPOSE_REGISTRATION(InsertDebugSignpost, C);
		INTERPOSE_REGISTRATION(PushDebugGroup, C);
		INTERPOSE_REGISTRATION(PopDebugGroup, C);
	}
	
	INTERPOSE_SELECTOR(id<MTLCommandEncoder>, device, Device, id<MTLDevice>);
	INTERPOSE_SELECTOR(id<MTLCommandEncoder>, label, Label, NSString*);
	INTERPOSE_SELECTOR(id<MTLCommandEncoder>, setLabel:, SetLabel, void, NSString*);
	INTERPOSE_SELECTOR(id<MTLCommandEncoder>, endEncoding, EndEncoding, void);
	INTERPOSE_SELECTOR(id<MTLCommandEncoder>, insertDebugSignpost:, InsertDebugSignpost, void, NSString*);
	INTERPOSE_SELECTOR(id<MTLCommandEncoder>, pushDebugGroup:, PushDebugGroup, void, NSString*);
	INTERPOSE_SELECTOR(id<MTLCommandEncoder>, popDebugGroup, PopDebugGroup, void);
};

MTLPP_END

#endif /* imp_CommandEncoder_hpp */
