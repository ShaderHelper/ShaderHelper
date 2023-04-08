// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_ArgumentEncoder_hpp
#define imp_ArgumentEncoder_hpp

#include "imp_Object.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<id<MTLArgumentEncoder>, void> : public IMPTableBase<id<MTLArgumentEncoder>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<id<MTLArgumentEncoder>>(C)
	, INTERPOSE_CONSTRUCTOR(Device, C)
	, INTERPOSE_CONSTRUCTOR(Label, C)
	, INTERPOSE_CONSTRUCTOR(SetLabel, C)
	, INTERPOSE_CONSTRUCTOR(EncodedLength, C)
	, INTERPOSE_CONSTRUCTOR(Alignment, C)
	, INTERPOSE_CONSTRUCTOR(Setargumentbufferoffset, C)
	, INTERPOSE_CONSTRUCTOR(Setargumentbufferstartoffsetarrayelement, C)
	, INTERPOSE_CONSTRUCTOR(Setbufferoffsetatindex, C)
	, INTERPOSE_CONSTRUCTOR(Setbuffersoffsetswithrange, C)
	, INTERPOSE_CONSTRUCTOR(Settextureatindex, C)
	, INTERPOSE_CONSTRUCTOR(Settextureswithrange, C)
	, INTERPOSE_CONSTRUCTOR(Setsamplerstateatindex, C)
	, INTERPOSE_CONSTRUCTOR(Setsamplerstateswithrange, C)
	, INTERPOSE_CONSTRUCTOR(ConstantDataAtIndex, C)
	, INTERPOSE_CONSTRUCTOR(NewArgumentEncoderForBufferAtIndex, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, device, Device, id<MTLDevice>);
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, label, Label, NSString*);
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, setLabel:, SetLabel, void, NSString*);
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, encodedLength, EncodedLength, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, alignment, Alignment, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, setArgumentBuffer:offset:, Setargumentbufferoffset, void, id <MTLBuffer>, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, setArgumentBuffer:startOffset:arrayElement:, Setargumentbufferstartoffsetarrayelement, void, id<MTLBuffer>, NSUInteger, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, setBuffer:offset:atIndex:, Setbufferoffsetatindex, void,  id <MTLBuffer>, NSUInteger, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, setBuffers:offsets:withRange:, Setbuffersoffsetswithrange, void, const id <MTLBuffer>  *, const NSUInteger *, NSRange);
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, setTexture:atIndex:, Settextureatindex, void,  id <MTLTexture>, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, setTextures:withRange:, Settextureswithrange, void, const id <MTLTexture>  *, NSRange);
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, setSamplerState:atIndex:, Setsamplerstateatindex, void,  id <MTLSamplerState>, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, setSamplerStates:withRange:, Setsamplerstateswithrange, void, const id <MTLSamplerState>  *, NSRange);
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, constantDataAtIndex:, ConstantDataAtIndex, void*, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLArgumentEncoder>, newArgumentEncoderForBufferAtIndex:, NewArgumentEncoderForBufferAtIndex, id<MTLArgumentEncoder>, NSUInteger);
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<id<MTLArgumentEncoder>, InterposeClass> : public IMPTable<id<MTLArgumentEncoder>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLArgumentEncoder>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableBase<id<MTLArgumentEncoder>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(Device, C);
		INTERPOSE_REGISTRATION(Label, C);
		INTERPOSE_REGISTRATION(SetLabel, C);
		INTERPOSE_REGISTRATION(EncodedLength, C);
		INTERPOSE_REGISTRATION(Alignment, C);
		INTERPOSE_REGISTRATION(Setargumentbufferoffset, C);
		INTERPOSE_REGISTRATION(Setargumentbufferstartoffsetarrayelement, C);
		INTERPOSE_REGISTRATION(Setbufferoffsetatindex, C);
		INTERPOSE_REGISTRATION(Setbuffersoffsetswithrange, C);
		INTERPOSE_REGISTRATION(Settextureatindex, C);
		INTERPOSE_REGISTRATION(Settextureswithrange, C);
		INTERPOSE_REGISTRATION(Setsamplerstateatindex, C);
		INTERPOSE_REGISTRATION(Setsamplerstateswithrange, C);
		INTERPOSE_REGISTRATION(ConstantDataAtIndex, C);
		INTERPOSE_REGISTRATION(NewArgumentEncoderForBufferAtIndex, C);
	}
};

MTLPP_END

#endif /* imp_ArgumentEncoder_hpp */
