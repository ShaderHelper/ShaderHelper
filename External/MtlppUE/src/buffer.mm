/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLBuffer.h>
#include "buffer.hpp"
#include "texture.hpp"
#include "command_encoder.hpp"
#include "debugger.hpp"

MTLPP_BEGIN

namespace mtlpp
{	
	Buffer::Buffer(ns::Protocol<id<MTLBuffer>>::type handle, UE::ITableCache* cache, ns::Ownership const retain)
	: Resource((ns::Protocol<id<MTLResource>>::type)handle, retain, (UE::ITable<ns::Protocol<id<MTLResource>>::type, void>*)UE::ITableCacheRef(cache).GetBuffer(handle))
	{
	}
	
	Buffer::Buffer(const Buffer& rhs)
	: Resource(rhs)
	, SubRange(rhs.SubRange)
	{
	}
	
#if MTLPP_CONFIG_RVALUE_REFERENCES
	Buffer::Buffer(Buffer&& rhs)
	: Resource((Resource&&)rhs)
	, SubRange(rhs.SubRange)
	{
	}
#endif
	
	Buffer& Buffer::operator=(const Buffer& rhs)
	{
		if (this != &rhs)
		{
			Resource::operator=(rhs);
			SubRange = rhs.SubRange;
		}
		return *this;
	}
#if MTLPP_CONFIG_RVALUE_REFERENCES
	Buffer& Buffer::operator=(Buffer&& rhs)
	{
		Resource::operator=((Resource&&)rhs);
		SubRange = rhs.SubRange;
		return *this;
	}
#endif
	
    NSUInteger Buffer::GetLength() const
    {
        Validate();
		if (!SubRange.Length)
		{
#if MTLPP_CONFIG_IMP_CACHE
			if (m_table)
				return (NSUInteger)((IMPTable<id<MTLBuffer>, void>*)m_table)->Length((id<MTLBuffer>)m_ptr);
			else
#endif
				return NSUInteger([(id<MTLBuffer>)m_ptr length]);
		}
		else
		{
			return SubRange.Length;
		}
    }
	
	NSUInteger Buffer::GetOffset() const
	{
		return SubRange.Location;
	}

    void* Buffer::GetContents()
    {
        Validate();
		uint8_t* contents;
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
		{
			contents = (uint8_t*)((IMPTable<id<MTLBuffer>, void>*)m_table)->Contents((id<MTLBuffer>)m_ptr);
		}
		else
#endif
		{
			contents = (uint8_t*) [(id<MTLBuffer>)m_ptr contents];
		}
		return (contents + GetOffset());
    }

    void Buffer::DidModify(const ns::Range& range)
    {
        Validate();
#if MTLPP_PLATFORM_MAC
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			((IMPTable<id<MTLBuffer>, void>*)m_table)->DidModifyRange((id<MTLBuffer>)m_ptr, NSMakeRange(range.Location + GetOffset(), range.Length));
		else
#endif
        	[(id<MTLBuffer>)m_ptr didModifyRange:NSMakeRange(range.Location + GetOffset(), range.Length)];
#endif
    }

    Texture Buffer::NewTexture(const TextureDescriptor& descriptor, NSUInteger offset, NSUInteger bytesPerRow)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 8_0)
		MTLTextureDescriptor* mtlTextureDescriptor = (MTLTextureDescriptor*)descriptor.GetPtr();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return Texture(((IMPTable<id<MTLBuffer>, void>*)m_table)->NewTextureWithDescriptorOffsetBytesPerRow((id<MTLBuffer>)m_ptr, mtlTextureDescriptor, GetOffset() + offset, bytesPerRow), ((UE::ITable<id<MTLBuffer>, void>*)m_table)->TableCache, ns::Ownership::Assign);
		else
#endif
		if (@available(macOS 10.13, iOS 8.0, *))
		{
			return Texture([(id<MTLBuffer>)m_ptr newTextureWithDescriptor:mtlTextureDescriptor offset:offset + GetOffset() bytesPerRow:bytesPerRow], nullptr, ns::Ownership::Assign);
		}
        return nullptr;
#else
        return Texture(nullptr, nullptr);
#endif
    }

    void Buffer::AddDebugMarker(const ns::String& marker, const ns::Range& range)
    {
		Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			((IMPTable<id<MTLBuffer>, void>*)m_table)->AddDebugMarkerRange((id<MTLBuffer>)m_ptr, marker.GetPtr(), NSMakeRange(range.Location + GetOffset(), range.Length));
		else
#endif
        	[(id<MTLBuffer>)m_ptr addDebugMarker:(NSString*)marker.GetPtr() range:NSMakeRange(range.Location + GetOffset(), range.Length)];
#endif
    }

    void Buffer::RemoveAllDebugMarkers()
    {
		Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			((IMPTable<id<MTLBuffer>, void>*)m_table)->RemoveAllDebugMarkers((id<MTLBuffer>)m_ptr);
		else
#endif
        	[(id<MTLBuffer>)m_ptr removeAllDebugMarkers];
#endif
    }
	
	Buffer Buffer::NewBuffer(const ns::Range& range)
	{
		Validate();
		assert(range.Location < GetLength());
		assert(range.Length && (range.Location + range.Length) <= GetLength());
		Buffer Temp = *this;
		Temp.SubRange = range;
		return Temp;
	}
	
	ns::AutoReleased<Buffer>	 Buffer::GetParentBuffer() const
	{
		Validate();
		ns::AutoReleased<Buffer> Result;
		if (SubRange.Location != 0 || SubRange.Length != 0)
		{
			Result = *this;
			Result.SubRange.Location = 0;
			Result.SubRange.Length = 0;
		}
		return Result;
	}
	
#if MTLPP_CONFIG_VALIDATE
	void*    ValidatedBuffer::GetContents()
	{
		if (GetParentBuffer())
		{
			Validator.ValidateUsage(mtlpp::ResourceUsage::Write, ns::Range(GetOffset(), GetLength()));
		}
		else
		{
			Validator.ValidateUsage(mtlpp::ResourceUsage::Write);
		}
		return Buffer::GetContents();
	}
	void     ValidatedBuffer::DidModify(const ns::Range& range)
	{
#if MTLPP_PLATFORM_MAC
		Buffer::DidModify(range);
		if (GetParentBuffer())
		{
			Validator.ValidateUsage(mtlpp::ResourceUsage::Write, ns::Range(GetOffset() + range.Location, range.Length));
		}
		else
		{
			Validator.ValidateUsage(mtlpp::ResourceUsage::Write, range);
		}
#endif
	}
	Texture  ValidatedBuffer::NewTexture(const TextureDescriptor& descriptor, NSUInteger offset, NSUInteger bytesPerRow)
	{
		Texture Tex;
#if MTLPP_IS_AVAILABLE(10_12, 8_0)
		Tex = Buffer::NewTexture(descriptor, offset, bytesPerRow);
		ValidatedTexture::Register(Tex);
#endif
		return Tex;
	}
	Buffer	 ValidatedBuffer::NewBuffer(const ns::Range& range)
	{
		Buffer Buf = Buffer::NewBuffer(range);
		Validator.AllocateRange(range);
		return Buf;
	}
	void	 ValidatedBuffer::ReleaseRange(const ns::Range& range)
	{
		Validator.ReleaseRange(range);
	}
	void	 ValidatedBuffer::ReleaseAllRanges()
	{
		Validator.ReleaseAllRanges(ns::Range(GetOffset(), GetLength()));
	}
#endif
}

MTLPP_END
