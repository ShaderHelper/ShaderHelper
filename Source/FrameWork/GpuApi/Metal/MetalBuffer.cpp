#include "CommonHeader.h"
#include "MetalBuffer.h"
#include "MetalDevice.h"
#include "MetalGpuRhiBackend.h"

namespace FW
{
    MetalBuffer::MetalBuffer(MTLBufferPtr InBuffer, const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState)
    : GpuBuffer(InBufferDesc, InResourceState)
    , Buffer(MoveTemp(InBuffer))
    {
        GMtlDeferredReleaseManager.AddResource(this);
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
        return Buffer;
    }
}
