/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_BlitCommandEncoder.hpp"
#include "command_encoder.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "fence.hpp"
#include "validation.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	struct ITable<id<MTLBlitCommandEncoder>, void> : public IMPTable<id<MTLBlitCommandEncoder>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLBlitCommandEncoder>, void>(C)
		{
		}
	};
}

namespace mtlpp
{
    enum class BlitOption
    {
        None                                             = 0,
        DepthFromDepthStencil                            = 1 << 0,
        StencilFromDepthStencil                          = 1 << 1,
        RowLinearPVRTC          MTLPP_AVAILABLE_AX(9_0)  = 1 << 2,
    }
    MTLPP_AVAILABLE(10_11, 9_0);

    class MTLPP_EXPORT BlitCommandEncoder : public CommandEncoder<ns::Protocol<id<MTLBlitCommandEncoder>>::type>
    {
    public:
        BlitCommandEncoder(ns::Ownership const retain = ns::Ownership::Retain) : CommandEncoder<ns::Protocol<id<MTLBlitCommandEncoder>>::type>(retain) { }
		BlitCommandEncoder(ns::Protocol<id<MTLBlitCommandEncoder>>::type handle, UE::ITableCache* cache = nullptr, ns::Ownership const retain = ns::Ownership::Retain) : CommandEncoder<ns::Protocol<id<MTLBlitCommandEncoder>>::type>(handle, retain, UE::ITableCacheRef(cache).GetBlitCommandEncoder(handle)) { }

		operator ns::Protocol<id<MTLBlitCommandEncoder>>::type() const = delete;
		
        MTLPP_VALIDATED void Synchronize(const Resource& resource) MTLPP_AVAILABLE_MAC(10_11);
        MTLPP_VALIDATED void Synchronize(const Texture& texture, NSUInteger slice, NSUInteger level) MTLPP_AVAILABLE_MAC(10_11);
        MTLPP_VALIDATED void Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin);
        MTLPP_VALIDATED void Copy(const Buffer& sourceBuffer, NSUInteger sourceOffset, NSUInteger sourceBytesPerRow, NSUInteger sourceBytesPerImage, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin);
        MTLPP_VALIDATED void Copy(const Buffer& sourceBuffer, NSUInteger sourceOffset, NSUInteger sourceBytesPerRow, NSUInteger sourceBytesPerImage, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin, BlitOption options) MTLPP_AVAILABLE(10_11, 9_0);
        MTLPP_VALIDATED void Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger destinationBytesPerRow, NSUInteger destinationBytesPerImage);
        MTLPP_VALIDATED void Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger destinationBytesPerRow, NSUInteger destinationBytesPerImage, BlitOption options) MTLPP_AVAILABLE(10_11, 9_0);
        MTLPP_VALIDATED void Copy(const Buffer& sourceBuffer, NSUInteger soruceOffset, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger size);
        MTLPP_VALIDATED void GenerateMipmaps(const Texture& texture);
        MTLPP_VALIDATED void Fill(const Buffer& buffer, const ns::Range& range, uint8_t value);
        void UpdateFence(const Fence& fence) MTLPP_AVAILABLE(10_13, 10_0);
        void WaitForFence(const Fence& fence) MTLPP_AVAILABLE(10_13, 10_0);
    };
	
#if MTLPP_CONFIG_VALIDATE
	class MTLPP_EXPORT ValidatedBlitCommandEncoder : public ns::AutoReleased<BlitCommandEncoder>
	{
		CommandEncoderValidationTable Validator;
		
	public:
		ValidatedBlitCommandEncoder()
		{
		}
		
		ValidatedBlitCommandEncoder(BlitCommandEncoder& Wrapped)
		: ns::AutoReleased<BlitCommandEncoder>(Wrapped)
		, Validator(Wrapped.GetAssociatedObject<CommandEncoderValidationTable>(CommandEncoderValidationTable::kTableAssociationKey).GetPtr())
		{
		}
		
		MTLPP_VALIDATED void Synchronize(const Resource& resource) MTLPP_AVAILABLE_MAC(10_11);
		MTLPP_VALIDATED void Synchronize(const Texture& texture, NSUInteger slice, NSUInteger level) MTLPP_AVAILABLE_MAC(10_11);
		MTLPP_VALIDATED void Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin);
		MTLPP_VALIDATED void Copy(const Buffer& sourceBuffer, NSUInteger sourceOffset, NSUInteger sourceBytesPerRow, NSUInteger sourceBytesPerImage, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin);
		MTLPP_VALIDATED void Copy(const Buffer& sourceBuffer, NSUInteger sourceOffset, NSUInteger sourceBytesPerRow, NSUInteger sourceBytesPerImage, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin, BlitOption options) MTLPP_AVAILABLE(10_11, 9_0);
		MTLPP_VALIDATED void Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger destinationBytesPerRow, NSUInteger destinationBytesPerImage);
		MTLPP_VALIDATED void Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger destinationBytesPerRow, NSUInteger destinationBytesPerImage, BlitOption options) MTLPP_AVAILABLE(10_11, 9_0);
		MTLPP_VALIDATED void Copy(const Buffer& sourceBuffer, NSUInteger soruceOffset, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger size);
		MTLPP_VALIDATED void GenerateMipmaps(const Texture& texture);
		MTLPP_VALIDATED void Fill(const Buffer& buffer, const ns::Range& range, uint8_t value);
	};
	
	template <>
	class MTLPP_EXPORT Validator<BlitCommandEncoder>
	{
	public:
		Validator(BlitCommandEncoder& Val, bool bEnable)
		: Resource(Val)
		{
			if (bEnable)
			{
				Validation = ValidatedBlitCommandEncoder(Val);
			}
		}
		
		ValidatedBlitCommandEncoder& operator*()
		{
			assert(Validation.GetPtr() != nullptr);
			return Validation;
		}
		
		BlitCommandEncoder* operator->()
		{
			return Validation.GetPtr() == nullptr ? &Resource : &Validation;
		}
		
	private:
		BlitCommandEncoder& Resource;
		ValidatedBlitCommandEncoder Validation;
	};
#endif
}

MTLPP_END
