// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_State_hpp
#define imp_State_hpp

#include "imp_Object.hpp"

MTLPP_BEGIN

template<typename ObjC>
struct IMPTableState : public IMPTableBase<ObjC>
{
	IMPTableState()
	{
	}
	
	IMPTableState(Class C)
	: IMPTableBase<ObjC>(C)
	, INTERPOSE_CONSTRUCTOR(Device, C)
	, INTERPOSE_CONSTRUCTOR(Label, C)
	{
	}
	
	template<typename InterposeClass>
	void RegisterInterpose(Class C)
	{
		IMPTableBase<ObjC>::template RegisterInterpose<InterposeClass>(C);
	}
	
	INTERPOSE_SELECTOR(id, device, Device, id<MTLDevice>);
	INTERPOSE_SELECTOR(id, label, Label, NSString*);
};

MTLPP_END

#endif /* imp_State_hpp */
