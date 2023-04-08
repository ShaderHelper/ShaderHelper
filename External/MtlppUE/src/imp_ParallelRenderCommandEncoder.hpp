// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_ParallelRenderCommandEncoder_hpp
#define imp_ParallelRenderCommandEncoder_hpp

#include "imp_CommandEncoder.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<id<MTLParallelRenderCommandEncoder>, void> : public IMPTableCommandEncoder<id<MTLParallelRenderCommandEncoder>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableCommandEncoder<id<MTLParallelRenderCommandEncoder>>(C)
	, INTERPOSE_CONSTRUCTOR(RenderCommandEncoder, C)
	, INTERPOSE_CONSTRUCTOR(SetcolorstoreactionAtindex, C)
	, INTERPOSE_CONSTRUCTOR(Setdepthstoreaction, C)
	, INTERPOSE_CONSTRUCTOR(Setstencilstoreaction, C)
	, INTERPOSE_CONSTRUCTOR(SetcolorstoreactionoptionsAtindex, C)
	, INTERPOSE_CONSTRUCTOR(Setdepthstoreactionoptions, C)
	, INTERPOSE_CONSTRUCTOR(Setstencilstoreactionoptions, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLParallelRenderCommandEncoder>, renderCommandEncoder,	RenderCommandEncoder,	id <MTLRenderCommandEncoder>);
	INTERPOSE_SELECTOR(id<MTLParallelRenderCommandEncoder>, setColorStoreAction:atIndex:,	SetcolorstoreactionAtindex,	void, MTLStoreAction,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLParallelRenderCommandEncoder>, setDepthStoreAction:,	Setdepthstoreaction,	void, MTLStoreAction);
	INTERPOSE_SELECTOR(id<MTLParallelRenderCommandEncoder>, setStencilStoreAction:,	Setstencilstoreaction,	void, MTLStoreAction);
	INTERPOSE_SELECTOR(id<MTLParallelRenderCommandEncoder>, setColorStoreActionOptions:atIndex:,	SetcolorstoreactionoptionsAtindex,	void, MTLStoreActionOptions,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLParallelRenderCommandEncoder>, setDepthStoreActionOptions:,	Setdepthstoreactionoptions,	void, MTLStoreActionOptions);
	INTERPOSE_SELECTOR(id<MTLParallelRenderCommandEncoder>, setStencilStoreActionOptions:,	Setstencilstoreactionoptions,	void, MTLStoreActionOptions);
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<id<MTLParallelRenderCommandEncoder>, InterposeClass> : public IMPTable<id<MTLParallelRenderCommandEncoder>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLParallelRenderCommandEncoder>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableCommandEncoder<id<MTLParallelRenderCommandEncoder>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(RenderCommandEncoder, C);
		INTERPOSE_REGISTRATION(SetcolorstoreactionAtindex, C);
		INTERPOSE_REGISTRATION(Setdepthstoreaction, C);
		INTERPOSE_REGISTRATION(Setstencilstoreaction, C);
		INTERPOSE_REGISTRATION(SetcolorstoreactionoptionsAtindex, C);
		INTERPOSE_REGISTRATION(Setdepthstoreactionoptions, C);
		INTERPOSE_REGISTRATION(Setstencilstoreactionoptions, C);
	}
};

MTLPP_END

#endif /* imp_ParallelRenderCommandEncoder_hpp */
