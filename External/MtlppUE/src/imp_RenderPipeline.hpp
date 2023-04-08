// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_RenderPipeline_hpp
#define imp_RenderPipeline_hpp

#include "imp_State.hpp"

MTLPP_BEGIN

template<>
struct IMPTable<MTLRenderPipelineColorAttachmentDescriptor*, void> : public IMPTableBase<MTLRenderPipelineColorAttachmentDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLRenderPipelineColorAttachmentDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(pixelFormat, C)
	, INTERPOSE_CONSTRUCTOR(isBlendingEnabled, C)
	, INTERPOSE_CONSTRUCTOR(sourceRGBBlendFactor, C)
	, INTERPOSE_CONSTRUCTOR(destinationRGBBlendFactor, C)
	, INTERPOSE_CONSTRUCTOR(rgbBlendOperation, C)
	, INTERPOSE_CONSTRUCTOR(sourceAlphaBlendFactor, C)
	, INTERPOSE_CONSTRUCTOR(destinationAlphaBlendFactor, C)
	, INTERPOSE_CONSTRUCTOR(alphaBlendOperation, C)
	, INTERPOSE_CONSTRUCTOR(writeMask, C)
	, INTERPOSE_CONSTRUCTOR(setPixelFormat, C)
	, INTERPOSE_CONSTRUCTOR(setBlendingEnabled, C)
	, INTERPOSE_CONSTRUCTOR(setSourceRGBBlendFactor, C)
	, INTERPOSE_CONSTRUCTOR(setDestinationRGBBlendFactor, C)
	, INTERPOSE_CONSTRUCTOR(setRgbBlendOperation, C)
	, INTERPOSE_CONSTRUCTOR(setSourceAlphaBlendFactor, C)
	, INTERPOSE_CONSTRUCTOR(setDestinationAlphaBlendFactor, C)
	, INTERPOSE_CONSTRUCTOR(setAlphaBlendOperation, C)
	, INTERPOSE_CONSTRUCTOR(setWriteMask, C)
	{
	}
	
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, pixelFormat, pixelFormat, MTLPixelFormat);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, isBlendingEnabled, isBlendingEnabled, BOOL);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, sourceRGBBlendFactor, sourceRGBBlendFactor, MTLBlendFactor);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, destinationRGBBlendFactor, destinationRGBBlendFactor, MTLBlendFactor);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, rgbBlendOperation, rgbBlendOperation, MTLBlendOperation);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, sourceAlphaBlendFactor, sourceAlphaBlendFactor, MTLBlendFactor);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, destinationAlphaBlendFactor, destinationAlphaBlendFactor, MTLBlendFactor);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, alphaBlendOperation, alphaBlendOperation, MTLBlendOperation);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, writeMask, writeMask, MTLColorWriteMask);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, setPixelFormat:, setPixelFormat, void, MTLPixelFormat);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, setBlendingEnabled:, setBlendingEnabled, void, BOOL);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, setSourceRGBBlendFactor:, setSourceRGBBlendFactor, void, MTLBlendFactor);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, setDestinationRGBBlendFactor:, setDestinationRGBBlendFactor, void, MTLBlendFactor);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, setRgbBlendOperation:, setRgbBlendOperation, void, MTLBlendOperation);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, setSourceAlphaBlendFactor:, setSourceAlphaBlendFactor, void, MTLBlendFactor);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, setDestinationAlphaBlendFactor:, setDestinationAlphaBlendFactor, void, MTLBlendFactor);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, setAlphaBlendOperation:, setAlphaBlendOperation, void, MTLBlendOperation);
	INTERPOSE_SELECTOR(MTLRenderPipelineColorAttachmentDescriptor*, setWriteMask:, setWriteMask, void, MTLColorWriteMask);
	
};

template<>
struct IMPTable<MTLRenderPipelineDescriptor*, void> : public IMPTableBase<MTLRenderPipelineDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLRenderPipelineDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(label, C)
	, INTERPOSE_CONSTRUCTOR(setLabel, C)
	, INTERPOSE_CONSTRUCTOR(vertexFunction, C)
	, INTERPOSE_CONSTRUCTOR(setVertexFunction, C)
	, INTERPOSE_CONSTRUCTOR(fragmentFunction, C)
	, INTERPOSE_CONSTRUCTOR(setFragmentFunction, C)
	, INTERPOSE_CONSTRUCTOR(vertexDescriptor, C)
	, INTERPOSE_CONSTRUCTOR(setVertexDescriptor, C)
	, INTERPOSE_CONSTRUCTOR(sampleCount, C)
	, INTERPOSE_CONSTRUCTOR(setSampleCount, C)
	, INTERPOSE_CONSTRUCTOR(rasterSampleCount, C)
	, INTERPOSE_CONSTRUCTOR(setRasterSampleCount, C)
	, INTERPOSE_CONSTRUCTOR(isAlphaToCoverageEnabled, C)
	, INTERPOSE_CONSTRUCTOR(setAlphaToCoverageEnabled, C)
	, INTERPOSE_CONSTRUCTOR(isAlphaToOneEnabled, C)
	, INTERPOSE_CONSTRUCTOR(setAlphaToOneEnabled, C)
	, INTERPOSE_CONSTRUCTOR(isRasterizationEnabled, C)
	, INTERPOSE_CONSTRUCTOR(setRasterizationEnabled, C)
	, INTERPOSE_CONSTRUCTOR(colorAttachments, C)
	, INTERPOSE_CONSTRUCTOR(depthAttachmentPixelFormat, C)
	, INTERPOSE_CONSTRUCTOR(setDepthAttachmentPixelFormat, C)
	, INTERPOSE_CONSTRUCTOR(stencilAttachmentPixelFormat, C)
	, INTERPOSE_CONSTRUCTOR(setStencilAttachmentPixelFormat, C)
#if TARGET_OS_OSX
	, INTERPOSE_CONSTRUCTOR(inputPrimitiveTopology, C)
	, INTERPOSE_CONSTRUCTOR(setInputPrimitiveTopology, C)
#endif
	, INTERPOSE_CONSTRUCTOR(tessellationPartitionMode, C)
	, INTERPOSE_CONSTRUCTOR(setTessellationPartitionMode, C)
	, INTERPOSE_CONSTRUCTOR(maxTessellationFactor, C)
	, INTERPOSE_CONSTRUCTOR(setMaxTessellationFactor, C)
	, INTERPOSE_CONSTRUCTOR(isTessellationFactorScaleEnabled, C)
	, INTERPOSE_CONSTRUCTOR(setTessellationFactorScaleEnabled, C)
	, INTERPOSE_CONSTRUCTOR(tessellationFactorFormat, C)
	, INTERPOSE_CONSTRUCTOR(setTessellationFactorFormat, C)
	, INTERPOSE_CONSTRUCTOR(tessellationControlPointIndexType, C)
	, INTERPOSE_CONSTRUCTOR(setTessellationControlPointIndexType, C)
	, INTERPOSE_CONSTRUCTOR(tessellationFactorStepFunction, C)
	, INTERPOSE_CONSTRUCTOR(setTessellationFactorStepFunction, C)
	, INTERPOSE_CONSTRUCTOR(tessellationOutputWindingOrder, C)
	, INTERPOSE_CONSTRUCTOR(setTessellationOutputWindingOrder, C)
	, INTERPOSE_CONSTRUCTOR(vertexBuffers, C)
	, INTERPOSE_CONSTRUCTOR(fragmentBuffers, C)
	, INTERPOSE_CONSTRUCTOR(reset, C)
	{
	}
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, label, label, NSString*);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setLabel:, setLabel, void, NSString*);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, vertexFunction, vertexFunction, id <MTLFunction>);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setVertexFunction:, setVertexFunction, void, id <MTLFunction>);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, fragmentFunction, fragmentFunction, id <MTLFunction>);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setFragmentFunction:, setFragmentFunction, void, id <MTLFunction>);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, vertexDescriptor, vertexDescriptor, MTLVertexDescriptor*);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setVertexDescriptor:, setVertexDescriptor, void, MTLVertexDescriptor*);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, sampleCount, sampleCount, NSUInteger);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setSampleCount:, setSampleCount, void, NSUInteger);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, rasterSampleCount, rasterSampleCount, NSUInteger);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setRasterSampleCount:, setRasterSampleCount, void, NSUInteger);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, isAlphaToCoverageEnabled, isAlphaToCoverageEnabled, BOOL);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setAlphaToCoverageEnabled:, setAlphaToCoverageEnabled, void, BOOL);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, isAlphaToOneEnabled, isAlphaToOneEnabled, BOOL);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setAlphaToOneEnabled:, setAlphaToOneEnabled, void, BOOL);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, isRasterizationEnabled, isRasterizationEnabled, BOOL);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setRasterizationEnabled:, setRasterizationEnabled, void, BOOL);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, colorAttachments, colorAttachments, MTLRenderPipelineColorAttachmentDescriptorArray*);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, depthAttachmentPixelFormat, depthAttachmentPixelFormat, MTLPixelFormat);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setDepthAttachmentPixelFormat:, setDepthAttachmentPixelFormat, void, MTLPixelFormat);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, stencilAttachmentPixelFormat, stencilAttachmentPixelFormat, MTLPixelFormat);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setStencilAttachmentPixelFormat:, setStencilAttachmentPixelFormat, void, MTLPixelFormat);

#if TARGET_OS_OSX
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, inputPrimitiveTopology, inputPrimitiveTopology, MTLPrimitiveTopologyClass);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setInputPrimitiveTopology:, setInputPrimitiveTopology, void, MTLPrimitiveTopologyClass);
#endif
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, tessellationPartitionMode, tessellationPartitionMode, MTLTessellationPartitionMode);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setTessellationPartitionMode:, setTessellationPartitionMode, void, MTLTessellationPartitionMode);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, maxTessellationFactor, maxTessellationFactor, NSUInteger);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setMaxTessellationFactor:, setMaxTessellationFactor, void, NSUInteger);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, isTessellationFactorScaleEnabled, isTessellationFactorScaleEnabled, BOOL);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setTessellationFactorScaleEnabled:, setTessellationFactorScaleEnabled, void, BOOL);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, tessellationFactorFormat, tessellationFactorFormat, MTLTessellationFactorFormat);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setTessellationFactorFormat:, setTessellationFactorFormat, void, MTLTessellationFactorFormat);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, tessellationControlPointIndexType, tessellationControlPointIndexType, MTLTessellationControlPointIndexType);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setTessellationControlPointIndexType:, setTessellationControlPointIndexType, void, MTLTessellationControlPointIndexType);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, tessellationFactorStepFunction, tessellationFactorStepFunction, MTLTessellationFactorStepFunction);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setTessellationFactorStepFunction:, setTessellationFactorStepFunction, void, MTLTessellationFactorStepFunction);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, tessellationOutputWindingOrder, tessellationOutputWindingOrder, MTLWinding);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, setTessellationOutputWindingOrder:, setTessellationOutputWindingOrder, void, MTLWinding);
	
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, vertexBuffers, vertexBuffers, MTLPipelineBufferDescriptorArray*);
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, fragmentBuffers, fragmentBuffers, MTLPipelineBufferDescriptorArray*);
	
	INTERPOSE_SELECTOR(MTLRenderPipelineDescriptor*, reset, reset, void);
};

template<>
struct IMPTable<id<MTLRenderPipelineState>, void> : public IMPTableState<id<MTLRenderPipelineState>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableState<id<MTLRenderPipelineState>>(C)
	, INTERPOSE_CONSTRUCTOR(maxTotalThreadsPerThreadgroup, C)
	, INTERPOSE_CONSTRUCTOR(threadgroupSizeMatchesTileSize, C)
	, INTERPOSE_CONSTRUCTOR(imageblockSampleLength, C)
	, INTERPOSE_CONSTRUCTOR(imageblockMemoryLengthForDimensions, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLRenderPipelineState>, maxTotalThreadsPerThreadgroup, maxTotalThreadsPerThreadgroup, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderPipelineState>, threadgroupSizeMatchesTileSize, threadgroupSizeMatchesTileSize, BOOL);
	INTERPOSE_SELECTOR(id<MTLRenderPipelineState>, imageblockSampleLength, imageblockSampleLength, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderPipelineState>, imageblockMemoryLengthForDimensions:, imageblockMemoryLengthForDimensions, NSUInteger, MTLPPSize);
};

template<typename InterposeClass>
struct IMPTable<id<MTLRenderPipelineState>, InterposeClass> : public IMPTable<id<MTLRenderPipelineState>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLRenderPipelineState>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableState<id<MTLRenderPipelineState>>::RegisterInterpose<InterposeClass>(C);
	}
};

MTLPP_END

#endif /* imp_RenderPipeline_hpp */
