// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_ComputeCommandEncoder_hpp
#define imp_ComputeCommandEncoder_hpp

#include "imp_CommandEncoder.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<id<MTLComputeCommandEncoder>, void> : public IMPTableCommandEncoder<id<MTLComputeCommandEncoder>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableCommandEncoder<id<MTLComputeCommandEncoder>>(C)
	, INTERPOSE_CONSTRUCTOR(Setcomputepipelinestate, C)
	, INTERPOSE_CONSTRUCTOR(Setbyteslengthatindex, C)
	, INTERPOSE_CONSTRUCTOR(Setbufferoffsetatindex, C)
	, INTERPOSE_CONSTRUCTOR(SetBufferOffsetatindex, C)
	, INTERPOSE_CONSTRUCTOR(Setbuffersoffsetswithrange, C)
	, INTERPOSE_CONSTRUCTOR(Settextureatindex, C)
	, INTERPOSE_CONSTRUCTOR(Settextureswithrange, C)
	, INTERPOSE_CONSTRUCTOR(Setsamplerstateatindex, C)
	, INTERPOSE_CONSTRUCTOR(Setsamplerstateswithrange, C)
	, INTERPOSE_CONSTRUCTOR(Setsamplerstatelodminclamplodmaxclampatindex, C)
	, INTERPOSE_CONSTRUCTOR(Setsamplerstateslodminclampslodmaxclampswithrange, C)
	, INTERPOSE_CONSTRUCTOR(Setthreadgroupmemorylengthatindex, C)
	, INTERPOSE_CONSTRUCTOR(Setstageinregion, C)
	, INTERPOSE_CONSTRUCTOR(Dispatchthreadgroupsthreadsperthreadgroup, C)
	, INTERPOSE_CONSTRUCTOR(Dispatchthreadgroupswithindirectbufferindirectbufferoffsetthreadsperthreadgroup, C)
	, INTERPOSE_CONSTRUCTOR(Dispatchthreadsthreadsperthreadgroup, C)
	, INTERPOSE_CONSTRUCTOR(Updatefence, C)
	, INTERPOSE_CONSTRUCTOR(Waitforfence, C)
	, INTERPOSE_CONSTRUCTOR(Useresourceusage, C)
	, INTERPOSE_CONSTRUCTOR(Useresourcescountusage, C)
	, INTERPOSE_CONSTRUCTOR(Useheap, C)
	, INTERPOSE_CONSTRUCTOR(Useheapscount, C)
	, INTERPOSE_CONSTRUCTOR(SetImageblockWidthHeight, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, setComputePipelineState:, Setcomputepipelinestate, void, id <MTLComputePipelineState>);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, setBytes:length:atIndex:, Setbyteslengthatindex, void, const void *, NSUInteger, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, setBuffer:offset:atIndex:, Setbufferoffsetatindex, void,  id <MTLBuffer>, NSUInteger, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, setBufferOffset:atIndex:, SetBufferOffsetatindex, void, NSUInteger, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, setBuffers:offsets:withRange:, Setbuffersoffsetswithrange, void, const id <MTLBuffer>  *, const NSUInteger *, NSRange);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, setTexture:atIndex:, Settextureatindex, void,  id <MTLTexture>, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, setTextures:withRange:, Settextureswithrange, void, const id <MTLTexture>  *, NSRange);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, setSamplerState:atIndex:, Setsamplerstateatindex, void,  id <MTLSamplerState>, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, setSamplerStates:withRange:, Setsamplerstateswithrange, void, const id <MTLSamplerState>  *, NSRange);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, setSamplerState:lodMinClamp:lodMaxClamp:atIndex:, Setsamplerstatelodminclamplodmaxclampatindex, void,  id <MTLSamplerState>, float, float, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, setSamplerStates:lodMinClamps:lodMaxClamps:withRange:, Setsamplerstateslodminclampslodmaxclampswithrange, void, const id <MTLSamplerState>  *, const float *, const float *, NSRange);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, setThreadgroupMemoryLength:atIndex:, Setthreadgroupmemorylengthatindex, void, NSUInteger, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, setStageInRegion:, Setstageinregion, void, MTLPPRegion);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, dispatchThreadgroups:threadsPerThreadgroup:, Dispatchthreadgroupsthreadsperthreadgroup, void, MTLPPSize, MTLPPSize);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, dispatchThreadgroupsWithIndirectBuffer:indirectBufferOffset:threadsPerThreadgroup:, Dispatchthreadgroupswithindirectbufferindirectbufferoffsetthreadsperthreadgroup, void, id <MTLBuffer>, NSUInteger, MTLPPSize);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, dispatchThreads:threadsPerThreadgroup:, Dispatchthreadsthreadsperthreadgroup, void, MTLPPSize, MTLPPSize);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, updateFence:, Updatefence, void, id <MTLFence>);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, waitForFence:, Waitforfence, void, id <MTLFence>);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, useResource:usage:, Useresourceusage, void, id <MTLResource>, MTLResourceUsage);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, useResources:count:usage:, Useresourcescountusage, void, const id <MTLResource> *, NSUInteger, MTLResourceUsage);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, useHeap:, Useheap, void, id <MTLHeap>);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, useHeaps:count:, Useheapscount, void, const id <MTLHeap> *, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLComputeCommandEncoder>, setImageblockWidth:height:, SetImageblockWidthHeight, void, NSUInteger, NSUInteger);
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<id<MTLComputeCommandEncoder>, InterposeClass> : public IMPTable<id<MTLComputeCommandEncoder>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLComputeCommandEncoder>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableCommandEncoder<id<MTLComputeCommandEncoder>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(Setcomputepipelinestate, C);
		INTERPOSE_REGISTRATION(Setbyteslengthatindex, C);
		INTERPOSE_REGISTRATION(Setbufferoffsetatindex, C);
		INTERPOSE_REGISTRATION(SetBufferOffsetatindex, C);
		INTERPOSE_REGISTRATION(Setbuffersoffsetswithrange, C);
		INTERPOSE_REGISTRATION(Settextureatindex, C);
		INTERPOSE_REGISTRATION(Settextureswithrange, C);
		INTERPOSE_REGISTRATION(Setsamplerstateatindex, C);
		INTERPOSE_REGISTRATION(Setsamplerstateswithrange, C);
		INTERPOSE_REGISTRATION(Setsamplerstatelodminclamplodmaxclampatindex, C);
		INTERPOSE_REGISTRATION(Setsamplerstateslodminclampslodmaxclampswithrange, C);
		INTERPOSE_REGISTRATION(Setthreadgroupmemorylengthatindex, C);
		INTERPOSE_REGISTRATION(Setstageinregion, C);
		INTERPOSE_REGISTRATION(Dispatchthreadgroupsthreadsperthreadgroup, C);
		INTERPOSE_REGISTRATION(Dispatchthreadgroupswithindirectbufferindirectbufferoffsetthreadsperthreadgroup, C);
		INTERPOSE_REGISTRATION(Dispatchthreadsthreadsperthreadgroup, C);
		INTERPOSE_REGISTRATION(Updatefence, C);
		INTERPOSE_REGISTRATION(Waitforfence, C);
		INTERPOSE_REGISTRATION(Useresourceusage, C);
		INTERPOSE_REGISTRATION(Useresourcescountusage, C);
		INTERPOSE_REGISTRATION(Useheap, C);
		INTERPOSE_REGISTRATION(Useheapscount, C);
		INTERPOSE_REGISTRATION(SetImageblockWidthHeight, C);
	}
};

MTLPP_END

#endif /* imp_ComputeCommandEncoder_hpp */
