// Copyright Epic Games, Inc. All Rights Reserved.
#ifndef imp_Drawable_hpp
#define imp_Drawable_hpp

#include "imp_Object.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<id<MTLDrawable>, void> : public IMPTableBase<id<MTLDrawable>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<id<MTLDrawable>>(C)
	, INTERPOSE_CONSTRUCTOR(Texture, C)
	, INTERPOSE_CONSTRUCTOR(Layer, C)
	, INTERPOSE_CONSTRUCTOR(Present, C)
	, INTERPOSE_CONSTRUCTOR(PresentAtTime, C)
	, INTERPOSE_CONSTRUCTOR(PresentAfterMinimumDuration, C)
	, INTERPOSE_CONSTRUCTOR(AddPresentedHandler, C)
	, INTERPOSE_CONSTRUCTOR(PresentedTime, C)
	, INTERPOSE_CONSTRUCTOR(DrawableID, C)
	{
	}

	INTERPOSE_SELECTOR(id<MTLDrawable>, texture, Texture, id <MTLTexture>);
	INTERPOSE_SELECTOR(id<MTLDrawable>, layer, Layer, CAMetalLayer*);
	INTERPOSE_SELECTOR(id<MTLDrawable>, present, Present, void);
	INTERPOSE_SELECTOR(id<MTLDrawable>, presentAtTime:, PresentAtTime, void, CFTimeInterval);
	INTERPOSE_SELECTOR(id<MTLDrawable>, presentAfterMinimumDuration:, PresentAfterMinimumDuration, void, CFTimeInterval);
	INTERPOSE_SELECTOR(id<MTLDrawable>, addPresentedHandler:, AddPresentedHandler, void, MTLDrawablePresentedHandler);
	INTERPOSE_SELECTOR(id<MTLDrawable>, presentedTime, PresentedTime, CFTimeInterval);
	INTERPOSE_SELECTOR(id<MTLDrawable>, drawableID, DrawableID, NSUInteger);
	
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<id<MTLDrawable>, InterposeClass> : public IMPTable<id<MTLDrawable>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLDrawable>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableBase<id<MTLDrawable>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(Texture, C);
		INTERPOSE_REGISTRATION(Layer, C);
		INTERPOSE_REGISTRATION(Present, C);
		INTERPOSE_REGISTRATION(PresentAtTime, C);
		INTERPOSE_REGISTRATION(PresentAfterMinimumDuration, C);
		INTERPOSE_REGISTRATION(AddPresentedHandler, C);
	}
};

template<>
struct MTLPP_EXPORT IMPTable<CAMetalLayer*, void> : public IMPTableBase<CAMetalLayer*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<CAMetalLayer*>(C)
	, INTERPOSE_CONSTRUCTOR(init, C)
	, INTERPOSE_CONSTRUCTOR(initWithLayer, C)
	, INTERPOSE_CONSTRUCTOR(Device, C)
	, INTERPOSE_CONSTRUCTOR(Setdevice, C)
	, INTERPOSE_CONSTRUCTOR(PixelFormat, C)
	, INTERPOSE_CONSTRUCTOR(SetpixelFormat, C)
	, INTERPOSE_CONSTRUCTOR(framebufferOnly, C)
	, INTERPOSE_CONSTRUCTOR(SetframebufferOnly, C)
	, INTERPOSE_CONSTRUCTOR(DrawableSize, C)
	, INTERPOSE_CONSTRUCTOR(SetddrawableSize, C)
	, INTERPOSE_CONSTRUCTOR(NextDrawable, C)
	, INTERPOSE_CONSTRUCTOR(MaximumDrawableCount, C)
	, INTERPOSE_CONSTRUCTOR(SetmaximumDrawableCount, C)
	, INTERPOSE_CONSTRUCTOR(PresentsWithTransaction, C)
	, INTERPOSE_CONSTRUCTOR(SetpresentsWithTransaction, C)
	, INTERPOSE_CONSTRUCTOR(Colorspace, C)
	, INTERPOSE_CONSTRUCTOR(Setcolorspace, C)
	, INTERPOSE_CONSTRUCTOR(WantsExtendedDynamicRangeContent, C)
	, INTERPOSE_CONSTRUCTOR(SetwantsExtendedDynamicRangeContent, C)
	, INTERPOSE_CONSTRUCTOR(DisplaySyncEnabled, C)
	, INTERPOSE_CONSTRUCTOR(SetdisplaySyncEnabled, C)
	, INTERPOSE_CONSTRUCTOR(AllowsNextDrawableTimeout, C)
	, INTERPOSE_CONSTRUCTOR(SetallowsNextDrawableTimeout, C)
	{
	}
	
	INTERPOSE_SELECTOR(CAMetalLayer*, init, init, id);
	INTERPOSE_SELECTOR(CAMetalLayer*, initWithLayer:, initWithLayer, id, id);
	INTERPOSE_SELECTOR(CAMetalLayer*, device, Device, id <MTLDevice>);
	INTERPOSE_SELECTOR(CAMetalLayer*, setDevice:, Setdevice, void, id <MTLDevice>);
	INTERPOSE_SELECTOR(CAMetalLayer*, pixelFormat, PixelFormat, MTLPixelFormat);
	INTERPOSE_SELECTOR(CAMetalLayer*, setPixelFormat:, SetpixelFormat, void, MTLPixelFormat);
	INTERPOSE_SELECTOR(CAMetalLayer*, framebufferOnly, framebufferOnly, BOOL);
	INTERPOSE_SELECTOR(CAMetalLayer*, setFramebufferOnly:, SetframebufferOnly, void, BOOL);
	INTERPOSE_SELECTOR(CAMetalLayer*, drawableSize, DrawableSize, CGSize);
	INTERPOSE_SELECTOR(CAMetalLayer*, setDrawableSize:, SetddrawableSize, void, CGSize);
	INTERPOSE_SELECTOR(CAMetalLayer*, nextDrawable, NextDrawable, id <CAMetalDrawable>);
	INTERPOSE_SELECTOR(CAMetalLayer*, maximumDrawableCount, MaximumDrawableCount, NSUInteger);
	INTERPOSE_SELECTOR(CAMetalLayer*, setMaximumDrawableCount:, SetmaximumDrawableCount, void, NSUInteger);
	INTERPOSE_SELECTOR(CAMetalLayer*, presentsWithTransaction, PresentsWithTransaction, BOOL);
	INTERPOSE_SELECTOR(CAMetalLayer*, setPresentsWithTransaction:, SetpresentsWithTransaction, void, BOOL);
	INTERPOSE_SELECTOR(CAMetalLayer*, colorspace, Colorspace, CGColorSpaceRef);
	INTERPOSE_SELECTOR(CAMetalLayer*, setColorspace:, Setcolorspace, void, CGColorSpaceRef);
	INTERPOSE_SELECTOR(CAMetalLayer*, wantsExtendedDynamicRangeContent, WantsExtendedDynamicRangeContent, BOOL);
	INTERPOSE_SELECTOR(CAMetalLayer*, setWantsExtendedDynamicRangeContent:, SetwantsExtendedDynamicRangeContent, void, BOOL);
	INTERPOSE_SELECTOR(CAMetalLayer*, displaySyncEnabled, DisplaySyncEnabled, BOOL);
	INTERPOSE_SELECTOR(CAMetalLayer*, setDisplaySyncEnabled:, SetdisplaySyncEnabled, void, BOOL);
	INTERPOSE_SELECTOR(CAMetalLayer*, allowsNextDrawableTimeout, AllowsNextDrawableTimeout, BOOL);
	INTERPOSE_SELECTOR(CAMetalLayer*, setAllowsNextDrawableTimeout:, SetallowsNextDrawableTimeout, void, BOOL);
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<CAMetalLayer*, InterposeClass> : public IMPTable<CAMetalLayer*, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<CAMetalLayer*, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableBase<CAMetalLayer*>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(Setdevice, C);
		INTERPOSE_REGISTRATION(SetpixelFormat, C);
		INTERPOSE_REGISTRATION(SetframebufferOnly, C);
		INTERPOSE_REGISTRATION(SetddrawableSize, C);
		INTERPOSE_REGISTRATION(NextDrawable, C);
		INTERPOSE_REGISTRATION(SetmaximumDrawableCount, C);
		INTERPOSE_REGISTRATION(SetpresentsWithTransaction, C);
		INTERPOSE_REGISTRATION(Setcolorspace, C);
		INTERPOSE_REGISTRATION(SetwantsExtendedDynamicRangeContent, C);
		INTERPOSE_REGISTRATION(SetdisplaySyncEnabled, C);
		INTERPOSE_REGISTRATION(SetallowsNextDrawableTimeout, C);
	}
};

MTLPP_END

#endif /* imp_Drawable_hpp */
