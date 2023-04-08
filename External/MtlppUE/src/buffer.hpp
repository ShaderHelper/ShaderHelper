/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Buffer.hpp"
#include "pixel_format.hpp"
#include "resource.hpp"
#include "command_buffer_fence.hpp"
#include "validation.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    class Texture;
    class TextureDescriptor;
	
    class MTLPP_EXPORT Buffer : public Resource
    {
		ns::Range SubRange;
    public:
		Buffer(ns::Ownership const retain = ns::Ownership::Retain) : Resource(retain) { }
		Buffer(ns::Protocol<id<MTLBuffer>>::type handle, UE::ITableCache* cache = nullptr, ns::Ownership const retain = ns::Ownership::Retain);

		Buffer(const Buffer& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
		Buffer(Buffer&& rhs);
#endif
		
		Buffer& operator=(const Buffer& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
		Buffer& operator=(Buffer&& rhs);
#endif
        
		inline const ns::Protocol<id<MTLBuffer>>::type GetPtr() const { return (ns::Protocol<id<MTLBuffer>>::type)m_ptr; }
		operator ns::Protocol<id<MTLBuffer>>::type() const { return (ns::Protocol<id<MTLBuffer>>::type)m_ptr; }
		
		inline bool operator !=(const Buffer &other) const
		{
#if MTLPP_CONFIG_IMP_CACHE
			return (GetPtr() != other.GetPtr() || m_table != other.m_table || SubRange.Location != other.SubRange.Location || SubRange.Length != other.SubRange.Length);
#else
			return (GetPtr() != other.GetPtr() || SubRange.Location != other.SubRange.Location || SubRange.Length != other.SubRange.Length);
#endif
		}
		
		inline bool operator ==(const Buffer &other) const
		{
			return !(operator !=(other));
		}
		
        NSUInteger GetLength() const;
        MTLPP_VALIDATED void*    GetContents();
        MTLPP_VALIDATED void     DidModify(const ns::Range& range) MTLPP_AVAILABLE_MAC(10_11);
        MTLPP_VALIDATED Texture  NewTexture(const TextureDescriptor& descriptor, NSUInteger offset, NSUInteger bytesPerRow) MTLPP_AVAILABLE(10_13, 8_0);
        void     AddDebugMarker(const ns::String& marker, const ns::Range& range) MTLPP_AVAILABLE(10_12, 10_0);
        void     RemoveAllDebugMarkers() MTLPP_AVAILABLE(10_12, 10_0);
		
		/**
		 * Convenience extensions to create sub-buffer views of larger buffers that are subdivided into independent ranges.
		 * User is responsible for respecting overwrite behaviour for Shared/Managed buffers!
		 */
		NSUInteger GetOffset() const;
		MTLPP_VALIDATED Buffer	 NewBuffer(const ns::Range& range);
		ns::AutoReleased<Buffer>	 GetParentBuffer() const;
    }
    MTLPP_AVAILABLE(10_11, 8_0);
	
#if MTLPP_CONFIG_VALIDATE
	class MTLPP_EXPORT ValidatedBuffer : public ns::AutoReleased<Buffer>
	{
		BufferValidationTable Validator;
		
	public:
		static void Register(Buffer& Wrapped)
		{
			BufferValidationTable Register(Wrapped);
		}
		
		ValidatedBuffer()
		: Validator(nullptr)
		{
		}
		
		ValidatedBuffer(Buffer& Wrapped)
		: ns::AutoReleased<Buffer>(Wrapped)
		, Validator(Wrapped.GetAssociatedObject<BufferValidationTable>(BufferValidationTable::kTableAssociationKey).GetPtr())
		{
		}
		
		MTLPP_VALIDATED void*    GetContents();
		MTLPP_VALIDATED void     DidModify(const ns::Range& range) MTLPP_AVAILABLE_MAC(10_11);
		MTLPP_VALIDATED Texture  NewTexture(const TextureDescriptor& descriptor, NSUInteger offset, NSUInteger bytesPerRow) MTLPP_AVAILABLE(10_13, 8_0);
		MTLPP_VALIDATED Buffer	 NewBuffer(const ns::Range& range);
		void	 ReleaseRange(const ns::Range& range);
		void	 ReleaseAllRanges();
	};
	
	template <>
	class MTLPP_EXPORT Validator<Buffer>
	{
		public:
		Validator(Buffer& Val, bool bEnable)
		: Resource(Val)
		{
			if (bEnable)
			{
				Validation = ValidatedBuffer(Val);
			}
		}
		
		ValidatedBuffer& operator*()
		{
			assert(Validation.GetPtr() != nullptr);
			return Validation;
		}
		
		Buffer* operator->()
		{
			return Validation.GetPtr() == nullptr ? &Resource : &Validation;
		}
		
		private:
		Buffer& Resource;
		ValidatedBuffer Validation;
	};
#endif
}

MTLPP_END
