// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_ComputePipeline_hpp
#define imp_ComputePipeline_hpp

#include "imp_State.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<MTLComputePipelineDescriptor*, void> : public IMPTableBase<MTLComputePipelineDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLComputePipelineDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(label, C)
	, INTERPOSE_CONSTRUCTOR(setLabel, C)
	, INTERPOSE_CONSTRUCTOR(computeFunction, C)
	, INTERPOSE_CONSTRUCTOR(setComputeFunction, C)
	, INTERPOSE_CONSTRUCTOR(threadGroupSizeIsMultipleOfThreadExecutionWidth, C)
	, INTERPOSE_CONSTRUCTOR(setThreadGroupSizeIsMultipleOfThreadExecutionWidth, C)
	, INTERPOSE_CONSTRUCTOR(stageInputDescriptor, C)
	, INTERPOSE_CONSTRUCTOR(setStageInputDescriptor, C)
	, INTERPOSE_CONSTRUCTOR(buffers, C)
	, INTERPOSE_CONSTRUCTOR(setBuffers, C)
	, INTERPOSE_CONSTRUCTOR(maxTotalThreadsPerThreadgroup, C)
	, INTERPOSE_CONSTRUCTOR(setMaxTotalThreadsPerThreadgroup, C)
	, INTERPOSE_CONSTRUCTOR(reset, C)
	{
	}
	
	INTERPOSE_SELECTOR(MTLComputePipelineDescriptor*, label, label, NSString *);
	INTERPOSE_SELECTOR(MTLComputePipelineDescriptor*, setLabel:, setLabel, void, NSString *);
	
	INTERPOSE_SELECTOR(MTLComputePipelineDescriptor*, computeFunction, computeFunction, id <MTLFunction>);
	INTERPOSE_SELECTOR(MTLComputePipelineDescriptor*, setComputeFunction:, setComputeFunction, void, id <MTLFunction>);
	
	INTERPOSE_SELECTOR(MTLComputePipelineDescriptor*, threadGroupSizeIsMultipleOfThreadExecutionWidth, threadGroupSizeIsMultipleOfThreadExecutionWidth, BOOL);
	INTERPOSE_SELECTOR(MTLComputePipelineDescriptor*, setThreadGroupSizeIsMultipleOfThreadExecutionWidth:, setThreadGroupSizeIsMultipleOfThreadExecutionWidth, void, BOOL);
	
	INTERPOSE_SELECTOR(MTLComputePipelineDescriptor*, stageInputDescriptor, stageInputDescriptor, MTLStageInputOutputDescriptor*);
	INTERPOSE_SELECTOR(MTLComputePipelineDescriptor*, setStageInputDescriptor:, setStageInputDescriptor, void, MTLStageInputOutputDescriptor*);
	
	INTERPOSE_SELECTOR(MTLComputePipelineDescriptor*, buffers, buffers, MTLPipelineBufferDescriptorArray*);
	INTERPOSE_SELECTOR(MTLComputePipelineDescriptor*, setBuffers:, setBuffers, void, MTLPipelineBufferDescriptorArray*);
	
	INTERPOSE_SELECTOR(MTLComputePipelineDescriptor*, maxTotalThreadsPerThreadgroup, maxTotalThreadsPerThreadgroup, NSUInteger);
	INTERPOSE_SELECTOR(MTLComputePipelineDescriptor*, setMaxTotalThreadsPerThreadgroup:, setMaxTotalThreadsPerThreadgroup, void, NSUInteger);
	
	INTERPOSE_SELECTOR(MTLComputePipelineDescriptor*, reset, reset, void);
};

template<>
struct MTLPP_EXPORT IMPTable<id<MTLComputePipelineState>, void> : public IMPTableState<id<MTLComputePipelineState>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableState<id<MTLComputePipelineState>>(C)
	, INTERPOSE_CONSTRUCTOR(maxTotalThreadsPerThreadgroup, C)
	, INTERPOSE_CONSTRUCTOR(threadExecutionWidth, C)
	, INTERPOSE_CONSTRUCTOR(staticThreadgroupMemoryLength, C)
	, INTERPOSE_CONSTRUCTOR(imageblockMemoryLengthForDimensions, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLComputePipelineState>, maxTotalThreadsPerThreadgroup, maxTotalThreadsPerThreadgroup, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLComputePipelineState>, threadExecutionWidth, threadExecutionWidth, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLComputePipelineState>, staticThreadgroupMemoryLength, staticThreadgroupMemoryLength, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLComputePipelineState>, imageblockMemoryLengthForDimensions:, imageblockMemoryLengthForDimensions,NSUInteger,MTLPPSize);
};

template<typename InterposeClass>
struct IMPTable<id<MTLComputePipelineState>, InterposeClass> : public IMPTable<id<MTLComputePipelineState>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLComputePipelineState>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableState<id<MTLComputePipelineState>>::RegisterInterpose<InterposeClass>(C);
	}
};

MTLPP_END

#endif /* imp_ComputePipeline_hpp */
