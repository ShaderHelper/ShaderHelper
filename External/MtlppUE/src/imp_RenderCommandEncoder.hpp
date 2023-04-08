// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_RenderCommandEncoder_hpp
#define imp_RenderCommandEncoder_hpp

#include "imp_CommandEncoder.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<id<MTLRenderCommandEncoder>, void> : public IMPTableCommandEncoder<id<MTLRenderCommandEncoder>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableCommandEncoder<id<MTLRenderCommandEncoder>>(C)
	, INTERPOSE_CONSTRUCTOR(Setrenderpipelinestate, C)
	, INTERPOSE_CONSTRUCTOR(SetvertexbytesLengthAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetvertexbufferOffsetAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetvertexbufferoffsetAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetvertexbuffersOffsetsWithrange, C)
	, INTERPOSE_CONSTRUCTOR(SetvertextextureAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetvertextexturesWithrange, C)
	, INTERPOSE_CONSTRUCTOR(SetvertexsamplerstateAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetvertexsamplerstatesWithrange, C)
	, INTERPOSE_CONSTRUCTOR(SetvertexsamplerstateLodminclampLodmaxclampAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetvertexsamplerstatesLodminclampsLodmaxclampsWithrange, C)
	, INTERPOSE_CONSTRUCTOR(Setviewport, C)
	, INTERPOSE_CONSTRUCTOR(SetviewportsCount, C)
	, INTERPOSE_CONSTRUCTOR(Setfrontfacingwinding, C)
	, INTERPOSE_CONSTRUCTOR(Setcullmode, C)
	, INTERPOSE_CONSTRUCTOR(Setdepthclipmode, C)
	, INTERPOSE_CONSTRUCTOR(SetdepthbiasSlopescaleClamp, C)
	, INTERPOSE_CONSTRUCTOR(Setscissorrect, C)
	, INTERPOSE_CONSTRUCTOR(SetscissorrectsCount, C)
	, INTERPOSE_CONSTRUCTOR(Settrianglefillmode, C)
	, INTERPOSE_CONSTRUCTOR(SetfragmentbytesLengthAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetfragmentbufferOffsetAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetfragmentbufferoffsetAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetfragmentbuffersOffsetsWithrange, C)
	, INTERPOSE_CONSTRUCTOR(SetfragmenttextureAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetfragmenttexturesWithrange, C)
	, INTERPOSE_CONSTRUCTOR(SetfragmentsamplerstateAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetfragmentsamplerstatesWithrange, C)
	, INTERPOSE_CONSTRUCTOR(SetfragmentsamplerstateLodminclampLodmaxclampAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetfragmentsamplerstatesLodminclampsLodmaxclampsWithrange, C)
	, INTERPOSE_CONSTRUCTOR(SetblendcolorredGreenBlueAlpha, C)
	, INTERPOSE_CONSTRUCTOR(Setdepthstencilstate, C)
	, INTERPOSE_CONSTRUCTOR(Setstencilreferencevalue, C)
	, INTERPOSE_CONSTRUCTOR(SetstencilfrontreferencevalueBackreferencevalue, C)
	, INTERPOSE_CONSTRUCTOR(SetvisibilityresultmodeOffset, C)
	, INTERPOSE_CONSTRUCTOR(SetcolorstoreactionAtindex, C)
	, INTERPOSE_CONSTRUCTOR(Setdepthstoreaction, C)
	, INTERPOSE_CONSTRUCTOR(Setstencilstoreaction, C)
	, INTERPOSE_CONSTRUCTOR(SetcolorstoreactionoptionsAtindex, C)
	, INTERPOSE_CONSTRUCTOR(Setdepthstoreactionoptions, C)
	, INTERPOSE_CONSTRUCTOR(Setstencilstoreactionoptions, C)
	, INTERPOSE_CONSTRUCTOR(DrawprimitivesVertexstartVertexcountInstancecount, C)
	, INTERPOSE_CONSTRUCTOR(DrawprimitivesVertexstartVertexcount, C)
	, INTERPOSE_CONSTRUCTOR(DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffsetInstancecount, C)
	, INTERPOSE_CONSTRUCTOR(DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffset, C)
	, INTERPOSE_CONSTRUCTOR(DrawprimitivesVertexstartVertexcountInstancecountBaseinstance, C)
	, INTERPOSE_CONSTRUCTOR(DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffsetInstancecountBasevertexBaseinstance, C)
	, INTERPOSE_CONSTRUCTOR(DrawprimitivesIndirectbufferIndirectbufferoffset, C)
	, INTERPOSE_CONSTRUCTOR(DrawindexedprimitivesIndextypeIndexbufferIndexbufferoffsetIndirectbufferIndirectbufferoffset, C)
	, INTERPOSE_CONSTRUCTOR(Texturebarrier, C)
	, INTERPOSE_CONSTRUCTOR(UpdatefenceAfterstages, C)
	, INTERPOSE_CONSTRUCTOR(WaitforfenceBeforestages, C)
	, INTERPOSE_CONSTRUCTOR(SettessellationfactorbufferOffsetInstancestride, C)
	, INTERPOSE_CONSTRUCTOR(Settessellationfactorscale, C)
	, INTERPOSE_CONSTRUCTOR(DrawpatchesPatchstartPatchcountPatchindexbufferPatchindexbufferoffsetInstancecountBaseinstance, C)
	, INTERPOSE_CONSTRUCTOR(DrawpatchesPatchindexbufferPatchindexbufferoffsetIndirectbufferIndirectbufferoffset, C)
	, INTERPOSE_CONSTRUCTOR(DrawindexedpatchesPatchstartPatchcountPatchindexbufferPatchindexbufferoffsetControlpointindexbufferControlpointindexbufferoffsetInstancecountBaseinstance, C)
	, INTERPOSE_CONSTRUCTOR(DrawindexedpatchesPatchindexbufferPatchindexbufferoffsetControlpointindexbufferControlpointindexbufferoffsetIndirectbufferIndirectbufferoffset, C)
	, INTERPOSE_CONSTRUCTOR(UseresourceUsage, C)
	, INTERPOSE_CONSTRUCTOR(UseresourcesCountUsage, C)
	, INTERPOSE_CONSTRUCTOR(Useheap, C)
	, INTERPOSE_CONSTRUCTOR(UseheapsCount, C)
	, INTERPOSE_CONSTRUCTOR(SetTilebytesLengthAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetTilebufferOffsetAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetTilebufferoffsetAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetTilebuffersOffsetsWithrange, C)
	, INTERPOSE_CONSTRUCTOR(SetTiletextureAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetTiletexturesWithrange, C)
	, INTERPOSE_CONSTRUCTOR(SetTilesamplerstateAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetTilesamplerstatesWithrange, C)
	, INTERPOSE_CONSTRUCTOR(SetTilesamplerstateLodminclampLodmaxclampAtindex, C)
	, INTERPOSE_CONSTRUCTOR(SetTilesamplerstatesLodminclampsLodmaxclampsWithrange, C)
	, INTERPOSE_CONSTRUCTOR(dispatchThreadsPerTile, C)
	, INTERPOSE_CONSTRUCTOR(tileWidth, C)
	, INTERPOSE_CONSTRUCTOR(tileHeight, C)
	, INTERPOSE_CONSTRUCTOR(setThreadgroupMemoryLength, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setRenderPipelineState:,	Setrenderpipelinestate,	void, id <MTLRenderPipelineState>);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setVertexBytes:length:atIndex:,	SetvertexbytesLengthAtindex,	void, const void *,NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setVertexBuffer:offset:atIndex:,	SetvertexbufferOffsetAtindex,	void,id <MTLBuffer>,NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setVertexBufferOffset:atIndex:,	SetvertexbufferoffsetAtindex,	void, NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setVertexBuffers:offsets:withRange:,	SetvertexbuffersOffsetsWithrange,	void, const id <MTLBuffer> *,const NSUInteger *,NSRange);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setVertexTexture:atIndex:,	SetvertextextureAtindex,	void,id <MTLTexture>,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setVertexTextures:withRange:,	SetvertextexturesWithrange,	void, const id <MTLTexture> *,NSRange);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setVertexSamplerState:atIndex:,	SetvertexsamplerstateAtindex,	void,id <MTLSamplerState>,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setVertexSamplerStates:withRange:,	SetvertexsamplerstatesWithrange,	void, const id <MTLSamplerState> *,NSRange);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setVertexSamplerState:lodMinClamp:lodMaxClamp:atIndex:,	SetvertexsamplerstateLodminclampLodmaxclampAtindex,	void,id <MTLSamplerState>,float,float,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setVertexSamplerStates:lodMinClamps:lodMaxClamps:withRange:,	SetvertexsamplerstatesLodminclampsLodmaxclampsWithrange,	void, const id <MTLSamplerState> *,const float *,const float *,NSRange);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setViewport:,	Setviewport,	void, MTLPPViewport);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setViewports:count:,	SetviewportsCount,	void, const MTLPPViewport *,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setFrontFacingWinding:,	Setfrontfacingwinding,	void, MTLWinding);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setCullMode:,	Setcullmode,	void, MTLCullMode);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setDepthClipMode:,	Setdepthclipmode,	void, MTLDepthClipMode);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setDepthBias:slopeScale:clamp:,	SetdepthbiasSlopescaleClamp,	void, float,float,float);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setScissorRect:,	Setscissorrect,	void, MTLPPScissorRect);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setScissorRects:count:,	SetscissorrectsCount,	void, const MTLPPScissorRect *,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setTriangleFillMode:,	Settrianglefillmode,	void, MTLTriangleFillMode);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setFragmentBytes:length:atIndex:,	SetfragmentbytesLengthAtindex,	void, const void *,NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setFragmentBuffer:offset:atIndex:,	SetfragmentbufferOffsetAtindex,	void,id <MTLBuffer>,NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setFragmentBufferOffset:atIndex:,	SetfragmentbufferoffsetAtindex,	void, NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setFragmentBuffers:offsets:withRange:,	SetfragmentbuffersOffsetsWithrange,	void, const id <MTLBuffer> *,const NSUInteger *,NSRange);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setFragmentTexture:atIndex:,	SetfragmenttextureAtindex,	void,id <MTLTexture>,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setFragmentTextures:withRange:,	SetfragmenttexturesWithrange,	void, const id <MTLTexture> *,NSRange);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setFragmentSamplerState:atIndex:,	SetfragmentsamplerstateAtindex,	void,id <MTLSamplerState>,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setFragmentSamplerStates:withRange:,	SetfragmentsamplerstatesWithrange,	void, const id <MTLSamplerState> *,NSRange);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setFragmentSamplerState:lodMinClamp:lodMaxClamp:atIndex:,	SetfragmentsamplerstateLodminclampLodmaxclampAtindex,	void,id <MTLSamplerState>,float,float,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setFragmentSamplerStates:lodMinClamps:lodMaxClamps:withRange:,	SetfragmentsamplerstatesLodminclampsLodmaxclampsWithrange,	void, const id <MTLSamplerState> *,const float *,const float *,NSRange);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setBlendColorRed:green:blue:alpha:,	SetblendcolorredGreenBlueAlpha,	void, float,float,float,float);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setDepthStencilState:,	Setdepthstencilstate,	void,id <MTLDepthStencilState>);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setStencilReferenceValue:,	Setstencilreferencevalue,	void, uint32_t);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setStencilFrontReferenceValue:backReferenceValue:,	SetstencilfrontreferencevalueBackreferencevalue,	void, uint32_t,uint32_t);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setVisibilityResultMode:offset:,	SetvisibilityresultmodeOffset,	void, MTLVisibilityResultMode,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setColorStoreAction:atIndex:,	SetcolorstoreactionAtindex,	void, MTLStoreAction,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setDepthStoreAction:,	Setdepthstoreaction,	void, MTLStoreAction);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setStencilStoreAction:,	Setstencilstoreaction,	void, MTLStoreAction);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setColorStoreActionOptions:atIndex:,	SetcolorstoreactionoptionsAtindex,	void, MTLStoreActionOptions,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setDepthStoreActionOptions:,	Setdepthstoreactionoptions,	void, MTLStoreActionOptions);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setStencilStoreActionOptions:,	Setstencilstoreactionoptions,	void, MTLStoreActionOptions);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, drawPrimitives:vertexStart:vertexCount:instanceCount:,	DrawprimitivesVertexstartVertexcountInstancecount,	void, MTLPrimitiveType,NSUInteger,NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, drawPrimitives:vertexStart:vertexCount:,	DrawprimitivesVertexstartVertexcount,	void, MTLPrimitiveType,NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, drawIndexedPrimitives:indexCount:indexType:indexBuffer:indexBufferOffset:instanceCount:,	DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffsetInstancecount,	void, MTLPrimitiveType,NSUInteger,MTLIndexType,id <MTLBuffer>,NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, drawIndexedPrimitives:indexCount:indexType:indexBuffer:indexBufferOffset:,	DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffset,	void, MTLPrimitiveType,NSUInteger,MTLIndexType,id <MTLBuffer>,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, drawPrimitives:vertexStart:vertexCount:instanceCount:baseInstance:,	DrawprimitivesVertexstartVertexcountInstancecountBaseinstance,	void, MTLPrimitiveType,NSUInteger,NSUInteger,NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, drawIndexedPrimitives:indexCount:indexType:indexBuffer:indexBufferOffset:instanceCount:baseVertex:baseInstance:,	DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffsetInstancecountBasevertexBaseinstance,	void, MTLPrimitiveType,NSUInteger,MTLIndexType,id <MTLBuffer>,NSUInteger,NSUInteger,NSInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, drawPrimitives:indirectBuffer:indirectBufferOffset:,	DrawprimitivesIndirectbufferIndirectbufferoffset,	void, MTLPrimitiveType,id <MTLBuffer>,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, drawIndexedPrimitives:indexType:indexBuffer:indexBufferOffset:indirectBuffer:indirectBufferOffset:,	DrawindexedprimitivesIndextypeIndexbufferIndexbufferoffsetIndirectbufferIndirectbufferoffset,	void, MTLPrimitiveType,MTLIndexType,id <MTLBuffer>,NSUInteger,id <MTLBuffer>,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, textureBarrier,	Texturebarrier,	void );
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, updateFence:afterStages:,	UpdatefenceAfterstages,	void, id <MTLFence>,MTLRenderStages);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, waitForFence:beforeStages:,	WaitforfenceBeforestages,	void, id <MTLFence>,MTLRenderStages);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setTessellationFactorBuffer:offset:instanceStride:,	SettessellationfactorbufferOffsetInstancestride,	void,id <MTLBuffer>,NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setTessellationFactorScale:,	Settessellationfactorscale,	void, float);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, drawPatches:patchStart:patchCount:patchIndexBuffer:patchIndexBufferOffset:instanceCount:baseInstance:,	DrawpatchesPatchstartPatchcountPatchindexbufferPatchindexbufferoffsetInstancecountBaseinstance,	void, NSUInteger,NSUInteger,NSUInteger, id <MTLBuffer>,NSUInteger,NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, drawPatches:patchIndexBuffer:patchIndexBufferOffset:indirectBuffer:indirectBufferOffset:,	DrawpatchesPatchindexbufferPatchindexbufferoffsetIndirectbufferIndirectbufferoffset,	void, NSUInteger, id <MTLBuffer>,NSUInteger,id <MTLBuffer>,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, drawIndexedPatches:patchStart:patchCount:patchIndexBuffer:patchIndexBufferOffset:controlPointIndexBuffer:controlPointIndexBufferOffset:instanceCount:baseInstance:,	DrawindexedpatchesPatchstartPatchcountPatchindexbufferPatchindexbufferoffsetControlpointindexbufferControlpointindexbufferoffsetInstancecountBaseinstance,	void, NSUInteger,NSUInteger,NSUInteger, id <MTLBuffer>,NSUInteger,id <MTLBuffer>,NSUInteger,NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, drawIndexedPatches:patchIndexBuffer:patchIndexBufferOffset:controlPointIndexBuffer:controlPointIndexBufferOffset:indirectBuffer:indirectBufferOffset:,	DrawindexedpatchesPatchindexbufferPatchindexbufferoffsetControlpointindexbufferControlpointindexbufferoffsetIndirectbufferIndirectbufferoffset,	void, NSUInteger, id <MTLBuffer>,NSUInteger,id <MTLBuffer>,NSUInteger,id <MTLBuffer>,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, useResource:usage:,	UseresourceUsage,	void, id <MTLResource>,MTLResourceUsage);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, useResources:count:usage:,	UseresourcesCountUsage,	void, const id <MTLResource> *,NSUInteger,MTLResourceUsage);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, useHeap:,	Useheap,	void, id <MTLHeap>);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, useHeaps:count:,	UseheapsCount,	void, const id <MTLHeap> *, NSUInteger );
	
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setTileBytes:length:atIndex:,	SetTilebytesLengthAtindex,	void, const void *,NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setTileBuffer:offset:atIndex:,	SetTilebufferOffsetAtindex,	void,id <MTLBuffer>,NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setTileBufferOffset:atIndex:,	SetTilebufferoffsetAtindex,	void, NSUInteger,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setTileBuffers:offsets:withRange:,	SetTilebuffersOffsetsWithrange,	void, const id <MTLBuffer> *,const NSUInteger *,NSRange);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setTileTexture:atIndex:,	SetTiletextureAtindex,	void,id <MTLTexture>,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setTileTextures:withRange:,	SetTiletexturesWithrange,	void, const id <MTLTexture> *,NSRange);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setTileSamplerState:atIndex:,	SetTilesamplerstateAtindex,	void,id <MTLSamplerState>,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setTileSamplerStates:withRange:,	SetTilesamplerstatesWithrange,	void, const id <MTLSamplerState> *,NSRange);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setTileSamplerState:lodMinClamp:lodMaxClamp:atIndex:,	SetTilesamplerstateLodminclampLodmaxclampAtindex,	void,id <MTLSamplerState>,float,float,NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setTileSamplerStates:lodMinClamps:lodMaxClamps:withRange:,	SetTilesamplerstatesLodminclampsLodmaxclampsWithrange,	void, const id <MTLSamplerState> *,const float *,const float *,NSRange);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, dispatchThreadsPerTile:, dispatchThreadsPerTile,	void, MTLPPSize);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, tileWidth,	tileWidth,	NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, tileHeight,	tileHeight,	NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderCommandEncoder>, setThreadgroupMemoryLength:offset:atIndex:,	setThreadgroupMemoryLength,	void, NSUInteger, NSUInteger, NSUInteger);
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<id<MTLRenderCommandEncoder>, InterposeClass> : public IMPTable<id<MTLRenderCommandEncoder>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLRenderCommandEncoder>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableCommandEncoder<id<MTLRenderCommandEncoder>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(Setrenderpipelinestate, C);
		INTERPOSE_REGISTRATION(SetvertexbytesLengthAtindex, C);
		INTERPOSE_REGISTRATION(SetvertexbufferOffsetAtindex, C);
		INTERPOSE_REGISTRATION(SetvertexbufferoffsetAtindex, C);
		INTERPOSE_REGISTRATION(SetvertexbuffersOffsetsWithrange, C);
		INTERPOSE_REGISTRATION(SetvertextextureAtindex, C);
		INTERPOSE_REGISTRATION(SetvertextexturesWithrange, C);
		INTERPOSE_REGISTRATION(SetvertexsamplerstateAtindex, C);
		INTERPOSE_REGISTRATION(SetvertexsamplerstatesWithrange, C);
		INTERPOSE_REGISTRATION(SetvertexsamplerstateLodminclampLodmaxclampAtindex, C);
		INTERPOSE_REGISTRATION(SetvertexsamplerstatesLodminclampsLodmaxclampsWithrange, C);
		INTERPOSE_REGISTRATION(Setviewport, C);
		INTERPOSE_REGISTRATION(SetviewportsCount, C);
		INTERPOSE_REGISTRATION(Setfrontfacingwinding, C);
		INTERPOSE_REGISTRATION(Setcullmode, C);
		INTERPOSE_REGISTRATION(Setdepthclipmode, C);
		INTERPOSE_REGISTRATION(SetdepthbiasSlopescaleClamp, C);
		INTERPOSE_REGISTRATION(Setscissorrect, C);
		INTERPOSE_REGISTRATION(SetscissorrectsCount, C);
		INTERPOSE_REGISTRATION(Settrianglefillmode, C);
		INTERPOSE_REGISTRATION(SetfragmentbytesLengthAtindex, C);
		INTERPOSE_REGISTRATION(SetfragmentbufferOffsetAtindex, C);
		INTERPOSE_REGISTRATION(SetfragmentbufferoffsetAtindex, C);
		INTERPOSE_REGISTRATION(SetfragmentbuffersOffsetsWithrange, C);
		INTERPOSE_REGISTRATION(SetfragmenttextureAtindex, C);
		INTERPOSE_REGISTRATION(SetfragmenttexturesWithrange, C);
		INTERPOSE_REGISTRATION(SetfragmentsamplerstateAtindex, C);
		INTERPOSE_REGISTRATION(SetfragmentsamplerstatesWithrange, C);
		INTERPOSE_REGISTRATION(SetfragmentsamplerstateLodminclampLodmaxclampAtindex, C);
		INTERPOSE_REGISTRATION(SetfragmentsamplerstatesLodminclampsLodmaxclampsWithrange, C);
		INTERPOSE_REGISTRATION(SetblendcolorredGreenBlueAlpha, C);
		INTERPOSE_REGISTRATION(Setdepthstencilstate, C);
		INTERPOSE_REGISTRATION(Setstencilreferencevalue, C);
		INTERPOSE_REGISTRATION(SetstencilfrontreferencevalueBackreferencevalue, C);
		INTERPOSE_REGISTRATION(SetvisibilityresultmodeOffset, C);
		INTERPOSE_REGISTRATION(SetcolorstoreactionAtindex, C);
		INTERPOSE_REGISTRATION(Setdepthstoreaction, C);
		INTERPOSE_REGISTRATION(Setstencilstoreaction, C);
		INTERPOSE_REGISTRATION(SetcolorstoreactionoptionsAtindex, C);
		INTERPOSE_REGISTRATION(Setdepthstoreactionoptions, C);
		INTERPOSE_REGISTRATION(Setstencilstoreactionoptions, C);
		INTERPOSE_REGISTRATION(DrawprimitivesVertexstartVertexcountInstancecount, C);
		INTERPOSE_REGISTRATION(DrawprimitivesVertexstartVertexcount, C);
		INTERPOSE_REGISTRATION(DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffsetInstancecount, C);
		INTERPOSE_REGISTRATION(DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffset, C);
		INTERPOSE_REGISTRATION(DrawprimitivesVertexstartVertexcountInstancecountBaseinstance, C);
		INTERPOSE_REGISTRATION(DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffsetInstancecountBasevertexBaseinstance, C);
		INTERPOSE_REGISTRATION(DrawprimitivesIndirectbufferIndirectbufferoffset, C);
		INTERPOSE_REGISTRATION(DrawindexedprimitivesIndextypeIndexbufferIndexbufferoffsetIndirectbufferIndirectbufferoffset, C);
		INTERPOSE_REGISTRATION(Texturebarrier, C);
		INTERPOSE_REGISTRATION(UpdatefenceAfterstages, C);
		INTERPOSE_REGISTRATION(WaitforfenceBeforestages, C);
		INTERPOSE_REGISTRATION(SettessellationfactorbufferOffsetInstancestride, C);
		INTERPOSE_REGISTRATION(Settessellationfactorscale, C);
		INTERPOSE_REGISTRATION(DrawpatchesPatchstartPatchcountPatchindexbufferPatchindexbufferoffsetInstancecountBaseinstance, C);
		INTERPOSE_REGISTRATION(DrawpatchesPatchindexbufferPatchindexbufferoffsetIndirectbufferIndirectbufferoffset, C);
		INTERPOSE_REGISTRATION(DrawindexedpatchesPatchstartPatchcountPatchindexbufferPatchindexbufferoffsetControlpointindexbufferControlpointindexbufferoffsetInstancecountBaseinstance, C);
		INTERPOSE_REGISTRATION(DrawindexedpatchesPatchindexbufferPatchindexbufferoffsetControlpointindexbufferControlpointindexbufferoffsetIndirectbufferIndirectbufferoffset, C);
		INTERPOSE_REGISTRATION(UseresourceUsage, C);
		INTERPOSE_REGISTRATION(UseresourcesCountUsage, C);
		INTERPOSE_REGISTRATION(Useheap, C);
		INTERPOSE_REGISTRATION(UseheapsCount, C);
		INTERPOSE_REGISTRATION(SetTilebytesLengthAtindex, C);
		INTERPOSE_REGISTRATION(SetTilebufferOffsetAtindex, C);
		INTERPOSE_REGISTRATION(SetTilebufferoffsetAtindex, C);
		INTERPOSE_REGISTRATION(SetTilebuffersOffsetsWithrange, C);
		INTERPOSE_REGISTRATION(SetTiletextureAtindex, C);
		INTERPOSE_REGISTRATION(SetTiletexturesWithrange, C);
		INTERPOSE_REGISTRATION(SetTilesamplerstateAtindex, C);
		INTERPOSE_REGISTRATION(SetTilesamplerstatesWithrange, C);
		INTERPOSE_REGISTRATION(SetTilesamplerstateLodminclampLodmaxclampAtindex, C);
		INTERPOSE_REGISTRATION(SetTilesamplerstatesLodminclampsLodmaxclampsWithrange, C);
		INTERPOSE_REGISTRATION(setThreadgroupMemoryLength, C);
		INTERPOSE_REGISTRATION(dispatchThreadsPerTile, C);
	}
};

MTLPP_END

#endif /* imp_RenderCommandEncoder_hpp */
