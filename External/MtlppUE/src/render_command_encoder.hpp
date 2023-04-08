/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_RenderCommandEncoder.hpp"
#include "command_encoder.hpp"
#include "command_buffer.hpp"
#include "render_pass.hpp"
#include "fence.hpp"
#include "buffer.hpp"
#include "heap.hpp"
#include "resource.hpp"
#include "sampler.hpp"
#include "texture.hpp"
#include "stage_input_output_descriptor.hpp"
#include "validation.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	struct ITable<id<MTLRenderCommandEncoder>, void> : public IMPTable<id<MTLRenderCommandEncoder>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLRenderCommandEncoder>, void>(C)
		{
		}
	};
}

namespace mtlpp
{
    enum class PrimitiveType
    {
        Point         = 0,
        Line          = 1,
        LineStrip     = 2,
        Triangle      = 3,
        TriangleStrip = 4,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class VisibilityResultMode
    {
        Disabled                             = 0,
        Boolean                              = 1,
        Counting MTLPP_AVAILABLE(10_11, 9_0) = 2,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

	struct MTLPP_EXPORT ScissorRect : public MTLPPScissorRect
    {
		ScissorRect()
		{
			x = y = width = height = 0;
		}
		
		ScissorRect(NSUInteger _x, NSUInteger _y, NSUInteger _width, NSUInteger _height)
		{
			x = _x;
			y = _y;
			width = _width;
			height = _height;
		}
		
		ScissorRect(MTLPPScissorRect const& Rect)
		: MTLPPScissorRect(Rect)
		{}
    };

    struct MTLPP_EXPORT Viewport : public MTLPPViewport
    {
		Viewport()
		{
			originX = originY = width = height = znear = zfar = 0.0;
		}
		
		Viewport(double _originX, double _originY, double _width, double _height, double _znear, double _zfar)
		{
			originX = _originX;
			originY = _originY;
			width = _width;
			height = _height;
			znear = _znear;
			zfar = _zfar;
		}
		
		Viewport(MTLPPViewport const& Rect)
		: MTLPPViewport(Rect)
		{}
    };

    enum class CullMode
    {
        None  = 0,
        Front = 1,
        Back  = 2,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class Winding
    {
        Clockwise        = 0,
        CounterClockwise = 1,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class DepthClipMode
    {
        Clip  = 0,
        Clamp = 1,
    }
    MTLPP_AVAILABLE(10_11, 9_0);

    enum class TriangleFillMode
    {
        Fill  = 0,
        Lines = 1,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    struct DrawPrimitivesIndirectArguments
    {
        uint32_t VertexCount;
        uint32_t InstanceCount;
        uint32_t VertexStart;
        uint32_t BaseInstance;
    };

    struct DrawIndexedPrimitivesIndirectArguments
    {
        uint32_t IndexCount;
        uint32_t InstanceCount;
        uint32_t IndexStart;
        int32_t  BaseVertex;
        uint32_t BaseInstance;
    };

    struct DrawPatchIndirectArguments
    {
        uint32_t PatchCount;
        uint32_t InstanceCount;
        uint32_t PatchStart;
        uint32_t BaseInstance;
    };

    struct QuadTessellationFactorsHalf
    {
        uint16_t EdgeTessellationFactor[4];
        uint16_t InsideTessellationFactor[2];
    };

    struct riangleTessellationFactorsHalf
    {
        uint16_t EdgeTessellationFactor[3];
        uint16_t InsideTessellationFactor;
    };

    enum RenderStages
    {
        Vertex   = (1 << 0),
        Fragment = (1 << 1),
    }
    MTLPP_AVAILABLE(10_13, 10_0);

	class Heap;
	
	class MTLPP_EXPORT RenderCommandEncoder : public CommandEncoder<ns::Protocol<id<MTLRenderCommandEncoder>>::type>
    {
    public:
        RenderCommandEncoder(ns::Ownership const retain = ns::Ownership::Retain) : CommandEncoder<ns::Protocol<id<MTLRenderCommandEncoder>>::type>(retain) { }
		RenderCommandEncoder(ns::Protocol<id<MTLRenderCommandEncoder>>::type handle, UE::ITableCache* cache = nullptr, ns::Ownership const retain = ns::Ownership::Retain) : CommandEncoder<ns::Protocol<id<MTLRenderCommandEncoder>>::type>(handle, retain, UE::ITableCacheRef(cache).GetRenderCommandEncoder(handle)) { }

		operator ns::Protocol<id<MTLRenderCommandEncoder>>::type() const = delete;
		
        void SetRenderPipelineState(const RenderPipelineState& pipelineState);
        void SetVertexData(const void* bytes, NSUInteger length, NSUInteger index) MTLPP_AVAILABLE(10_11, 8_3);
        void SetVertexBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index);
        void SetVertexBufferOffset(NSUInteger offset, NSUInteger index) MTLPP_AVAILABLE(10_11, 8_3);
        void SetVertexBuffers(const Buffer* buffers, const NSUInteger* offsets, const ns::Range& range);
        void SetVertexTexture(const Texture& texture, NSUInteger index);
        void SetVertexTextures(const Texture* textures, const ns::Range& range);
        void SetVertexSamplerState(const SamplerState& sampler, NSUInteger index);
        void SetVertexSamplerStates(const SamplerState::Type* samplers, const ns::Range& range);
        void SetVertexSamplerState(const SamplerState& sampler, float lodMinClamp, float lodMaxClamp, NSUInteger index);
		void SetVertexSamplerStates(const SamplerState::Type* samplers, const float* lodMinClamps, const float* lodMaxClamps, const ns::Range& range);
        void SetViewport(const Viewport& viewport);
		void SetViewports(const Viewport* viewports, NSUInteger count) MTLPP_AVAILABLE_MAC(10_13);
        void SetFrontFacingWinding(Winding frontFacingWinding);
        void SetCullMode(CullMode cullMode);
        void SetDepthClipMode(DepthClipMode depthClipMode) MTLPP_AVAILABLE(10_11, 11_0);
        void SetDepthBias(float depthBias, float slopeScale, float clamp);
        void SetScissorRect(const ScissorRect& rect);
		void SetScissorRects(const ScissorRect* rect, NSUInteger count) MTLPP_AVAILABLE_MAC(10_13);
        void SetTriangleFillMode(TriangleFillMode fillMode);
        void SetFragmentData(const void* bytes, NSUInteger length, NSUInteger index);
        void SetFragmentBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index);
        void SetFragmentBufferOffset(NSUInteger offset, NSUInteger index) MTLPP_AVAILABLE(10_11, 8_3);
        void SetFragmentBuffers(const Buffer* buffers, const NSUInteger* offsets, const ns::Range& range);
        void SetFragmentTexture(const Texture& texture, NSUInteger index);
        void SetFragmentTextures(const Texture* textures, const ns::Range& range);
        void SetFragmentSamplerState(const SamplerState& sampler, NSUInteger index);
        void SetFragmentSamplerStates(const SamplerState::Type* samplers, const ns::Range& range);
        void SetFragmentSamplerState(const SamplerState& sampler, float lodMinClamp, float lodMaxClamp, NSUInteger index);
        void SetFragmentSamplerStates(const SamplerState::Type* samplers, const float* lodMinClamps, const float* lodMaxClamps, const ns::Range& range);
        void SetBlendColor(float red, float green, float blue, float alpha);
        void SetDepthStencilState(const DepthStencilState& depthStencilState);
        void SetStencilReferenceValue(uint32_t referenceValue);
        void SetStencilReferenceValue(uint32_t frontReferenceValue, uint32_t backReferenceValue) MTLPP_AVAILABLE(10_11, 9_0);
        void SetVisibilityResultMode(VisibilityResultMode mode, NSUInteger offset);
        void SetColorStoreAction(StoreAction storeAction, NSUInteger colorAttachmentIndex) MTLPP_AVAILABLE(10_12, 10_0);
        void SetDepthStoreAction(StoreAction storeAction) MTLPP_AVAILABLE(10_12, 10_0);
        void SetStencilStoreAction(StoreAction storeAction) MTLPP_AVAILABLE(10_12, 10_0);
		void SetColorStoreActionOptions(StoreActionOptions storeAction, NSUInteger colorAttachmentIndex) MTLPP_AVAILABLE(10_13, 11_0);
		void SetDepthStoreActionOptions(StoreActionOptions storeAction) MTLPP_AVAILABLE(10_13, 11_0);
		void SetStencilStoreActionOptions(StoreActionOptions storeAction) MTLPP_AVAILABLE(10_13, 11_0);
        void Draw(PrimitiveType primitiveType, NSUInteger vertexStart, NSUInteger vertexCount);
        void Draw(PrimitiveType primitiveType, NSUInteger vertexStart, NSUInteger vertexCount, NSUInteger instanceCount) MTLPP_AVAILABLE(10_11, 9_0);
        void Draw(PrimitiveType primitiveType, NSUInteger vertexStart, NSUInteger vertexCount, NSUInteger instanceCount, NSUInteger baseInstance) MTLPP_AVAILABLE(10_11, 9_0);
        void Draw(PrimitiveType primitiveType, Buffer const& indirectBuffer, NSUInteger indirectBufferOffset) MTLPP_AVAILABLE(10_11, 9_0);
        void DrawIndexed(PrimitiveType primitiveType, NSUInteger indexCount, IndexType indexType, const Buffer& indexBuffer, NSUInteger indexBufferOffset);
        void DrawIndexed(PrimitiveType primitiveType, NSUInteger indexCount, IndexType indexType, const Buffer& indexBuffer, NSUInteger indexBufferOffset, NSUInteger instanceCount) MTLPP_AVAILABLE(10_11, 9_0);
        void DrawIndexed(PrimitiveType primitiveType, NSUInteger indexCount, IndexType indexType, const Buffer& indexBuffer, NSUInteger indexBufferOffset, NSUInteger instanceCount, NSUInteger baseVertex, NSUInteger baseInstance) MTLPP_AVAILABLE(10_11, 9_0);
        void DrawIndexed(PrimitiveType primitiveType, IndexType indexType, const Buffer& indexBuffer, NSUInteger indexBufferOffset, const Buffer& indirectBuffer, NSUInteger indirectBufferOffset) MTLPP_AVAILABLE(10_11, 9_0);
        void TextureBarrier() MTLPP_AVAILABLE_MAC(10_11);
        void UpdateFence(const Fence& fence, RenderStages afterStages) MTLPP_AVAILABLE(10_13, 10_0);
        void WaitForFence(const Fence& fence, RenderStages beforeStages) MTLPP_AVAILABLE(10_13, 10_0);
        void SetTessellationFactorBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger instanceStride) MTLPP_AVAILABLE(10_12, 10_0);
        void SetTessellationFactorScale(float scale) MTLPP_AVAILABLE(10_12, 10_0);
        void DrawPatches(NSUInteger numberOfPatchControlPoints, NSUInteger patchStart, NSUInteger patchCount, const Buffer& patchIndexBuffer, NSUInteger patchIndexBufferOffset, NSUInteger instanceCount, NSUInteger baseInstance) MTLPP_AVAILABLE(10_12, 10_0);
        void DrawPatches(NSUInteger numberOfPatchControlPoints, const Buffer& patchIndexBuffer, NSUInteger patchIndexBufferOffset, const Buffer& indirectBuffer, NSUInteger indirectBufferOffset) MTLPP_AVAILABLE(10_12, NA);
        void DrawIndexedPatches(NSUInteger numberOfPatchControlPoints, NSUInteger patchStart, NSUInteger patchCount, const Buffer& patchIndexBuffer, NSUInteger patchIndexBufferOffset, const Buffer& controlPointIndexBuffer, NSUInteger controlPointIndexBufferOffset, NSUInteger instanceCount, NSUInteger baseInstance) MTLPP_AVAILABLE(10_12, 10_0);
        void DrawIndexedPatches(NSUInteger numberOfPatchControlPoints, const Buffer& patchIndexBuffer, NSUInteger patchIndexBufferOffset, const Buffer& controlPointIndexBuffer, NSUInteger controlPointIndexBufferOffset, const Buffer& indirectBuffer, NSUInteger indirectBufferOffset) MTLPP_AVAILABLE(10_12, NA);
		MTLPP_VALIDATED void UseResource(const Resource& resource, ResourceUsage usage) MTLPP_AVAILABLE(10_13, 11_0);
		MTLPP_VALIDATED void UseResources(const Resource* resource, NSUInteger count, ResourceUsage usage) MTLPP_AVAILABLE(10_13, 11_0);
		void UseHeap(Heap const& heap) MTLPP_AVAILABLE(10_13, 11_0);
		void UseHeaps(Heap::Type const* heaps, NSUInteger count) MTLPP_AVAILABLE(10_13, 11_0);
		
		NSUInteger GetTileWidth() MTLPP_AVAILABLE_IOS(11_0);
		
		NSUInteger GetTileHeight() MTLPP_AVAILABLE_IOS(11_0);
		
		void SetTileData(const void* bytes, NSUInteger length, NSUInteger index) MTLPP_AVAILABLE_IOS(11_0);
		
		void SetTileBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index) MTLPP_AVAILABLE_IOS(11_0);
		
		void SetTileBufferOffset(NSUInteger offset, NSUInteger index) MTLPP_AVAILABLE_IOS(11_0);
		
		void SetTileBuffers(const Buffer* buffers, const NSUInteger* offsets, const ns::Range& range) MTLPP_AVAILABLE_IOS(11_0);
		
		void SetTileTexture(const Texture& texture, NSUInteger index) MTLPP_AVAILABLE_IOS(11_0);
		
		void SetTileTextures(const Texture* textures, const ns::Range& range) MTLPP_AVAILABLE_IOS(11_0);
		
		void SetTileSamplerState(const SamplerState& sampler, NSUInteger index) MTLPP_AVAILABLE_IOS(11_0);
		
		void SetTileSamplerStates(const SamplerState::Type* samplers, const ns::Range& range) MTLPP_AVAILABLE_IOS(11_0);
		
		void SetTileSamplerState(const SamplerState& sampler, float lodMinClamp, float lodMaxClamp, NSUInteger index) MTLPP_AVAILABLE_IOS(11_0);
		
		void SetTileSamplerStates(const SamplerState::Type* samplers, const float* lodMinClamps, const float* lodMaxClamps, const ns::Range& range) MTLPP_AVAILABLE_IOS(11_0);
		
		void DispatchThreadsPerTile(Size const& threadsPerTile) MTLPP_AVAILABLE_IOS(11_0);
		
		void SetThreadgroupMemoryLength(NSUInteger length, NSUInteger offset, NSUInteger index) MTLPP_AVAILABLE_IOS(11_0);

    }
    MTLPP_AVAILABLE(10_11, 8_0);
	
#if MTLPP_CONFIG_VALIDATE
	class MTLPP_EXPORT ValidatedRenderCommandEncoder : public ns::AutoReleased<RenderCommandEncoder>
	{
		CommandEncoderValidationTable Validator;
		
	public:
		ValidatedRenderCommandEncoder()
		: Validator(nullptr)
		{
		}
		
		ValidatedRenderCommandEncoder(RenderCommandEncoder& Wrapped)
		: ns::AutoReleased<RenderCommandEncoder>(Wrapped)
		, Validator(Wrapped.GetAssociatedObject<CommandEncoderValidationTable>(CommandEncoderValidationTable::kTableAssociationKey).GetPtr())
		{
		}
		
		MTLPP_VALIDATED void UseResource(const Resource& resource, ResourceUsage usage);
		MTLPP_VALIDATED void UseResources(const Resource* resource, NSUInteger count, ResourceUsage usage);
	};
	
	template <>
	class MTLPP_EXPORT Validator<RenderCommandEncoder>
	{
		public:
		Validator(RenderCommandEncoder& Val, bool bEnable)
		: Resource(Val)
		{
			if (bEnable)
			{
				Validation = ValidatedRenderCommandEncoder(Val);
			}
		}
		
		ValidatedRenderCommandEncoder& operator*()
		{
			assert(Validation.GetPtr() != nullptr);
			return Validation;
		}
		
		RenderCommandEncoder* operator->()
		{
			return Validation.GetPtr() == nullptr ? &Resource : &Validation;
		}
		
		private:
		RenderCommandEncoder& Resource;
		ValidatedRenderCommandEncoder Validation;
	};
#endif
}

MTLPP_END
