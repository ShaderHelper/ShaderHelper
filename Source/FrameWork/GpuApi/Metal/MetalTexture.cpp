#include "CommonHeader.h"
#include "MetalTexture.h"
#include "MetalMap.h"
#include "MetalDevice.h"
#import <CoreVideo/CVMetalTexture.h>
#import <CoreVideo/CVMetalTextureCache.h>
#include "MetalCommandRecorder.h"
#include "MetalGpuRhiBackend.h"

namespace FW
{
    MetalSampler::MetalSampler(MTLSamplerStatePtr InSampler, const GpuSamplerDesc& InDesc)
    : GpuSampler(InDesc)
	, Sampler(MoveTemp(InSampler))
    {
        GDeferredReleaseOneFrame.Add(this);
    }

    MetalTexture::MetalTexture(MTLTexturePtr InTex, GpuTextureDesc InDesc, CVPixelBufferRef InSharedHandle)
    : GpuTexture(MoveTemp(InDesc), GpuResourceState::Unknown)
    , Tex(MoveTemp(InTex))
    , SharedHandle(InSharedHandle)
    {
        GDeferredReleaseOneFrame.Add(this);
    }

    static void SetTextureUsage(GpuTextureUsage InUsage, MTL::TextureDescriptor* OutTexDesc)
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
        OutTexDesc->setUsage(Usage);
    }

    MTLSamplerDescriptor* MapSamplerDesc(const GpuSamplerDesc& InSamplerDesc)
    {
        MTLSamplerDescriptor* SamplerDesc = [MTLSamplerDescriptor new];
        switch(InSamplerDesc.Filter)
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
        SamplerDesc.supportArgumentBuffers = true;
        
        return [SamplerDesc autorelease];
    }

    TRefCountPtr<MetalSampler> CreateMetalSampler(const GpuSamplerDesc& InSamplerDesc)
    {
        MTLSamplerStatePtr Sampler = NS::TransferPtr(GDevice->newSamplerState((MTL::SamplerDescriptor*)MapSamplerDesc(InSamplerDesc)));
        return new MetalSampler(MoveTemp(Sampler), InSamplerDesc);
    }

    TRefCountPtr<MetalTexture> CreateMetalTexture2D(const GpuTextureDesc& InTexDesc)
    {
        TRefCountPtr<MetalTexture> RetTexture;
        if(EnumHasAnyFlags(InTexDesc.Usage, GpuTextureUsage::Shared))
        {
            RetTexture = CreateSharedMetalTexture(InTexDesc);
        }
        else
        {
            MTL::PixelFormat TexFormat = (MTL::PixelFormat)MapTextureFormat(InTexDesc.Format);
            MTL::TextureDescriptor* TexDesc = MTL::TextureDescriptor::texture2DDescriptor(TexFormat, InTexDesc.Width, InTexDesc.Height, InTexDesc.NumMips > 1);
            TexDesc->setTextureType(MTL::TextureType2D);
            TexDesc->setStorageMode(MTL::StorageModePrivate);
            SetTextureUsage(InTexDesc.Usage, TexDesc);
            
            MTLTexturePtr Tex = NS::TransferPtr(GDevice->newTexture(TexDesc));
            RetTexture = new MetalTexture(MoveTemp(Tex), InTexDesc);
        }
        
        if (!InTexDesc.InitialData.IsEmpty()) {
            const uint32 BytesImage = InTexDesc.InitialData.Num();
			TRefCountPtr<MetalBuffer> UploadBuffer = CreateMetalBuffer({BytesImage, GpuBufferUsage::Upload});
            uint8* BufferData = (uint8*)UploadBuffer->GetContents();
            FMemory::Memcpy(BufferData, InTexDesc.InitialData.GetData(), BytesImage);
            auto CmdRecorder = GMtlGpuRhi->BeginRecording("InitTexture");
            {
                CmdRecorder->CopyBufferToTexture(UploadBuffer, RetTexture);
            }
            GMtlGpuRhi->EndRecording(CmdRecorder);
            GMtlGpuRhi->Submit({CmdRecorder});
            
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
			AUX::Unreachable();
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
                        (id<MTLDevice>)GDevice,
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
        return new MetalTexture(NS::TransferPtr((MTL::Texture*)Texture), InTexDesc, CVPixelBuffer);
    }
}
