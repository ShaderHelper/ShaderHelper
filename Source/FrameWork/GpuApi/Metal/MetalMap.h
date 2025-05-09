#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"
#include "MetalTexture.h"

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
            MetalTexture* Rt = static_cast<MetalTexture*>(RtInfo.Texture);
            RawPassDesc.colorAttachments[i].texture = (id<MTLTexture>)Rt->GetResource();
            RawPassDesc.colorAttachments[i].loadAction = MapLoadAction(RtInfo.LoadAction);
            RawPassDesc.colorAttachments[i].storeAction = MapStoreAction(RtInfo.StoreAction);
            RawPassDesc.colorAttachments[i].clearColor = MTLClearColorMake(RtInfo.ClearColor.x, RtInfo.ClearColor.y, RtInfo.ClearColor.z, RtInfo.ClearColor.w);
        }
        
        if(PassRtNum <= 0)
        {
            //targetless rasterization
            RawPassDesc.renderTargetWidth = 1;
            RawPassDesc.renderTargetHeight = 1;
            RawPassDesc.defaultRasterSampleCount = 1;
        }
        return [RawPassDesc autorelease];
    }
    
    inline MTLPixelFormat MapTextureFormat(const GpuTextureFormat& InTextureFormat)
    {
        switch(InTextureFormat)
        {
        case GpuTextureFormat::R8G8B8A8_UNORM:        return MTLPixelFormatRGBA8Unorm;
        case GpuTextureFormat::B8G8R8A8_UNORM:        return MTLPixelFormatBGRA8Unorm;
        case GpuTextureFormat::R10G10B10A2_UNORM:     return MTLPixelFormatRGB10A2Unorm;
        case GpuTextureFormat::R16G16B16A16_UNORM:    return MTLPixelFormatRGBA16Unorm;
        case GpuTextureFormat::R16G16B16A16_UINT:     return MTLPixelFormatRGBA16Uint;
        case GpuTextureFormat::R32G32B32A32_UINT:     return MTLPixelFormatRGBA32Uint;
        case GpuTextureFormat::R16G16B16A16_FLOAT:    return MTLPixelFormatRGBA16Float;
        case GpuTextureFormat::R32G32B32A32_FLOAT:    return MTLPixelFormatRGBA32Float;
        case GpuTextureFormat::R11G11B10_FLOAT:       return MTLPixelFormatRG11B10Float;
		case GpuTextureFormat::R16_FLOAT:             return MTLPixelFormatR16Float;
        case GpuTextureFormat::R32_FLOAT:             return MTLPixelFormatR32Float;
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
