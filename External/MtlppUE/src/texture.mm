/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLTexture.h>
#include "texture.hpp"
#include "command_encoder.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    TextureDescriptor::TextureDescriptor() :
        ns::Object<MTLTextureDescriptor*>([[MTLTextureDescriptor alloc] init], ns::Ownership::Assign)
    {
    }
	
	TextureDescriptor::TextureDescriptor(ns::Ownership const retain) :
	ns::Object<MTLTextureDescriptor*>(retain)
	{
	}

    ns::AutoReleased<TextureDescriptor> TextureDescriptor::Texture2DDescriptor(PixelFormat pixelFormat, NSUInteger width, NSUInteger height, bool mipmapped)
    {
        return ns::AutoReleased<TextureDescriptor>([MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormat(pixelFormat)
                                                                                              width:width
                                                                                             height:height
                                                                                          mipmapped:mipmapped]);
    }

    ns::AutoReleased<TextureDescriptor> TextureDescriptor::TextureCubeDescriptor(PixelFormat pixelFormat, NSUInteger size, bool mipmapped)
    {
        return ns::AutoReleased<TextureDescriptor>([MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormat(pixelFormat)
                                                                                                 size:size
                                                                                            mipmapped:mipmapped]);
    }
	
	ns::AutoReleased<TextureDescriptor> TextureDescriptor::TextureBufferDescriptor(PixelFormat pixelFormat, NSUInteger size, ResourceOptions options, TextureUsage usage)
	{
#if MTLPP_IS_AVAILABLE(10_14, 12_0)
		return ns::AutoReleased<TextureDescriptor>([MTLTextureDescriptor textureBufferDescriptorWithPixelFormat:MTLPixelFormat(pixelFormat)
																										 width:size resourceOptions:options usage:usage]);
#else
		return ns::AutoReleased<TextureDescriptor>();
#endif
	}

    TextureType TextureDescriptor::GetTextureType() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return TextureType(m_table->Texturetype(m_ptr));
#else
        return TextureType([(MTLTextureDescriptor*)m_ptr textureType]);
#endif
    }

    PixelFormat TextureDescriptor::GetPixelFormat() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return PixelFormat(m_table->Pixelformat(m_ptr));
#else
        return PixelFormat([(MTLTextureDescriptor*)m_ptr pixelFormat]);
#endif
    }

    NSUInteger TextureDescriptor::GetWidth() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(m_table->Width(m_ptr));
#else
        return NSUInteger([(MTLTextureDescriptor*)m_ptr width]);
#endif
    }

    NSUInteger TextureDescriptor::GetHeight() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(m_table->Height(m_ptr));
#else
        return NSUInteger([(MTLTextureDescriptor*)m_ptr height]);
#endif
    }

    NSUInteger TextureDescriptor::GetDepth() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(m_table->Depth(m_ptr));
#else
        return NSUInteger([(MTLTextureDescriptor*)m_ptr depth]);
#endif
    }

    NSUInteger TextureDescriptor::GetMipmapLevelCount() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(m_table->Mipmaplevelcount(m_ptr));
#else
        return NSUInteger([(MTLTextureDescriptor*)m_ptr mipmapLevelCount]);
#endif
    }

    NSUInteger TextureDescriptor::GetSampleCount() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(m_table->Samplecount(m_ptr));
#else
        return NSUInteger([(MTLTextureDescriptor*)m_ptr sampleCount]);
#endif
    }

    NSUInteger TextureDescriptor::GetArrayLength() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(m_table->Arraylength(m_ptr));
#else
        return NSUInteger([(MTLTextureDescriptor*)m_ptr arrayLength]);
#endif
    }

    ResourceOptions TextureDescriptor::GetResourceOptions() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return ResourceOptions(m_table->ResourceOptions(m_ptr));
#else
        return ResourceOptions([(MTLTextureDescriptor*)m_ptr resourceOptions]);
#endif
    }

    CpuCacheMode TextureDescriptor::GetCpuCacheMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
        return CpuCacheMode(m_table->CpuCacheMode(m_ptr));
#else
        return CpuCacheMode([(MTLTextureDescriptor*)m_ptr cpuCacheMode]);
#endif
#else
        return CpuCacheMode(0);
#endif
    }

    StorageMode TextureDescriptor::GetStorageMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
        return StorageMode(m_table->StorageMode(m_ptr));
#else
        return StorageMode([(MTLTextureDescriptor*)m_ptr storageMode]);
#endif
#else
        return StorageMode(0);
#endif
    }

    TextureUsage TextureDescriptor::GetUsage() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
        return TextureUsage(m_table->Usage(m_ptr));
#else
        return TextureUsage([(MTLTextureDescriptor*)m_ptr usage]);
#endif
#else
        return TextureUsage(0);
#endif
    }
	
	bool TextureDescriptor::GetAllowGPUOptimisedContents() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_14, 12_0)
		return TextureUsage([(MTLTextureDescriptor*)m_ptr allowGPUOptimizedContents]);
#else
		return TextureUsage(0);
#endif
	}

    void TextureDescriptor::SetTextureType(TextureType textureType)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setTexturetype(m_ptr, MTLTextureType(textureType));
#else
        [(MTLTextureDescriptor*)m_ptr setTextureType:MTLTextureType(textureType)];
#endif
    }

    void TextureDescriptor::SetPixelFormat(PixelFormat pixelFormat)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setPixelformat(m_ptr, MTLPixelFormat(pixelFormat));
#else
        [(MTLTextureDescriptor*)m_ptr setPixelFormat:MTLPixelFormat(pixelFormat)];
#endif
    }

    void TextureDescriptor::SetWidth(NSUInteger width)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setWidth(m_ptr, width);
#else
        [(MTLTextureDescriptor*)m_ptr setWidth:width];
#endif
    }

    void TextureDescriptor::SetHeight(NSUInteger height)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setHeight(m_ptr, height);
#else
        [(MTLTextureDescriptor*)m_ptr setHeight:height];
#endif
    }

    void TextureDescriptor::SetDepth(NSUInteger depth)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setDepth(m_ptr, depth);
#else
        [(MTLTextureDescriptor*)m_ptr setDepth:depth];
#endif
    }

    void TextureDescriptor::SetMipmapLevelCount(NSUInteger mipmapLevelCount)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setMipmaplevelcount(m_ptr, mipmapLevelCount);
#else
        [(MTLTextureDescriptor*)m_ptr setMipmapLevelCount:mipmapLevelCount];
#endif
    }

    void TextureDescriptor::SetSampleCount(NSUInteger sampleCount)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setSamplecount(m_ptr, sampleCount);
#else
        [(MTLTextureDescriptor*)m_ptr setSampleCount:sampleCount];
#endif
    }

    void TextureDescriptor::SetArrayLength(NSUInteger arrayLength)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setArraylength(m_ptr, arrayLength);
#else
        [(MTLTextureDescriptor*)m_ptr setArrayLength:arrayLength];
#endif
    }

    void TextureDescriptor::SetResourceOptions(ResourceOptions resourceOptions)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setResourceOptions(m_ptr, MTLResourceOptions(resourceOptions));
#else
		[(MTLTextureDescriptor*)m_ptr setResourceOptions:MTLResourceOptions(resourceOptions)];
#endif
    }

    void TextureDescriptor::SetCpuCacheMode(CpuCacheMode cpuCacheMode)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setCpuCacheMode(m_ptr, MTLCPUCacheMode(cpuCacheMode));
#else
        [(MTLTextureDescriptor*)m_ptr setCpuCacheMode:MTLCPUCacheMode(cpuCacheMode)];
#endif
#endif
    }

    void TextureDescriptor::SetStorageMode(StorageMode storageMode)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setStorageMode(m_ptr, MTLStorageMode(storageMode));
#else
        [(MTLTextureDescriptor*)m_ptr setStorageMode:MTLStorageMode(storageMode)];
#endif
#endif
    }

    void TextureDescriptor::SetUsage(TextureUsage usage)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->setUsage(m_ptr, MTLTextureUsage(usage));
#else
        [(MTLTextureDescriptor*)m_ptr setUsage:MTLTextureUsage(usage)];
#endif
#endif
    }
	
	void TextureDescriptor::SetAllowGPUOptimisedContents(bool optimise)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_14, 12_0)
		[(MTLTextureDescriptor*)m_ptr setAllowGPUOptimizedContents:optimise];
#endif
	}

    ns::AutoReleased<Resource> Texture::GetRootResource() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 8_0)
#   if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return ns::AutoReleased<Resource>();
#   else
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ns::AutoReleased<Resource>(((IMPTable<id<MTLTexture>, void>*)m_table)->Rootresource((id<MTLTexture>)m_ptr));
		else
#endif
        	return ns::AutoReleased<Resource>([(id<MTLTexture>)m_ptr rootResource]);
#   endif
#else
        return ns::AutoReleased<Resource>();
#endif
    }

    ns::AutoReleased<Texture> Texture::GetParentTexture() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
		{
			id<MTLTexture> handle = ((IMPTable<id<MTLTexture>, void>*)m_table)->Parenttexture((id<MTLTexture>)m_ptr);
			UE::ITableCache* cache = ((UE::ITable<id<MTLTexture>, void>*)m_table)->TableCache;
			ITable* table = (UE::ITable<ns::Protocol<id<MTLResource>>::type, void>*)UE::ITableCacheRef(cache).GetTexture(handle);
			return ns::AutoReleased<Texture>(handle, table);
		}
		else
#endif
        	return ns::AutoReleased<Texture>([(id<MTLTexture>)m_ptr parentTexture]);
#else
        return Texture(nullptr, nullptr);
#endif
    }

    NSUInteger Texture::GetParentRelativeLevel() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ((IMPTable<id<MTLTexture>, void>*)m_table)->Parentrelativelevel((id<MTLTexture>)m_ptr);
		else
#endif
        	return NSUInteger([(id<MTLTexture>)m_ptr parentRelativeLevel]);
#else
        return 0;
#endif

    }

    NSUInteger Texture::GetParentRelativeSlice() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ((IMPTable<id<MTLTexture>, void>*)m_table)->Parentrelativeslice((id<MTLTexture>)m_ptr);
		else
#endif
        	return NSUInteger([(id<MTLTexture>)m_ptr parentRelativeSlice]);
#else
        return 0;
#endif

    }

    ns::AutoReleased<Buffer> Texture::GetBuffer() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
		{
			id<MTLBuffer> handle = ((IMPTable<id<MTLTexture>, void>*)m_table)->Buffer((id<MTLTexture>)m_ptr);
			UE::ITableCache* cache = ((UE::ITable<id<MTLTexture>, void>*)m_table)->TableCache;
			ITable* table = (UE::ITable<ns::Protocol<id<MTLResource>>::type, void>*)UE::ITableCacheRef(cache).GetBuffer(handle);
			return ns::AutoReleased<Buffer>(handle, table);
		}
		else
#endif
        	return ns::AutoReleased<Buffer>([(id<MTLTexture>)m_ptr buffer]);
#else
        return ns::AutoReleased<Buffer>();
#endif

    }

    NSUInteger Texture::GetBufferOffset() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ((IMPTable<id<MTLTexture>, void>*)m_table)->Bufferoffset((id<MTLTexture>)m_ptr);
		else
#endif
        	return NSUInteger([(id<MTLTexture>)m_ptr bufferOffset]);
#else
        return 0;
#endif

    }

    NSUInteger Texture::GetBufferBytesPerRow() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ((IMPTable<id<MTLTexture>, void>*)m_table)->Bufferbytesperrow((id<MTLTexture>)m_ptr);
		else
#endif
        	return NSUInteger([(id<MTLTexture>)m_ptr bufferBytesPerRow]);
#else
        return 0;
#endif

    }
	
	ns::AutoReleased<ns::IOSurface> Texture::GetIOSurface() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ns::AutoReleased<ns::IOSurface>(((IMPTable<id<MTLTexture>, void>*)m_table)->Iosurface((id<MTLTexture>)m_ptr));
		else
#endif
			return ns::AutoReleased<ns::IOSurface>([(id<MTLTexture>)m_ptr iosurface]);
#else
		return ns::AutoReleased<ns::IOSurface>();
#endif
	}

    NSUInteger Texture::GetIOSurfacePlane() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ((IMPTable<id<MTLTexture>, void>*)m_table)->Iosurfaceplane((id<MTLTexture>)m_ptr);
		else
#endif
        	return NSUInteger([(id<MTLTexture>)m_ptr iosurfacePlane]);
#else
        return 0;
#endif
    }

    TextureType Texture::GetTextureType() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return (TextureType)((IMPTable<id<MTLTexture>, void>*)m_table)->Texturetype((id<MTLTexture>)m_ptr);
		else
#endif
        	return TextureType([(id<MTLTexture>)m_ptr textureType]);
    }

    PixelFormat Texture::GetPixelFormat() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return (PixelFormat)((IMPTable<id<MTLTexture>, void>*)m_table)->Pixelformat((id<MTLTexture>)m_ptr);
		else
#endif
        	return PixelFormat([(id<MTLTexture>)m_ptr pixelFormat]);
    }

    NSUInteger Texture::GetWidth() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ((IMPTable<id<MTLTexture>, void>*)m_table)->Width((id<MTLTexture>)m_ptr);
		else
#endif
        	return NSUInteger([(id<MTLTexture>)m_ptr width]);
    }

    NSUInteger Texture::GetHeight() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ((IMPTable<id<MTLTexture>, void>*)m_table)->Height((id<MTLTexture>)m_ptr);
		else
#endif
        	return NSUInteger([(id<MTLTexture>)m_ptr height]);
    }

    NSUInteger Texture::GetDepth() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ((IMPTable<id<MTLTexture>, void>*)m_table)->Depth((id<MTLTexture>)m_ptr);
		else
#endif
        	return NSUInteger([(id<MTLTexture>)m_ptr depth]);
    }

    NSUInteger Texture::GetMipmapLevelCount() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ((IMPTable<id<MTLTexture>, void>*)m_table)->Mipmaplevelcount((id<MTLTexture>)m_ptr);
		else
#endif
        	return NSUInteger([(id<MTLTexture>)m_ptr mipmapLevelCount]);
    }

    NSUInteger Texture::GetSampleCount() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ((IMPTable<id<MTLTexture>, void>*)m_table)->Samplecount((id<MTLTexture>)m_ptr);
		else
#endif
        	return NSUInteger([(id<MTLTexture>)m_ptr sampleCount]);
    }

    NSUInteger Texture::GetArrayLength() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ((IMPTable<id<MTLTexture>, void>*)m_table)->Arraylength((id<MTLTexture>)m_ptr);
		else
#endif
        	return NSUInteger([(id<MTLTexture>)m_ptr arrayLength]);
    }

    TextureUsage Texture::GetUsage() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return TextureUsage(((IMPTable<id<MTLTexture>, void>*)m_table)->Usage((id<MTLTexture>)m_ptr));
		else
#endif
        	return TextureUsage([(id<MTLTexture>)m_ptr usage]);
    }

    bool Texture::IsFrameBufferOnly() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return ((IMPTable<id<MTLTexture>, void>*)m_table)->Isframebufferonly((id<MTLTexture>)m_ptr);
		else
#endif
        	return [(id<MTLTexture>)m_ptr isFramebufferOnly];
    }
	
	bool Texture::GetAllowGPUOptimisedContents() const
	{
		Validate();
		return [(id<MTLTexture>)m_ptr allowGPUOptimizedContents];
	}

    void Texture::GetBytes(void* pixelBytes, NSUInteger bytesPerRow, NSUInteger bytesPerImage, const Region& fromRegion, NSUInteger mipmapLevel, NSUInteger slice)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			((IMPTable<id<MTLTexture>, void>*)m_table)->Getbytesbytesperrowbytesperimagefromregionmipmaplevelslice((id<MTLTexture>)m_ptr, pixelBytes, bytesPerRow, bytesPerImage, fromRegion, mipmapLevel, slice);
		else
#endif
        	[(id<MTLTexture>)m_ptr getBytes:pixelBytes
                                     bytesPerRow:bytesPerRow
                                   bytesPerImage:bytesPerImage
                                      fromRegion:MTLRegionMake3D(fromRegion.origin.x, fromRegion.origin.y, fromRegion.origin.z, fromRegion.size.width, fromRegion.size.height, fromRegion.size.depth)
                                     mipmapLevel:mipmapLevel
                                           slice:slice];
    }

    void Texture::Replace(const Region& region, NSUInteger mipmapLevel, NSUInteger slice, void const* pixelBytes, NSUInteger bytesPerRow, NSUInteger bytesPerImage)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			((IMPTable<id<MTLTexture>, void>*)m_table)->Replaceregionmipmaplevelslicewithbytesbytesperrowbytesperimage((id<MTLTexture>)m_ptr, region, mipmapLevel, slice, pixelBytes, bytesPerRow, bytesPerImage);
		else
#endif
        	[(id<MTLTexture>)m_ptr replaceRegion:MTLRegionMake3D(region.origin.x, region.origin.y, region.origin.z, region.size.width, region.size.height, region.size.depth)
                                          mipmapLevel:mipmapLevel
                                                slice:slice
                                            withBytes:pixelBytes
                                          bytesPerRow:bytesPerRow
                                        bytesPerImage:bytesPerImage];
    }

    void Texture::GetBytes(void* pixelBytes, NSUInteger bytesPerRow, const Region& fromRegion, NSUInteger mipmapLevel)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			((IMPTable<id<MTLTexture>, void>*)m_table)->Getbytesbytesperrowfromregionmipmaplevel((id<MTLTexture>)m_ptr, pixelBytes, bytesPerRow, fromRegion, mipmapLevel);
		else
#endif
        	[(id<MTLTexture>)m_ptr getBytes:pixelBytes
                                     bytesPerRow:bytesPerRow
                                      fromRegion:MTLRegionMake3D(fromRegion.origin.x, fromRegion.origin.y, fromRegion.origin.z, fromRegion.size.width, fromRegion.size.height, fromRegion.size.depth)
                                     mipmapLevel:mipmapLevel];
    }

    void Texture::Replace(const Region& region, NSUInteger mipmapLevel, void  const* pixelBytes, NSUInteger bytesPerRow)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			((IMPTable<id<MTLTexture>, void>*)m_table)->Replaceregionmipmaplevelwithbytesbytesperrow((id<MTLTexture>)m_ptr, region, mipmapLevel, pixelBytes, bytesPerRow);
		else
#endif
        	[(id<MTLTexture>)m_ptr replaceRegion:MTLRegionMake3D(region.origin.x, region.origin.y, region.origin.z, region.size.width, region.size.height, region.size.depth)
                                          mipmapLevel:mipmapLevel
                                            withBytes:pixelBytes
                                          bytesPerRow:bytesPerRow];
    }

    Texture Texture::NewTextureView(PixelFormat pixelFormat)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return Texture(((IMPTable<id<MTLTexture>, void>*)m_table)->Newtextureviewwithpixelformat((id<MTLTexture>)m_ptr, MTLPixelFormat(pixelFormat)), ((UE::ITable<id<MTLTexture>, void>*)m_table)->TableCache, ns::Ownership::Assign);
		else
#endif
        	return Texture([(id<MTLTexture>)m_ptr newTextureViewWithPixelFormat:MTLPixelFormat(pixelFormat)], nullptr, ns::Ownership::Assign);
    }

    Texture Texture::NewTextureView(PixelFormat pixelFormat, TextureType textureType, const ns::Range& mipmapLevelRange, const ns::Range& sliceRange)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		if (m_table)
			return Texture(((IMPTable<id<MTLTexture>, void>*)m_table)->Newtextureviewwithpixelformattexturetypelevelsslices((id<MTLTexture>)m_ptr, MTLPixelFormat(pixelFormat), MTLTextureType(textureType), NSMakeRange(mipmapLevelRange.Location, mipmapLevelRange.Length), NSMakeRange(sliceRange.Location, sliceRange.Length)), ((UE::ITable<id<MTLTexture>, void>*)m_table)->TableCache, ns::Ownership::Assign);
		else
#endif
        	return Texture([(id<MTLTexture>)m_ptr newTextureViewWithPixelFormat:MTLPixelFormat(pixelFormat)
                                                                                             textureType:MTLTextureType(textureType)
                                                                                                  levels:NSMakeRange(mipmapLevelRange.Location, mipmapLevelRange.Length)
                                                                                                  slices:NSMakeRange(sliceRange.Location, sliceRange.Length)], nullptr, ns::Ownership::Assign);
    }
	
	Texture::Texture(ns::Protocol<id<MTLTexture>>::type handle, UE::ITableCache* cache, ns::Ownership const retain)
	: Resource((ns::Protocol<id<MTLResource>>::type)handle, retain, (UE::ITable<ns::Protocol<id<MTLResource>>::type, void>*)UE::ITableCacheRef(cache).GetTexture(handle))
	{
	}
	
	Texture::Texture(const Texture& rhs)
	: Resource(rhs)
	{
		
	}
#if MTLPP_CONFIG_RVALUE_REFERENCES
	Texture::Texture(Texture&& rhs)
	: Resource((Resource&&)rhs)
	{
		
	}
#endif
	
	Texture& Texture::operator=(const Texture& rhs)
	{
		if (this != &rhs)
		{
			Resource::operator=(rhs);
		}
		return *this;
	}
	
#if MTLPP_CONFIG_RVALUE_REFERENCES
	Texture& Texture::operator=(Texture&& rhs)
	{
		Resource::operator=((Resource&&)rhs);
		return *this;
	}
#endif
	
#if MTLPP_CONFIG_VALIDATE
	void ValidatedTexture::GetBytes(void* pixelBytes, NSUInteger bytesPerRow, NSUInteger bytesPerImage, const Region& fromRegion, NSUInteger mipmapLevel, NSUInteger slice)
	{
		Validator.ValidateUsage(mtlpp::ResourceUsage::Read);
	}
	
	void ValidatedTexture::Replace(const Region& region, NSUInteger mipmapLevel, NSUInteger slice, void const* pixelBytes, NSUInteger bytesPerRow, NSUInteger bytesPerImage)
	{
		Validator.ValidateUsage(mtlpp::ResourceUsage::Write);
	}
	
	void ValidatedTexture::GetBytes(void* pixelBytes, NSUInteger bytesPerRow, const Region& fromRegion, NSUInteger mipmapLevel)
	{
		Validator.ValidateUsage(mtlpp::ResourceUsage::Read);
	}
	
	void ValidatedTexture::Replace(const Region& region, NSUInteger mipmapLevel, void const* pixelBytes, NSUInteger bytesPerRow)
	{
		Validator.ValidateUsage(mtlpp::ResourceUsage::Write);
	}
#endif
}

MTLPP_END
