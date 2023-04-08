/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLRenderCommandEncoder.h>
#include <Metal/MTLBuffer.h>
#include <Metal/MTLRenderPipeline.h>
#include <Metal/MTLSampler.h>

#include "render_command_encoder.hpp"
#include "buffer.hpp"
#include "depth_stencil.hpp"
#include "render_pipeline.hpp"
#include "sampler.hpp"
#include "texture.hpp"
#include "heap.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    void RenderCommandEncoder::SetRenderPipelineState(const RenderPipelineState& pipelineState)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setrenderpipelinestate(m_ptr, pipelineState.GetPtr());
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setRenderPipelineState:(id<MTLRenderPipelineState>)pipelineState.GetPtr()];
#endif
    }

    void RenderCommandEncoder::SetVertexData(const void* bytes, NSUInteger length, NSUInteger index)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 8_3)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetvertexbytesLengthAtindex(m_ptr, bytes, length, index);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setVertexBytes:bytes length:length atIndex:index];
#endif
#endif
    }

    void RenderCommandEncoder::SetVertexBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetvertexbufferOffsetAtindex(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset + buffer.GetOffset(), index);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setVertexBuffer:(id<MTLBuffer>)buffer.GetPtr()
                                                              offset:offset + buffer.GetOffset()
                                                             atIndex:index];
#endif
    }
	
    void RenderCommandEncoder::SetVertexBufferOffset(NSUInteger offset, NSUInteger index)
    {
#if MTLPP_IS_AVAILABLE(10_11, 8_3)
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetvertexbufferoffsetAtindex(m_ptr, offset, index);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setVertexBufferOffset:offset atIndex:index];
#endif
#endif
    }

	void RenderCommandEncoder::SetVertexBuffers(const Buffer* buffers, const NSUInteger* offsets, const ns::Range& range)
    {
        Validate();
		id<MTLBuffer>* array = (id<MTLBuffer>*)alloca(range.Length * sizeof(id<MTLBuffer>));
		NSUInteger* theOffsets = (NSUInteger*)alloca(range.Length * sizeof(NSUInteger));
		for (NSUInteger i = 0; i < range.Length; i++)
		{
			array[i] = (id<MTLBuffer>)buffers[i].GetPtr();
			theOffsets[i] = offsets[i] + (buffers[i] ? buffers[i].GetOffset() : 0);
		}
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetvertexbuffersOffsetsWithrange(m_ptr, (const id<MTLBuffer>*)array, (NSUInteger const*)theOffsets, NSMakeRange(range.Location, range.Length));
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setVertexBuffers:array offsets:theOffsets withRange:NSMakeRange(range.Location, range.Length)];
#endif
    }

    void RenderCommandEncoder::SetVertexTexture(const Texture& texture, NSUInteger index)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetvertextextureAtindex(m_ptr, (id<MTLTexture>)texture.GetPtr(), index);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setVertexTexture:(id<MTLTexture>)texture.GetPtr()
                                                              atIndex:index];
#endif
    }


    void RenderCommandEncoder::SetVertexTextures(const Texture* textures, const ns::Range& range)
    {
        Validate();
		id<MTLTexture>* array = (id<MTLTexture>*)alloca(range.Length * sizeof(id<MTLTexture>));
		for (NSUInteger i = 0; i < range.Length; i++)
		{
			array[i] = (id<MTLTexture>)textures[i].GetPtr();
		}
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetvertextexturesWithrange(m_ptr, (const id<MTLTexture>*)array, NSMakeRange(range.Location, range.Length));
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setVertexTextures:array withRange:NSMakeRange(range.Location, range.Length)];
#endif
    }

    void RenderCommandEncoder::SetVertexSamplerState(const SamplerState& sampler, NSUInteger index)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetvertexsamplerstateAtindex(m_ptr, sampler.GetPtr(), index);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setVertexSamplerState:(id<MTLSamplerState>)sampler.GetPtr()
                                                                   atIndex:index];
#endif

    }

    void RenderCommandEncoder::SetVertexSamplerStates(const SamplerState::Type* samplers, const ns::Range& range)
    {
        Validate();

#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetvertexsamplerstatesWithrange(m_ptr, samplers, NSMakeRange(range.Location, range.Length));
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setVertexSamplerStates:samplers withRange:NSMakeRange(range.Location, range.Length)];
#endif
    }

    void RenderCommandEncoder::SetVertexSamplerState(const SamplerState& sampler, float lodMinClamp, float lodMaxClamp, NSUInteger index)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetvertexsamplerstateLodminclampLodmaxclampAtindex(m_ptr, sampler.GetPtr(), lodMinClamp, lodMaxClamp, index);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setVertexSamplerState:(id<MTLSamplerState>)sampler.GetPtr()
                                                               lodMinClamp:lodMinClamp
                                                               lodMaxClamp:lodMaxClamp
                                                                   atIndex:index];
#endif
    }

    void RenderCommandEncoder::SetVertexSamplerStates(const SamplerState::Type* samplers, const float* lodMinClamps, const float* lodMaxClamps, const ns::Range& range)
    {
        Validate();

#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetvertexsamplerstatesLodminclampsLodmaxclampsWithrange(m_ptr, samplers, lodMinClamps, lodMaxClamps, NSMakeRange(range.Location, range.Length));
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setVertexSamplerStates:samplers
                                                               lodMinClamps:lodMinClamps
                                                               lodMaxClamps:lodMaxClamps
                                                                  withRange:NSMakeRange(range.Location, range.Length)];
#endif
    }

    void RenderCommandEncoder::SetViewport(const Viewport& viewport)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setviewport(m_ptr, viewport);
#else
        MTLViewport mtlViewport = { viewport.originX, viewport.originY, viewport.width, viewport.height, viewport.znear, viewport.zfar };
        [(id<MTLRenderCommandEncoder>)m_ptr setViewport:mtlViewport];
#endif
    }
	
	void RenderCommandEncoder::SetViewports(const Viewport* viewports, NSUInteger count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_13)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetviewportsCount(m_ptr, viewports, count);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setViewports:(const MTLViewport *)viewports count:count];
#endif
#endif
	}

    void RenderCommandEncoder::SetFrontFacingWinding(Winding frontFacingWinding)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setfrontfacingwinding(m_ptr, (MTLWinding)frontFacingWinding);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setFrontFacingWinding:MTLWinding(frontFacingWinding)];
#endif
    }

    void RenderCommandEncoder::SetCullMode(CullMode cullMode)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setcullmode(m_ptr, (MTLCullMode)cullMode);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setCullMode:MTLCullMode(cullMode)];
#endif
    }

    void RenderCommandEncoder::SetDepthClipMode(DepthClipMode depthClipMode)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setdepthclipmode(m_ptr, (MTLDepthClipMode)depthClipMode);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setDepthClipMode:MTLDepthClipMode(depthClipMode)];
#endif
#endif
    }

    void RenderCommandEncoder::SetDepthBias(float depthBias, float slopeScale, float clamp)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetdepthbiasSlopescaleClamp(m_ptr, depthBias, slopeScale, clamp);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setDepthBias:depthBias slopeScale:slopeScale clamp:clamp];
#endif
    }

    void RenderCommandEncoder::SetScissorRect(const ScissorRect& rect)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setscissorrect(m_ptr, rect);
#else
        MTLScissorRect mtlRect { rect.x, rect.y, rect.width, rect.height };
        [(id<MTLRenderCommandEncoder>)m_ptr setScissorRect:mtlRect];
#endif
    }
	
	void RenderCommandEncoder::SetScissorRects(const ScissorRect* rect, NSUInteger count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_13)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetscissorrectsCount(m_ptr, rect, count);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setScissorRects:(const MTLScissorRect *)rect count:count];
#endif
#endif
	}

    void RenderCommandEncoder::SetTriangleFillMode(TriangleFillMode fillMode)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Settrianglefillmode(m_ptr, (MTLTriangleFillMode)fillMode);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setTriangleFillMode:MTLTriangleFillMode(fillMode)];
#endif
    }

	void RenderCommandEncoder::SetFragmentData(const void* bytes, NSUInteger length, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_11, 8_3)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetfragmentbytesLengthAtindex(m_ptr, bytes, length, index);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setFragmentBytes:bytes
													  length:length
													 atIndex:index];
#endif
#endif
	}

	void RenderCommandEncoder::SetFragmentBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index)
	{
		Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetfragmentbufferOffsetAtindex(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset + buffer.GetOffset(), index);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setFragmentBuffer:(id<MTLBuffer>)buffer.GetPtr()
													   offset:offset + buffer.GetOffset()
													  atIndex:index];
#endif
	}
	void RenderCommandEncoder::SetFragmentBufferOffset(NSUInteger offset, NSUInteger index)
	{
#if MTLPP_IS_AVAILABLE(10_11, 8_3)
		Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetfragmentbufferoffsetAtindex(m_ptr, offset, index);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setFragmentBufferOffset:offset atIndex:index];
#endif
#endif
	}

	void RenderCommandEncoder::SetFragmentBuffers(const Buffer* buffers, const NSUInteger* offsets, const ns::Range& range)
	{
		Validate();
		id<MTLBuffer>* array = (id<MTLBuffer>*)alloca(range.Length * sizeof(id<MTLBuffer>));
		NSUInteger* theOffsets = (NSUInteger*)alloca(range.Length * sizeof(NSUInteger));
		for (NSUInteger i = 0; i < range.Length; i++)
		{
			array[i] = (id<MTLBuffer>)buffers[i].GetPtr();
			theOffsets[i] = offsets[i] + (buffers[i] ? buffers[i].GetOffset() : 0);
		}
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetfragmentbuffersOffsetsWithrange(m_ptr, (const id<MTLBuffer>*)array, (NSUInteger const*)theOffsets, NSMakeRange(range.Location, range.Length));
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setFragmentBuffers:(id<MTLBuffer>  _Nullable const * _Nonnull)array offsets:(const NSUInteger * _Nonnull)theOffsets withRange:NSMakeRange(range.Location, range.Length)];
#endif
	}

	void RenderCommandEncoder::SetFragmentTexture(const Texture& texture, NSUInteger index)
	{
		Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetfragmenttextureAtindex(m_ptr, (id<MTLTexture>)texture.GetPtr(), index);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setFragmentTexture:(id<MTLTexture>)texture.GetPtr()
													   atIndex:index];
#endif
	}

	void RenderCommandEncoder::SetFragmentTextures(const Texture* textures, const ns::Range& range)
	{
		Validate();
		id<MTLTexture>* array = (id<MTLTexture>*)alloca(range.Length * sizeof(id<MTLTexture>));
		for (NSUInteger i = 0; i < range.Length; i++)
		{
			array[i] = (id<MTLTexture>)textures[i].GetPtr();
		}
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetfragmenttexturesWithrange(m_ptr, (const id<MTLTexture>*)array, NSMakeRange(range.Location, range.Length));
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setFragmentTextures:array withRange:NSMakeRange(range.Location, range.Length)];
#endif
	}

	void RenderCommandEncoder::SetFragmentSamplerState(const SamplerState& sampler, NSUInteger index)
	{
		Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetfragmentsamplerstateAtindex(m_ptr, sampler.GetPtr(), index);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setFragmentSamplerState:sampler.GetPtr() atIndex:index];
#endif
	}

	void RenderCommandEncoder::SetFragmentSamplerStates(const SamplerState::Type* samplers, const ns::Range& range)
	{
		Validate();

#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetfragmentsamplerstatesWithrange(m_ptr, samplers, NSMakeRange(range.Location, range.Length));
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setFragmentSamplerStates:samplers withRange:NSMakeRange(range.Location, range.Length)];
#endif
	}

	void RenderCommandEncoder::SetFragmentSamplerState(const SamplerState& sampler, float lodMinClamp, float lodMaxClamp, NSUInteger index)
	{
		Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetfragmentsamplerstateLodminclampLodmaxclampAtindex(m_ptr, sampler.GetPtr(), lodMinClamp, lodMaxClamp, index);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setFragmentSamplerState:sampler.GetPtr() lodMinClamp:lodMinClamp lodMaxClamp:lodMaxClamp atIndex:index];
#endif
	}

	void RenderCommandEncoder::SetFragmentSamplerStates(const SamplerState::Type* samplers, const float* lodMinClamps, const float* lodMaxClamps, const ns::Range& range)
	{
		Validate();

#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetfragmentsamplerstatesLodminclampsLodmaxclampsWithrange(m_ptr, samplers, lodMinClamps, lodMaxClamps, NSMakeRange(range.Location, range.Length));
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setFragmentSamplerStates:(id<MTLSamplerState>  _Nullable const * _Nonnull)samplers lodMinClamps:(const float * _Nonnull)lodMinClamps lodMaxClamps:(const float * _Nonnull)lodMaxClamps withRange:NSMakeRange(range.Location, range.Length)];
#endif
    }

    void RenderCommandEncoder::SetBlendColor(float red, float green, float blue, float alpha)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetblendcolorredGreenBlueAlpha(m_ptr, red, green, blue, alpha);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setBlendColorRed:red green:green blue:blue alpha:alpha];
#endif
    }

    void RenderCommandEncoder::SetDepthStencilState(const DepthStencilState& depthStencilState)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setdepthstencilstate(m_ptr, depthStencilState.GetPtr());
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setDepthStencilState:(id<MTLDepthStencilState>)depthStencilState.GetPtr()];
#endif
    }

    void RenderCommandEncoder::SetStencilReferenceValue(uint32_t referenceValue)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setstencilreferencevalue(m_ptr, referenceValue);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setStencilReferenceValue:referenceValue];
#endif
    }

    void RenderCommandEncoder::SetStencilReferenceValue(uint32_t frontReferenceValue, uint32_t backReferenceValue)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetstencilfrontreferencevalueBackreferencevalue(m_ptr, frontReferenceValue, backReferenceValue);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setStencilFrontReferenceValue:frontReferenceValue backReferenceValue:backReferenceValue];
#endif
    }

    void RenderCommandEncoder::SetVisibilityResultMode(VisibilityResultMode mode, NSUInteger offset)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetvisibilityresultmodeOffset(m_ptr, (MTLVisibilityResultMode)mode, offset);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setVisibilityResultMode:MTLVisibilityResultMode(mode) offset:offset];
#endif
    }

    void RenderCommandEncoder::SetColorStoreAction(StoreAction storeAction, NSUInteger colorAttachmentIndex)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetcolorstoreactionAtindex(m_ptr, (MTLStoreAction) storeAction, colorAttachmentIndex);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setColorStoreAction:MTLStoreAction(storeAction) atIndex:colorAttachmentIndex];
#endif
#endif
    }

    void RenderCommandEncoder::SetDepthStoreAction(StoreAction storeAction)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setdepthstoreaction(m_ptr, (MTLStoreAction)storeAction);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setDepthStoreAction:MTLStoreAction(storeAction)];
#endif
#endif
    }

    void RenderCommandEncoder::SetStencilStoreAction(StoreAction storeAction)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setstencilstoreaction(m_ptr, (MTLStoreAction)storeAction);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setStencilStoreAction:MTLStoreAction(storeAction)];
#endif
#endif
    }
	
	void RenderCommandEncoder::SetColorStoreActionOptions(StoreActionOptions storeAction, NSUInteger colorAttachmentIndex)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetcolorstoreactionoptionsAtindex(m_ptr, (MTLStoreActionOptions)storeAction, colorAttachmentIndex);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setColorStoreActionOptions:(MTLStoreActionOptions)storeAction atIndex:colorAttachmentIndex];
#endif
#endif
	}
	
	void RenderCommandEncoder::SetDepthStoreActionOptions(StoreActionOptions storeAction)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setdepthstoreactionoptions(m_ptr, (MTLStoreActionOptions)storeAction);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setDepthStoreActionOptions:(MTLStoreActionOptions)storeAction];
#endif
#endif
	}
	
	void RenderCommandEncoder::SetStencilStoreActionOptions(StoreActionOptions storeAction)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setstencilstoreactionoptions(m_ptr, (MTLStoreActionOptions)storeAction);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setStencilStoreActionOptions:(MTLStoreActionOptions)storeAction];
#endif
#endif
	}
	
    void RenderCommandEncoder::Draw(PrimitiveType primitiveType, NSUInteger vertexStart, NSUInteger vertexCount)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->DrawprimitivesVertexstartVertexcount(m_ptr, (MTLPrimitiveType)primitiveType, vertexStart, vertexCount);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr drawPrimitives:MTLPrimitiveType(primitiveType)
                                                        vertexStart:vertexStart
                                                        vertexCount:vertexCount];
#endif
    }

    void RenderCommandEncoder::Draw(PrimitiveType primitiveType, NSUInteger vertexStart, NSUInteger vertexCount, NSUInteger instanceCount)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->DrawprimitivesVertexstartVertexcountInstancecount(m_ptr, (MTLPrimitiveType)primitiveType, vertexStart, vertexCount, instanceCount);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr drawPrimitives:MTLPrimitiveType(primitiveType)
                                                        vertexStart:vertexStart
                                                        vertexCount:vertexCount
                                                      instanceCount:instanceCount];
#endif
#endif
    }

    void RenderCommandEncoder::Draw(PrimitiveType primitiveType, NSUInteger vertexStart, NSUInteger vertexCount, NSUInteger instanceCount, NSUInteger baseInstance)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->DrawprimitivesVertexstartVertexcountInstancecountBaseinstance(m_ptr, (MTLPrimitiveType)primitiveType, vertexStart, vertexCount, instanceCount, baseInstance);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr drawPrimitives:MTLPrimitiveType(primitiveType)
                                                        vertexStart:vertexStart
                                                        vertexCount:vertexCount
                                                      instanceCount:instanceCount
                                                       baseInstance:baseInstance];
#endif
#endif
    }

    void RenderCommandEncoder::Draw(PrimitiveType primitiveType, Buffer const& indirectBuffer, NSUInteger indirectBufferOffset)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->DrawprimitivesIndirectbufferIndirectbufferoffset(m_ptr, MTLPrimitiveType(primitiveType), (id<MTLBuffer>)indirectBuffer.GetPtr(), indirectBufferOffset + indirectBuffer.GetOffset());
#else
        [(id<MTLRenderCommandEncoder>)m_ptr drawPrimitives:MTLPrimitiveType(primitiveType)
                                                     indirectBuffer:(id<MTLBuffer>)indirectBuffer.GetPtr()
                                               indirectBufferOffset:indirectBufferOffset + indirectBuffer.GetOffset()];
#endif
    }

    void RenderCommandEncoder::DrawIndexed(PrimitiveType primitiveType, NSUInteger indexCount, IndexType indexType, const Buffer& indexBuffer, NSUInteger indexBufferOffset)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffset(m_ptr,MTLPrimitiveType(primitiveType), indexCount, (MTLIndexType)indexType, (id<MTLBuffer>)indexBuffer.GetPtr(), indexBufferOffset + indexBuffer.GetOffset());
#else
        [(id<MTLRenderCommandEncoder>)m_ptr drawIndexedPrimitives:MTLPrimitiveType(primitiveType)
                                                                indexCount:indexCount
                                                                 indexType:MTLIndexType(indexType)
                                                               indexBuffer:(id<MTLBuffer>)indexBuffer.GetPtr()
                                                         indexBufferOffset:indexBufferOffset + indexBuffer.GetOffset()];
#endif
    }

    void RenderCommandEncoder::DrawIndexed(PrimitiveType primitiveType, NSUInteger indexCount, IndexType indexType, const Buffer& indexBuffer, NSUInteger indexBufferOffset, NSUInteger instanceCount)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffsetInstancecount(m_ptr,MTLPrimitiveType(primitiveType), indexCount, (MTLIndexType)indexType, (id<MTLBuffer>)indexBuffer.GetPtr(), indexBufferOffset + indexBuffer.GetOffset(), instanceCount);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr drawIndexedPrimitives:MTLPrimitiveType(primitiveType)
                                                                indexCount:indexCount indexType:MTLIndexType(indexType)
                                                               indexBuffer:(id<MTLBuffer>)indexBuffer.GetPtr()
                                                         indexBufferOffset:indexBufferOffset + indexBuffer.GetOffset()
													instanceCount:instanceCount];
#endif
#endif
    }

    void RenderCommandEncoder::DrawIndexed(PrimitiveType primitiveType, NSUInteger indexCount, IndexType indexType, const Buffer& indexBuffer, NSUInteger indexBufferOffset, NSUInteger instanceCount, NSUInteger baseVertex, NSUInteger baseInstance)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffsetInstancecountBasevertexBaseinstance(m_ptr,MTLPrimitiveType(primitiveType), indexCount, (MTLIndexType)indexType, (id<MTLBuffer>)indexBuffer.GetPtr(), indexBufferOffset + indexBuffer.GetOffset(), instanceCount, baseVertex, baseInstance);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr drawIndexedPrimitives:MTLPrimitiveType(primitiveType)
                                                                indexCount:indexCount
                                                                 indexType:MTLIndexType(indexType)
                                                               indexBuffer:(id<MTLBuffer>)indexBuffer.GetPtr()
                                                         indexBufferOffset:indexBufferOffset + indexBuffer.GetOffset()
                                                             instanceCount:instanceCount
                                                                baseVertex:baseVertex
                                                              baseInstance:baseInstance];
#endif
#endif
    }

    void RenderCommandEncoder::DrawIndexed(PrimitiveType primitiveType, IndexType indexType, const Buffer& indexBuffer, NSUInteger indexBufferOffset, const Buffer& indirectBuffer, NSUInteger indirectBufferOffset)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->DrawindexedprimitivesIndextypeIndexbufferIndexbufferoffsetIndirectbufferIndirectbufferoffset(m_ptr, MTLPrimitiveType(primitiveType), MTLIndexType(indexType), (id<MTLBuffer>)indexBuffer.GetPtr(), indexBufferOffset + indexBuffer.GetOffset(), (id<MTLBuffer>)indirectBuffer.GetPtr(), indirectBufferOffset + indirectBuffer.GetOffset());
#else
        [(id<MTLRenderCommandEncoder>)m_ptr drawIndexedPrimitives:MTLPrimitiveType(primitiveType)
                                                                 indexType:MTLIndexType(indexType)
                                                               indexBuffer:(id<MTLBuffer>)indexBuffer.GetPtr()
                                                         indexBufferOffset:indexBufferOffset + indexBuffer.GetOffset()
                                                            indirectBuffer:(id<MTLBuffer>)indirectBuffer.GetPtr()
                                                      indirectBufferOffset:indirectBufferOffset + indirectBuffer.GetOffset()];
#endif
    }

    void RenderCommandEncoder::TextureBarrier()
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Texturebarrier(m_ptr);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr textureBarrier];
#endif
#endif
    }

    void RenderCommandEncoder::UpdateFence(const Fence& fence, RenderStages afterStages)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->UpdatefenceAfterstages(m_ptr, fence.GetPtr(), (MTLRenderStages)afterStages);
#else
		if(@available(macOS 10.13, iOS 10.0, *))
		[(id<MTLRenderCommandEncoder>)m_ptr updateFence:(id<MTLFence>)fence.GetPtr() afterStages:MTLRenderStages(afterStages)];
#endif
#endif
    }

    void RenderCommandEncoder::WaitForFence(const Fence& fence, RenderStages beforeStages)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->WaitforfenceBeforestages(m_ptr, fence.GetPtr(), (MTLRenderStages)beforeStages);
#else
		if(@available(macOS 10.13, iOS 10.0, *))
		[(id<MTLRenderCommandEncoder>)m_ptr waitForFence:(id<MTLFence>)fence.GetPtr() beforeStages:MTLRenderStages(beforeStages)];
#endif
#endif
    }

    void RenderCommandEncoder::SetTessellationFactorBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger instanceStride)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SettessellationfactorbufferOffsetInstancestride(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset + buffer.GetOffset(), instanceStride);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setTessellationFactorBuffer:(id<MTLBuffer>)buffer.GetPtr() offset:offset + buffer.GetOffset() instanceStride:instanceStride];
#endif
#endif
    }

    void RenderCommandEncoder::SetTessellationFactorScale(float scale)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Settessellationfactorscale(m_ptr, scale);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr setTessellationFactorScale:scale];
#endif
#endif
    }

    void RenderCommandEncoder::DrawPatches(NSUInteger numberOfPatchControlPoints, NSUInteger patchStart, NSUInteger patchCount, const Buffer& patchIndexBuffer, NSUInteger patchIndexBufferOffset, NSUInteger instanceCount, NSUInteger baseInstance)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->DrawpatchesPatchstartPatchcountPatchindexbufferPatchindexbufferoffsetInstancecountBaseinstance(m_ptr, numberOfPatchControlPoints, patchStart, patchCount, (id<MTLBuffer>)patchIndexBuffer.GetPtr(), patchIndexBufferOffset + patchIndexBuffer.GetOffset(), instanceCount, baseInstance);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr drawPatches:numberOfPatchControlPoints
                                                      patchStart:patchStart
                                                      patchCount:patchCount
                                                patchIndexBuffer:(id<MTLBuffer>)patchIndexBuffer.GetPtr()
                                          patchIndexBufferOffset:patchIndexBufferOffset + patchIndexBuffer.GetOffset()
                                                   instanceCount:instanceCount
                                                    baseInstance:baseInstance];
#endif
#endif
    }

    void RenderCommandEncoder::DrawPatches(NSUInteger numberOfPatchControlPoints, const Buffer& patchIndexBuffer, NSUInteger patchIndexBufferOffset, const Buffer& indirectBuffer, NSUInteger indirectBufferOffset)
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_12)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->DrawpatchesPatchindexbufferPatchindexbufferoffsetIndirectbufferIndirectbufferoffset(m_ptr, numberOfPatchControlPoints, (id<MTLBuffer>)patchIndexBuffer.GetPtr(), patchIndexBufferOffset + patchIndexBuffer.GetOffset(), (id<MTLBuffer>)indirectBuffer.GetPtr(), indirectBufferOffset + indirectBuffer.GetOffset());
#else
		if (@available(macOS 10.12, *))
		{
			[(id<MTLRenderCommandEncoder>)m_ptr drawPatches:numberOfPatchControlPoints
													patchIndexBuffer:(id<MTLBuffer>)patchIndexBuffer.GetPtr()
											  patchIndexBufferOffset:patchIndexBufferOffset + patchIndexBuffer.GetOffset()
													  indirectBuffer:(id<MTLBuffer>)indirectBuffer.GetPtr()
												indirectBufferOffset:indirectBufferOffset + indirectBuffer.GetOffset()];
		}
#endif
#endif
    }

    void RenderCommandEncoder::DrawIndexedPatches(NSUInteger numberOfPatchControlPoints, NSUInteger patchStart, NSUInteger patchCount, const Buffer& patchIndexBuffer, NSUInteger patchIndexBufferOffset, const Buffer& controlPointIndexBuffer, NSUInteger controlPointIndexBufferOffset, NSUInteger instanceCount, NSUInteger baseInstance)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->DrawindexedpatchesPatchstartPatchcountPatchindexbufferPatchindexbufferoffsetControlpointindexbufferControlpointindexbufferoffsetInstancecountBaseinstance(m_ptr, numberOfPatchControlPoints, patchStart, patchCount, (id<MTLBuffer>)patchIndexBuffer.GetPtr(), patchIndexBufferOffset + patchIndexBuffer.GetOffset(), (id<MTLBuffer>)controlPointIndexBuffer.GetPtr(), controlPointIndexBufferOffset + controlPointIndexBuffer.GetOffset(), instanceCount, baseInstance);
#else
        [(id<MTLRenderCommandEncoder>)m_ptr drawIndexedPatches:numberOfPatchControlPoints
                                                             patchStart:patchStart
                                                             patchCount:patchCount
                                                       patchIndexBuffer:(id<MTLBuffer>)patchIndexBuffer.GetPtr()
                                                 patchIndexBufferOffset:patchIndexBufferOffset + patchIndexBuffer.GetOffset()
                                                controlPointIndexBuffer:(id<MTLBuffer>)controlPointIndexBuffer.GetPtr()
                                          controlPointIndexBufferOffset:controlPointIndexBufferOffset + controlPointIndexBuffer.GetOffset()
                                                          instanceCount:instanceCount
                                                           baseInstance:baseInstance];
#endif
#endif
    }

    void RenderCommandEncoder::DrawIndexedPatches(NSUInteger numberOfPatchControlPoints, const Buffer& patchIndexBuffer, NSUInteger patchIndexBufferOffset, const Buffer& controlPointIndexBuffer, NSUInteger controlPointIndexBufferOffset, const Buffer& indirectBuffer, NSUInteger indirectBufferOffset)
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_12)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->DrawindexedpatchesPatchindexbufferPatchindexbufferoffsetControlpointindexbufferControlpointindexbufferoffsetIndirectbufferIndirectbufferoffset(m_ptr, numberOfPatchControlPoints, (id<MTLBuffer>)patchIndexBuffer.GetPtr(), patchIndexBufferOffset + patchIndexBuffer.GetOffset(), (id<MTLBuffer>)controlPointIndexBuffer.GetPtr(), controlPointIndexBufferOffset + controlPointIndexBuffer.GetOffset(), (id<MTLBuffer>)indirectBuffer.GetPtr(), indirectBufferOffset + indirectBuffer.GetOffset());
#else
		if (@available(macOS 10.12, *))
		{
			[(id<MTLRenderCommandEncoder>)m_ptr drawIndexedPatches:numberOfPatchControlPoints
                                                       patchIndexBuffer:(id<MTLBuffer>)patchIndexBuffer.GetPtr()
                                                 patchIndexBufferOffset:patchIndexBufferOffset + patchIndexBuffer.GetOffset()
                                                controlPointIndexBuffer:(id<MTLBuffer>)controlPointIndexBuffer.GetPtr()
                                          controlPointIndexBufferOffset:controlPointIndexBufferOffset + controlPointIndexBuffer.GetOffset()
                                                         indirectBuffer:(id<MTLBuffer>)indirectBuffer.GetPtr()
                                                   indirectBufferOffset:indirectBufferOffset + indirectBuffer.GetOffset()];
		}
#endif
#endif
    }
	
	void RenderCommandEncoder::UseResource(const Resource& resource, ResourceUsage usage)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->UseresourceUsage(m_ptr, resource.GetPtr(), MTLResourceUsage(usage));
#else
		[(id<MTLRenderCommandEncoder>)m_ptr useResource:(id<MTLResource>)resource.GetPtr() usage:(MTLResourceUsage)usage];
#endif
#endif
	}
	
	void RenderCommandEncoder::UseResources(const Resource* resource, NSUInteger count, ResourceUsage usage)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
        id<MTLResource>* array = (id<MTLResource>*)alloca(count * sizeof(id<MTLBuffer>));
        for (NSUInteger i = 0; i < count; i++)
        {
            array[i] = (id<MTLResource>)resource[i].GetPtr();
        }
        
#if MTLPP_CONFIG_IMP_CACHE
		m_table->UseresourcesCountUsage(m_ptr, array, count, MTLResourceUsage(usage));
#else
		[(id<MTLRenderCommandEncoder>)m_ptr useResources:array count:count usage:(MTLResourceUsage)usage];
#endif
#endif
	}
	
	void RenderCommandEncoder::UseHeap(const Heap& heap)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Useheap(m_ptr, heap.GetPtr());
#else
		[(id<MTLRenderCommandEncoder>)m_ptr useHeap:(id<MTLHeap>)heap.GetPtr()];
#endif
#endif
	}
	
	void RenderCommandEncoder::UseHeaps(const Heap::Type* heap, NSUInteger count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->UseheapsCount(m_ptr, heap, count);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr useHeaps:heap count:count];
#endif
#endif
	}
	
	NSUInteger RenderCommandEncoder::GetTileWidth()
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->tileWidth(m_ptr);
#else
		return [m_ptr tileWidth];
#endif
#else
		return 0;
#endif
	}
	
	NSUInteger RenderCommandEncoder::GetTileHeight()
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->tileHeight(m_ptr);
#else
		return [m_ptr tileHeight];
#endif
#else
		return 0;
#endif
	}
	
	void RenderCommandEncoder::SetTileData(const void* bytes, NSUInteger length, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetTilebytesLengthAtindex(m_ptr, bytes, length, index);
#else
		[m_ptr setTileBytes:bytes length:length atIndex:index];
#endif
#endif
	}
	
	void RenderCommandEncoder::SetTileBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetTilebufferOffsetAtindex(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset + buffer.GetOffset(), index);
#else
		[m_ptr setTileBuffer:buffer.GetPtr() offset:offset + buffer.GetOffset() atIndex:index];
#endif
#endif
	}
	void RenderCommandEncoder::SetTileBufferOffset(NSUInteger offset, NSUInteger index)
	{
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetTilebufferoffsetAtindex(m_ptr, offset, index);
#else
		[m_ptr setTileBufferOffset:offset atIndex:index];
#endif
#endif
	}
	
	void RenderCommandEncoder::SetTileBuffers(const Buffer* buffers, const NSUInteger* offsets, const ns::Range& range)
	{
		Validate();
		
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		id<MTLBuffer>* array = (id<MTLBuffer>*)alloca(range.Length * sizeof(id<MTLBuffer>));
		NSUInteger* theOffsets = (NSUInteger*)alloca(range.Length * sizeof(NSUInteger));
		for (NSUInteger i = 0; i < range.Length; i++)
		{
			array[i] = (id<MTLBuffer>)buffers[i].GetPtr();
			theOffsets[i] = offsets[i] + (buffers[i] ? buffers[i].GetOffset() : 0);
		}
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetTilebuffersOffsetsWithrange(m_ptr, (const id<MTLBuffer>*)array, (NSUInteger const*)theOffsets, NSMakeRange(range.Location, range.Length));
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setTileBuffers:array offsets:theOffsets withRange:NSMakeRange(range.Location, range.Length)];
#endif
#endif
	}
	
	void RenderCommandEncoder::SetTileTexture(const Texture& texture, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetTiletextureAtindex(m_ptr, (id<MTLTexture>)texture.GetPtr(), index);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setTileTexture:(id<MTLTexture>)texture.GetPtr()
													 atIndex:index];
#endif
#endif
	}
	
	
	void RenderCommandEncoder::SetTileTextures(const Texture* textures, const ns::Range& range)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		id<MTLTexture>* array = (id<MTLTexture>*)alloca(range.Length * sizeof(id<MTLTexture>));
		for (NSUInteger i = 0; i < range.Length; i++)
		{
			array[i] = (id<MTLTexture>)textures[i].GetPtr();
		}
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetTiletexturesWithrange(m_ptr, (const id<MTLTexture>*)array, NSMakeRange(range.Location, range.Length));
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setTileTextures:array withRange:NSMakeRange(range.Location, range.Length)];
#endif
#endif
	}
	
	void RenderCommandEncoder::SetTileSamplerState(const SamplerState& sampler, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetTilesamplerstateAtindex(m_ptr, sampler.GetPtr(), index);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setTileSamplerState:(id<MTLSamplerState>)sampler.GetPtr()
														  atIndex:index];
#endif
#endif
	}
	
	void RenderCommandEncoder::SetTileSamplerStates(const SamplerState::Type* samplers, const ns::Range& range)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetTilesamplerstatesWithrange(m_ptr, samplers, NSMakeRange(range.Location, range.Length));
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setTileSamplerStates:samplers withRange:NSMakeRange(range.Location, range.Length)];
#endif
#endif
	}
	
	void RenderCommandEncoder::SetTileSamplerState(const SamplerState& sampler, float lodMinClamp, float lodMaxClamp, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetTilesamplerstateLodminclampLodmaxclampAtindex(m_ptr, sampler.GetPtr(), lodMinClamp, lodMaxClamp, index);
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setTileSamplerState:(id<MTLSamplerState>)sampler.GetPtr()
													  lodMinClamp:lodMinClamp
													  lodMaxClamp:lodMaxClamp
														  atIndex:index];
#endif
#endif
	}
	
	void RenderCommandEncoder::SetTileSamplerStates(const SamplerState::Type* samplers, const float* lodMinClamps, const float* lodMaxClamps, const ns::Range& range)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetTilesamplerstatesLodminclampsLodmaxclampsWithrange(m_ptr, samplers, lodMinClamps, lodMaxClamps, NSMakeRange(range.Location, range.Length));
#else
		[(id<MTLRenderCommandEncoder>)m_ptr setTileSamplerStates:samplers
													  lodMinClamps:lodMinClamps
													  lodMaxClamps:lodMaxClamps
														 withRange:NSMakeRange(range.Location, range.Length)];
#endif
#endif
	}
	
	void RenderCommandEncoder::DispatchThreadsPerTile(Size const& threadsPerTile)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->dispatchThreadsPerTile(m_ptr, threadsPerTile);
#else
		[m_ptr dispatchThreadsPerTile:MTLSizeMake(threadsPerTile.width, threadsPerTile.height, threadsPerTile.depth)];
#endif
#endif
	}
	
	void RenderCommandEncoder::SetThreadgroupMemoryLength(NSUInteger length, NSUInteger offset, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setThreadgroupMemoryLength(m_ptr, length, offset, index);
#else
		[m_ptr setThreadgroupMemoryLength:length offset:offset atIndex:index];
#endif
#endif
	}
	
#if MTLPP_CONFIG_VALIDATE
	void ValidatedRenderCommandEncoder::UseResource(const Resource& resource, ResourceUsage usage)
	{
		Validator.UseResource(resource.GetPtr(), usage);
		
		if (class_conformsToProtocol(object_getClass(resource.GetPtr()), @protocol(MTLBuffer)) && ((const Buffer&)resource).GetParentBuffer())
		{
			Validator.UseResource(((const Buffer&)resource).GetPtr(), ns::Range(((const Buffer&)resource).GetOffset(), ((const Buffer&)resource).GetLength()), usage);
		}
		
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		if (@available(macOS 10.13, iOS 11.0, *))
		{
			RenderCommandEncoder::UseResource(resource, usage);
		}
#endif
	}
	
	void ValidatedRenderCommandEncoder::UseResources(const Resource* resource, NSUInteger count, ResourceUsage usage)
	{
		for (NSUInteger i = 0; i < count; ++i)
		{
			Validator.UseResource(resource[i].GetPtr(), usage);
			if (class_conformsToProtocol(object_getClass(resource[i].GetPtr()), @protocol(MTLBuffer)) && ((const Buffer&)resource[i]).GetParentBuffer())
			{
				Validator.UseResource(((const Buffer&)resource[i]).GetPtr(), ns::Range(((const Buffer&)resource[i]).GetOffset(), ((const Buffer&)resource[i]).GetLength()), usage);
			}
		}
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		if (@available(macOS 10.13, iOS 11.0, *))
		{
			RenderCommandEncoder::UseResources(resource, count, usage);
		}
#endif
	}
#endif
}


MTLPP_END
