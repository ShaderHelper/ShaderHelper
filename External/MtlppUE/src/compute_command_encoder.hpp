/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_ComputeCommandEncoder.hpp"
#include "ns.hpp"
#include "command_encoder.hpp"
#include "texture.hpp"
#include "command_buffer.hpp"
#include "fence.hpp"
#include "heap.hpp"
#include "sampler.hpp"
#include "texture.hpp"
#include "buffer.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	struct ITable<id<MTLComputeCommandEncoder>, void> : public IMPTable<id<MTLComputeCommandEncoder>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLComputeCommandEncoder>, void>(C)
		{
		}
	};
}

namespace mtlpp
{
	class MTLPP_EXPORT ComputeCommandEncoder : public CommandEncoder<ns::Protocol<id<MTLComputeCommandEncoder>>::type>
    {
    public:
        ComputeCommandEncoder(ns::Ownership const retain = ns::Ownership::Retain) : CommandEncoder<ns::Protocol<id<MTLComputeCommandEncoder>>::type>(retain) { }
		ComputeCommandEncoder(ns::Protocol<id<MTLComputeCommandEncoder>>::type handle, UE::ITableCache* cache = nullptr, ns::Ownership const retain = ns::Ownership::Retain) : CommandEncoder<ns::Protocol<id<MTLComputeCommandEncoder>>::type>(handle, retain, UE::ITableCacheRef(cache).GetComputeCommandEncoder(handle)) { }
		
		operator ns::Protocol<id<MTLComputeCommandEncoder>>::type() const = delete;
		
        void SetComputePipelineState(const ComputePipelineState& state);
        void SetBytes(const void* data, NSUInteger length, NSUInteger index) MTLPP_AVAILABLE(10_11, 8_3);
        void SetBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index);
        void SetBufferOffset(NSUInteger offset, NSUInteger index) MTLPP_AVAILABLE(10_11, 8_3);
        void SetBuffers(const Buffer* buffers, const NSUInteger* offsets, const ns::Range& range);
        void SetTexture(const Texture& texture, NSUInteger index);
        void SetTextures(const Texture* textures, const ns::Range& range);
        void SetSamplerState(const SamplerState& sampler, NSUInteger index);
        void SetSamplerStates(const SamplerState::Type* samplers, const ns::Range& range);
        void SetSamplerState(const SamplerState& sampler, float lodMinClamp, float lodMaxClamp, NSUInteger index);
        void SetSamplerStates(const SamplerState::Type* samplers, const float* lodMinClamps, const float* lodMaxClamps, const ns::Range& range);
        void SetThreadgroupMemory(NSUInteger length, NSUInteger index);
		void SetImageblock(NSUInteger width, NSUInteger height) MTLPP_AVAILABLE_IOS(11_0);
        void SetStageInRegion(const Region& region) MTLPP_AVAILABLE(10_12, 10_0);
        void DispatchThreadgroups(const Size& threadgroupsPerGrid, const Size& threadsPerThreadgroup);
        void DispatchThreadgroupsWithIndirectBuffer(const Buffer& indirectBuffer, NSUInteger indirectBufferOffset, const Size& threadsPerThreadgroup) MTLPP_AVAILABLE(10_11, 9_0);
		void DispatchThreads(const Size& threadsPerGrid, const Size& threadsPerThreadgroup) MTLPP_AVAILABLE(10_13, 11_0);
        void UpdateFence(const Fence& fence) MTLPP_AVAILABLE(10_13, 10_0);
        void WaitForFence(const Fence& fence) MTLPP_AVAILABLE(10_13, 10_0);
		MTLPP_VALIDATED void UseResource(const Resource& resource, ResourceUsage usage) MTLPP_AVAILABLE(10_13, 11_0);
		MTLPP_VALIDATED void UseResources(const Resource* resource, NSUInteger count, ResourceUsage usage) MTLPP_AVAILABLE(10_13, 11_0);
		void UseHeap(const Heap& heap) MTLPP_AVAILABLE(10_13, 11_0);
		void UseHeaps(const Heap::Type* heap, NSUInteger count) MTLPP_AVAILABLE(10_13, 11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
	
#if MTLPP_CONFIG_VALIDATE
	class MTLPP_EXPORT ValidatedComputeCommandEncoder : public ns::AutoReleased<ComputeCommandEncoder>
	{
		CommandEncoderValidationTable Validator;
		
	public:
		ValidatedComputeCommandEncoder()
		: Validator(nullptr)
		{
		}
		
		ValidatedComputeCommandEncoder(ComputeCommandEncoder& Wrapped)
		: ns::AutoReleased<ComputeCommandEncoder>(Wrapped)
		, Validator(Wrapped.GetAssociatedObject<CommandEncoderValidationTable>(CommandEncoderValidationTable::kTableAssociationKey).GetPtr())
		{
		}
		
		MTLPP_VALIDATED void UseResource(const Resource& resource, ResourceUsage usage);
		MTLPP_VALIDATED void UseResources(const Resource* resource, NSUInteger count, ResourceUsage usage);
	};
	
	template <>
	class MTLPP_EXPORT Validator<ComputeCommandEncoder>
	{
		public:
		Validator(ComputeCommandEncoder& Val, bool bEnable)
		: Resource(Val)
		{
			if (bEnable)
			{
				Validation = ValidatedComputeCommandEncoder(Val);
			}
		}
		
		ValidatedComputeCommandEncoder& operator*()
		{
			assert(Validation.GetPtr() != nullptr);
			return Validation;
		}
		
		ComputeCommandEncoder* operator->()
		{
			return Validation.GetPtr() == nullptr ? &Resource : &Validation;
		}
		
		private:
		ComputeCommandEncoder& Resource;
		ValidatedComputeCommandEncoder Validation;
	};
#endif
}

MTLPP_END
