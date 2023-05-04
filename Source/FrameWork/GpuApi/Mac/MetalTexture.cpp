#include "CommonHeader.h"
#include "MetalTexture.h"
#include "MetalMap.h"
#include "MetalDevice.h"

namespace FRAMEWORK
{
    static void SetTextureUsage(GpuTextureUsage InUsage, mtlpp::TextureDescriptor& OutTexDesc)
    {
        MTLTextureUsage Usage = MTLTextureUsageUnknown;
        if(EnumHasAnyFlags(InUsage, GpuTextureUsage::RenderTarget))
        {
            Usage |= MTLTextureUsageRenderTarget;
        }
        
        if(EnumHasAnyFlags(InUsage, GpuTextureUsage::ShaderResource))
        {
            Usage |= MTLTextureUsageShaderRead;
        }
        OutTexDesc.SetUsage((mtlpp::TextureUsage)Usage);
    }

    TRefCountPtr<MetalTexture> CreateMetalTexture2D(const GpuTextureDesc& InTexDesc)
    {
        mtlpp::PixelFormat TexFormat = (mtlpp::PixelFormat)MapTextureFormat(InTexDesc.Format);
        mtlpp::TextureDescriptor TexDesc = mtlpp::TextureDescriptor::Texture2DDescriptor(TexFormat, InTexDesc.Width, InTexDesc.Height, false);
        SetTextureUsage(InTexDesc.Usage, TexDesc);
        mtlpp::Texture Tex = GDevice.NewTexture(MoveTemp(TexDesc));
        return new MetalTexture(MoveTemp(Tex));
    }
}
