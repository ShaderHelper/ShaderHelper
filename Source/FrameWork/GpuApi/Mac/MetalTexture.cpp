#include "CommonHeader.h"
#include "MetalTexture.h"
#include "MetalMap.h"
#include "MetalDevice.h"
#import <CoreVideo/CVMetalTexture.h>
#import <CoreVideo/CVMetalTextureCache.h>
#include "MetalCommandList.h"

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

    mtlpp::SamplerDescriptor MapSamplerDesc(const GpuSamplerDesc& InSamplerDesc)
    {
        MTLSamplerDescriptor* SamplerDesc = [MTLSamplerDescriptor new];
        switch(InSamplerDesc.Filer)
        {
        case SamplerFilter::Point:
            SamplerDesc.minFilter = MTLSamplerMinMagFilterNearest;
            SamplerDesc.magFilter = MTLSamplerMinMagFilterNearest;
            SamplerDesc.mipFilter = MTLSamplerMipFilterNearest;
            break;
        case SamplerFilter::Bilinear:
            SamplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
            SamplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
            SamplerDesc.mipFilter = MTLSamplerMipFilterNearest;
            break;
        case SamplerFilter::Trilinear:
            SamplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
            SamplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
            SamplerDesc.mipFilter = MTLSamplerMipFilterLinear;
            break;
        }
        SamplerDesc.sAddressMode = MapSamplerAddressMode(InSamplerDesc.AddressU);
        SamplerDesc.tAddressMode = MapSamplerAddressMode(InSamplerDesc.AddressV);
        SamplerDesc.rAddressMode = MapSamplerAddressMode(InSamplerDesc.AddressW);
        SamplerDesc.compareFunction = MapCompareFunction(InSamplerDesc.Compare);
        
        return mtlpp::SamplerDescriptor{SamplerDesc, ns::Ownership::Assign};
    }

    TRefCountPtr<MetalSampler> CreateMetalSampler(const GpuSamplerDesc& InSamplerDesc)
    {
        mtlpp::SamplerState Sampler = GDevice.NewSamplerState(MapSamplerDesc(InSamplerDesc));
        return new MetalSampler(MoveTemp(Sampler));
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
        TRefCountPtr<MetalTexture> RetTexture = new MetalTexture(MoveTemp(Tex), InTexDesc);
        
        if (!InTexDesc.InitialData.IsEmpty()) {
            const uint32 BytesImage = InTexDesc.InitialData.Num();
            TRefCountPtr<MetalBuffer> UploadBuffer = CreateMetalBuffer(BytesImage, GpuBufferUsage::Dynamic);
            uint8* BufferData = (uint8*)UploadBuffer->GetContents();
            FMemory::Memcpy(BufferData, InTexDesc.InitialData.GetData(), BytesImage);
            
            mtlpp::BlitCommandEncoder BlitCommandEncoder = GetCommandListContext()->GetBlitCommandEncoder();
            
            const uint32 BytesPerTexel = GetTextureFormatByteSize(InTexDesc.Format);
            const uint32 BytesPerRow = InTexDesc.Width * BytesPerTexel;
            [BlitCommandEncoder.GetPtr() copyFromBuffer:UploadBuffer->GetResource() sourceOffset:0 sourceBytesPerRow:BytesPerRow sourceBytesPerImage:BytesImage sourceSize:MTLSizeMake(InTexDesc.Width, InTexDesc.Height, 1) toTexture:RetTexture->GetResource() destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
            BlitCommandEncoder.EndEncoding();
            
            RetTexture->UploadBuffer = MoveTemp(UploadBuffer);
        }
        
        return RetTexture;
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
