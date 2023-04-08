// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_CommandBuffer_hpp
#define imp_CommandBuffer_hpp

#include "imp_Object.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<id<MTLCommandBuffer>, void> : public IMPTableBase<id<MTLCommandBuffer>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<id<MTLCommandBuffer>>(C)
	, INTERPOSE_CONSTRUCTOR(Device, C)
	, INTERPOSE_CONSTRUCTOR(CommandQueue, C)
	, INTERPOSE_CONSTRUCTOR(RetainedReferences, C)
	, INTERPOSE_CONSTRUCTOR(Label, C)
	, INTERPOSE_CONSTRUCTOR(SetLabel, C)
	, INTERPOSE_CONSTRUCTOR(KernelStartTime, C)
	, INTERPOSE_CONSTRUCTOR(KernelEndTime, C)
	, INTERPOSE_CONSTRUCTOR(GPUStartTime, C)
	, INTERPOSE_CONSTRUCTOR(GPUEndTime, C)
	, INTERPOSE_CONSTRUCTOR(Enqueue, C)
	, INTERPOSE_CONSTRUCTOR(Commit, C)
	, INTERPOSE_CONSTRUCTOR(AddScheduledHandler, C)
	, INTERPOSE_CONSTRUCTOR(PresentDrawable, C)
	, INTERPOSE_CONSTRUCTOR(PresentDrawableAtTime, C)
	, INTERPOSE_CONSTRUCTOR(WaitUntilScheduled, C)
	, INTERPOSE_CONSTRUCTOR(AddCompletedHandler, C)
	, INTERPOSE_CONSTRUCTOR(WaitUntilCompleted, C)
	, INTERPOSE_CONSTRUCTOR(Status, C)
	, INTERPOSE_CONSTRUCTOR(Error, C)
	, INTERPOSE_CONSTRUCTOR(BlitCommandEncoder, C)
	, INTERPOSE_CONSTRUCTOR(RenderCommandEncoderWithDescriptor, C)
	, INTERPOSE_CONSTRUCTOR(ComputeCommandEncoder, C)
	, INTERPOSE_CONSTRUCTOR(ComputeCommandEncoderWithType, C)
	, INTERPOSE_CONSTRUCTOR(ParallelRenderCommandEncoderWithDescriptor, C)
	, INTERPOSE_CONSTRUCTOR(PushDebugGroup, C)
	, INTERPOSE_CONSTRUCTOR(PopDebugGroup, C)
	, INTERPOSE_CONSTRUCTOR(PresentDrawableAfterMinimumDuration, C)
	{
	}

	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, device, Device, id<MTLDevice>);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, commandQueue, CommandQueue, id<MTLCommandQueue>);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, retainedReferences, RetainedReferences, BOOL);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, label, Label, NSString*);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, setLabel:, SetLabel, void, NSString*);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, kernelStartTime, KernelStartTime, CFTimeInterval);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, kernelEndTime, KernelEndTime, CFTimeInterval);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, GPUStartTime, GPUStartTime, CFTimeInterval);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, GPUEndTime, GPUEndTime, CFTimeInterval);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, enqueue, Enqueue, void);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, commit, Commit, void);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, addScheduledHandler:, AddScheduledHandler, void, MTLCommandBufferHandler);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, presentDrawable:, PresentDrawable, void, id<MTLDrawable>);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, presentDrawable:atTime:, PresentDrawableAtTime, void, id<MTLDrawable>, CFTimeInterval);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, waitUntilScheduled, WaitUntilScheduled, void);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, addCompletedHandler:, AddCompletedHandler, void, MTLCommandBufferHandler);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, waitUntilCompleted, WaitUntilCompleted, void);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, status, Status, MTLCommandBufferStatus);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, error, Error, NSError *);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, blitCommandEncoder, BlitCommandEncoder, id<MTLBlitCommandEncoder>);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, renderCommandEncoderWithDescriptor:, RenderCommandEncoderWithDescriptor, id<MTLRenderCommandEncoder>, MTLRenderPassDescriptor*);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, computeCommandEncoder, ComputeCommandEncoder, id<MTLComputeCommandEncoder>);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, computeCommandEncoderWithDispatchType:, ComputeCommandEncoderWithType, id<MTLComputeCommandEncoder>, MTLDispatchType);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, parallelRenderCommandEncoderWithDescriptor:, ParallelRenderCommandEncoderWithDescriptor, id<MTLParallelRenderCommandEncoder>, MTLRenderPassDescriptor*);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, pushDebugGroup:, PushDebugGroup, void, NSString*);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, popDebugGroup, PopDebugGroup, void);
	INTERPOSE_SELECTOR(id<MTLCommandBuffer>, presentDrawable:afterMinimumDuration:, PresentDrawableAfterMinimumDuration, void, id <MTLDrawable>, CFTimeInterval);
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<id<MTLCommandBuffer>, InterposeClass> : public IMPTable<id<MTLCommandBuffer>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLCommandBuffer>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableBase<id<MTLCommandBuffer>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(SetLabel, C);
		INTERPOSE_REGISTRATION(Enqueue, C);
		INTERPOSE_REGISTRATION(Commit, C);
		INTERPOSE_REGISTRATION(AddScheduledHandler, C);
		INTERPOSE_REGISTRATION(PresentDrawable, C);
		INTERPOSE_REGISTRATION(PresentDrawableAtTime, C);
		INTERPOSE_REGISTRATION(WaitUntilScheduled, C);
		INTERPOSE_REGISTRATION(AddCompletedHandler, C);
		INTERPOSE_REGISTRATION(WaitUntilCompleted, C);
		INTERPOSE_REGISTRATION(BlitCommandEncoder, C);
		INTERPOSE_REGISTRATION(RenderCommandEncoderWithDescriptor, C);
		INTERPOSE_REGISTRATION(ComputeCommandEncoder, C);
		INTERPOSE_REGISTRATION(ComputeCommandEncoderWithType, C);
		INTERPOSE_REGISTRATION(ParallelRenderCommandEncoderWithDescriptor, C);
		INTERPOSE_REGISTRATION(PushDebugGroup, C);
		INTERPOSE_REGISTRATION(PopDebugGroup, C);
		INTERPOSE_REGISTRATION(PresentDrawableAfterMinimumDuration, C);
	}
};

MTLPP_END

#endif /* imp_CommandBuffer_hpp */
