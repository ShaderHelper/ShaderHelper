/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "ns.hpp"
#include "types.hpp"

MTLPP_BEGIN

MTLPP_CLASS(ResourceValidationTableImpl);
MTLPP_CLASS(BufferValidationTableImpl);
MTLPP_CLASS(CommandEncoderValidationTableImpl);
MTLPP_CLASS(ParallelEncoderValidationTableImpl);
MTLPP_CLASS(CommandBufferValidationTableImpl);
MTLPP_CLASS(DeviceValidationTableImpl);

namespace mtlpp
{
	class Device;
    class CommandBuffer;
	class BlitCommandEncoder;
	class ComputeCommandEncoder;
	class RenderCommandEncoder;
	class ParallelRenderCommandEncoder;
	class Resource;
	class Buffer;
	
#if MTLPP_CONFIG_VALIDATE
	class MTLPP_EXPORT ResourceValidationTable : public ns::Object<ResourceValidationTableImpl*, ns::CallingConvention::ObjectiveC>
	{
	public:
		ResourceValidationTable(Resource& Resource);
		ResourceValidationTable(ResourceValidationTableImpl* Table);
		
		bool ValidateUsage(NSUInteger Usage) const;
		
		static char const* kTableAssociationKey;
	};
	
	class MTLPP_EXPORT BufferValidationTable : public ns::Object<BufferValidationTableImpl*, ns::CallingConvention::ObjectiveC>
	{
	public:
		BufferValidationTable(Buffer& Resource);
		BufferValidationTable(BufferValidationTableImpl* Table);
		BufferValidationTable(ns::Ownership retain);
		
		bool ValidateUsage(NSUInteger Usage) const;
		bool ValidateUsage(NSUInteger Usage, ns::Range Range) const;
		
		void AllocateRange(ns::Range Range);
		void ReleaseRange(ns::Range Range);
		void ReleaseAllRanges(ns::Range Range);
		
		static char const* kTableAssociationKey;
	};
	
	class MTLPP_EXPORT CommandEncoderValidationTable : public ns::Object<CommandEncoderValidationTableImpl*, ns::CallingConvention::ObjectiveC>
	{
	public:
		CommandEncoderValidationTable(BlitCommandEncoder& Encoder);
		CommandEncoderValidationTable(ComputeCommandEncoder& Encoder);
		CommandEncoderValidationTable(RenderCommandEncoder& Encoder);
		CommandEncoderValidationTable(ParallelRenderCommandEncoder& Encoder);
		CommandEncoderValidationTable(CommandEncoderValidationTableImpl* Table = nullptr);
		
		void UseResource(ns::Protocol<id<MTLBuffer>>::type Resource, ns::Range Range, NSUInteger Usage);
		bool ValidateUsage(ns::Protocol<id<MTLBuffer>>::type Resource, ns::Range Range, NSUInteger Usage) const;
		
		void UseResource(ns::Protocol<id<MTLResource>>::type Resource, NSUInteger Usage);
		bool ValidateUsage(ns::Protocol<id<MTLResource>>::type Resource, NSUInteger Usage) const;
		
		static char const* kTableAssociationKey;
	};
	
	class MTLPP_EXPORT ParallelEncoderValidationTable : public ns::Object<ParallelEncoderValidationTableImpl*, ns::CallingConvention::ObjectiveC>
	{
	public:
		ParallelEncoderValidationTable(ParallelRenderCommandEncoder& Encoder);
		ParallelEncoderValidationTable(ParallelEncoderValidationTableImpl* Table);
		
		void AddEncoderValidator(RenderCommandEncoder& Encoder);
		
		static char const* kTableAssociationKey;
	};
	
	class MTLPP_EXPORT CommandBufferValidationTable : public ns::Object<CommandBufferValidationTableImpl*, ns::CallingConvention::ObjectiveC>
	{
	public:
		CommandBufferValidationTable(CommandBuffer& Buffer);
		CommandBufferValidationTable(CommandBufferValidationTableImpl* Table);
		CommandBufferValidationTable(ns::Ownership retain);
		
		void AddEncoderValidator(BlitCommandEncoder& Encoder);
		void AddEncoderValidator(ComputeCommandEncoder& Encoder);
		CommandEncoderValidationTable AddEncoderValidator(RenderCommandEncoder& Encoder);
		CommandEncoderValidationTable AddEncoderValidator(ParallelRenderCommandEncoder& Encoder);
		void Enqueue(CommandBuffer& Buffer);
		
		bool ValidateUsage(ns::Protocol<id<MTLBuffer>>::type Resource, ns::Range Range, NSUInteger Usage) const;
		bool ValidateUsage(ns::Protocol<id<MTLResource>>::type Resource, NSUInteger Usage) const;
		
		static char const* kTableAssociationKey;
	};
	
	class MTLPP_EXPORT DeviceValidationTable : public ns::Object<DeviceValidationTableImpl*, ns::CallingConvention::ObjectiveC>
	{
	public:
		DeviceValidationTable(Device& Device);
		DeviceValidationTable(DeviceValidationTableImpl* Table);
		
		void Enqueue(CommandBuffer& Buffer);
		
		bool ValidateUsage(ns::Protocol<id<MTLBuffer>>::type Resource, ns::Range Range, NSUInteger Usage) const;
		bool ValidateUsage(ns::Protocol<id<MTLResource>>::type Resource, NSUInteger Usage) const;
		
		static char const* kTableAssociationKey;
	};
	
	template <typename T>
	class MTLPP_EXPORT Validator
	{
	public:
		Validator(T& Val, bool bEnable)
		: Resource(Val)
		{
		}
		
		T& operator*()
		{
			return Resource;
		}
		
		T* operator->()
		{
			return &Resource;
		}
		
	private:
		T& Resource;
	};
#endif
}

MTLPP_END
