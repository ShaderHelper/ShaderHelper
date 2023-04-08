/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Texture.hpp"
#include "resource.hpp"
#include "buffer.hpp"
#include "types.hpp"
#include "validation.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	inline ITable<MTLTextureDescriptor*, void>* CreateIMPTable(MTLTextureDescriptor* handle)
	{
		static ITable<MTLTextureDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
}

namespace mtlpp
{
    enum class TextureType
    {
        Texture1D                                      		 = 0,
        Texture1DArray                                 		 = 1,
        Texture2D                                      		 = 2,
        Texture2DArray                                 		 = 3,
        Texture2DMultisample                           		 = 4,
        TextureCube                                    		 = 5,
        TextureCubeArray     	MTLPP_AVAILABLE(10_11, 11_0) = 6,
        Texture3D                                       	 = 7,
		Texture2DMultisampleArray MTLPP_AVAILABLE_MAC(10_14) = 8,
		TextureBuffer 			MTLPP_AVAILABLE(10_14, 12_0) = 9
    }
    MTLPP_AVAILABLE(10_11, 8_0);

	enum TextureUsage : NSUInteger
    {
        Unknown         = 0x0000,
        ShaderRead      = 0x0001,
        ShaderWrite     = 0x0002,
        RenderTarget    = 0x0004,
        PixelFormatView = 0x0010,
    }
    MTLPP_AVAILABLE(10_11, 9_0);


    class MTLPP_EXPORT TextureDescriptor : public ns::Object<MTLTextureDescriptor*>
    {
    public:
        TextureDescriptor();
		TextureDescriptor(ns::Ownership const retain);
        TextureDescriptor(MTLTextureDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLTextureDescriptor*>(handle, retain) { }

        static ns::AutoReleased<TextureDescriptor> Texture2DDescriptor(PixelFormat pixelFormat, NSUInteger width, NSUInteger height, bool mipmapped);
        static ns::AutoReleased<TextureDescriptor> TextureCubeDescriptor(PixelFormat pixelFormat, NSUInteger size, bool mipmapped);
		static ns::AutoReleased<TextureDescriptor> TextureBufferDescriptor(PixelFormat pixelFormat, NSUInteger size, ResourceOptions options, TextureUsage usage) MTLPP_AVAILABLE(10_14, 12_0);

        TextureType     GetTextureType() const;
        PixelFormat     GetPixelFormat() const;
        NSUInteger        GetWidth() const;
        NSUInteger        GetHeight() const;
        NSUInteger        GetDepth() const;
        NSUInteger        GetMipmapLevelCount() const;
        NSUInteger        GetSampleCount() const;
        NSUInteger        GetArrayLength() const;
        ResourceOptions GetResourceOptions() const;
        CpuCacheMode    GetCpuCacheMode() const MTLPP_AVAILABLE(10_11, 9_0);
        StorageMode     GetStorageMode() const MTLPP_AVAILABLE(10_11, 9_0);
        TextureUsage    GetUsage() const MTLPP_AVAILABLE(10_11, 9_0);
		bool    GetAllowGPUOptimisedContents() const MTLPP_AVAILABLE(10_14, 12_0);

        void SetTextureType(TextureType textureType);
        void SetPixelFormat(PixelFormat pixelFormat);
        void SetWidth(NSUInteger width);
        void SetHeight(NSUInteger height);
        void SetDepth(NSUInteger depth);
        void SetMipmapLevelCount(NSUInteger mipmapLevelCount);
        void SetSampleCount(NSUInteger sampleCount);
        void SetArrayLength(NSUInteger arrayLength);
        void SetResourceOptions(ResourceOptions resourceOptions);
        void SetCpuCacheMode(CpuCacheMode cpuCacheMode) MTLPP_AVAILABLE(10_11, 9_0);
        void SetStorageMode(StorageMode storageMode) MTLPP_AVAILABLE(10_11, 9_0);
        void SetUsage(TextureUsage usage) MTLPP_AVAILABLE(10_11, 9_0);
		void SetAllowGPUOptimisedContents(bool optimise) MTLPP_AVAILABLE(10_14, 12_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class MTLPP_EXPORT Texture : public Resource
    {
    public:
		Texture(ns::Ownership const retain = ns::Ownership::Retain) : Resource(retain) { }
		Texture(ns::Protocol<id<MTLTexture>>::type handle, UE::ITableCache* cache = nullptr, ns::Ownership const retain = ns::Ownership::Retain);
		
		Texture(const Texture& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
		Texture(Texture&& rhs);
#endif
		
		Texture& operator=(const Texture& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
		Texture& operator=(Texture&& rhs);
#endif
		
		inline const ns::Protocol<id<MTLTexture>>::type GetPtr() const { return (ns::Protocol<id<MTLTexture>>::type)m_ptr; }
		operator ns::Protocol<id<MTLTexture>>::type() const { return (ns::Protocol<id<MTLTexture>>::type)m_ptr; }

		inline bool operator !=(const Texture &other) const
		{
#if MTLPP_CONFIG_IMP_CACHE
			return (GetPtr() != other.GetPtr() || m_table != other.m_table);
#else
			return (GetPtr() != other.GetPtr());
#endif
		}
		
		inline bool operator ==(const Texture &other) const
		{
			return !(operator !=(other));
		}
		
        ns::AutoReleased<Resource>     GetRootResource() const MTLPP_DEPRECATED(10_11, 10_12, 8_0, 10_0);
        ns::AutoReleased<Texture>      GetParentTexture() const MTLPP_AVAILABLE(10_11, 9_0);
        NSUInteger     GetParentRelativeLevel() const MTLPP_AVAILABLE(10_11, 9_0);
        NSUInteger     GetParentRelativeSlice() const MTLPP_AVAILABLE(10_11, 9_0);
        ns::AutoReleased<Buffer>       GetBuffer() const MTLPP_AVAILABLE(10_12, 9_0);
        NSUInteger     GetBufferOffset() const MTLPP_AVAILABLE(10_12, 9_0);
        NSUInteger     GetBufferBytesPerRow() const MTLPP_AVAILABLE(10_12, 9_0);
		ns::AutoReleased<ns::IOSurface> GetIOSurface() const MTLPP_AVAILABLE(10_11, NA);
        NSUInteger     GetIOSurfacePlane() const MTLPP_AVAILABLE(10_11, NA);
        TextureType  GetTextureType() const;
        PixelFormat  GetPixelFormat() const;
        NSUInteger     GetWidth() const;
        NSUInteger     GetHeight() const;
        NSUInteger     GetDepth() const;
        NSUInteger     GetMipmapLevelCount() const;
        NSUInteger     GetSampleCount() const;
        NSUInteger     GetArrayLength() const;
        TextureUsage GetUsage() const;
        bool         IsFrameBufferOnly() const;
		bool    GetAllowGPUOptimisedContents() const MTLPP_AVAILABLE(10_14, 12_0);

        MTLPP_VALIDATED void GetBytes(void* pixelBytes, NSUInteger bytesPerRow, NSUInteger bytesPerImage, const Region& fromRegion, NSUInteger mipmapLevel, NSUInteger slice);
        MTLPP_VALIDATED void Replace(const Region& region, NSUInteger mipmapLevel, NSUInteger slice, void const* pixelBytes, NSUInteger bytesPerRow, NSUInteger bytesPerImage);
        MTLPP_VALIDATED void GetBytes(void* pixelBytes, NSUInteger bytesPerRow, const Region& fromRegion, NSUInteger mipmapLevel);
        MTLPP_VALIDATED void Replace(const Region& region, NSUInteger mipmapLevel, void const* pixelBytes, NSUInteger bytesPerRow);
        Texture NewTextureView(PixelFormat pixelFormat);
        Texture NewTextureView(PixelFormat pixelFormat, TextureType textureType, const ns::Range& mipmapLevelRange, const ns::Range& sliceRange) MTLPP_AVAILABLE(10_11, 9_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
	
#if MTLPP_CONFIG_VALIDATE
	class MTLPP_EXPORT ValidatedTexture : public ns::AutoReleased<Texture>
	{
		ResourceValidationTable Validator;
		
	public:
		static void Register(Texture& Wrapped)
		{
			ResourceValidationTable Register(Wrapped);
		}
		
		ValidatedTexture()
		: Validator(nullptr)
		{
		}
		
		ValidatedTexture(Texture& Wrapped)
		: ns::AutoReleased<Texture>(Wrapped)
		, Validator(Wrapped.GetAssociatedObject<ResourceValidationTable>(ResourceValidationTable::kTableAssociationKey).GetPtr())
		{
		}
		
		MTLPP_VALIDATED void GetBytes(void* pixelBytes, NSUInteger bytesPerRow, NSUInteger bytesPerImage, const Region& fromRegion, NSUInteger mipmapLevel, NSUInteger slice);
		MTLPP_VALIDATED void Replace(const Region& region, NSUInteger mipmapLevel, NSUInteger slice, void const* pixelBytes, NSUInteger bytesPerRow, NSUInteger bytesPerImage);
		MTLPP_VALIDATED void GetBytes(void* pixelBytes, NSUInteger bytesPerRow, const Region& fromRegion, NSUInteger mipmapLevel);
		MTLPP_VALIDATED void Replace(const Region& region, NSUInteger mipmapLevel, void const* pixelBytes, NSUInteger bytesPerRow);
	};
	
	template <>
	class MTLPP_EXPORT Validator<Texture>
	{
		public:
		Validator(Texture& Val, bool bEnable)
		: Resource(Val)
		{
			if (bEnable)
			{
				Validation = ValidatedTexture(Val);
			}
		}
		
		ValidatedTexture& operator*()
		{
			assert(Validation.GetPtr() != nullptr);
			return Validation;
		}
		
		Texture* operator->()
		{
			return Validation.GetPtr() == nullptr ? &Resource : &Validation;
		}
		
		private:
		Texture& Resource;
		ValidatedTexture Validation;
	};
#endif
}

MTLPP_END
