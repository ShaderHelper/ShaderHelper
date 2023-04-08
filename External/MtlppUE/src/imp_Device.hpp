// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_Device_hpp
#define imp_Device_hpp

#include "imp_Object.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<id<MTLDevice>, void> : public IMPTableBase<id<MTLDevice>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<id<MTLDevice>>(C)
	, INTERPOSE_CONSTRUCTOR(name, C)
	, INTERPOSE_CONSTRUCTOR(registryID, C)
	, INTERPOSE_CONSTRUCTOR(maxThreadsPerThreadgroup, C)
	, INTERPOSE_CONSTRUCTOR(isLowPower, C)
	, INTERPOSE_CONSTRUCTOR(isHeadless, C)
	, INTERPOSE_CONSTRUCTOR(isRemovable, C)
	, INTERPOSE_CONSTRUCTOR(recommendedMaxWorkingSetSize, C)
	, INTERPOSE_CONSTRUCTOR(isDepth24Stencil8PixelFormatSupported, C)
	, INTERPOSE_CONSTRUCTOR(readWriteTextureSupport, C)
	, INTERPOSE_CONSTRUCTOR(argumentBuffersSupport, C)
	, INTERPOSE_CONSTRUCTOR(areRasterOrderGroupsSupported, C)
	, INTERPOSE_CONSTRUCTOR(currentAllocatedSize, C)
	, NewCommandQueue(C)
	, NewCommandQueueWithMaxCommandBufferCount(C)
	, NewHeapWithDescriptor(C)
	, NewBufferWithLength(C)
	, NewBufferWithBytes(C)
	, NewBufferWithBytesNoCopy(C)
	, NewDepthStencilStateWithDescriptor(C)
	, NewTextureWithDescriptor(C)
	, NewTextureWithDescriptorIOSurface(C)
	, NewSamplerStateWithDescriptor(C)
	, NewDefaultLibrary(C)
	, NewDefaultLibraryWithBundle(C)
	, NewLibraryWithFile(C)
	, NewLibraryWithURL(C)
	, NewLibraryWithData(C)
	, NewLibraryWithSourceOptionsError(C)
	, NewLibraryWithSourceOptionsCompletionHandler(C)
	, NewRenderPipelineStateWithDescriptorError(C)
	, NewRenderPipelineStateWithDescriptorOptionsReflectionError(C)
	, NewRenderPipelineStateWithDescriptorCompletionHandler(C)
	, NewRenderPipelineStateWithDescriptorOptionsCompletionHandler(C)
	, NewComputePipelineStateWithFunctionError(C)
	, NewComputePipelineStateWithFunctionOptionsReflectionError(C)
	, NewComputePipelineStateWithFunctionCompletionHandler(C)
	, NewComputePipelineStateWithFunctionOptionsCompletionHandler(C)
	, NewComputePipelineStateWithDescriptorOptionsReflectionError(C)
	, NewComputePipelineStateWithDescriptorOptionsCompletionHandler(C)
	, NewFence(C)
	, NewArgumentEncoderWithArguments(C)
	, INTERPOSE_CONSTRUCTOR(heapTextureSizeAndAlignWithDescriptor, C)
	, INTERPOSE_CONSTRUCTOR(heapBufferSizeAndAlignWithLengthoptions, C)
	, INTERPOSE_CONSTRUCTOR(maxThreadgroupMemoryLength, C)
	, INTERPOSE_CONSTRUCTOR(supportsFeatureSet, C)
	, INTERPOSE_CONSTRUCTOR(supportsTextureSampleCount, C)
	, INTERPOSE_CONSTRUCTOR(minimumLinearTextureAlignmentForPixelFormat, C)
	, INTERPOSE_CONSTRUCTOR(areProgrammableSamplePositionsSupported, C)
	, INTERPOSE_CONSTRUCTOR(getDefaultSamplePositionscount, C)
	, INTERPOSE_CONSTRUCTOR(newRenderPipelineStateWithTileDescriptoroptionsreflectionerror, C)
	, INTERPOSE_CONSTRUCTOR(newRenderPipelineStateWithTileDescriptoroptionscompletionHandler, C)
	{
	}

	INTERPOSE_SELECTOR(id<MTLDevice>, name, name, NSString *);
	INTERPOSE_SELECTOR(id<MTLDevice>, registryID, registryID, uint64_t);
	INTERPOSE_SELECTOR(id<MTLDevice>, maxThreadsPerThreadgroup, maxThreadsPerThreadgroup, MTLPPSize);
	INTERPOSE_SELECTOR(id<MTLDevice>, isLowPower, isLowPower, BOOL);
	INTERPOSE_SELECTOR(id<MTLDevice>, isHeadless, isHeadless, BOOL);
	INTERPOSE_SELECTOR(id<MTLDevice>, isRemovable, isRemovable, BOOL);
	INTERPOSE_SELECTOR(id<MTLDevice>, recommendedMaxWorkingSetSize, recommendedMaxWorkingSetSize, uint64_t);
	INTERPOSE_SELECTOR(id<MTLDevice>, isDepth24Stencil8PixelFormatSupported, isDepth24Stencil8PixelFormatSupported, BOOL);
	INTERPOSE_SELECTOR(id<MTLDevice>, readWriteTextureSupport, readWriteTextureSupport, MTLReadWriteTextureTier);
	INTERPOSE_SELECTOR(id<MTLDevice>, argumentBuffersSupport, argumentBuffersSupport, MTLArgumentBuffersTier);
	INTERPOSE_SELECTOR(id<MTLDevice>, areRasterOrderGroupsSupported, areRasterOrderGroupsSupported, BOOL);
	INTERPOSE_SELECTOR(id<MTLDevice>, currentAllocatedSize, currentAllocatedSize, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLDevice>, newCommandQueue, NewCommandQueue, id<MTLCommandQueue>);
	INTERPOSE_SELECTOR(id<MTLDevice>, newCommandQueueWithMaxCommandBufferCount:, NewCommandQueueWithMaxCommandBufferCount, id<MTLCommandQueue>, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLDevice>, newHeapWithDescriptor:, NewHeapWithDescriptor, id<MTLHeap>, MTLHeapDescriptor*);
	INTERPOSE_SELECTOR(id<MTLDevice>, newBufferWithLength:options:, NewBufferWithLength, id<MTLBuffer>, NSUInteger, MTLResourceOptions);
	INTERPOSE_SELECTOR(id<MTLDevice>, newBufferWithBytes:length:options:, NewBufferWithBytes, id<MTLBuffer>, const void*, NSUInteger, MTLResourceOptions);
	INTERPOSE_SELECTOR(id<MTLDevice>, newBufferWithBytesNoCopy:length:options:deallocator:, NewBufferWithBytesNoCopy, id<MTLBuffer>, const void*, NSUInteger, MTLResourceOptions, void (^ __nullable)(void *pointer, NSUInteger length));
	INTERPOSE_SELECTOR(id<MTLDevice>, newDepthStencilStateWithDescriptor:, NewDepthStencilStateWithDescriptor, id<MTLDepthStencilState>, MTLDepthStencilDescriptor*);
	INTERPOSE_SELECTOR(id<MTLDevice>, newTextureWithDescriptor:, NewTextureWithDescriptor, id<MTLTexture>, MTLTextureDescriptor*);
	INTERPOSE_SELECTOR(id<MTLDevice>, newTextureWithDescriptor:iosurface:plane:, NewTextureWithDescriptorIOSurface, id<MTLTexture>, MTLTextureDescriptor*, IOSurfaceRef, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLDevice>, newSamplerStateWithDescriptor:, NewSamplerStateWithDescriptor, id<MTLSamplerState>, MTLSamplerDescriptor*);
	INTERPOSE_SELECTOR(id<MTLDevice>, newDefaultLibrary, NewDefaultLibrary, id<MTLLibrary>);
	INTERPOSE_SELECTOR(id<MTLDevice>, newDefaultLibraryWithBundle:error:, NewDefaultLibraryWithBundle, id<MTLLibrary>, NSBundle*, __autoreleasing NSError **);
	INTERPOSE_SELECTOR(id<MTLDevice>, newLibraryWithFile:error:, NewLibraryWithFile, id<MTLLibrary>, NSString*, __autoreleasing NSError **);
	INTERPOSE_SELECTOR(id<MTLDevice>, newLibraryWithURL:error:, NewLibraryWithURL, id<MTLLibrary>, NSURL*, __autoreleasing NSError **);
	INTERPOSE_SELECTOR(id<MTLDevice>, newLibraryWithData:error:, NewLibraryWithData, id<MTLLibrary>, dispatch_data_t, __autoreleasing NSError **);
	INTERPOSE_SELECTOR(id<MTLDevice>, newLibraryWithSource:options:error:, NewLibraryWithSourceOptionsError, id<MTLLibrary>, NSString*, MTLCompileOptions*, NSError**);
	INTERPOSE_SELECTOR(id<MTLDevice>, newLibraryWithSource:options:completionHandler:, NewLibraryWithSourceOptionsCompletionHandler, void, NSString*, MTLCompileOptions*, MTLNewLibraryCompletionHandler);
	INTERPOSE_SELECTOR(id<MTLDevice>, newRenderPipelineStateWithDescriptor:error:, NewRenderPipelineStateWithDescriptorError, id<MTLRenderPipelineState>, MTLRenderPipelineDescriptor*, NSError**);
	INTERPOSE_SELECTOR(id<MTLDevice>, newRenderPipelineStateWithDescriptor:options:reflection:error:, NewRenderPipelineStateWithDescriptorOptionsReflectionError, id<MTLRenderPipelineState>, MTLRenderPipelineDescriptor*, MTLPipelineOption, MTLAutoreleasedRenderPipelineReflection*, NSError**);
	INTERPOSE_SELECTOR(id<MTLDevice>, newRenderPipelineStateWithDescriptor:completionHandler:, NewRenderPipelineStateWithDescriptorCompletionHandler, void, MTLRenderPipelineDescriptor*, MTLNewRenderPipelineStateCompletionHandler);
	INTERPOSE_SELECTOR(id<MTLDevice>, newRenderPipelineStateWithDescriptor:options:completionHandler:, NewRenderPipelineStateWithDescriptorOptionsCompletionHandler, void, MTLRenderPipelineDescriptor*, MTLPipelineOption, MTLNewRenderPipelineStateWithReflectionCompletionHandler);
	INTERPOSE_SELECTOR(id<MTLDevice>, newComputePipelineStateWithFunction:error:, NewComputePipelineStateWithFunctionError, id<MTLComputePipelineState>, id<MTLFunction>, NSError**);
	INTERPOSE_SELECTOR(id<MTLDevice>, newComputePipelineStateWithFunction:options:reflection:error:, NewComputePipelineStateWithFunctionOptionsReflectionError, id<MTLComputePipelineState>, id<MTLFunction>, MTLPipelineOption, MTLAutoreleasedComputePipelineReflection *, NSError**);
	INTERPOSE_SELECTOR(id<MTLDevice>, newComputePipelineStateWithFunction:completionHandler:, NewComputePipelineStateWithFunctionCompletionHandler, void, id<MTLFunction>, MTLNewComputePipelineStateCompletionHandler);
	INTERPOSE_SELECTOR(id<MTLDevice>, newComputePipelineStateWithFunction:options:completionHandler:, NewComputePipelineStateWithFunctionOptionsCompletionHandler, void, id<MTLFunction>, MTLPipelineOption, MTLNewComputePipelineStateWithReflectionCompletionHandler);
	INTERPOSE_SELECTOR(id<MTLDevice>, newComputePipelineStateWithDescriptor:options:reflection:error:, NewComputePipelineStateWithDescriptorOptionsReflectionError, id<MTLComputePipelineState>, MTLComputePipelineDescriptor*, MTLPipelineOption, MTLAutoreleasedComputePipelineReflection *, NSError**);
	INTERPOSE_SELECTOR(id<MTLDevice>, newComputePipelineStateWithDescriptor:options:completionHandler:, NewComputePipelineStateWithDescriptorOptionsCompletionHandler, void, MTLComputePipelineDescriptor*, MTLPipelineOption, MTLNewComputePipelineStateWithReflectionCompletionHandler);
	INTERPOSE_SELECTOR(id<MTLDevice>, newFence, NewFence, id<MTLFence>);
	INTERPOSE_SELECTOR(id<MTLDevice>, newArgumentEncoderWithArguments:, NewArgumentEncoderWithArguments, id<MTLArgumentEncoder>, NSArray <MTLArgumentDescriptor *> *);
	INTERPOSE_SELECTOR(id<MTLDevice>, heapTextureSizeAndAlignWithDescriptor:, heapTextureSizeAndAlignWithDescriptor, MTLPPSizeAndAlign, MTLTextureDescriptor*);
	INTERPOSE_SELECTOR(id<MTLDevice>, heapBufferSizeAndAlignWithLength:options:, heapBufferSizeAndAlignWithLengthoptions, MTLPPSizeAndAlign, NSUInteger, MTLResourceOptions);
	INTERPOSE_SELECTOR(id<MTLDevice>, maxThreadgroupMemoryLength, maxThreadgroupMemoryLength, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLDevice>, supportsFeatureSet:, supportsFeatureSet, BOOL, MTLFeatureSet);
	INTERPOSE_SELECTOR(id<MTLDevice>, supportsTextureSampleCount:, supportsTextureSampleCount, BOOL, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLDevice>, minimumLinearTextureAlignmentForPixelFormat:, minimumLinearTextureAlignmentForPixelFormat, NSUInteger, MTLPixelFormat);
	INTERPOSE_SELECTOR(id<MTLDevice>, areProgrammableSamplePositionsSupported, areProgrammableSamplePositionsSupported, BOOL);
	INTERPOSE_SELECTOR(id<MTLDevice>, getDefaultSamplePositions:count:, getDefaultSamplePositionscount, void, MTLPPSamplePosition*, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLDevice>, newRenderPipelineStateWithTileDescriptor:options:reflection:error:, newRenderPipelineStateWithTileDescriptoroptionsreflectionerror, id <MTLRenderPipelineState>, MTLTileRenderPipelineDescriptor*, MTLPipelineOption, MTLAutoreleasedRenderPipelineReflection*, NSError**);
	INTERPOSE_SELECTOR(id<MTLDevice>, newRenderPipelineStateWithTileDescriptor:options:completionHandler:, newRenderPipelineStateWithTileDescriptoroptionscompletionHandler, void, MTLTileRenderPipelineDescriptor*, MTLPipelineOption, MTLNewRenderPipelineStateWithReflectionCompletionHandler);
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<id<MTLDevice>, InterposeClass> : public IMPTable<id<MTLDevice>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLDevice>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableBase<id<MTLDevice>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(NewCommandQueue, C);
		INTERPOSE_REGISTRATION(NewCommandQueueWithMaxCommandBufferCount, C);
		INTERPOSE_REGISTRATION(NewHeapWithDescriptor, C);
		INTERPOSE_REGISTRATION(NewBufferWithLength, C);
		INTERPOSE_REGISTRATION(NewBufferWithBytes, C);
		INTERPOSE_REGISTRATION(NewBufferWithBytesNoCopy, C);
		INTERPOSE_REGISTRATION(NewDepthStencilStateWithDescriptor, C);
		INTERPOSE_REGISTRATION(NewTextureWithDescriptor, C);
		INTERPOSE_REGISTRATION(NewTextureWithDescriptorIOSurface, C);
		INTERPOSE_REGISTRATION(NewSamplerStateWithDescriptor, C);
		INTERPOSE_REGISTRATION(NewDefaultLibrary, C);
		INTERPOSE_REGISTRATION(NewDefaultLibraryWithBundle, C);
		INTERPOSE_REGISTRATION(NewLibraryWithFile, C);
		INTERPOSE_REGISTRATION(NewLibraryWithURL, C);
		INTERPOSE_REGISTRATION(NewLibraryWithData, C);
		INTERPOSE_REGISTRATION(NewLibraryWithSourceOptionsError, C);
		INTERPOSE_REGISTRATION(NewLibraryWithSourceOptionsCompletionHandler, C);
		INTERPOSE_REGISTRATION(NewRenderPipelineStateWithDescriptorError, C);
		INTERPOSE_REGISTRATION(NewRenderPipelineStateWithDescriptorOptionsReflectionError, C);
		INTERPOSE_REGISTRATION(NewRenderPipelineStateWithDescriptorCompletionHandler, C);
		INTERPOSE_REGISTRATION(NewRenderPipelineStateWithDescriptorOptionsCompletionHandler, C);
		INTERPOSE_REGISTRATION(NewComputePipelineStateWithFunctionError, C);
		INTERPOSE_REGISTRATION(NewComputePipelineStateWithFunctionOptionsReflectionError, C);
		INTERPOSE_REGISTRATION(NewComputePipelineStateWithFunctionCompletionHandler, C);
		INTERPOSE_REGISTRATION(NewComputePipelineStateWithFunctionOptionsCompletionHandler, C);
		INTERPOSE_REGISTRATION(NewComputePipelineStateWithDescriptorOptionsReflectionError, C);
		INTERPOSE_REGISTRATION(NewComputePipelineStateWithDescriptorOptionsCompletionHandler, C);
		INTERPOSE_REGISTRATION(NewFence, C);
		INTERPOSE_REGISTRATION(NewArgumentEncoderWithArguments, C);
		INTERPOSE_REGISTRATION(getDefaultSamplePositionscount, C);
		INTERPOSE_REGISTRATION(newRenderPipelineStateWithTileDescriptoroptionsreflectionerror, C);
		INTERPOSE_REGISTRATION(newRenderPipelineStateWithTileDescriptoroptionscompletionHandler, C);
	}
};

MTLPP_END

#endif /* imp_Device_hpp */

