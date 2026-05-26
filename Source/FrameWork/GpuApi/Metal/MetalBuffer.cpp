#include "CommonHeader.h"
#include "MetalBuffer.h"
#include "MetalDevice.h"
#include "MetalGpuRhiBackend.h"
#include "MetalMap.h"

namespace FW
{
    MetalBuffer::MetalBuffer(MTLBufferPtr InBuffer, const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState)
    : GpuBuffer(InBufferDesc, InResourceState)
    , Buffer(MoveTemp(InBuffer))
    {
        GMtlDeferredReleaseManager.AddResource(this);
    }

    static MTLTexturePtr CreateMetalTypedBufferView(MTL::Buffer* InBuffer, const GpuBufferDesc& InBufferDesc)
    {
        MTL::PixelFormat Format = MapTextureFormat(InBufferDesc.TypedInit.Format);
        NS::UInteger FormatSize = GetFormatByteSize(InBufferDesc.TypedInit.Format);
        NS::UInteger NumElements = InBufferDesc.ByteSize / FormatSize;

        MTLTextureDescriptorPtr Desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
        Desc->setTextureType(MTL::TextureTypeTextureBuffer);
        Desc->setPixelFormat(Format);
        Desc->setWidth(NumElements);
        Desc->setHeight(1);
        Desc->setStorageMode(InBuffer->storageMode());
        MTL::TextureUsage TexUsage = MTL::TextureUsageShaderRead;
        if (EnumHasAnyFlags(InBufferDesc.Usage, GpuBufferUsage::RWTyped))
        {
            TexUsage |= MTL::TextureUsageShaderWrite;
        }
        Desc->setUsage(TexUsage);

        return NS::TransferPtr(InBuffer->newTexture(Desc.get(), 0, InBufferDesc.ByteSize));
    }

    TRefCountPtr<MetalBuffer> CreateMetalBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState)
    {
		TRefCountPtr<MetalBuffer> Buffer;
        if(EnumHasAnyFlags(InBufferDesc.Usage, GpuBufferUsage::StaticMask))
        {
			if(InBufferDesc.InitialData.Num() > 0)
			{
				TRefCountPtr<MetalBuffer> UploadBuffer = CreateMetalBuffer({
					.ByteSize = InBufferDesc.ByteSize,
					.Usage = GpuBufferUsage::Upload,
					.InitialData = InBufferDesc.InitialData
				});
				Buffer = new MetalBuffer(NS::TransferPtr(GDevice->newBuffer(InBufferDesc.ByteSize, MTL::ResourceStorageModePrivate)), InBufferDesc, InResourceState);
				auto CmdRecorder = GMtlGpuRhi->BeginRecording();
				{
					CmdRecorder->CopyBufferToBuffer(UploadBuffer, 0, Buffer, 0, UploadBuffer->GetByteSize());
				}
				GMtlGpuRhi->EndRecording(CmdRecorder);
				GMtlGpuRhi->Submit({CmdRecorder});
			}
			else
			{
				Buffer = new MetalBuffer(NS::TransferPtr(GDevice->newBuffer(InBufferDesc.ByteSize, MTL::ResourceStorageModePrivate)), InBufferDesc, InResourceState);
			}

        }
        else
        {
			if(InBufferDesc.InitialData.Num() > 0)
			{
				Buffer = new MetalBuffer(NS::TransferPtr(GDevice->newBuffer(InBufferDesc.InitialData.GetData(), InBufferDesc.InitialData.Num(), MTL::ResourceStorageModeShared)), InBufferDesc, InResourceState);
			}
			else
			{
				Buffer = new MetalBuffer(NS::TransferPtr(GDevice->newBuffer(InBufferDesc.ByteSize, MTL::ResourceStorageModeShared)), InBufferDesc, InResourceState);
			}

        }

        if (EnumHasAnyFlags(InBufferDesc.Usage, GpuBufferUsage::Typed | GpuBufferUsage::RWTyped))
        {
            Buffer->SetTypedView(CreateMetalTypedBufferView(Buffer->GetResource(), InBufferDesc));
        }
        return Buffer;
    }
}
