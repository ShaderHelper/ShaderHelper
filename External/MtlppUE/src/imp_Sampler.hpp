// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_Sampler_hpp
#define imp_Sampler_hpp

#include "imp_State.hpp"

MTLPP_BEGIN

template<>
struct IMPTable<MTLSamplerDescriptor*, void> : public IMPTableBase<MTLSamplerDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLSamplerDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(minFilter, C)
	, INTERPOSE_CONSTRUCTOR(setminFilter, C)
	, INTERPOSE_CONSTRUCTOR(magFilter, C)
	, INTERPOSE_CONSTRUCTOR(setmagFilter, C)
	, INTERPOSE_CONSTRUCTOR(mipFilter, C)
	, INTERPOSE_CONSTRUCTOR(setmipFilter, C)
	, INTERPOSE_CONSTRUCTOR(maxAnisotropy, C)
	, INTERPOSE_CONSTRUCTOR(setmaxAnisotropy, C)
	, INTERPOSE_CONSTRUCTOR(sAddressMode, C)
	, INTERPOSE_CONSTRUCTOR(setsAddressMode, C)
	, INTERPOSE_CONSTRUCTOR(tAddressMode, C)
	, INTERPOSE_CONSTRUCTOR(settAddressMode, C)
	, INTERPOSE_CONSTRUCTOR(rAddressMode, C)
	, INTERPOSE_CONSTRUCTOR(setrAddressMode, C)
#if TARGET_OS_OSX
	, INTERPOSE_CONSTRUCTOR(borderColor, C)
	, INTERPOSE_CONSTRUCTOR(setborderColor, C)
#endif
	, INTERPOSE_CONSTRUCTOR(normalizedCoordinates, C)
	, INTERPOSE_CONSTRUCTOR(setnormalizedCoordinates, C)
	, INTERPOSE_CONSTRUCTOR(lodMinClamp, C)
	, INTERPOSE_CONSTRUCTOR(setlodMinClamp, C)
	, INTERPOSE_CONSTRUCTOR(lodMaxClamp, C)
	, INTERPOSE_CONSTRUCTOR(setlodMaxClamp, C)
	, INTERPOSE_CONSTRUCTOR(lodAverage, C)
	, INTERPOSE_CONSTRUCTOR(setlodAverage, C)
	, INTERPOSE_CONSTRUCTOR(compareFunction, C)
	, INTERPOSE_CONSTRUCTOR(setcompareFunction, C)
	, INTERPOSE_CONSTRUCTOR(supportArgumentBuffers, C)
	, INTERPOSE_CONSTRUCTOR(setsupportArgumentBuffers, C)
	, INTERPOSE_CONSTRUCTOR(label, C)
	, INTERPOSE_CONSTRUCTOR(setlabel, C)
	{
	}
	
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, minFilter, minFilter, MTLSamplerMinMagFilter);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setMinFilter:, setminFilter, void, MTLSamplerMinMagFilter);
	
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, magFilter, magFilter, MTLSamplerMinMagFilter);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setMagFilter:, setmagFilter, void, MTLSamplerMinMagFilter);
	
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, mipFilter, mipFilter, MTLSamplerMipFilter);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setMipFilter:, setmipFilter, void, MTLSamplerMipFilter);
	
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, maxAnisotropy, maxAnisotropy, NSUInteger);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setMaxAnisotropy:, setmaxAnisotropy, void, NSUInteger);
	
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, sAddressMode, sAddressMode, MTLSamplerAddressMode);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setSAddressMode:, setsAddressMode, void, MTLSamplerAddressMode);
	
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, tAddressMode, tAddressMode, MTLSamplerAddressMode);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setTAddressMode:, settAddressMode, void, MTLSamplerAddressMode);
	
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, rAddressMode, rAddressMode, MTLSamplerAddressMode);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setRAddressMode:, setrAddressMode, void, MTLSamplerAddressMode);
	
#if TARGET_OS_OSX
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, borderColor, borderColor, MTLSamplerBorderColor);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setBorderColor:, setborderColor, void, MTLSamplerBorderColor);
#endif
	
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, normalizedCoordinates, normalizedCoordinates, BOOL);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setNormalizedCoordinates:, setnormalizedCoordinates, void, BOOL);
	
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, lodMinClamp, lodMinClamp, float);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setLodMinClamp:, setlodMinClamp, void, float);
	
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, lodMaxClamp, lodMaxClamp, float);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setLodMaxClamp:, setlodMaxClamp, void, float);
	
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, lodAverage, lodAverage, BOOL);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setLodAverage:, setlodAverage, void, BOOL);
	
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, compareFunction, compareFunction, MTLCompareFunction);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setCompareFunction:, setcompareFunction, void, MTLCompareFunction);
	
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, supportArgumentBuffers, supportArgumentBuffers, BOOL);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setSupportArgumentBuffers:, setsupportArgumentBuffers, void, BOOL);
	
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, label, label, NSString *);
	INTERPOSE_SELECTOR(MTLSamplerDescriptor*, setLabel:, setlabel, void, NSString *);
};

template<>
struct IMPTable<id<MTLSamplerState>, void> : public IMPTableState<id<MTLSamplerState>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableState<id<MTLSamplerState>>(C)
	{
	}
};

template<typename InterposeClass>
struct IMPTable<id<MTLSamplerState>, InterposeClass> : public IMPTable<id<MTLSamplerState>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLSamplerState>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableState<id<MTLSamplerState>>::RegisterInterpose<InterposeClass>(C);
	}
};

MTLPP_END

#endif /* imp_Sampler_hpp */
