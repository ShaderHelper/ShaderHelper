// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_StageInputOutputDescriptor_hpp
#define imp_StageInputOutputDescriptor_hpp

#include "imp_Resource.hpp"

MTLPP_BEGIN

template<>
struct IMPTable<MTLBufferLayoutDescriptor*, void> : public IMPTableBase<MTLBufferLayoutDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLBufferLayoutDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(stride, C)
	, INTERPOSE_CONSTRUCTOR(setStride, C)
	, INTERPOSE_CONSTRUCTOR(stepFunction, C)
	, INTERPOSE_CONSTRUCTOR(setStepFunction, C)
	, INTERPOSE_CONSTRUCTOR(stepRate, C)
	, INTERPOSE_CONSTRUCTOR(setStepRate, C)
	
	{
	}
	
	INTERPOSE_SELECTOR(MTLBufferLayoutDescriptor*, stride, stride, NSUInteger);
	INTERPOSE_SELECTOR(MTLBufferLayoutDescriptor*, setStride:, setStride, void, NSUInteger);
	
	INTERPOSE_SELECTOR(MTLBufferLayoutDescriptor*, stepFunction, stepFunction, MTLStepFunction);
	INTERPOSE_SELECTOR(MTLBufferLayoutDescriptor*, setStepFunction:, setStepFunction, void, MTLStepFunction);
	
	INTERPOSE_SELECTOR(MTLBufferLayoutDescriptor*, stepRate, stepRate, NSUInteger);
	INTERPOSE_SELECTOR(MTLBufferLayoutDescriptor*, setStepRate:, setStepRate, void, NSUInteger);
};

template<>
struct IMPTable<MTLAttributeDescriptor*, void> : public IMPTableBase<MTLAttributeDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLAttributeDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(format, C)
	, INTERPOSE_CONSTRUCTOR(setFormat, C)
	, INTERPOSE_CONSTRUCTOR(offset, C)
	, INTERPOSE_CONSTRUCTOR(setOffset, C)
	, INTERPOSE_CONSTRUCTOR(bufferIndex, C)
	, INTERPOSE_CONSTRUCTOR(setBufferIndex, C)
	
	{
	}
	
	INTERPOSE_SELECTOR(MTLAttributeDescriptor*, format, format, MTLAttributeFormat);
	INTERPOSE_SELECTOR(MTLAttributeDescriptor*, setFormat:, setFormat, void, MTLAttributeFormat);
	
	INTERPOSE_SELECTOR(MTLAttributeDescriptor*, offset, offset, NSUInteger);
	INTERPOSE_SELECTOR(MTLAttributeDescriptor*, setOffset:, setOffset, void, NSUInteger);
	
	INTERPOSE_SELECTOR(MTLAttributeDescriptor*, bufferIndex, bufferIndex, NSUInteger);
	INTERPOSE_SELECTOR(MTLAttributeDescriptor*, setBufferIndex:, setBufferIndex, void, NSUInteger);
};

template<>
struct IMPTable<MTLStageInputOutputDescriptor*, void> : public IMPTableBase<MTLStageInputOutputDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLStageInputOutputDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(indexType, C)
	, INTERPOSE_CONSTRUCTOR(setIndexType, C)
	, INTERPOSE_CONSTRUCTOR(indexBufferIndex, C)
	, INTERPOSE_CONSTRUCTOR(setIndexBufferIndex, C)
	, INTERPOSE_CONSTRUCTOR(layouts, C)
	, INTERPOSE_CONSTRUCTOR(attributes, C)
	, INTERPOSE_CONSTRUCTOR(reset, C)
	{
	}
	
	INTERPOSE_SELECTOR(MTLStageInputOutputDescriptor*, indexType, indexType, MTLIndexType);
	INTERPOSE_SELECTOR(MTLStageInputOutputDescriptor*, setIndexType:, setIndexType, void, MTLIndexType);

	INTERPOSE_SELECTOR(MTLStageInputOutputDescriptor*, indexBufferIndex, indexBufferIndex, NSUInteger);
	INTERPOSE_SELECTOR(MTLStageInputOutputDescriptor*, setIndexBufferIndex:, setIndexBufferIndex, void, NSUInteger);

	INTERPOSE_SELECTOR(MTLStageInputOutputDescriptor*, layouts, layouts, MTLBufferLayoutDescriptorArray*);
	INTERPOSE_SELECTOR(MTLStageInputOutputDescriptor*, attributes, attributes, MTLAttributeDescriptorArray*);
	INTERPOSE_SELECTOR(MTLStageInputOutputDescriptor*, reset, reset, void);
};

MTLPP_END

#endif /* imp_StageInputOutputDescriptor_hpp */
