#include "CommonHeader.h"
#include "MetalTexture.h"
#include "MetalMap.h"
#include "MetalDevice.h"
#import <CoreVideo/CVMetalTexture.h>
#import <CoreVideo/CVMetalTextureCache.h>

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
        if(EnumHasAnyFlags(InTexDesc.Usage, GpuTextureUsage::Shared))
        {
            return CreateSharedMetalTexture(InTexDesc);
        }
        
        mtlpp::PixelFormat TexFormat = (mtlpp::PixelFormat)MapTextureFormat(InTexDesc.Format);
        mtlpp::TextureDescriptor TexDesc = mtlpp::TextureDescriptor::Texture2DDescriptor(TexFormat, InTexDesc.Width, InTexDesc.Height, InTexDesc.NumMips > 1);
        TexDesc.SetTextureType(mtlpp::TextureType::Texture2D);
        TexDesc.SetStorageMode(mtlpp::StorageMode::Private);
        SetTextureUsage(InTexDesc.Usage, TexDesc);
        
        mtlpp::Texture Tex = GDevice.NewTexture(MoveTemp(TexDesc));
        return new MetalTexture(MoveTemp(Tex), InTexDesc);
    }

    static OSType MTLPixelFormatToCVPixelFormat(MTLPixelFormat InFormat)
    {
        switch(InFormat)
        {
        case MTLPixelFormatBGRA8Unorm:  return kCVPixelFormatType_32BGRA;
        default:
            check(false);
            return kCVPixelFormatType_32BGRA;
        }
    }

    TRefCountPtr<MetalTexture> CreateSharedMetalTexture(const GpuTextureDesc& InTexDesc)
    {
        MTLPixelFormat TexFormat = MapTextureFormat(InTexDesc.Format);
        CVPixelBufferRef CVPixelBuffer;
        
        NSDictionary* BufferPropertyies = @{
            (NSString*)kCVPixelBufferMetalCompatibilityKey : @YES,
            (NSString*)kCVPixelBufferOpenGLCompatibilityKey : @YES
        };
        CVReturn cvret = CVPixelBufferCreate(kCFAllocatorDefault,
                                InTexDesc.Width, InTexDesc.Height,
                                MTLPixelFormatToCVPixelFormat(TexFormat),
                                (CFDictionaryRef)BufferPropertyies,
                                &CVPixelBuffer);
        
        CVMetalTextureRef CVMTLTexture;
        CVMetalTextureCacheRef CVMTLTextureCache;
        
        cvret = CVMetalTextureCacheCreate(
                        kCFAllocatorDefault,
                        nil,
                        GDevice,
                        nil,
                        &CVMTLTextureCache);
        
        cvret = CVMetalTextureCacheCreateTextureFromImage(
                        kCFAllocatorDefault,
                        CVMTLTextureCache,
                        CVPixelBuffer, nil,
                        TexFormat,
                        InTexDesc.Width, InTexDesc.Height,
                        0,
                        &CVMTLTexture);
        
        id<MTLTexture> Texture = CVMetalTextureGetTexture(CVMTLTexture);
        [Texture retain];
        
        CFRelease(CVMTLTextureCache);
        CVBufferRelease(CVMTLTexture);
        return new MetalTexture(mtlpp::Texture(Texture, nullptr, ns::Ownership::Assign), InTexDesc, CVPixelBuffer);
    }
}
