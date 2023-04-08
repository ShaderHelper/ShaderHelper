/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLBlitCommandEncoder.h>
#include "blit_command_encoder.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    void BlitCommandEncoder::Synchronize(const Resource& resource)
    {
        Validate();
#if MTLPP_PLATFORM_MAC
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SynchronizeResource(m_ptr, resource.GetPtr());
#else
        [(id<MTLBlitCommandEncoder>)m_ptr
            synchronizeResource:(id<MTLResource>)resource.GetPtr()];
#endif
#endif
    }

    void BlitCommandEncoder::Synchronize(const Texture& texture, NSUInteger slice, NSUInteger level)
    {
        Validate();
#if MTLPP_PLATFORM_MAC
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SynchronizeTextureSliceLevel(m_ptr, (id<MTLTexture>)texture.GetPtr(), slice, level);
#else
        [(id<MTLBlitCommandEncoder>)m_ptr
            synchronizeTexture:(id<MTLTexture>)texture.GetPtr()
            slice:slice
            level:level];
#endif
#endif
    }

    void BlitCommandEncoder::Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin)
    {
        Validate();
		
#if MTLPP_CONFIG_IMP_CACHE
		m_table->CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoTexturedestinationSlicedestinationLeveldestinationOrigin(m_ptr, (id<MTLTexture>)sourceTexture.GetPtr(), sourceSlice, sourceLevel, sourceOrigin, sourceSize, (id<MTLTexture>)destinationTexture.GetPtr(), destinationSlice, destinationLevel, destinationOrigin);
#else
        [(id<MTLBlitCommandEncoder>)m_ptr
            copyFromTexture:(id<MTLTexture>)sourceTexture.GetPtr()
            sourceSlice:sourceSlice
            sourceLevel:sourceLevel
            sourceOrigin:MTLOriginMake(sourceOrigin.x, sourceOrigin.y, sourceOrigin.z)
            sourceSize:MTLSizeMake(sourceSize.width, sourceSize.height, sourceSize.depth)
            toTexture:(id<MTLTexture>)destinationTexture.GetPtr()
            destinationSlice:destinationSlice
            destinationLevel:destinationLevel
            destinationOrigin:MTLOriginMake(destinationOrigin.x, destinationOrigin.y, destinationOrigin.z)];
#endif
    }

    void BlitCommandEncoder::Copy(const Buffer& sourceBuffer, NSUInteger sourceOffset, NSUInteger sourceBytesPerRow, NSUInteger sourceBytesPerImage, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->CopyFromBuffersourceOffsetsourceBytesPerRowsourceBytesPerImagesourceSizetoTexturedestinationSlicedestinationLeveldestinationOrigin(m_ptr, (id<MTLBuffer>)sourceBuffer.GetPtr(), sourceOffset + sourceBuffer.GetOffset(), sourceBytesPerRow, sourceBytesPerImage, sourceSize, (id<MTLTexture>)destinationTexture.GetPtr(), destinationSlice, destinationLevel, destinationOrigin);
#else
        [(id<MTLBlitCommandEncoder>)m_ptr
            copyFromBuffer:(id<MTLBuffer>)sourceBuffer.GetPtr()
            sourceOffset:sourceOffset + sourceBuffer.GetOffset()
            sourceBytesPerRow:sourceBytesPerRow
            sourceBytesPerImage:sourceBytesPerImage
            sourceSize:MTLSizeMake(sourceSize.width, sourceSize.height, sourceSize.depth)
            toTexture:(id<MTLTexture>)destinationTexture.GetPtr()
            destinationSlice:destinationSlice
            destinationLevel:destinationLevel
            destinationOrigin:MTLOriginMake(destinationOrigin.x, destinationOrigin.y, destinationOrigin.z)];
#endif
    }

    void BlitCommandEncoder::Copy(const Buffer& sourceBuffer, NSUInteger sourceOffset, NSUInteger sourceBytesPerRow, NSUInteger sourceBytesPerImage, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin, BlitOption options)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->CopyFromBuffersourceOffsetsourceBytesPerRowsourceBytesPerImagesourceSizetoTexturedestinationSlicedestinationLeveldestinationOriginoptions(m_ptr, (id<MTLBuffer>)sourceBuffer.GetPtr(), sourceOffset + sourceBuffer.GetOffset(), sourceBytesPerRow, sourceBytesPerImage, sourceSize, (id<MTLTexture>)destinationTexture.GetPtr(), destinationSlice, destinationLevel, destinationOrigin, MTLBlitOption(options));
#else
        [(id<MTLBlitCommandEncoder>)m_ptr
            copyFromBuffer:(id<MTLBuffer>)sourceBuffer.GetPtr()
            sourceOffset:sourceOffset + sourceBuffer.GetOffset()
            sourceBytesPerRow:sourceBytesPerRow
            sourceBytesPerImage:sourceBytesPerImage
            sourceSize:MTLSizeMake(sourceSize.width, sourceSize.height, sourceSize.depth)
            toTexture:(id<MTLTexture>)destinationTexture.GetPtr()
            destinationSlice:destinationSlice
            destinationLevel:destinationLevel
            destinationOrigin:MTLOriginMake(destinationOrigin.x, destinationOrigin.y, destinationOrigin.z)
            options:MTLBlitOption(options)];
#endif
    }

    void BlitCommandEncoder::Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger destinationBytesPerRow, NSUInteger destinationBytesPerImage)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoBufferdestinationOffsetdestinationBytesPerRowdestinationBytesPerImage(m_ptr, (id<MTLTexture>)sourceTexture.GetPtr(), sourceSlice, sourceLevel, sourceOrigin, sourceSize, (id<MTLBuffer>)destinationBuffer.GetPtr(), destinationOffset + destinationBuffer.GetOffset(), destinationBytesPerRow, destinationBytesPerImage);
#else
        [(id<MTLBlitCommandEncoder>)m_ptr
            copyFromTexture:(id<MTLTexture>)sourceTexture.GetPtr()
            sourceSlice:sourceSlice
            sourceLevel:sourceLevel
            sourceOrigin:MTLOriginMake(sourceOrigin.x, sourceOrigin.y, sourceOrigin.z)
            sourceSize:MTLSizeMake(sourceSize.width, sourceSize.height, sourceSize.depth)
            toBuffer:(id<MTLBuffer>)destinationBuffer.GetPtr()
            destinationOffset:destinationOffset + destinationBuffer.GetOffset()
            destinationBytesPerRow:destinationBytesPerRow
            destinationBytesPerImage:destinationBytesPerImage];
#endif
    }

    void BlitCommandEncoder::Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger destinationBytesPerRow, NSUInteger destinationBytesPerImage, BlitOption options)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoBufferdestinationOffsetdestinationBytesPerRowdestinationBytesPerImageoptions(m_ptr, (id<MTLTexture>)sourceTexture.GetPtr(), sourceSlice, sourceLevel, sourceOrigin, sourceSize, (id<MTLBuffer>)destinationBuffer.GetPtr(), destinationOffset + destinationBuffer.GetOffset(), destinationBytesPerRow, destinationBytesPerImage, MTLBlitOption(options));
#else
        [(id<MTLBlitCommandEncoder>)m_ptr
            copyFromTexture:(id<MTLTexture>)sourceTexture.GetPtr()
            sourceSlice:sourceSlice
            sourceLevel:sourceLevel
            sourceOrigin:MTLOriginMake(sourceOrigin.x, sourceOrigin.y, sourceOrigin.z)
            sourceSize:MTLSizeMake(sourceSize.width, sourceSize.height, sourceSize.depth)
            toBuffer:(id<MTLBuffer>)destinationBuffer.GetPtr()
            destinationOffset:destinationOffset + destinationBuffer.GetOffset()
            destinationBytesPerRow:destinationBytesPerRow
            destinationBytesPerImage:destinationBytesPerImage
            options:MTLBlitOption(options)];
#endif
    }

    void BlitCommandEncoder::Copy(const Buffer& sourceBuffer, NSUInteger sourceOffset, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger size)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->CopyFromBufferSourceOffsetToBufferDestinationOffsetSize(m_ptr, (id<MTLBuffer>)sourceBuffer.GetPtr(), sourceOffset + sourceBuffer.GetOffset(), (id<MTLBuffer>)destinationBuffer.GetPtr(), destinationOffset + destinationBuffer.GetOffset(), size);
#else
        [(id<MTLBlitCommandEncoder>)m_ptr
            copyFromBuffer:(id<MTLBuffer>)sourceBuffer.GetPtr()
            sourceOffset:sourceOffset + sourceBuffer.GetOffset()
            toBuffer:(id<MTLBuffer>)destinationBuffer.GetPtr()
            destinationOffset:destinationOffset + destinationBuffer.GetOffset()
            size:size];
#endif
    }

    void BlitCommandEncoder::GenerateMipmaps(const Texture& texture)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->GenerateMipmapsForTexture(m_ptr, (id<MTLTexture>)texture.GetPtr());
#else
        [(id<MTLBlitCommandEncoder>)m_ptr
            generateMipmapsForTexture:(id<MTLTexture>)texture.GetPtr()];
#endif
    }

    void BlitCommandEncoder::Fill(const Buffer& buffer, const ns::Range& range, uint8_t value)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->FillBufferRangeValue(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), NSMakeRange(range.Location + buffer.GetOffset(), range.Length), value);
#else
        [(id<MTLBlitCommandEncoder>)m_ptr
            fillBuffer:(id<MTLBuffer>)buffer.GetPtr()
            range:NSMakeRange(range.Location + buffer.GetOffset(), range.Length)
            value:value];
#endif
    }

    void BlitCommandEncoder::UpdateFence(const Fence& fence)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->UpdateFence(m_ptr, fence.GetPtr());
#else
		if(@available(macOS 10.13, iOS 10.0, *))
			[(id<MTLBlitCommandEncoder>)m_ptr updateFence:(id<MTLFence>)fence.GetPtr()];
#endif
#endif
    }

    void BlitCommandEncoder::WaitForFence(const Fence& fence)
    {
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->WaitForFence(m_ptr, fence.GetPtr());
#else
		
		if(@available(macOS 10.13, iOS 10.0, *))
			[(id<MTLBlitCommandEncoder>)m_ptr waitForFence:(id<MTLFence>)fence.GetPtr()];
#endif
#endif
    }
	
#if MTLPP_CONFIG_VALIDATE
	void ValidatedBlitCommandEncoder::Synchronize(const Resource& resource)
	{
#if MTLPP_PLATFORM_MAC
		Validator.UseResource(resource, mtlpp::ResourceUsage::Write);
		BlitCommandEncoder::Synchronize(resource);
#endif
	}
	void ValidatedBlitCommandEncoder::Synchronize(const Texture& texture, NSUInteger slice, NSUInteger level)
	{
#if MTLPP_PLATFORM_MAC
		Validator.UseResource(texture, mtlpp::ResourceUsage::Write);
		BlitCommandEncoder::Synchronize(texture, slice, level);
#endif
	}
	void ValidatedBlitCommandEncoder::Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin)
	{
		Validator.UseResource(sourceTexture, mtlpp::ResourceUsage::Read);
		Validator.UseResource(destinationTexture, mtlpp::ResourceUsage::Write);
		
		BlitCommandEncoder::Copy(sourceTexture, sourceSlice, sourceLevel, sourceOrigin, sourceSize, destinationTexture, destinationSlice, destinationLevel, destinationOrigin);
	}
	void ValidatedBlitCommandEncoder::Copy(const Buffer& sourceBuffer, NSUInteger sourceOffset, NSUInteger sourceBytesPerRow, NSUInteger sourceBytesPerImage, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin)
	{
		Validator.UseResource(sourceBuffer, mtlpp::ResourceUsage::Read);
		Validator.UseResource(sourceBuffer, ns::Range(sourceOffset + sourceBuffer.GetOffset(), sourceSize.depth * sourceBytesPerImage), mtlpp::ResourceUsage::Read);
		Validator.UseResource(destinationTexture, mtlpp::ResourceUsage::Write);
		
		BlitCommandEncoder::Copy(sourceBuffer, sourceOffset, sourceBytesPerRow, sourceBytesPerImage, sourceSize, destinationTexture, destinationSlice, destinationLevel, destinationOrigin);
	}
	void ValidatedBlitCommandEncoder::Copy(const Buffer& sourceBuffer, NSUInteger sourceOffset, NSUInteger sourceBytesPerRow, NSUInteger sourceBytesPerImage, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin, BlitOption options)
	{
		Validator.UseResource(sourceBuffer, mtlpp::ResourceUsage::Read);
		Validator.UseResource(sourceBuffer, ns::Range(sourceOffset + sourceBuffer.GetOffset(), sourceSize.depth * sourceBytesPerImage), mtlpp::ResourceUsage::Read);
		Validator.UseResource(destinationTexture, mtlpp::ResourceUsage::Write);
		
		BlitCommandEncoder::Copy(sourceBuffer, sourceOffset, sourceBytesPerRow, sourceBytesPerImage, sourceSize, destinationTexture, destinationSlice, destinationLevel, destinationOrigin, options);
	}
	void ValidatedBlitCommandEncoder::Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger destinationBytesPerRow, NSUInteger destinationBytesPerImage)
	{
		Validator.UseResource(sourceTexture, mtlpp::ResourceUsage::Read);
		Validator.UseResource(destinationBuffer, mtlpp::ResourceUsage::Write);
		Validator.UseResource(destinationBuffer, ns::Range(destinationOffset + destinationBuffer.GetOffset(), sourceSize.depth * destinationBytesPerImage), mtlpp::ResourceUsage::Write);

		BlitCommandEncoder::Copy(sourceTexture, sourceSlice, sourceLevel, sourceOrigin, sourceSize, destinationBuffer, destinationOffset, destinationBytesPerRow, destinationBytesPerImage);
	}
	void ValidatedBlitCommandEncoder::Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger destinationBytesPerRow, NSUInteger destinationBytesPerImage, BlitOption options)
	{
		Validator.UseResource(sourceTexture, mtlpp::ResourceUsage::Read);
		Validator.UseResource(destinationBuffer, mtlpp::ResourceUsage::Write);
		Validator.UseResource(destinationBuffer, ns::Range(destinationOffset + destinationBuffer.GetOffset(), sourceSize.depth * destinationBytesPerImage), mtlpp::ResourceUsage::Write);
		
		BlitCommandEncoder::Copy(sourceTexture, sourceSlice, sourceLevel, sourceOrigin, sourceSize, destinationBuffer, destinationOffset, destinationBytesPerRow, destinationBytesPerImage, options);
	}
	void ValidatedBlitCommandEncoder::Copy(const Buffer& sourceBuffer, NSUInteger soruceOffset, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger size)
	{
		Validator.UseResource(sourceBuffer, mtlpp::ResourceUsage::Read);
		Validator.UseResource(sourceBuffer, ns::Range(soruceOffset + sourceBuffer.GetOffset(), size), mtlpp::ResourceUsage::Read);

		Validator.UseResource(destinationBuffer, mtlpp::ResourceUsage::Write);
		Validator.UseResource(destinationBuffer, ns::Range(destinationOffset + destinationBuffer.GetOffset(), size), mtlpp::ResourceUsage::Write);
		
		BlitCommandEncoder::Copy(sourceBuffer, soruceOffset, destinationBuffer, destinationOffset, size);
	}
	void ValidatedBlitCommandEncoder::GenerateMipmaps(const Texture& texture)
	{
		Validator.UseResource(texture, mtlpp::ResourceUsage::Write);
		BlitCommandEncoder::GenerateMipmaps(texture);
	}
	void ValidatedBlitCommandEncoder::Fill(const Buffer& buffer, const ns::Range& range, uint8_t value)
	{
		Validator.UseResource(buffer, mtlpp::ResourceUsage::Write);
		Validator.UseResource(buffer, ns::Range(range.Location + buffer.GetOffset(), range.Length), mtlpp::ResourceUsage::Write);
		BlitCommandEncoder::Fill(buffer, range, value);
	}
#endif
}

MTLPP_END
