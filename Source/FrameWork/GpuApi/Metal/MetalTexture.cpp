#include "CommonHeader.h"
#include "MetalTexture.h"
#include "MetalMap.h"
#include "MetalDevice.h"
#import <CoreVideo/CVMetalTexture.h>
#import <CoreVideo/CVMetalTextureCache.h>
#include "MetalGpuRhiBackend.h"

namespace FW
{
    MetalSampler::MetalSampler(MTLSamplerStatePtr InSampler, const GpuSamplerDesc& InDesc)
    : GpuSampler(InDesc)
	, Sampler(MoveTemp(InSampler))
    {
        GMtlDeferredReleaseManager.AddResource(this);
    }

    MetalTexture::MetalTexture(MTLTexturePtr InTex, GpuTextureDesc InDesc, GpuResourceState InitState, CVPixelBufferRef InSharedHandle)
    : GpuTexture(MoveTemp(InDesc), InitState)
    , Tex(MoveTemp(InTex))
    , SharedHandle(InSharedHandle)
    {
        GMtlDeferredReleaseManager.AddResource(this);

        GpuTextureViewDesc DefaultViewDesc;
        DefaultViewDesc.Texture = this;
        DefaultViewDesc.BaseMipLevel = 0;
        DefaultViewDesc.MipLevelCount = GetNumMips();
        DefaultViewDesc.ArrayLayerCount = GetArrayLayerCount();
        DefaultView = GMtlGpuRhi->CreateTextureView(DefaultViewDesc);
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

        if(EnumHasAnyFlags(InUsage, GpuTextureUsage::UnorderedAccess))
        {
            Usage |= MTLTextureUsageShaderWrite;
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

    TRefCountPtr<MetalTexture> CreateMetalTexture2D(const GpuTextureDesc& InTexDesc, GpuResourceState InitState)
    {
        TRefCountPtr<MetalTexture> RetTexture;
        if(EnumHasAnyFlags(InTexDesc.Usage, GpuTextureUsage::Shared) && InTexDesc.Dimension == GpuTextureDimension::Tex2D)
        {
            RetTexture = CreateSharedMetalTexture(InTexDesc, InitState);
        }
        else
        {
            MTL::PixelFormat TexFormat = (MTL::PixelFormat)MapTextureFormat(InTexDesc.Format);
            MTL::TextureDescriptor* TexDesc = MTL::TextureDescriptor::texture2DDescriptor(TexFormat, InTexDesc.Width, InTexDesc.Height, InTexDesc.NumMips > 1);
            TexDesc->setMipmapLevelCount(InTexDesc.NumMips);
            if (InTexDesc.Dimension == GpuTextureDimension::TexCube)
            {
                TexDesc->setTextureType(MTL::TextureTypeCube);
            }
            else if (InTexDesc.Dimension == GpuTextureDimension::Tex3D)
            {
                TexDesc->setTextureType(MTL::TextureType3D);
                TexDesc->setDepth(InTexDesc.Depth);
            }
            else
            {
                TexDesc->setTextureType(MTL::TextureType2D);
            }
            TexDesc->setStorageMode(MTL::StorageModePrivate);
            SetTextureUsage(InTexDesc.Usage, TexDesc);
            
            MTLTexturePtr Tex = NS::TransferPtr(GDevice->newTexture(TexDesc));
            RetTexture = new MetalTexture(MoveTemp(Tex), InTexDesc, InitState);
        }

        if (!InTexDesc.InitialData.IsEmpty()) {
            const uint32 BytesPerTexel = GetFormatByteSize(InTexDesc.Format);
            const uint32 FaceByteSize = InTexDesc.Width * InTexDesc.Height * BytesPerTexel;
            auto CmdRecorder = GMtlGpuRhi->BeginRecording("InitTexture");
            {
                if (InTexDesc.Dimension == GpuTextureDimension::TexCube) {
                    for (uint32 Face = 0; Face < 6; Face++) {
                        TRefCountPtr<MetalBuffer> FaceUpload = CreateMetalBuffer({FaceByteSize, GpuBufferUsage::Upload});
                        FMemory::Memcpy(FaceUpload->GetContents(), InTexDesc.InitialData.GetData() + Face * FaceByteSize, FaceByteSize);
                        CmdRecorder->CopyBufferToTexture(FaceUpload, RetTexture, Face, 0);
                    }
                } else {
                    const uint32 BytesTotal = InTexDesc.InitialData.Num();
                    TRefCountPtr<MetalBuffer> UploadBuffer = CreateMetalBuffer({BytesTotal, GpuBufferUsage::Upload});
                    FMemory::Memcpy(UploadBuffer->GetContents(), InTexDesc.InitialData.GetData(), BytesTotal);
                    CmdRecorder->CopyBufferToTexture(UploadBuffer, RetTexture);
                    RetTexture->UploadBuffer = MoveTemp(UploadBuffer);
                }
            }
            GMtlGpuRhi->EndRecording(CmdRecorder);
            GMtlGpuRhi->Submit({CmdRecorder});
        }
        
        return RetTexture;
    }

    static OSType MTLPixelFormatToCVPixelFormat(MTLPixelFormat InFormat)
    {
        switch(InFormat)
        {
        case MTLPixelFormatR8G8B8A8Unorm: return kCVPixelFormatType_32RGBA;
        case MTLPixelFormatBGRA8Unorm:  return kCVPixelFormatType_32BGRA;
		case MTLPixelFormatR8Unorm:		return kCVPixelFormatType_OneComponent8;
		case MTLPixelFormatRGBA32Float: return kCVPixelFormatType_128RGBAFloat;
        default:
			AUX::Unreachable();
        }
    }

    TRefCountPtr<MetalTexture> CreateSharedMetalTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState)
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
        return new MetalTexture(NS::TransferPtr((MTL::Texture*)Texture), InTexDesc, InitState, CVPixelBuffer);
    }
}
