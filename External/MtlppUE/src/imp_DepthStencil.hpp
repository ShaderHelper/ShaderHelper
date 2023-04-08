// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_DepthStencil_hpp
#define imp_DepthStencil_hpp

#include "imp_State.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<MTLStencilDescriptor*, void> : public IMPTableBase<MTLStencilDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLStencilDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(stencilCompareFunction, C)
	, INTERPOSE_CONSTRUCTOR(stencilFailureOperation, C)
	, INTERPOSE_CONSTRUCTOR(depthFailureOperation, C)
	, INTERPOSE_CONSTRUCTOR(depthStencilPassOperation, C)
	, INTERPOSE_CONSTRUCTOR(readMask, C)
	, INTERPOSE_CONSTRUCTOR(writeMask, C)
	, INTERPOSE_CONSTRUCTOR(setStencilCompareFunction, C)
	, INTERPOSE_CONSTRUCTOR(setStencilFailureOperation, C)
	, INTERPOSE_CONSTRUCTOR(setDepthFailureOperation, C)
	, INTERPOSE_CONSTRUCTOR(setDepthStencilPassOperation, C)
	, INTERPOSE_CONSTRUCTOR(setReadMask, C)
	, INTERPOSE_CONSTRUCTOR(setWriteMask, C)
	{
	}
	
	INTERPOSE_SELECTOR(MTLStencilDescriptor*, stencilCompareFunction, stencilCompareFunction, MTLCompareFunction);
	INTERPOSE_SELECTOR(MTLStencilDescriptor*, stencilFailureOperation, stencilFailureOperation, MTLStencilOperation);
	INTERPOSE_SELECTOR(MTLStencilDescriptor*, depthFailureOperation, depthFailureOperation, MTLStencilOperation);
	INTERPOSE_SELECTOR(MTLStencilDescriptor*, depthStencilPassOperation, depthStencilPassOperation, MTLStencilOperation);
	INTERPOSE_SELECTOR(MTLStencilDescriptor*, readMask, readMask, uint32_t);
	INTERPOSE_SELECTOR(MTLStencilDescriptor*, writeMask, writeMask, uint32_t);
	
	INTERPOSE_SELECTOR(MTLStencilDescriptor*, setStencilCompareFunction:, setStencilCompareFunction, void, MTLCompareFunction);
	INTERPOSE_SELECTOR(MTLStencilDescriptor*, setStencilFailureOperation:, setStencilFailureOperation, void, MTLStencilOperation);
	INTERPOSE_SELECTOR(MTLStencilDescriptor*, setDepthFailureOperation:, setDepthFailureOperation, void, MTLStencilOperation);
	INTERPOSE_SELECTOR(MTLStencilDescriptor*, setDepthStencilPassOperation:, setDepthStencilPassOperation, void, MTLStencilOperation);
	INTERPOSE_SELECTOR(MTLStencilDescriptor*, setReadMask:, setReadMask, void, uint32_t);
	INTERPOSE_SELECTOR(MTLStencilDescriptor*, setWriteMask:, setWriteMask, void, uint32_t);
};

template<>
struct MTLPP_EXPORT IMPTable<MTLDepthStencilDescriptor*, void> : public IMPTableBase<MTLDepthStencilDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLDepthStencilDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(depthCompareFunction, C)
	, INTERPOSE_CONSTRUCTOR(isDepthWriteEnabled, C)
	, INTERPOSE_CONSTRUCTOR(frontFaceStencil, C)
	, INTERPOSE_CONSTRUCTOR(backFaceStencil, C)
	, INTERPOSE_CONSTRUCTOR(label, C)
	, INTERPOSE_CONSTRUCTOR(setDepthCompareFunction, C)
	, INTERPOSE_CONSTRUCTOR(setDepthWriteEnabled, C)
	, INTERPOSE_CONSTRUCTOR(setFrontFaceStencil, C)
	, INTERPOSE_CONSTRUCTOR(setBackFaceStencil, C)
	, INTERPOSE_CONSTRUCTOR(setLabel, C)
	{
	}
	
	INTERPOSE_SELECTOR(MTLDepthStencilDescriptor*, depthCompareFunction, depthCompareFunction, MTLCompareFunction);
	INTERPOSE_SELECTOR(MTLDepthStencilDescriptor*, isDepthWriteEnabled, isDepthWriteEnabled, BOOL);
	INTERPOSE_SELECTOR(MTLDepthStencilDescriptor*, frontFaceStencil, frontFaceStencil, MTLStencilDescriptor*);
	INTERPOSE_SELECTOR(MTLDepthStencilDescriptor*, backFaceStencil, backFaceStencil, MTLStencilDescriptor*);
	INTERPOSE_SELECTOR(MTLDepthStencilDescriptor*, label, label, NSString*);

	INTERPOSE_SELECTOR(MTLDepthStencilDescriptor*, setDepthCompareFunction:, setDepthCompareFunction, void, MTLCompareFunction);
	INTERPOSE_SELECTOR(MTLDepthStencilDescriptor*, setDepthWriteEnabled:, setDepthWriteEnabled, void, BOOL);
	INTERPOSE_SELECTOR(MTLDepthStencilDescriptor*, setFrontFaceStencil:, setFrontFaceStencil, void, MTLStencilDescriptor*);
	INTERPOSE_SELECTOR(MTLDepthStencilDescriptor*, setBackFaceStencil:, setBackFaceStencil, void, MTLStencilDescriptor*);
	INTERPOSE_SELECTOR(MTLDepthStencilDescriptor*, setLabel:, setLabel, void, NSString*);
};

template<>
struct IMPTable<id<MTLDepthStencilState>, void> : public IMPTableState<id<MTLDepthStencilState>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableState<id<MTLDepthStencilState>>(C)
	{
	}
};

template<typename InterposeClass>
struct IMPTable<id<MTLDepthStencilState>, InterposeClass> : public IMPTable<id<MTLDepthStencilState>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLDepthStencilState>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableState<id<MTLDepthStencilState>>::RegisterInterpose<InterposeClass>(C);
	}
};

MTLPP_END

#endif /* imp_DepthStencil_hpp */
