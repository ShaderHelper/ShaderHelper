// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_Texture_hpp
#define imp_Texture_hpp

#include "imp_Resource.hpp"

MTLPP_BEGIN

template<>
struct IMPTable<MTLTextureDescriptor*, void> : public IMPTableBase<MTLTextureDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLTextureDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(Texturetype, C)
	, INTERPOSE_CONSTRUCTOR(Pixelformat, C)
	, INTERPOSE_CONSTRUCTOR(Width, C)
	, INTERPOSE_CONSTRUCTOR(Height, C)
	, INTERPOSE_CONSTRUCTOR(Depth, C)
	, INTERPOSE_CONSTRUCTOR(Mipmaplevelcount, C)
	, INTERPOSE_CONSTRUCTOR(Samplecount, C)
	, INTERPOSE_CONSTRUCTOR(Arraylength, C)
	, INTERPOSE_CONSTRUCTOR(ResourceOptions, C)
	, INTERPOSE_CONSTRUCTOR(CpuCacheMode, C)
	, INTERPOSE_CONSTRUCTOR(StorageMode, C)
	, INTERPOSE_CONSTRUCTOR(Usage, C)
	, INTERPOSE_CONSTRUCTOR(setTexturetype, C)
	, INTERPOSE_CONSTRUCTOR(setPixelformat, C)
	, INTERPOSE_CONSTRUCTOR(setWidth, C)
	, INTERPOSE_CONSTRUCTOR(setHeight, C)
	, INTERPOSE_CONSTRUCTOR(setDepth, C)
	, INTERPOSE_CONSTRUCTOR(setMipmaplevelcount, C)
	, INTERPOSE_CONSTRUCTOR(setSamplecount, C)
	, INTERPOSE_CONSTRUCTOR(setArraylength, C)
	, INTERPOSE_CONSTRUCTOR(setResourceOptions, C)
	, INTERPOSE_CONSTRUCTOR(setCpuCacheMode, C)
	, INTERPOSE_CONSTRUCTOR(setStorageMode, C)
	, INTERPOSE_CONSTRUCTOR(setUsage, C)

	{
	}
	
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, textureType, Texturetype, MTLTextureType);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, pixelFormat, Pixelformat, MTLPixelFormat);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, width, Width, NSUInteger);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, height, Height, NSUInteger);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, depth, Depth, NSUInteger);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, mipmapLevelCount, Mipmaplevelcount, NSUInteger);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, sampleCount, Samplecount, NSUInteger);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, arrayLength, Arraylength, NSUInteger);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, resourceOptions, ResourceOptions, MTLResourceOptions);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, cpuCacheMode, CpuCacheMode, MTLCPUCacheMode);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, storageMode, StorageMode, MTLStorageMode);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, usage, Usage, MTLTextureUsage);
	
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, setTextureType:, setTexturetype, void, MTLTextureType);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, setPixelFormat:, setPixelformat, void, MTLPixelFormat);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, setWidth:, setWidth, void, NSUInteger);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, setHeight:, setHeight, void, NSUInteger);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, setDepth:, setDepth, void, NSUInteger);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, setMipmapLevelCount:, setMipmaplevelcount, void, NSUInteger);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, setSampleCount:, setSamplecount, void, NSUInteger);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, setArrayLength:, setArraylength, void, NSUInteger);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, setResourceOptions:, setResourceOptions, void, MTLResourceOptions);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, setCpuCacheMode:, setCpuCacheMode, void, MTLCPUCacheMode);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, setStorageMode:, setStorageMode, void, MTLStorageMode);
	INTERPOSE_SELECTOR(MTLTextureDescriptor*, setUsage:, setUsage, void, MTLTextureUsage);
};

template<>
struct IMPTable<id<MTLTexture>, void> : public IMPTableResource<id<MTLTexture>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableResource<id<MTLTexture>>(C)
	, INTERPOSE_CONSTRUCTOR(Rootresource, C)
	, INTERPOSE_CONSTRUCTOR(Parenttexture, C)
	, INTERPOSE_CONSTRUCTOR(Parentrelativelevel, C)
	, INTERPOSE_CONSTRUCTOR(Parentrelativeslice, C)
	, INTERPOSE_CONSTRUCTOR(Buffer, C)
	, INTERPOSE_CONSTRUCTOR(Bufferoffset, C)
	, INTERPOSE_CONSTRUCTOR(Bufferbytesperrow, C)
	, INTERPOSE_CONSTRUCTOR(Iosurface, C)
	, INTERPOSE_CONSTRUCTOR(Iosurfaceplane, C)
	, INTERPOSE_CONSTRUCTOR(Texturetype, C)
	, INTERPOSE_CONSTRUCTOR(Pixelformat, C)
	, INTERPOSE_CONSTRUCTOR(Width, C)
	, INTERPOSE_CONSTRUCTOR(Height, C)
	, INTERPOSE_CONSTRUCTOR(Depth, C)
	, INTERPOSE_CONSTRUCTOR(Mipmaplevelcount, C)
	, INTERPOSE_CONSTRUCTOR(Samplecount, C)
	, INTERPOSE_CONSTRUCTOR(Arraylength, C)
	, INTERPOSE_CONSTRUCTOR(Usage, C)
	, INTERPOSE_CONSTRUCTOR(Isframebufferonly, C)
	, INTERPOSE_CONSTRUCTOR(Getbytesbytesperrowbytesperimagefromregionmipmaplevelslice, C)
	, INTERPOSE_CONSTRUCTOR(Replaceregionmipmaplevelslicewithbytesbytesperrowbytesperimage, C)
	, INTERPOSE_CONSTRUCTOR(Getbytesbytesperrowfromregionmipmaplevel, C)
	, INTERPOSE_CONSTRUCTOR(Replaceregionmipmaplevelwithbytesbytesperrow, C)
	, INTERPOSE_CONSTRUCTOR(Newtextureviewwithpixelformat, C)
	, INTERPOSE_CONSTRUCTOR(Newtextureviewwithpixelformattexturetypelevelsslices, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLTexture>, rootResource, Rootresource, id <MTLResource>);
	INTERPOSE_SELECTOR(id<MTLTexture>, parentTexture, Parenttexture, id <MTLTexture> );
	INTERPOSE_SELECTOR(id<MTLTexture>, parentRelativeLevel, Parentrelativelevel, NSUInteger );
	INTERPOSE_SELECTOR(id<MTLTexture>, parentRelativeSlice, Parentrelativeslice, NSUInteger );
	INTERPOSE_SELECTOR(id<MTLTexture>, buffer, Buffer, id <MTLBuffer> );
	INTERPOSE_SELECTOR(id<MTLTexture>, bufferOffset, Bufferoffset, NSUInteger );
	INTERPOSE_SELECTOR(id<MTLTexture>, bufferBytesPerRow, Bufferbytesperrow, NSUInteger );
	INTERPOSE_SELECTOR(id<MTLTexture>, iosurface, Iosurface, IOSurfaceRef);
	INTERPOSE_SELECTOR(id<MTLTexture>, iosurfacePlane, Iosurfaceplane, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLTexture>, textureType, Texturetype, MTLTextureType);
	INTERPOSE_SELECTOR(id<MTLTexture>, pixelFormat, Pixelformat, MTLPixelFormat);
	INTERPOSE_SELECTOR(id<MTLTexture>, width, Width, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLTexture>, height, Height, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLTexture>, depth, Depth, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLTexture>, mipmapLevelCount, Mipmaplevelcount, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLTexture>, sampleCount, Samplecount, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLTexture>, arrayLength, Arraylength, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLTexture>, usage, Usage, MTLTextureUsage);
	INTERPOSE_SELECTOR(id<MTLTexture>, isFramebufferOnly, Isframebufferonly, BOOL);
	INTERPOSE_SELECTOR(id<MTLTexture>, getBytes:bytesPerRow:bytesPerImage:fromRegion:mipmapLevel:slice:, Getbytesbytesperrowbytesperimagefromregionmipmaplevelslice, void, void * , NSUInteger , NSUInteger , MTLPPRegion , NSUInteger , NSUInteger );
	INTERPOSE_SELECTOR(id<MTLTexture>, replaceRegion:mipmapLevel:slice:withBytes:bytesPerRow:bytesPerImage:, Replaceregionmipmaplevelslicewithbytesbytesperrowbytesperimage, void, MTLPPRegion , NSUInteger , NSUInteger , const void * , NSUInteger , NSUInteger );
	INTERPOSE_SELECTOR(id<MTLTexture>, getBytes:bytesPerRow:fromRegion:mipmapLevel:, Getbytesbytesperrowfromregionmipmaplevel, void, void * , NSUInteger , MTLPPRegion , NSUInteger );
	INTERPOSE_SELECTOR(id<MTLTexture>, replaceRegion:mipmapLevel:withBytes:bytesPerRow:, Replaceregionmipmaplevelwithbytesbytesperrow, void, MTLPPRegion , NSUInteger , const void * , NSUInteger );
	INTERPOSE_SELECTOR(id<MTLTexture>, newTextureViewWithPixelFormat:, Newtextureviewwithpixelformat, id<MTLTexture>, MTLPixelFormat );
	INTERPOSE_SELECTOR(id<MTLTexture>, newTextureViewWithPixelFormat:textureType:levels:slices:, Newtextureviewwithpixelformattexturetypelevelsslices, id<MTLTexture>, MTLPixelFormat , MTLTextureType , NSRange , NSRange );
};

template<typename InterposeClass>
struct IMPTable<id<MTLTexture>, InterposeClass> : public IMPTable<id<MTLTexture>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLTexture>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableResource<id<MTLTexture>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(Rootresource, C);
		INTERPOSE_REGISTRATION(Parenttexture, C);
		INTERPOSE_REGISTRATION(Parentrelativelevel, C);
		INTERPOSE_REGISTRATION(Parentrelativeslice, C);
		INTERPOSE_REGISTRATION(Buffer, C);
		INTERPOSE_REGISTRATION(Bufferoffset, C);
		INTERPOSE_REGISTRATION(Bufferbytesperrow, C);
		INTERPOSE_REGISTRATION(Iosurface, C);
		INTERPOSE_REGISTRATION(Iosurfaceplane, C);
		INTERPOSE_REGISTRATION(Texturetype, C);
		INTERPOSE_REGISTRATION(Pixelformat, C);
		INTERPOSE_REGISTRATION(Width, C);
		INTERPOSE_REGISTRATION(Height, C);
		INTERPOSE_REGISTRATION(Depth, C);
		INTERPOSE_REGISTRATION(Mipmaplevelcount, C);
		INTERPOSE_REGISTRATION(Samplecount, C);
		INTERPOSE_REGISTRATION(Arraylength, C);
		INTERPOSE_REGISTRATION(Usage, C);
		INTERPOSE_REGISTRATION(Isframebufferonly, C);
		INTERPOSE_REGISTRATION(Getbytesbytesperrowbytesperimagefromregionmipmaplevelslice, C);
		INTERPOSE_REGISTRATION(Replaceregionmipmaplevelslicewithbytesbytesperrowbytesperimage, C);
		INTERPOSE_REGISTRATION(Getbytesbytesperrowfromregionmipmaplevel, C);
		INTERPOSE_REGISTRATION(Replaceregionmipmaplevelwithbytesbytesperrow, C);
		INTERPOSE_REGISTRATION(Newtextureviewwithpixelformat, C);
		INTERPOSE_REGISTRATION(Newtextureviewwithpixelformattexturetypelevelsslices, C);
	}
};

MTLPP_END

#endif /* imp_Texture_hpp */
