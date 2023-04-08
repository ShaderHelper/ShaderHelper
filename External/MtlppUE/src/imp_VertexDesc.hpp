// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_VertexDesc_hpp
#define imp_VertexDesc_hpp

#include "imp_Resource.hpp"

MTLPP_BEGIN

template<>
struct IMPTable<MTLVertexBufferLayoutDescriptor*, void> : public IMPTableBase<MTLVertexBufferLayoutDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLVertexBufferLayoutDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(stride, C)
	, INTERPOSE_CONSTRUCTOR(setStride, C)
	, INTERPOSE_CONSTRUCTOR(stepFunction, C)
	, INTERPOSE_CONSTRUCTOR(setStepFunction, C)
	, INTERPOSE_CONSTRUCTOR(stepRate, C)
	, INTERPOSE_CONSTRUCTOR(setStepRate, C)
	
	{
	}
	
	INTERPOSE_SELECTOR(MTLVertexBufferLayoutDescriptor*, stride, stride, NSUInteger);
	INTERPOSE_SELECTOR(MTLVertexBufferLayoutDescriptor*, setStride:, setStride, void, NSUInteger);
	
	INTERPOSE_SELECTOR(MTLVertexBufferLayoutDescriptor*, stepFunction, stepFunction, MTLVertexStepFunction);
	INTERPOSE_SELECTOR(MTLVertexBufferLayoutDescriptor*, setStepFunction:, setStepFunction, void, MTLVertexStepFunction);
	
	INTERPOSE_SELECTOR(MTLVertexBufferLayoutDescriptor*, stepRate, stepRate, NSUInteger);
	INTERPOSE_SELECTOR(MTLVertexBufferLayoutDescriptor*, setStepRate:, setStepRate, void, NSUInteger);
};

template<>
struct IMPTable<MTLVertexAttributeDescriptor*, void> : public IMPTableBase<MTLVertexAttributeDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLVertexAttributeDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(format, C)
	, INTERPOSE_CONSTRUCTOR(setFormat, C)
	, INTERPOSE_CONSTRUCTOR(offset, C)
	, INTERPOSE_CONSTRUCTOR(setOffset, C)
	, INTERPOSE_CONSTRUCTOR(bufferIndex, C)
	, INTERPOSE_CONSTRUCTOR(setBufferIndex, C)
	
	{
	}
	
	INTERPOSE_SELECTOR(MTLVertexAttributeDescriptor*, format, format, MTLVertexFormat);
	INTERPOSE_SELECTOR(MTLVertexAttributeDescriptor*, setFormat:, setFormat, void, MTLVertexFormat);
	
	INTERPOSE_SELECTOR(MTLVertexAttributeDescriptor*, offset, offset, NSUInteger);
	INTERPOSE_SELECTOR(MTLVertexAttributeDescriptor*, setOffset:, setOffset, void, NSUInteger);
	
	INTERPOSE_SELECTOR(MTLVertexAttributeDescriptor*, bufferIndex, bufferIndex, NSUInteger);
	INTERPOSE_SELECTOR(MTLVertexAttributeDescriptor*, setBufferIndex:, setBufferIndex, void, NSUInteger);
};

template<>
struct IMPTable<MTLVertexDescriptor*, void> : public IMPTableBase<MTLVertexDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLVertexDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(layouts, C)
	, INTERPOSE_CONSTRUCTOR(attributes, C)
	, INTERPOSE_CONSTRUCTOR(reset, C)

	{
	}
	
	INTERPOSE_SELECTOR(MTLVertexDescriptor*, layouts, layouts, MTLVertexBufferLayoutDescriptorArray*);
	INTERPOSE_SELECTOR(MTLVertexDescriptor*, attributes, attributes, MTLVertexAttributeDescriptorArray*);
	INTERPOSE_SELECTOR(MTLVertexDescriptor*, reset, reset, void);
};

MTLPP_END

#endif /* imp_VertexDesc_hpp */
