// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_Buffer_hpp
#define imp_Buffer_hpp

#include "imp_Resource.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<id<MTLBuffer>, void> : public IMPTableResource<id<MTLBuffer>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableResource<id<MTLBuffer>>(C)
	, INTERPOSE_CONSTRUCTOR(Length, C)
	, INTERPOSE_CONSTRUCTOR(Contents, C)
	, INTERPOSE_CONSTRUCTOR(DidModifyRange, C)
	, INTERPOSE_CONSTRUCTOR(NewTextureWithDescriptorOffsetBytesPerRow, C)
	, INTERPOSE_CONSTRUCTOR(AddDebugMarkerRange, C)
	, INTERPOSE_CONSTRUCTOR(RemoveAllDebugMarkers, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLBuffer>, length, Length, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLBuffer>, contents, Contents, void*);
	INTERPOSE_SELECTOR(id<MTLBuffer>, didModifyRange:, DidModifyRange, void, NSRange);
	INTERPOSE_SELECTOR(id<MTLBuffer>, newTextureWithDescriptor:offset:bytesPerRow:, NewTextureWithDescriptorOffsetBytesPerRow, id<MTLTexture>, MTLTextureDescriptor*, NSUInteger, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLBuffer>, addDebugMarker:range:, AddDebugMarkerRange, void, NSString*, NSRange);
	INTERPOSE_SELECTOR(id<MTLBuffer>, removeAllDebugMarkers, RemoveAllDebugMarkers, void);
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<id<MTLBuffer>, InterposeClass> : public IMPTable<id<MTLBuffer>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLBuffer>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableResource<id<MTLBuffer>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(Contents, C);
		INTERPOSE_REGISTRATION(DidModifyRange, C);
		INTERPOSE_REGISTRATION(NewTextureWithDescriptorOffsetBytesPerRow, C);
		INTERPOSE_REGISTRATION(AddDebugMarkerRange, C);
		INTERPOSE_REGISTRATION(RemoveAllDebugMarkers, C);
	}
};

MTLPP_END

#endif /* imp_Buffer_hpp */
