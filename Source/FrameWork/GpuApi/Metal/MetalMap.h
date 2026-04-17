#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"
#include "MetalTexture.h"
#include "MetalDevice.h"
namespace FW
{
    inline MTLColorWriteMask MapWriteMask(BlendMask InMask)
    {
        MTLColorWriteMask Mask = 0;
        Mask |= (bool)(InMask & BlendMask::R) ? MTLColorWriteMaskRed : 0;
        Mask |= (bool)(InMask & BlendMask::G) ? MTLColorWriteMaskGreen : 0;
        Mask |= (bool)(InMask & BlendMask::B) ? MTLColorWriteMaskBlue : 0;
        Mask |= (bool)(InMask & BlendMask::A) ? MTLColorWriteMaskAlpha : 0;
        return Mask;
    }

    inline MTLBlendFactor MapBlendFactor(BlendFactor InFactor)
    {
        switch (InFactor)
        {
        case BlendFactor::Zero:            return MTLBlendFactorZero;
        case BlendFactor::One:             return MTLBlendFactorOne;
        case BlendFactor::SrcColor:        return MTLBlendFactorSourceColor;
        case BlendFactor::InvSrcColor:     return MTLBlendFactorOneMinusSourceColor;
        case BlendFactor::DestColor:       return MTLBlendFactorDestinationColor;
        case BlendFactor::InvDestColor:    return MTLBlendFactorOneMinusDestinationColor;
        case BlendFactor::SrcAlpha:        return MTLBlendFactorSourceAlpha;
        case BlendFactor::InvSrcAlpha:     return MTLBlendFactorOneMinusSourceAlpha;
        case BlendFactor::DestAlpha:       return MTLBlendFactorDestinationAlpha;
        case BlendFactor::InvDestAlpha:    return MTLBlendFactorOneMinusDestinationAlpha;
        default:
			AUX::Unreachable();
        }
    }

    inline MTLBlendOperation MapBlendOp(BlendOp InOp)
    {
        switch (InOp)
        {
        case BlendOp::Add:            return MTLBlendOperationAdd;
        case BlendOp::Substract:      return MTLBlendOperationSubtract;
        case BlendOp::Min:            return MTLBlendOperationMin;
        case BlendOp::Max:            return MTLBlendOperationMax;
        default:
			AUX::Unreachable();
        }
    }

    inline MTLLoadAction MapLoadAction(RenderTargetLoadAction InLoadAction)
    {
        switch(InLoadAction)
        {
        case RenderTargetLoadAction::DontCare:      return MTLLoadActionDontCare;
        case RenderTargetLoadAction::Load:          return MTLLoadActionLoad;
        case RenderTargetLoadAction::Clear:         return MTLLoadActionClear;
        default:
			AUX::Unreachable();
        }
    }

    inline MTLStoreAction MapStoreAction(RenderTargetStoreAction InStoreAction)
    {
        switch(InStoreAction)
        {
        case RenderTargetStoreAction::DontCare:      return MTLStoreActionDontCare;
        case RenderTargetStoreAction::Store:         return MTLStoreActionStore;
        default:
			AUX::Unreachable();
        }
    }
    
    inline MTLRenderPassDescriptor* MapRenderPassDesc(const GpuRenderPassDesc& PassDesc)
    {
        MTLRenderPassDescriptor* RawPassDesc = [MTLRenderPassDescriptor new];
        uint32 PassRtNum = PassDesc.ColorRenderTargets.Num();
        for(uint32 i = 0 ; i < PassRtNum; i++)
        {
            const GpuRenderTargetInfo& RtInfo = PassDesc.ColorRenderTargets[i];
            MetalTextureView* RtView = static_cast<MetalTextureView*>(RtInfo.View);
            RawPassDesc.colorAttachments[i].texture = (id<MTLTexture>)RtView->GetResource();
            RawPassDesc.colorAttachments[i].loadAction = MapLoadAction(RtInfo.LoadAction);
            if (RtInfo.ResolveTarget)
            {
                MetalTextureView* ResolveView = static_cast<MetalTextureView*>(RtInfo.ResolveTarget);
                RawPassDesc.colorAttachments[i].resolveTexture = (id<MTLTexture>)ResolveView->GetResource();
                RawPassDesc.colorAttachments[i].storeAction = RtInfo.StoreAction == RenderTargetStoreAction::Store
                    ? MTLStoreActionStoreAndMultisampleResolve
                    : MTLStoreActionMultisampleResolve;
            }
            else
            {
                RawPassDesc.colorAttachments[i].storeAction = MapStoreAction(RtInfo.StoreAction);
            }
            RawPassDesc.colorAttachments[i].clearColor = MTLClearColorMake(RtInfo.ClearColor.x, RtInfo.ClearColor.y, RtInfo.ClearColor.z, RtInfo.ClearColor.w);
        }

        if (PassDesc.DepthStencilTarget.IsSet())
        {
            const GpuDepthStencilTargetInfo& DepthInfo = *PassDesc.DepthStencilTarget;
            MetalTextureView* DsView = static_cast<MetalTextureView*>(DepthInfo.View);
            RawPassDesc.depthAttachment.texture = (id<MTLTexture>)DsView->GetResource();
            RawPassDesc.depthAttachment.loadAction = MapLoadAction(DepthInfo.LoadAction);
            RawPassDesc.depthAttachment.storeAction = MapStoreAction(DepthInfo.StoreAction);
            RawPassDesc.depthAttachment.clearDepth = DepthInfo.ClearDepth;
        }

        if (PassDesc.TimestampWrites && GSupportStageBoundaryCounter)
        {
            const GpuRenderPassTimestampWrites& TsWrites = *PassDesc.TimestampWrites;
            MetalQuerySet* MtlQuerySet = static_cast<MetalQuerySet*>(TsWrites.QuerySet);
            RawPassDesc.sampleBufferAttachments[0].sampleBuffer = (id<MTLCounterSampleBuffer>)MtlQuerySet->GetSampleBuffer();
            RawPassDesc.sampleBufferAttachments[0].startOfVertexSampleIndex = TsWrites.BeginningOfPassWriteIndex;
            RawPassDesc.sampleBufferAttachments[0].endOfFragmentSampleIndex = TsWrites.EndOfPassWriteIndex;
            RawPassDesc.sampleBufferAttachments[0].startOfFragmentSampleIndex = MTLCounterDontSample;
            RawPassDesc.sampleBufferAttachments[0].endOfVertexSampleIndex = MTLCounterDontSample;
        }

        return [RawPassDesc autorelease];
    }

    inline MTLPixelFormat MapTextureFormat(const GpuFormat& InTextureFormat)
    {
        switch(InTextureFormat)
        {
        case GpuFormat::R8_UNORM:              return MTLPixelFormatR8Unorm;
        case GpuFormat::R8G8B8A8_UNORM:        return MTLPixelFormatRGBA8Unorm;
        case GpuFormat::B8G8R8A8_UNORM:        return MTLPixelFormatBGRA8Unorm;
        case GpuFormat::R10G10B10A2_UNORM:     return MTLPixelFormatRGB10A2Unorm;
        case GpuFormat::R16_UINT:              return MTLPixelFormatR16Uint;
        case GpuFormat::R32_UINT:              return MTLPixelFormatR32Uint;
        case GpuFormat::R16G16B16A16_UNORM:    return MTLPixelFormatRGBA16Unorm;
        case GpuFormat::R16G16B16A16_UINT:     return MTLPixelFormatRGBA16Uint;
        case GpuFormat::R32G32_UINT:           return MTLPixelFormatRG32Uint;
        case GpuFormat::R32G32B32A32_UINT:     return MTLPixelFormatRGBA32Uint;
        case GpuFormat::R32G32_FLOAT:          return MTLPixelFormatRG32Float;
        case GpuFormat::R16G16B16A16_FLOAT:    return MTLPixelFormatRGBA16Float;
        case GpuFormat::R32G32B32A32_FLOAT:    return MTLPixelFormatRGBA32Float;
        case GpuFormat::R11G11B10_FLOAT:       return MTLPixelFormatRG11B10Float;
        case GpuFormat::R16_FLOAT:             return MTLPixelFormatR16Float;
        case GpuFormat::R32_FLOAT:             return MTLPixelFormatR32Float;
        case GpuFormat::D32_FLOAT:             return MTLPixelFormatDepth32Float;
        default:
			AUX::Unreachable();
        }
    }
    
    inline MTLPrimitiveType MapPrimitiveType(PrimitiveType InType)
    {
        switch (InType)
        {
        case PrimitiveType::PointList:            return MTLPrimitiveTypePoint;
        case PrimitiveType::LineList:             return MTLPrimitiveTypeLine;
        case PrimitiveType::LineStrip:            return MTLPrimitiveTypeLineStrip;
        case PrimitiveType::TriangleList:         return MTLPrimitiveTypeTriangle;
        case PrimitiveType::TriangleStrip:        return MTLPrimitiveTypeTriangleStrip;
        default:
			AUX::Unreachable();
        }
    }

    inline MTLCullMode MapRasterizerCullMode(RasterizerCullMode InMode)
    {
        switch (InMode)
        {
        case RasterizerCullMode::None:     return MTLCullModeNone;
        case RasterizerCullMode::Front:    return MTLCullModeFront;
        case RasterizerCullMode::Back:     return MTLCullModeBack;
        default:
            AUX::Unreachable();
        }
    }

    inline MTLTriangleFillMode MapRasterizerFillMode(RasterizerFillMode InMode)
    {
        switch (InMode)
        {
        case RasterizerFillMode::WireFrame:    return MTLTriangleFillModeLines;
        case RasterizerFillMode::Solid:        return MTLTriangleFillModeFill;
        default:
            AUX::Unreachable();
        }
    }

	inline MTLVertexFormat MapVertexFormat(GpuFormat InFormat)
    {
        switch (InFormat)
        {
        case GpuFormat::R32_FLOAT: return MTLVertexFormatFloat;
        case GpuFormat::R32G32_FLOAT: return MTLVertexFormatFloat2;
        case GpuFormat::R32G32B32_FLOAT: return MTLVertexFormatFloat3;
        case GpuFormat::R32G32B32A32_FLOAT: return MTLVertexFormatFloat4;
        case GpuFormat::R32_UINT: return MTLVertexFormatUInt;
        case GpuFormat::R32G32_UINT: return MTLVertexFormatUInt2;
        case GpuFormat::R32G32B32_UINT: return MTLVertexFormatUInt3;
        case GpuFormat::R32G32B32A32_UINT: return MTLVertexFormatUInt4;
        default:
            AUX::Unreachable();
        }
    }

    inline MTLVertexStepFunction MapVertexStepMode(GpuVertexStepMode InMode)
    {
        switch (InMode)
        {
        case GpuVertexStepMode::Vertex: return MTLVertexStepFunctionPerVertex;
        case GpuVertexStepMode::Instance: return MTLVertexStepFunctionPerInstance;
        default:
            AUX::Unreachable();
        }
    }

	inline MTLIndexType MapIndexFormat(GpuFormat InFormat)
    {
        switch (InFormat)
        {
        case GpuFormat::R16_UINT: return MTLIndexTypeUInt16;
        case GpuFormat::R32_UINT: return MTLIndexTypeUInt32;
        default:
            AUX::Unreachable();
        }
    }

    inline MTLCompareFunction MapCompareFunction(CompareMode InMode)
    {
        switch(InMode)
        {
        case CompareMode::Less:             return MTLCompareFunctionLess;
        case CompareMode::LessEqual:        return MTLCompareFunctionLessEqual;
        case CompareMode::Never:            return MTLCompareFunctionNever;
        case CompareMode::Always:           return MTLCompareFunctionAlways;
        case CompareMode::Equal:            return MTLCompareFunctionEqual;
        case CompareMode::Greater:          return MTLCompareFunctionGreater;
        case CompareMode::GreaterEqual:     return MTLCompareFunctionGreaterEqual;
        case CompareMode::NotEqual:         return MTLCompareFunctionNotEqual;
        default:
			AUX::Unreachable();
        }
    }

    inline MTLSamplerAddressMode MapSamplerAddressMode(SamplerAddressMode InMode)
    {
        switch (InMode)
        {
        case SamplerAddressMode::Clamp:     return MTLSamplerAddressModeClampToEdge;
        case SamplerAddressMode::Mirror:    return MTLSamplerAddressModeMirrorRepeat;
        case SamplerAddressMode::Wrap:      return MTLSamplerAddressModeRepeat;
        default:
			AUX::Unreachable();
        }
    }
}
