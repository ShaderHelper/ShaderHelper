/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_RenderPass.hpp"
#include "ns.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	inline ITable<MTLRenderPassColorAttachmentDescriptor*, void>* CreateIMPTable(MTLRenderPassColorAttachmentDescriptor* handle)
	{
		static ITable<MTLRenderPassColorAttachmentDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLRenderPassDepthAttachmentDescriptor*, void>* CreateIMPTable(MTLRenderPassDepthAttachmentDescriptor* handle)
	{
		static ITable<MTLRenderPassDepthAttachmentDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLRenderPassStencilAttachmentDescriptor*, void>* CreateIMPTable(MTLRenderPassStencilAttachmentDescriptor* handle)
	{
		static ITable<MTLRenderPassStencilAttachmentDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLRenderPassDescriptor*, void>* CreateIMPTable(MTLRenderPassDescriptor* handle)
	{
		static ITable<MTLRenderPassDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
}

namespace mtlpp
{
    class Texture;
    class Buffer;
	struct SamplePosition;

    enum class LoadAction
    {
        DontCare = 0,
        Load     = 1,
        Clear    = 2,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class StoreAction
    {
        DontCare                                               = 0,
        Store                                                  = 1,
        MultisampleResolve                                     = 2,
        StoreAndMultisampleResolve MTLPP_AVAILABLE(10_12,10_0) = 3,
        Unknown                    MTLPP_AVAILABLE(10_12,10_0) = 4,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class MultisampleDepthResolveFilter
    {
        Sample0 = 0,
        Min     = 1,
        Max     = 2,
    }
    MTLPP_AVAILABLE_AX(9_0);

	enum class StoreActionOptions
	{
		None                  = 0,
		CustomSamplePositions = 1 << 0,
	}
	MTLPP_AVAILABLE(10_13, 11_0);
	
    struct ClearColor
    {
        ClearColor(double red, double green, double blue, double alpha) :
            Red(red),
            Green(green),
            Blue(blue),
            Alpha(alpha) { }

        double Red;
        double Green;
        double Blue;
        double Alpha;
    };

	template<typename T>
    class MTLPP_EXPORT RenderPassAttachmentDescriptor : public ns::Object<T>
    {
    public:
        inline RenderPassAttachmentDescriptor();
        inline RenderPassAttachmentDescriptor(T handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<T>(handle, retain) { }

		inline ns::AutoReleased<Texture>     GetTexture() const;
        inline NSUInteger    GetLevel() const;
        inline NSUInteger    GetSlice() const;
        inline NSUInteger    GetDepthPlane() const;
        inline ns::AutoReleased<Texture>     GetResolveTexture() const;
        inline NSUInteger    GetResolveLevel() const;
        inline NSUInteger    GetResolveSlice() const;
        inline NSUInteger    GetResolveDepthPlane() const;
        inline LoadAction  GetLoadAction() const;
        inline StoreAction GetStoreAction() const;
		inline StoreActionOptions GetStoreActionOptions() const MTLPP_AVAILABLE(10_13, 11_0);

        inline void SetTexture(const Texture& texture);
        inline void SetLevel(NSUInteger level);
        inline void SetSlice(NSUInteger slice);
        inline void SetDepthPlane(NSUInteger depthPlane);
        inline void SetResolveTexture(const Texture& texture);
        inline void SetResolveLevel(NSUInteger resolveLevel);
        inline void SetResolveSlice(NSUInteger resolveSlice);
        inline void SetResolveDepthPlane(NSUInteger resolveDepthPlane);
        inline void SetLoadAction(LoadAction loadAction);
        inline void SetStoreAction(StoreAction storeAction);
		inline void SetStoreActionOptions(StoreActionOptions options) MTLPP_AVAILABLE(10_13, 11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class MTLPP_EXPORT RenderPassColorAttachmentDescriptor : public RenderPassAttachmentDescriptor<MTLRenderPassColorAttachmentDescriptor*>
    {
    public:
		RenderPassColorAttachmentDescriptor(ns::Ownership const retain);
		RenderPassColorAttachmentDescriptor();
        RenderPassColorAttachmentDescriptor(MTLRenderPassColorAttachmentDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : RenderPassAttachmentDescriptor<MTLRenderPassColorAttachmentDescriptor*>(handle, retain) { }

        ClearColor GetClearColor() const;

        void SetClearColor(const ClearColor& clearColor);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class MTLPP_EXPORT RenderPassDepthAttachmentDescriptor : public RenderPassAttachmentDescriptor<MTLRenderPassDepthAttachmentDescriptor*>
    {
    public:
        RenderPassDepthAttachmentDescriptor(ns::Ownership const retain);
		RenderPassDepthAttachmentDescriptor();
        RenderPassDepthAttachmentDescriptor(MTLRenderPassDepthAttachmentDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : RenderPassAttachmentDescriptor<MTLRenderPassDepthAttachmentDescriptor*>(handle, retain) { }

        double                        GetClearDepth() const;
        MultisampleDepthResolveFilter GetDepthResolveFilter() const MTLPP_AVAILABLE_AX(9_0);

        void SetClearDepth(double clearDepth);
        void SetDepthResolveFilter(MultisampleDepthResolveFilter depthResolveFilter) MTLPP_AVAILABLE_AX(9_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class MTLPP_EXPORT RenderPassStencilAttachmentDescriptor : public RenderPassAttachmentDescriptor<MTLRenderPassStencilAttachmentDescriptor*>
    {
    public:
        RenderPassStencilAttachmentDescriptor(ns::Ownership const retain);
		RenderPassStencilAttachmentDescriptor();
        RenderPassStencilAttachmentDescriptor(MTLRenderPassStencilAttachmentDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : RenderPassAttachmentDescriptor<MTLRenderPassStencilAttachmentDescriptor*>(handle, retain) { }

        NSUInteger GetClearStencil() const;

        void SetClearStencil(uint32_t clearStencil);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class MTLPP_EXPORT RenderPassDescriptor : public ns::Object<MTLRenderPassDescriptor*>
    {
    public:
        RenderPassDescriptor();
        RenderPassDescriptor(MTLRenderPassDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLRenderPassDescriptor*>(handle, retain) { }

        ns::AutoReleased<ns::Array<RenderPassColorAttachmentDescriptor>> GetColorAttachments() const;
        ns::AutoReleased<RenderPassDepthAttachmentDescriptor>   GetDepthAttachment() const;
        ns::AutoReleased<RenderPassStencilAttachmentDescriptor> GetStencilAttachment() const;
        ns::AutoReleased<Buffer>                                GetVisibilityResultBuffer() const;
        NSUInteger                              GetRenderTargetArrayLength() const MTLPP_AVAILABLE_MAC(10_11);

        void SetDepthAttachment(const RenderPassDepthAttachmentDescriptor& depthAttachment);
        void SetStencilAttachment(const RenderPassStencilAttachmentDescriptor& stencilAttachment);
        void SetVisibilityResultBuffer(const Buffer& visibilityResultBuffer);
        void SetRenderTargetArrayLength(NSUInteger renderTargetArrayLength) MTLPP_AVAILABLE_MAC(10_11);
		
		NSUInteger GetImageblockSampleLength() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger GetThreadgroupMemoryLength() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger GetTileWidth() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger GetTileHeight() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger GetDefaultRasterSampleCount() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger GetRenderTargetWidth() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger GetRenderTargetHeight() const MTLPP_AVAILABLE_IOS(11_0);
		
		void SetImageblockSampleLength(NSUInteger Val) MTLPP_AVAILABLE_IOS(11_0);
		void SetThreadgroupMemoryLength(NSUInteger Val) MTLPP_AVAILABLE_IOS(11_0);
		void SetTileWidth(NSUInteger Val) MTLPP_AVAILABLE_IOS(11_0);
		void SetTileHeight(NSUInteger Val) MTLPP_AVAILABLE_IOS(11_0);
		void SetDefaultRasterSampleCount(NSUInteger Val) MTLPP_AVAILABLE_IOS(11_0);
		void SetRenderTargetWidth(NSUInteger Val) MTLPP_AVAILABLE_IOS(11_0);
		void SetRenderTargetHeight(NSUInteger Val) MTLPP_AVAILABLE_IOS(11_0);
		
		void SetSamplePositions(SamplePosition const* positions, NSUInteger count) MTLPP_AVAILABLE(10_13, 11_0);
		NSUInteger GetSamplePositions(SamplePosition* positions, NSUInteger count) MTLPP_AVAILABLE(10_13, 11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

#include "render_pass.inl"

MTLPP_END
