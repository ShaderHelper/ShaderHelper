// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_CommandQueue_hpp
#define imp_CommandQueue_hpp

#include "imp_Object.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<id<MTLCommandQueue>, void> : public IMPTableBase<id<MTLCommandQueue>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<id<MTLCommandQueue>>(C)
	, INTERPOSE_CONSTRUCTOR(Label, C)
	, INTERPOSE_CONSTRUCTOR(SetLabel, C)
	, INTERPOSE_CONSTRUCTOR(Device, C)
	, INTERPOSE_CONSTRUCTOR(CommandBuffer, C)
	, INTERPOSE_CONSTRUCTOR(CommandBufferWithUnretainedReferences, C)
	, INTERPOSE_CONSTRUCTOR(InsertDebugCaptureBoundary, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLCommandQueue>, label, Label, NSString*);
	INTERPOSE_SELECTOR(id<MTLCommandQueue>, setLabel:, SetLabel, void, NSString*);
	INTERPOSE_SELECTOR(id<MTLCommandQueue>, device, Device, id<MTLDevice>);
	INTERPOSE_SELECTOR(id<MTLCommandQueue>, commandBuffer, CommandBuffer, id <MTLCommandBuffer>);
	INTERPOSE_SELECTOR(id<MTLCommandQueue>, commandBufferWithUnretainedReferences, CommandBufferWithUnretainedReferences, id <MTLCommandBuffer>);
	INTERPOSE_SELECTOR(id<MTLCommandQueue>, insertDebugCaptureBoundary, InsertDebugCaptureBoundary, void);
	
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<id<MTLCommandQueue>, InterposeClass> : public IMPTable<id<MTLCommandQueue>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLCommandQueue>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableBase<id<MTLCommandQueue>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(SetLabel, C);
		INTERPOSE_REGISTRATION(CommandBuffer, C);
		INTERPOSE_REGISTRATION(CommandBufferWithUnretainedReferences, C);
		INTERPOSE_REGISTRATION(InsertDebugCaptureBoundary, C);
	}
};

MTLPP_END

#endif /* imp_CommandQueue_hpp */
