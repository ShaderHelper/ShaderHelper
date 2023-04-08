// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_BlitCommandEncoder_hpp
#define imp_BlitCommandEncoder_hpp

#include "imp_CommandEncoder.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<id<MTLBlitCommandEncoder>, void> : public IMPTableCommandEncoder<id<MTLBlitCommandEncoder>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableCommandEncoder<id<MTLBlitCommandEncoder>>(C)
	, INTERPOSE_CONSTRUCTOR(SynchronizeResource, C)
	, INTERPOSE_CONSTRUCTOR(SynchronizeTextureSliceLevel, C)
	, INTERPOSE_CONSTRUCTOR(CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoTexturedestinationSlicedestinationLeveldestinationOrigin, C)
	, INTERPOSE_CONSTRUCTOR(CopyFromBuffersourceOffsetsourceBytesPerRowsourceBytesPerImagesourceSizetoTexturedestinationSlicedestinationLeveldestinationOrigin, C)
	, INTERPOSE_CONSTRUCTOR(CopyFromBuffersourceOffsetsourceBytesPerRowsourceBytesPerImagesourceSizetoTexturedestinationSlicedestinationLeveldestinationOriginoptions, C)
	, INTERPOSE_CONSTRUCTOR(CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoBufferdestinationOffsetdestinationBytesPerRowdestinationBytesPerImage, C)
	, INTERPOSE_CONSTRUCTOR(CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoBufferdestinationOffsetdestinationBytesPerRowdestinationBytesPerImageoptions, C)
	, INTERPOSE_CONSTRUCTOR(GenerateMipmapsForTexture, C)
	, INTERPOSE_CONSTRUCTOR(FillBufferRangeValue, C)
	, INTERPOSE_CONSTRUCTOR(CopyFromBufferSourceOffsetToBufferDestinationOffsetSize, C)
	, INTERPOSE_CONSTRUCTOR(UpdateFence, C)
	, INTERPOSE_CONSTRUCTOR(WaitForFence, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLBlitCommandEncoder>, synchronizeResource:, SynchronizeResource, void, id<MTLResource>);
	INTERPOSE_SELECTOR(id<MTLBlitCommandEncoder>, synchronizeTexture:slice:level:, SynchronizeTextureSliceLevel, void, id<MTLTexture>, NSUInteger, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLBlitCommandEncoder>, copyFromTexture:sourceSlice:sourceLevel:sourceOrigin:sourceSize:toTexture:destinationSlice:destinationLevel:destinationOrigin:, CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoTexturedestinationSlicedestinationLeveldestinationOrigin, void, id<MTLTexture>, NSUInteger, NSUInteger, MTLPPOrigin, MTLPPSize, id<MTLTexture>, NSUInteger, NSUInteger, MTLPPOrigin);
	INTERPOSE_SELECTOR(id<MTLBlitCommandEncoder>, copyFromBuffer:sourceOffset:sourceBytesPerRow:sourceBytesPerImage:sourceSize:toTexture:destinationSlice:destinationLevel:destinationOrigin:, CopyFromBuffersourceOffsetsourceBytesPerRowsourceBytesPerImagesourceSizetoTexturedestinationSlicedestinationLeveldestinationOrigin, void, id<MTLBuffer>, NSUInteger, NSUInteger, NSUInteger, MTLPPSize, id<MTLTexture>, NSUInteger, NSUInteger, MTLPPOrigin);
	INTERPOSE_SELECTOR(id<MTLBlitCommandEncoder>, copyFromBuffer:sourceOffset:sourceBytesPerRow:sourceBytesPerImage:sourceSize:toTexture:destinationSlice:destinationLevel:destinationOrigin:options:, CopyFromBuffersourceOffsetsourceBytesPerRowsourceBytesPerImagesourceSizetoTexturedestinationSlicedestinationLeveldestinationOriginoptions, void, id<MTLBuffer>, NSUInteger, NSUInteger, NSUInteger, MTLPPSize, id<MTLTexture>, NSUInteger, NSUInteger, MTLPPOrigin, MTLBlitOption);
	INTERPOSE_SELECTOR(id<MTLBlitCommandEncoder>, copyFromTexture:sourceSlice:sourceLevel:sourceOrigin:sourceSize:toBuffer:destinationOffset:destinationBytesPerRow:destinationBytesPerImage:, CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoBufferdestinationOffsetdestinationBytesPerRowdestinationBytesPerImage, void, id<MTLTexture>, NSUInteger, NSUInteger, MTLPPOrigin, MTLPPSize, id<MTLBuffer>, NSUInteger, NSUInteger, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLBlitCommandEncoder>, copyFromTexture:sourceSlice:sourceLevel:sourceOrigin:sourceSize:toBuffer:destinationOffset:destinationBytesPerRow:destinationBytesPerImage:options:, CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoBufferdestinationOffsetdestinationBytesPerRowdestinationBytesPerImageoptions, void, id<MTLTexture>, NSUInteger, NSUInteger, MTLPPOrigin, MTLPPSize, id<MTLBuffer>, NSUInteger, NSUInteger, NSUInteger, MTLBlitOption);
	INTERPOSE_SELECTOR(id<MTLBlitCommandEncoder>, generateMipmapsForTexture:, GenerateMipmapsForTexture, void, id<MTLTexture>);
	INTERPOSE_SELECTOR(id<MTLBlitCommandEncoder>, fillBuffer:range:value:, FillBufferRangeValue, void, id <MTLBuffer>,  NSRange, uint8_t);
	INTERPOSE_SELECTOR(id<MTLBlitCommandEncoder>, copyFromBuffer:sourceOffset:toBuffer:destinationOffset:size:, CopyFromBufferSourceOffsetToBufferDestinationOffsetSize, void, id <MTLBuffer>, NSUInteger, id <MTLBuffer>, NSUInteger, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLBlitCommandEncoder>, updateFence:, UpdateFence, void, id <MTLFence>);
	INTERPOSE_SELECTOR(id<MTLBlitCommandEncoder>, waitForFence:, WaitForFence, void, id <MTLFence>);
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<id<MTLBlitCommandEncoder>, InterposeClass> : public IMPTable<id<MTLBlitCommandEncoder>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLBlitCommandEncoder>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableCommandEncoder<id<MTLBlitCommandEncoder>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(SynchronizeResource, C);
		INTERPOSE_REGISTRATION(SynchronizeTextureSliceLevel, C);
		INTERPOSE_REGISTRATION(CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoTexturedestinationSlicedestinationLeveldestinationOrigin, C);
		INTERPOSE_REGISTRATION(CopyFromBuffersourceOffsetsourceBytesPerRowsourceBytesPerImagesourceSizetoTexturedestinationSlicedestinationLeveldestinationOrigin, C);
		INTERPOSE_REGISTRATION(CopyFromBuffersourceOffsetsourceBytesPerRowsourceBytesPerImagesourceSizetoTexturedestinationSlicedestinationLeveldestinationOriginoptions, C);
		INTERPOSE_REGISTRATION(CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoBufferdestinationOffsetdestinationBytesPerRowdestinationBytesPerImage, C);
		INTERPOSE_REGISTRATION(CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoBufferdestinationOffsetdestinationBytesPerRowdestinationBytesPerImageoptions, C);
		INTERPOSE_REGISTRATION(GenerateMipmapsForTexture, C);
		INTERPOSE_REGISTRATION(FillBufferRangeValue, C);
		INTERPOSE_REGISTRATION(CopyFromBufferSourceOffsetToBufferDestinationOffsetSize, C);
		INTERPOSE_REGISTRATION(UpdateFence, C);
		INTERPOSE_REGISTRATION(WaitForFence, C);
	}
};

MTLPP_END

#endif /* imp_BlitCommandEncoder_hpp */
