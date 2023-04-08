/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLRenderPipeline.h>
#include <Metal/MTLSampler.h>
#include <Metal/MTLStageInputOutputDescriptor.h>
#include <Metal/MTLVertexDescriptor.h>

#include "render_pipeline.hpp"
#include "vertex_descriptor.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    RenderPipelineColorAttachmentDescriptor::RenderPipelineColorAttachmentDescriptor() :
        ns::Object<MTLRenderPipelineColorAttachmentDescriptor*>([[MTLRenderPipelineColorAttachmentDescriptor alloc] init], ns::Ownership::Assign)
    {
    }

    PixelFormat RenderPipelineColorAttachmentDescriptor::GetPixelFormat() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return PixelFormat(m_table->pixelFormat(m_ptr));
#else
        return PixelFormat([(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr pixelFormat]);
#endif
    }

    bool RenderPipelineColorAttachmentDescriptor::IsBlendingEnabled() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->isBlendingEnabled(m_ptr);
#else
        return [(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr isBlendingEnabled];
#endif
    }

    BlendFactor RenderPipelineColorAttachmentDescriptor::GetSourceRgbBlendFactor() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return BlendFactor(m_table->sourceRGBBlendFactor(m_ptr));
#else
        return BlendFactor([(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr sourceRGBBlendFactor]);
#endif
    }

    BlendFactor RenderPipelineColorAttachmentDescriptor::GetDestinationRgbBlendFactor() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return BlendFactor(m_table->destinationRGBBlendFactor(m_ptr));
#else
        return BlendFactor([(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr destinationRGBBlendFactor]);
#endif
    }

    BlendOperation RenderPipelineColorAttachmentDescriptor::GetRgbBlendOperation() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return BlendOperation(m_table->rgbBlendOperation(m_ptr));
#else
        return BlendOperation([(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr rgbBlendOperation]);
#endif
    }

    BlendFactor RenderPipelineColorAttachmentDescriptor::GetSourceAlphaBlendFactor() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return BlendFactor(m_table->sourceAlphaBlendFactor(m_ptr));
#else
        return BlendFactor([(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr sourceAlphaBlendFactor]);
#endif
    }

    BlendFactor RenderPipelineColorAttachmentDescriptor::GetDestinationAlphaBlendFactor() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return BlendFactor(m_table->destinationAlphaBlendFactor(m_ptr));
#else
        return BlendFactor([(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr destinationAlphaBlendFactor]);
#endif
    }

    BlendOperation RenderPipelineColorAttachmentDescriptor::GetAlphaBlendOperation() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return BlendOperation(m_table->alphaBlendOperation(m_ptr));
#else
        return BlendOperation([(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr alphaBlendOperation]);
#endif
    }

    ColorWriteMask RenderPipelineColorAttachmentDescriptor::GetWriteMask() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return ColorWriteMask(m_table->writeMask(m_ptr));
#else
        return ColorWriteMask([(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr writeMask]);
#endif
    }

    void RenderPipelineColorAttachmentDescriptor::SetPixelFormat(PixelFormat pixelFormat)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setPixelFormat(m_ptr, MTLPixelFormat(pixelFormat));
#else
        [(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr setPixelFormat:MTLPixelFormat(pixelFormat)];
#endif
    }

    void RenderPipelineColorAttachmentDescriptor::SetBlendingEnabled(bool blendingEnabled)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setBlendingEnabled(m_ptr, blendingEnabled);
#else
        [(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr setBlendingEnabled:blendingEnabled];
#endif
    }

    void RenderPipelineColorAttachmentDescriptor::SetSourceRgbBlendFactor(BlendFactor sourceRgbBlendFactor)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setSourceRGBBlendFactor(m_ptr, MTLBlendFactor(sourceRgbBlendFactor));
#else
        [(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr setSourceRGBBlendFactor:MTLBlendFactor(sourceRgbBlendFactor)];
#endif
    }

    void RenderPipelineColorAttachmentDescriptor::SetDestinationRgbBlendFactor(BlendFactor destinationRgbBlendFactor)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setDestinationRGBBlendFactor(m_ptr, MTLBlendFactor(destinationRgbBlendFactor));
#else
        [(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr setDestinationRGBBlendFactor:MTLBlendFactor(destinationRgbBlendFactor)];
#endif
    }

    void RenderPipelineColorAttachmentDescriptor::SetRgbBlendOperation(BlendOperation rgbBlendOperation)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setRgbBlendOperation(m_ptr, MTLBlendOperation(rgbBlendOperation));
#else
        [(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr setRgbBlendOperation:MTLBlendOperation(rgbBlendOperation)];
#endif
    }

    void RenderPipelineColorAttachmentDescriptor::SetSourceAlphaBlendFactor(BlendFactor sourceAlphaBlendFactor)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setSourceAlphaBlendFactor(m_ptr, MTLBlendFactor(sourceAlphaBlendFactor));
#else
        [(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr setSourceAlphaBlendFactor:MTLBlendFactor(sourceAlphaBlendFactor)];
#endif
    }

    void RenderPipelineColorAttachmentDescriptor::SetDestinationAlphaBlendFactor(BlendFactor destinationAlphaBlendFactor)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setDestinationAlphaBlendFactor(m_ptr, MTLBlendFactor(destinationAlphaBlendFactor));
#else
        [(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr setDestinationAlphaBlendFactor:MTLBlendFactor(destinationAlphaBlendFactor)];
#endif
    }

    void RenderPipelineColorAttachmentDescriptor::SetAlphaBlendOperation(BlendOperation alphaBlendOperation)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setAlphaBlendOperation(m_ptr, MTLBlendOperation(alphaBlendOperation));
#else
        [(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr setAlphaBlendOperation:MTLBlendOperation(alphaBlendOperation)];
#endif
    }

    void RenderPipelineColorAttachmentDescriptor::SetWriteMask(ColorWriteMask writeMask)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setWriteMask(m_ptr, MTLColorWriteMask(writeMask));
#else
        [(MTLRenderPipelineColorAttachmentDescriptor*)m_ptr setWriteMask:MTLColorWriteMask(writeMask)];
#endif
    }
	
	RenderPipelineReflection::RenderPipelineReflection() :
	ns::Object<MTLRenderPipelineReflection*>([[MTLRenderPipelineReflection alloc] init], ns::Ownership::Assign)
	{
	}
	
	const ns::AutoReleased<ns::Array<Argument>> RenderPipelineReflection::GetVertexArguments() const
	{
		Validate();
		return ns::AutoReleased<ns::Array<Argument>>([(MTLRenderPipelineReflection*)m_ptr vertexArguments]);
	}
	
	const ns::AutoReleased<ns::Array<Argument>> RenderPipelineReflection::GetFragmentArguments() const
	{
		Validate();
		return ns::AutoReleased<ns::Array<Argument>>([(MTLRenderPipelineReflection*)m_ptr fragmentArguments]);
	}
	
	const ns::AutoReleased<ns::Array<Argument>> RenderPipelineReflection::GetTileArguments() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return ns::AutoReleased<ns::Array<Argument>>([(MTLRenderPipelineReflection*)m_ptr tileArguments]);
#else
		return ns::AutoReleased<ns::Array<Argument>>();
#endif
	}

    RenderPipelineDescriptor::RenderPipelineDescriptor() :
        ns::Object<MTLRenderPipelineDescriptor*>([[MTLRenderPipelineDescriptor alloc] init], ns::Ownership::Assign)
    {
    }

    ns::AutoReleased<ns::String> RenderPipelineDescriptor::GetLabel() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<ns::String>(m_table->label(m_ptr));
#else
        return ns::AutoReleased<ns::String>([(MTLRenderPipelineDescriptor*)m_ptr label]);
#endif
    }

    ns::AutoReleased<Function> RenderPipelineDescriptor::GetVertexFunction() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<Function>(m_table->vertexFunction(m_ptr));
#else
        return ns::AutoReleased<Function>([(MTLRenderPipelineDescriptor*)m_ptr vertexFunction]);
#endif
    }

    ns::AutoReleased<Function> RenderPipelineDescriptor::GetFragmentFunction() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<Function>(m_table->fragmentFunction(m_ptr));
#else
		return ns::AutoReleased<Function>([(MTLRenderPipelineDescriptor*)m_ptr fragmentFunction]);
#endif
    }

    ns::AutoReleased<VertexDescriptor> RenderPipelineDescriptor::GetVertexDescriptor() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<VertexDescriptor>(m_table->vertexDescriptor(m_ptr));
#else
		return ns::AutoReleased<VertexDescriptor>([(MTLRenderPipelineDescriptor*)m_ptr vertexDescriptor]);
#endif
    }

    NSUInteger RenderPipelineDescriptor::GetSampleCount() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(m_table->sampleCount(m_ptr));
#else
        return NSUInteger([(MTLRenderPipelineDescriptor*)m_ptr sampleCount]);
#endif
    }

    bool RenderPipelineDescriptor::IsAlphaToCoverageEnabled() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->isAlphaToCoverageEnabled(m_ptr);
#else
        return [(MTLRenderPipelineDescriptor*)m_ptr isAlphaToCoverageEnabled];
#endif
    }

    bool RenderPipelineDescriptor::IsAlphaToOneEnabled() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->isAlphaToOneEnabled(m_ptr);
#else
        return [(MTLRenderPipelineDescriptor*)m_ptr isAlphaToOneEnabled];
#endif
    }

    bool RenderPipelineDescriptor::IsRasterizationEnabled() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->isRasterizationEnabled(m_ptr);
#else
        return [(MTLRenderPipelineDescriptor*)m_ptr isRasterizationEnabled];
#endif
    }

    ns::AutoReleased<ns::Array<RenderPipelineColorAttachmentDescriptor>> RenderPipelineDescriptor::GetColorAttachments() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<ns::Array<RenderPipelineColorAttachmentDescriptor>>((NSArray<RenderPipelineColorAttachmentDescriptor::Type>*)m_table->colorAttachments(m_ptr));
#else
        return ns::AutoReleased<ns::Array<RenderPipelineColorAttachmentDescriptor>>((NSArray*)[(MTLRenderPipelineDescriptor*)m_ptr colorAttachments]);
#endif
    }

    PixelFormat RenderPipelineDescriptor::GetDepthAttachmentPixelFormat() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return PixelFormat(m_table->depthAttachmentPixelFormat(m_ptr));
#else
        return PixelFormat([(MTLRenderPipelineDescriptor*)m_ptr depthAttachmentPixelFormat]);
#endif
    }

    PixelFormat RenderPipelineDescriptor::GetStencilAttachmentPixelFormat() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return PixelFormat(m_table->stencilAttachmentPixelFormat(m_ptr));
#else
        return PixelFormat([(MTLRenderPipelineDescriptor*)m_ptr stencilAttachmentPixelFormat]);
#endif
    }

    PrimitiveTopologyClass RenderPipelineDescriptor::GetInputPrimitiveTopology() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_12)
#if MTLPP_CONFIG_IMP_CACHE
        return PrimitiveTopologyClass(m_table->inputPrimitiveTopology(m_ptr));
#else
        return PrimitiveTopologyClass([(MTLRenderPipelineDescriptor*)m_ptr inputPrimitiveTopology]);
#endif
#else
        return PrimitiveTopologyClass(0);
#endif
    }

    TessellationPartitionMode RenderPipelineDescriptor::GetTessellationPartitionMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return TessellationPartitionMode(m_table->tessellationPartitionMode(m_ptr));
#else
        return TessellationPartitionMode([(MTLRenderPipelineDescriptor*)m_ptr tessellationPartitionMode]);
#endif
#else
        return TessellationPartitionMode(0);
#endif
    }

    NSUInteger RenderPipelineDescriptor::GetMaxTessellationFactor() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(m_table->maxTessellationFactor(m_ptr));
#else
        return NSUInteger([(MTLRenderPipelineDescriptor*)m_ptr maxTessellationFactor]);
#endif
#else
        return 0;
#endif
    }

    bool RenderPipelineDescriptor::IsTessellationFactorScaleEnabled() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return m_table->isTessellationFactorScaleEnabled(m_ptr);
#else
        return [(MTLRenderPipelineDescriptor*)m_ptr isTessellationFactorScaleEnabled];
#endif
#else
        return false;
#endif
    }

    TessellationFactorFormat RenderPipelineDescriptor::GetTessellationFactorFormat() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return TessellationFactorFormat(m_table->tessellationFactorFormat(m_ptr));
#else
        return TessellationFactorFormat([(MTLRenderPipelineDescriptor*)m_ptr tessellationFactorFormat]);
#endif
#else
        return TessellationFactorFormat(0);
#endif
    }

    TessellationControlPointIndexType RenderPipelineDescriptor::GetTessellationControlPointIndexType() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return TessellationControlPointIndexType(m_table->tessellationControlPointIndexType(m_ptr));
#else
        return TessellationControlPointIndexType([(MTLRenderPipelineDescriptor*)m_ptr tessellationControlPointIndexType]);
#endif
#else
        return TessellationControlPointIndexType(0);
#endif
    }

    TessellationFactorStepFunction RenderPipelineDescriptor::GetTessellationFactorStepFunction() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return TessellationFactorStepFunction(m_table->tessellationFactorStepFunction(m_ptr));
#else
        return TessellationFactorStepFunction([(MTLRenderPipelineDescriptor*)m_ptr tessellationFactorStepFunction]);
#endif
#else
        return TessellationFactorStepFunction(0);
#endif
    }

    Winding RenderPipelineDescriptor::GetTessellationOutputWindingOrder() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return Winding(m_table->tessellationOutputWindingOrder(m_ptr));
#else
        return Winding([(MTLRenderPipelineDescriptor*)m_ptr tessellationOutputWindingOrder]);
#endif
#else
        return Winding(0);
#endif
    }
	
	ns::AutoReleased<ns::Array<PipelineBufferDescriptor>> RenderPipelineDescriptor::GetVertexBuffers() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::Array<PipelineBufferDescriptor>>((NSArray<MTLPipelineBufferDescriptor*>*)m_table->vertexBuffers(m_ptr));
#else
		return ns::AutoReleased<ns::Array<PipelineBufferDescriptor>>((NSArray<MTLPipelineBufferDescriptor*>*)[(MTLRenderPipelineDescriptor*)m_ptr vertexBuffers]);
#endif
#else
		return ns::AutoReleased<ns::Array<PipelineBufferDescriptor>>>();
#endif
	}
	
	ns::AutoReleased<ns::Array<PipelineBufferDescriptor>> RenderPipelineDescriptor::GetFragmentBuffers() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::Array<PipelineBufferDescriptor>>((NSArray<MTLPipelineBufferDescriptor*>*)m_table->fragmentBuffers(m_ptr));
#else
		return ns::AutoReleased<ns::Array<PipelineBufferDescriptor>>((NSArray<MTLPipelineBufferDescriptor*>*)[(MTLRenderPipelineDescriptor*)m_ptr fragmentBuffers]);
#endif
#else
		return ns::AutoReleased<ns::Array<PipelineBufferDescriptor>>();
#endif
	}

    void RenderPipelineDescriptor::SetLabel(const ns::String& label)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setLabel(m_ptr, (NSString*)label.GetPtr());
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
    }

    void RenderPipelineDescriptor::SetVertexFunction(const Function& vertexFunction)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setVertexFunction(m_ptr, (id<MTLFunction>)vertexFunction.GetPtr());
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setVertexFunction:(id<MTLFunction>)vertexFunction.GetPtr()];
#endif
    }

    void RenderPipelineDescriptor::SetFragmentFunction(const Function& fragmentFunction)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setFragmentFunction(m_ptr, (id<MTLFunction>)fragmentFunction.GetPtr());
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setFragmentFunction:(id<MTLFunction>)fragmentFunction.GetPtr()];
#endif
    }

    void RenderPipelineDescriptor::SetVertexDescriptor(const VertexDescriptor& vertexDescriptor)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setVertexDescriptor(m_ptr, (MTLVertexDescriptor*)vertexDescriptor.GetPtr());
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setVertexDescriptor:(MTLVertexDescriptor*)vertexDescriptor.GetPtr()];
#endif
    }

    void RenderPipelineDescriptor::SetSampleCount(NSUInteger sampleCount)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setSampleCount(m_ptr, sampleCount);
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setSampleCount:sampleCount];
#endif
    }

    void RenderPipelineDescriptor::SetAlphaToCoverageEnabled(bool alphaToCoverageEnabled)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setAlphaToCoverageEnabled(m_ptr, alphaToCoverageEnabled);
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setAlphaToCoverageEnabled:alphaToCoverageEnabled];
#endif
    }

    void RenderPipelineDescriptor::SetAlphaToOneEnabled(bool alphaToOneEnabled)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setAlphaToOneEnabled(m_ptr, alphaToOneEnabled);
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setAlphaToOneEnabled:alphaToOneEnabled];
#endif
    }

    void RenderPipelineDescriptor::SetRasterizationEnabled(bool rasterizationEnabled)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setRasterizationEnabled(m_ptr, rasterizationEnabled);
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setRasterizationEnabled:rasterizationEnabled];
#endif
    }

    void RenderPipelineDescriptor::SetDepthAttachmentPixelFormat(PixelFormat depthAttachmentPixelFormat)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setDepthAttachmentPixelFormat(m_ptr, MTLPixelFormat(depthAttachmentPixelFormat));
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setDepthAttachmentPixelFormat:MTLPixelFormat(depthAttachmentPixelFormat)];
#endif
    }

    void RenderPipelineDescriptor::SetStencilAttachmentPixelFormat(PixelFormat depthAttachmentPixelFormat)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setStencilAttachmentPixelFormat(m_ptr, MTLPixelFormat(depthAttachmentPixelFormat));
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setStencilAttachmentPixelFormat:MTLPixelFormat(depthAttachmentPixelFormat)];
#endif
    }

    void RenderPipelineDescriptor::SetInputPrimitiveTopology(PrimitiveTopologyClass inputPrimitiveTopology)
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_12)
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setInputPrimitiveTopology(m_ptr, MTLPrimitiveTopologyClass(inputPrimitiveTopology));
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setInputPrimitiveTopology:MTLPrimitiveTopologyClass(inputPrimitiveTopology)];
#endif
#endif
    }

    void RenderPipelineDescriptor::SetTessellationPartitionMode(TessellationPartitionMode tessellationPartitionMode)
    {
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setTessellationPartitionMode(m_ptr, MTLTessellationPartitionMode(tessellationPartitionMode));
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setTessellationPartitionMode:MTLTessellationPartitionMode(tessellationPartitionMode)];
#endif
#endif
    }

    void RenderPipelineDescriptor::SetMaxTessellationFactor(NSUInteger maxTessellationFactor)
    {
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setMaxTessellationFactor(m_ptr, maxTessellationFactor);
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setMaxTessellationFactor:maxTessellationFactor];
#endif
#endif
    }

    void RenderPipelineDescriptor::SetTessellationFactorScaleEnabled(bool tessellationFactorScaleEnabled)
    {
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setTessellationFactorScaleEnabled(m_ptr, tessellationFactorScaleEnabled);
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setTessellationFactorScaleEnabled:tessellationFactorScaleEnabled];
#endif
#endif
    }

    void RenderPipelineDescriptor::SetTessellationFactorFormat(TessellationFactorFormat tessellationFactorFormat)
    {
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setTessellationFactorFormat(m_ptr, MTLTessellationFactorFormat(tessellationFactorFormat));
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setTessellationFactorFormat:MTLTessellationFactorFormat(tessellationFactorFormat)];
#endif
#endif
    }

    void RenderPipelineDescriptor::SetTessellationControlPointIndexType(TessellationControlPointIndexType tessellationControlPointIndexType)
    {
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setTessellationControlPointIndexType(m_ptr, MTLTessellationControlPointIndexType(tessellationControlPointIndexType));
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setTessellationControlPointIndexType:MTLTessellationControlPointIndexType(tessellationControlPointIndexType)];
#endif
#endif
    }

    void RenderPipelineDescriptor::SetTessellationFactorStepFunction(TessellationFactorStepFunction tessellationFactorStepFunction)
    {
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setTessellationFactorStepFunction(m_ptr, MTLTessellationFactorStepFunction(tessellationFactorStepFunction));
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setTessellationFactorStepFunction:MTLTessellationFactorStepFunction(tessellationFactorStepFunction)];
#endif
#endif
    }

    void RenderPipelineDescriptor::SetTessellationOutputWindingOrder(Winding tessellationOutputWindingOrder)
    {
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        m_table->setTessellationOutputWindingOrder(m_ptr, MTLWinding(tessellationOutputWindingOrder));
#else
        [(MTLRenderPipelineDescriptor*)m_ptr setTessellationOutputWindingOrder:MTLWinding(tessellationOutputWindingOrder)];
#endif
#endif
    }

    void RenderPipelineDescriptor::Reset()
    {
#if MTLPP_CONFIG_IMP_CACHE
		m_table->reset(m_ptr);
#else
        [(MTLRenderPipelineDescriptor*)m_ptr reset];
#endif
    }

    ns::AutoReleased<ns::String> RenderPipelineState::GetLabel() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::String>(m_table->Label(m_ptr));
#else
        return ns::AutoReleased<ns::String>([(id<MTLRenderPipelineState>)m_ptr label]);
#endif
    }

    ns::AutoReleased<Device> RenderPipelineState::GetDevice() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<Device>(m_table->Device(m_ptr));
#else
		return ns::AutoReleased<Device>([(id<MTLRenderPipelineState>)m_ptr device]);
#endif
    }
	
	NSUInteger RenderPipelineState::GetMaxTotalThreadsPerThreadgroup() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->maxTotalThreadsPerThreadgroup(m_ptr);
#else
		return [m_ptr maxTotalThreadsPerThreadgroup];
#endif
#else
		return 0;
#endif
	}
	
	bool RenderPipelineState::GetThreadgroupSizeMatchesTileSize() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->threadgroupSizeMatchesTileSize(m_ptr);
#else
		return [m_ptr threadgroupSizeMatchesTileSize];
#endif
#else
		return false;
#endif
	}
	
	NSUInteger RenderPipelineState::GetImageblockSampleLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->imageblockSampleLength(m_ptr);
#else
		return [m_ptr imageblockSampleLength];
#endif
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPipelineState::GetImageblockMemoryLengthForDimensions(Size const& imageblockDimensions) const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->imageblockMemoryLengthForDimensions(m_ptr, imageblockDimensions);
#else
		return [m_ptr imageblockMemoryLengthForDimensions:MTLSizeMake(imageblockDimensions.width, imageblockDimensions.height, imageblockDimensions.depth)];
#endif
#else
		return 0;
#endif
	}
	
	PixelFormat TileRenderPipelineColorAttachmentDescriptor::GetPixelFormat() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return (PixelFormat)[m_ptr pixelFormat];
#else
		return (PixelFormat)0;
#endif
	}
	
	void TileRenderPipelineColorAttachmentDescriptor::SetPixelFormat(PixelFormat pixelFormat)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[m_ptr setPixelFormat:(MTLPixelFormat)pixelFormat];
#endif
	}
	
	TileRenderPipelineDescriptor::TileRenderPipelineDescriptor()
#if MTLPP_IS_AVAILABLE_IOS(11_0)
	: ns::Object<MTLTileRenderPipelineDescriptor*>([[MTLTileRenderPipelineDescriptor alloc] init], ns::Ownership::Assign)
#endif
	{
	}
	
	ns::AutoReleased<ns::String> TileRenderPipelineDescriptor::GetLabel() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return ns::AutoReleased<ns::String>([m_ptr label]);
#else
		return ns::AutoReleased<ns::String>();
#endif
	}
	
	ns::AutoReleased<Function> TileRenderPipelineDescriptor::GetTileFunction() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return ns::AutoReleased<Function>([m_ptr tileFunction]);
#else
		return ns::AutoReleased<Function>();
#endif
	}
	
	NSUInteger TileRenderPipelineDescriptor::GetRasterSampleCount() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [m_ptr rasterSampleCount];
#else
		return 0;
#endif
	}
	
	ns::AutoReleased<ns::Array<TileRenderPipelineColorAttachmentDescriptor>> TileRenderPipelineDescriptor::GetColorAttachments() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return ns::AutoReleased<ns::Array<TileRenderPipelineColorAttachmentDescriptor>>((NSArray<MTLTileRenderPipelineColorAttachmentDescriptor*>*)[m_ptr colorAttachments]);
#else
		return ns::AutoReleased<ns::Array<TileRenderPipelineColorAttachmentDescriptor>>();
#endif
	}
	
	bool TileRenderPipelineDescriptor::GetThreadgroupSizeMatchesTileSize() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [m_ptr threadgroupSizeMatchesTileSize];
#else
		return false;
#endif
	}
	
	ns::AutoReleased<ns::Array<PipelineBufferDescriptor>> TileRenderPipelineDescriptor::GetTileBuffers() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return ns::AutoReleased<ns::Array<PipelineBufferDescriptor>>((NSArray<MTLPipelineBufferDescriptor*>*)[m_ptr tileBuffers]);
#else
		return ns::AutoReleased<ns::Array<PipelineBufferDescriptor>>();
#endif
	}
	
	void TileRenderPipelineDescriptor::SetLabel(const ns::String& label)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[m_ptr setLabel:label.GetPtr()];
#endif
	}
	
	void TileRenderPipelineDescriptor::SetTileFunction(const Function& tileFunction)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[m_ptr setTileFunction:tileFunction.GetPtr()];
#endif
	}
	
	void TileRenderPipelineDescriptor::SetRasterSampleCount(NSUInteger sampleCount)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[m_ptr setRasterSampleCount:sampleCount];
#endif
	}
	
	void TileRenderPipelineDescriptor::SetThreadgroupSizeMatchesTileSize(bool threadgroupSizeMatchesTileSize)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[m_ptr setThreadgroupSizeMatchesTileSize:threadgroupSizeMatchesTileSize];
#endif
	}
	
	void TileRenderPipelineDescriptor::Reset()
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[m_ptr reset];
#endif
	}
}

MTLPP_END
