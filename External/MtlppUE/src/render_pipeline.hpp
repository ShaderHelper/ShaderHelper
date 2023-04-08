/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_RenderPipeline.hpp"
#include "device.hpp"
#include "render_command_encoder.hpp"
#include "render_pass.hpp"
#include "pixel_format.hpp"
#include "argument.hpp"
#include "pipeline.hpp"
#include "function_constant_values.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	struct ITable<id<MTLRenderPipelineState>, void> : public IMPTable<id<MTLRenderPipelineState>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLRenderPipelineState>, void>(C)
		{
		}
	};
	
	template<>
	inline ITable<MTLRenderPipelineColorAttachmentDescriptor*, void>* CreateIMPTable(MTLRenderPipelineColorAttachmentDescriptor* handle)
	{
		static ITable<MTLRenderPipelineColorAttachmentDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLRenderPipelineReflection*, void>* CreateIMPTable(MTLRenderPipelineReflection* handle)
	{
		static ITable<MTLRenderPipelineReflection*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLRenderPipelineDescriptor*, void>* CreateIMPTable(MTLRenderPipelineDescriptor* handle)
	{
		static ITable<MTLRenderPipelineDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLTileRenderPipelineColorAttachmentDescriptor*, void>* CreateIMPTable(MTLTileRenderPipelineColorAttachmentDescriptor* handle)
	{
		static ITable<MTLTileRenderPipelineColorAttachmentDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLTileRenderPipelineDescriptor*, void>* CreateIMPTable(MTLTileRenderPipelineDescriptor* handle)
	{
		static ITable<MTLTileRenderPipelineDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
}

namespace mtlpp
{
	class PipelineBufferDescriptor;
    class VertexDescriptor;

    enum class BlendFactor
    {
        Zero                                                = 0,
        One                                                 = 1,
        SourceColor                                         = 2,
        OneMinusSourceColor                                 = 3,
        SourceAlpha                                         = 4,
        OneMinusSourceAlpha                                 = 5,
        DestinationColor                                    = 6,
        OneMinusDestinationColor                            = 7,
        DestinationAlpha                                    = 8,
        OneMinusDestinationAlpha                            = 9,
        SourceAlphaSaturated                                = 10,
        BlendColor                                          = 11,
        OneMinusBlendColor                                  = 12,
        BlendAlpha                                          = 13,
        OneMinusBlendAlpha                                  = 14,
        Source1Color             MTLPP_AVAILABLE(10_12, 8_0) = 15,
        OneMinusSource1Color     MTLPP_AVAILABLE(10_12, 8_0) = 16,
        Source1Alpha             MTLPP_AVAILABLE(10_12, 8_0) = 17,
        OneMinusSource1Alpha     MTLPP_AVAILABLE(10_12, 8_0) = 18,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class BlendOperation
    {
        Add             = 0,
        Subtract        = 1,
        ReverseSubtract = 2,
        Min             = 3,
        Max             = 4,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum ColorWriteMask
    {
        None  = 0,
        Red   = 0x1 << 3,
        Green = 0x1 << 2,
        Blue  = 0x1 << 1,
        Alpha = 0x1 << 0,
        All   = 0xf
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class PrimitiveTopologyClass
    {
        Unspecified = 0,
        Point       = 1,
        Line        = 2,
        Triangle    = 3,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class TessellationPartitionMode
    {
        ModePow2           = 0,
        ModeInteger        = 1,
        ModeFractionalOdd  = 2,
        ModeFractionalEven = 3,
    }
    MTLPP_AVAILABLE(10_12, 10_0);

    enum class TessellationFactorStepFunction
    {
        Constant               = 0,
        PerPatch               = 1,
        PerInstance            = 2,
        PerPatchAndPerInstance = 3,
    }
    MTLPP_AVAILABLE(10_12, 10_0);

    enum class TessellationFactorFormat
    {
        Half = 0,
    }
    MTLPP_AVAILABLE(10_12, 10_0);

    enum class TessellationControlPointIndexType
    {
        None   = 0,
        UInt16 = 1,
        UInt32 = 2,
    }
    MTLPP_AVAILABLE(10_12, 10_0);

    class MTLPP_EXPORT RenderPipelineColorAttachmentDescriptor : public ns::Object<MTLRenderPipelineColorAttachmentDescriptor*>
    {
    public:
        RenderPipelineColorAttachmentDescriptor();
		RenderPipelineColorAttachmentDescriptor(ns::Ownership const retain) : ns::Object<MTLRenderPipelineColorAttachmentDescriptor*>(retain) {}
        RenderPipelineColorAttachmentDescriptor(MTLRenderPipelineColorAttachmentDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLRenderPipelineColorAttachmentDescriptor*>(handle, retain) { }

        PixelFormat     GetPixelFormat() const;
        bool            IsBlendingEnabled() const;
        BlendFactor     GetSourceRgbBlendFactor() const;
        BlendFactor     GetDestinationRgbBlendFactor() const;
        BlendOperation  GetRgbBlendOperation() const;
        BlendFactor     GetSourceAlphaBlendFactor() const;
        BlendFactor     GetDestinationAlphaBlendFactor() const;
        BlendOperation  GetAlphaBlendOperation() const;
        ColorWriteMask  GetWriteMask() const;

        void SetPixelFormat(PixelFormat pixelFormat);
        void SetBlendingEnabled(bool blendingEnabled);
        void SetSourceRgbBlendFactor(BlendFactor sourceRgbBlendFactor);
        void SetDestinationRgbBlendFactor(BlendFactor destinationRgbBlendFactor);
        void SetRgbBlendOperation(BlendOperation rgbBlendOperation);
        void SetSourceAlphaBlendFactor(BlendFactor sourceAlphaBlendFactor);
        void SetDestinationAlphaBlendFactor(BlendFactor destinationAlphaBlendFactor);
        void SetAlphaBlendOperation(BlendOperation alphaBlendOperation);
        void SetWriteMask(ColorWriteMask writeMask);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
	
	class MTLPP_EXPORT RenderPipelineReflection : public ns::Object<MTLRenderPipelineReflection*>
	{
	public:
		RenderPipelineReflection();
		RenderPipelineReflection(ns::Ownership const retain) : ns::Object<MTLRenderPipelineReflection*>(retain) {}
		RenderPipelineReflection(MTLRenderPipelineReflection* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLRenderPipelineReflection*>(handle, retain) { }
		
		const ns::AutoReleased<ns::Array<Argument>> GetVertexArguments() const;
		const ns::AutoReleased<ns::Array<Argument>> GetFragmentArguments() const;
		const ns::AutoReleased<ns::Array<Argument>> GetTileArguments() const;
	}
	MTLPP_AVAILABLE(10_11, 8_0);
	typedef ns::AutoReleased<RenderPipelineReflection> AutoReleasedRenderPipelineReflection;

    class MTLPP_EXPORT RenderPipelineDescriptor : public ns::Object<MTLRenderPipelineDescriptor*>
    {
    public:
        RenderPipelineDescriptor();
        RenderPipelineDescriptor(MTLRenderPipelineDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLRenderPipelineDescriptor*>(handle, retain) { }

        ns::AutoReleased<ns::String>                                         GetLabel() const;
        ns::AutoReleased<Function>                                           GetVertexFunction() const;
        ns::AutoReleased<Function>                                           GetFragmentFunction() const;
        ns::AutoReleased<VertexDescriptor>                                   GetVertexDescriptor() const;
        NSUInteger                                           GetSampleCount() const;
        bool                                               IsAlphaToCoverageEnabled() const;
        bool                                               IsAlphaToOneEnabled() const;
        bool                                               IsRasterizationEnabled() const;
        ns::AutoReleased<ns::Array<RenderPipelineColorAttachmentDescriptor>> GetColorAttachments() const;
        PixelFormat                                        GetDepthAttachmentPixelFormat() const;
        PixelFormat                                        GetStencilAttachmentPixelFormat() const;
        PrimitiveTopologyClass                             GetInputPrimitiveTopology() const MTLPP_AVAILABLE_MAC(10_11);
        TessellationPartitionMode                          GetTessellationPartitionMode() const MTLPP_AVAILABLE(10_12, 10_0);
        NSUInteger                                           GetMaxTessellationFactor() const MTLPP_AVAILABLE(10_12, 10_0);
        bool                                               IsTessellationFactorScaleEnabled() const MTLPP_AVAILABLE(10_12, 10_0);
        TessellationFactorFormat                           GetTessellationFactorFormat() const MTLPP_AVAILABLE(10_12, 10_0);
        TessellationControlPointIndexType                  GetTessellationControlPointIndexType() const MTLPP_AVAILABLE(10_12, 10_0);
        TessellationFactorStepFunction                     GetTessellationFactorStepFunction() const MTLPP_AVAILABLE(10_12, 10_0);
        Winding                                            GetTessellationOutputWindingOrder() const MTLPP_AVAILABLE(10_12, 10_0);
		
		ns::AutoReleased<ns::Array<PipelineBufferDescriptor>> GetVertexBuffers() const MTLPP_AVAILABLE(10_13, 11_0);
		ns::AutoReleased<ns::Array<PipelineBufferDescriptor>> GetFragmentBuffers() const MTLPP_AVAILABLE(10_13, 11_0);


        void SetLabel(const ns::String& label);
        void SetVertexFunction(const Function& vertexFunction);
        void SetFragmentFunction(const Function& fragmentFunction);
        void SetVertexDescriptor(const VertexDescriptor& vertexDescriptor);
        void SetSampleCount(NSUInteger sampleCount);
        void SetAlphaToCoverageEnabled(bool alphaToCoverageEnabled);
        void SetAlphaToOneEnabled(bool alphaToOneEnabled);
        void SetRasterizationEnabled(bool rasterizationEnabled);
        void SetDepthAttachmentPixelFormat(PixelFormat depthAttachmentPixelFormat);
        void SetStencilAttachmentPixelFormat(PixelFormat stencilAttachmentPixelFormat);
        void SetInputPrimitiveTopology(PrimitiveTopologyClass inputPrimitiveTopology) MTLPP_AVAILABLE_MAC(10_11);
        void SetTessellationPartitionMode(TessellationPartitionMode tessellationPartitionMode) MTLPP_AVAILABLE(10_12, 10_0);
        void SetMaxTessellationFactor(NSUInteger maxTessellationFactor) MTLPP_AVAILABLE(10_12, 10_0);
        void SetTessellationFactorScaleEnabled(bool tessellationFactorScaleEnabled) MTLPP_AVAILABLE(10_12, 10_0);
        void SetTessellationFactorFormat(TessellationFactorFormat tessellationFactorFormat) MTLPP_AVAILABLE(10_12, 10_0);
        void SetTessellationControlPointIndexType(TessellationControlPointIndexType tessellationControlPointIndexType) MTLPP_AVAILABLE(10_12, 10_0);
        void SetTessellationFactorStepFunction(TessellationFactorStepFunction tessellationFactorStepFunction) MTLPP_AVAILABLE(10_12, 10_0);
        void SetTessellationOutputWindingOrder(Winding tessellationOutputWindingOrder) MTLPP_AVAILABLE(10_12, 10_0);
		
        void Reset();
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class MTLPP_EXPORT RenderPipelineState : public ns::Object<ns::Protocol<id<MTLRenderPipelineState>>::type>
    {
    public:
        RenderPipelineState() { }
		RenderPipelineState(ns::Protocol<id<MTLRenderPipelineState>>::type handle, UE::ITableCache* cache = nullptr, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<ns::Protocol<id<MTLRenderPipelineState>>::type>(handle, retain, UE::ITableCacheRef(cache).GetRenderPipelineState(handle)) { }

        ns::AutoReleased<ns::String> GetLabel() const;
        ns::AutoReleased<Device>     GetDevice() const;
		
		NSUInteger GetMaxTotalThreadsPerThreadgroup() const MTLPP_AVAILABLE_IOS(11_0);
		bool GetThreadgroupSizeMatchesTileSize() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger GetImageblockSampleLength() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger GetImageblockMemoryLengthForDimensions(Size const& imageblockDimensions) const MTLPP_AVAILABLE_IOS(11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
	
	class MTLPP_EXPORT TileRenderPipelineColorAttachmentDescriptor : public ns::Object<MTLTileRenderPipelineColorAttachmentDescriptor*>
	{
	public:
		TileRenderPipelineColorAttachmentDescriptor();
		TileRenderPipelineColorAttachmentDescriptor(ns::Ownership const retain) : ns::Object<MTLTileRenderPipelineColorAttachmentDescriptor*>(retain) {}
		TileRenderPipelineColorAttachmentDescriptor(MTLTileRenderPipelineColorAttachmentDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLTileRenderPipelineColorAttachmentDescriptor*>(handle, retain) { }
		
		PixelFormat     GetPixelFormat() const MTLPP_AVAILABLE_IOS(11_0);
		
		void SetPixelFormat(PixelFormat pixelFormat) MTLPP_AVAILABLE_IOS(11_0);
	};
	
	class MTLPP_EXPORT TileRenderPipelineDescriptor : public ns::Object<MTLTileRenderPipelineDescriptor*>
	{
	public:
		TileRenderPipelineDescriptor() MTLPP_AVAILABLE_IOS(11_0);
		TileRenderPipelineDescriptor(MTLTileRenderPipelineDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) MTLPP_AVAILABLE_IOS(11_0) : ns::Object<MTLTileRenderPipelineDescriptor*>(handle, retain) { }
		
		ns::AutoReleased<ns::String>                                         GetLabel() const MTLPP_AVAILABLE_IOS(11_0);
		ns::AutoReleased<Function>                                           GetTileFunction() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger                                           GetRasterSampleCount() const MTLPP_AVAILABLE_IOS(11_0);
		ns::AutoReleased<ns::Array<TileRenderPipelineColorAttachmentDescriptor>> GetColorAttachments() const MTLPP_AVAILABLE_IOS(11_0);
		bool                                        GetThreadgroupSizeMatchesTileSize() const MTLPP_AVAILABLE_IOS(11_0);
		ns::AutoReleased<ns::Array<PipelineBufferDescriptor>> GetTileBuffers() const MTLPP_AVAILABLE_IOS(11_0);
		
		
		void SetLabel(const ns::String& label) MTLPP_AVAILABLE_IOS(11_0);
		void SetTileFunction(const Function& tileFunction) MTLPP_AVAILABLE_IOS(11_0);
		void SetRasterSampleCount(NSUInteger sampleCount) MTLPP_AVAILABLE_IOS(11_0);
		void SetThreadgroupSizeMatchesTileSize(bool threadgroupSizeMatchesTileSize) MTLPP_AVAILABLE_IOS(11_0);
		
		void Reset() MTLPP_AVAILABLE_IOS(11_0);
	}
	MTLPP_AVAILABLE_IOS(11_0);
}

MTLPP_END
