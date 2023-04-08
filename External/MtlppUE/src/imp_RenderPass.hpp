// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_RenderPass_hpp
#define imp_RenderPass_hpp

#include "imp_Object.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<MTLRenderPassAttachmentDescriptor*, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: INTERPOSE_CONSTRUCTOR(texture, C)
	, INTERPOSE_CONSTRUCTOR(setTexture, C)
	, INTERPOSE_CONSTRUCTOR(level, C)
	, INTERPOSE_CONSTRUCTOR(setLevel, C)
	, INTERPOSE_CONSTRUCTOR(slice, C)
	, INTERPOSE_CONSTRUCTOR(setSlice, C)
	, INTERPOSE_CONSTRUCTOR(depthPlane, C)
	, INTERPOSE_CONSTRUCTOR(setDepthPlane, C)
	, INTERPOSE_CONSTRUCTOR(resolveTexture, C)
	, INTERPOSE_CONSTRUCTOR(setResolveTexture, C)
	, INTERPOSE_CONSTRUCTOR(resolvelevel, C)
	, INTERPOSE_CONSTRUCTOR(setResolveLevel, C)
	, INTERPOSE_CONSTRUCTOR(resolveslice, C)
	, INTERPOSE_CONSTRUCTOR(setResolveSlice, C)
	, INTERPOSE_CONSTRUCTOR(resolveDepthPlane, C)
	, INTERPOSE_CONSTRUCTOR(setResolveDepthPlane, C)
	, INTERPOSE_CONSTRUCTOR(loadAction, C)
	, INTERPOSE_CONSTRUCTOR(setLoadAction, C)
	, INTERPOSE_CONSTRUCTOR(storeAction, C)
	, INTERPOSE_CONSTRUCTOR(setStoreAction, C)
	, INTERPOSE_CONSTRUCTOR(storeActionOptions, C)
	, INTERPOSE_CONSTRUCTOR(setstoreActionOptions, C)
	{
	}
	
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, texture,	texture,	id <MTLTexture>);
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, setTexture:,	setTexture, void,	id <MTLTexture>);

	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, level,	level,	NSUInteger);
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, setLevel:,	setLevel, void,	NSUInteger);
	
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, slice,	slice,	NSUInteger);
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, setSlice:,	setSlice, void,	NSUInteger);
	
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, depthPlane,	depthPlane,	NSUInteger);
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, setDepthPlane:,	setDepthPlane, void,	NSUInteger);
	
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, resolveTexture,	resolveTexture,	id <MTLTexture>);
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, setResolveTexture:,	setResolveTexture, void,	id <MTLTexture>);
	
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, resolvelevel,	resolvelevel,	NSUInteger);
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, setResolveLevel:,	setResolveLevel, void,	NSUInteger);
	
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, resolveslice,	resolveslice,	NSUInteger);
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, setResolveSlice:,	setResolveSlice, void,	NSUInteger);
	
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, resolveDepthPlane,	resolveDepthPlane,	NSUInteger);
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, setResolveDepthPlane:,	setResolveDepthPlane, void,	NSUInteger);

	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, loadAction,	loadAction,	MTLLoadAction);
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, setLoadAction:,	setLoadAction, void,	MTLLoadAction);

	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, storeAction,	storeAction,	MTLStoreAction);
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, setStoreAction:,	setStoreAction, void,	MTLStoreAction);
	
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, storeActionOptions,	storeActionOptions,	MTLStoreActionOptions);
	INTERPOSE_SELECTOR(MTLRenderPassAttachmentDescriptor*, setStoreActionOptions:,	setstoreActionOptions, void,	MTLStoreActionOptions);
};

template<>
struct MTLPP_EXPORT IMPTable<MTLRenderPassColorAttachmentDescriptor*, void> : public IMPTableBase<MTLRenderPassColorAttachmentDescriptor*>, public IMPTable<MTLRenderPassAttachmentDescriptor*, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLRenderPassColorAttachmentDescriptor*>(C)
	, IMPTable<MTLRenderPassAttachmentDescriptor*, void>(C)
	, INTERPOSE_CONSTRUCTOR(clearColor, C)
	, INTERPOSE_CONSTRUCTOR(setClearColor, C)
	{
	}
	
	INTERPOSE_SELECTOR(MTLRenderPassColorAttachmentDescriptor*, clearColor,	clearColor,	MTLPPClearColor);
	INTERPOSE_SELECTOR(MTLRenderPassColorAttachmentDescriptor*, setClearColor:,	setClearColor, void,	MTLPPClearColor);
};

template<>
struct MTLPP_EXPORT IMPTable<MTLRenderPassDepthAttachmentDescriptor*, void> : public IMPTableBase<MTLRenderPassDepthAttachmentDescriptor*>, public IMPTable<MTLRenderPassAttachmentDescriptor*, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLRenderPassDepthAttachmentDescriptor*>(C)
	, IMPTable<MTLRenderPassAttachmentDescriptor*, void>(C)
	, INTERPOSE_CONSTRUCTOR(clearDepth, C)
	, INTERPOSE_CONSTRUCTOR(setClearDepth, C)
#if TARGET_OS_IPHONE
	, INTERPOSE_CONSTRUCTOR(depthResolveFilter, C)
	, INTERPOSE_CONSTRUCTOR(setDepthResolveFilter, C)
#endif
	{
	}
	
	INTERPOSE_SELECTOR(MTLRenderPassDepthAttachmentDescriptor*, clearDepth,	clearDepth,	double);
	INTERPOSE_SELECTOR(MTLRenderPassDepthAttachmentDescriptor*, setClearDepth:,	setClearDepth, void,	double);

#if TARGET_OS_IPHONE
	INTERPOSE_SELECTOR(MTLRenderPassDepthAttachmentDescriptor*, depthResolveFilter,	depthResolveFilter,	MTLMultisampleDepthResolveFilter);
	INTERPOSE_SELECTOR(MTLRenderPassDepthAttachmentDescriptor*, setDepthResolveFilter:,	setDepthResolveFilter, void,	MTLMultisampleDepthResolveFilter);
#endif
};

template<>
struct MTLPP_EXPORT IMPTable<MTLRenderPassStencilAttachmentDescriptor*, void> : public IMPTableBase<MTLRenderPassStencilAttachmentDescriptor*>, public IMPTable<MTLRenderPassAttachmentDescriptor*, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLRenderPassStencilAttachmentDescriptor*>(C)
	, IMPTable<MTLRenderPassAttachmentDescriptor*, void>(C)
	, INTERPOSE_CONSTRUCTOR(clearStencil, C)
	, INTERPOSE_CONSTRUCTOR(setClearStencil, C)
	{
	}
	
	INTERPOSE_SELECTOR(MTLRenderPassStencilAttachmentDescriptor*, clearStencil,	clearStencil,	uint32_t);
	INTERPOSE_SELECTOR(MTLRenderPassStencilAttachmentDescriptor*, setClearStencil:,	setClearStencil, void,	uint32_t);
};

template<>
struct MTLPP_EXPORT IMPTable<MTLRenderPassDescriptor*, void> : public IMPTableBase<MTLRenderPassDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLRenderPassDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(colorAttachments, C)
	, INTERPOSE_CONSTRUCTOR(depthAttachment, C)
	, INTERPOSE_CONSTRUCTOR(setDepthAttachment, C)
	, INTERPOSE_CONSTRUCTOR(stencilAttachment, C)
	, INTERPOSE_CONSTRUCTOR(setStencilAttachment, C)
	, INTERPOSE_CONSTRUCTOR(visibilityResultBuffer, C)
	, INTERPOSE_CONSTRUCTOR(setVisibilityResultBuffer, C)
	, INTERPOSE_CONSTRUCTOR(renderTargetArrayLength, C)
	, INTERPOSE_CONSTRUCTOR(setRenderTargetArrayLength, C)
	, INTERPOSE_CONSTRUCTOR(setSamplePositionscount, C)
	, INTERPOSE_CONSTRUCTOR(getSamplePositionscount, C)
	{
	}
	
	INTERPOSE_SELECTOR(MTLRenderPassDescriptor*, colorAttachments,	colorAttachments,	MTLRenderPassColorAttachmentDescriptorArray*);
	INTERPOSE_SELECTOR(MTLRenderPassDescriptor*, depthAttachment,	depthAttachment,	MTLRenderPassDepthAttachmentDescriptor*);
	INTERPOSE_SELECTOR(MTLRenderPassDescriptor*, setDepthAttachment:,	setDepthAttachment, void,	MTLRenderPassDepthAttachmentDescriptor*);
	INTERPOSE_SELECTOR(MTLRenderPassDescriptor*, stencilAttachment,	stencilAttachment,	MTLRenderPassStencilAttachmentDescriptor*);
	INTERPOSE_SELECTOR(MTLRenderPassDescriptor*, setStencilAttachment:,	setStencilAttachment, void,	MTLRenderPassStencilAttachmentDescriptor*);
	INTERPOSE_SELECTOR(MTLRenderPassDescriptor*, visibilityResultBuffer,	visibilityResultBuffer,	id <MTLBuffer>);
	INTERPOSE_SELECTOR(MTLRenderPassDescriptor*, setVisibilityResultBuffer:,	setVisibilityResultBuffer, void,	id <MTLBuffer>);
	INTERPOSE_SELECTOR(MTLRenderPassDescriptor*, renderTargetArrayLength,	renderTargetArrayLength,	NSUInteger);
	INTERPOSE_SELECTOR(MTLRenderPassDescriptor*, setRenderTargetArrayLength:,	setRenderTargetArrayLength, void,	NSUInteger);
	INTERPOSE_SELECTOR(MTLRenderPassDescriptor*, setSamplePositions:count:,	setSamplePositionscount,	void, const MTLPPSamplePosition *, NSUInteger);
	INTERPOSE_SELECTOR(MTLRenderPassDescriptor*, getSamplePositions:count:,	getSamplePositionscount,	NSUInteger, MTLPPSamplePosition *, NSUInteger);
};

MTLPP_END

#endif /* imp_RenderPass_hpp */
