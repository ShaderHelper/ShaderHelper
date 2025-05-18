#include "CommonHeader.h"

#include "MetalGpuRhiBackend.h"

#include "MetalArgumentBuffer.h"
#include "MetalBuffer.h"
#include "MetalCommandRecorder.h"
#include "MetalCommon.h"
#include "MetalDevice.h"
#include "MetalMap.h"
#include "MetalPipeline.h"
#include "MetalShader.h"
#include "MetalTexture.h"
#include "Common/Path/PathHelper.h"

namespace FW
{
MetalGpuRhiBackend::MetalGpuRhiBackend() { GMtlGpuRhi = this; }

MetalGpuRhiBackend::~MetalGpuRhiBackend() { }

void MetalGpuRhiBackend::InitApiEnv()
{
	InitMetalCore();
}

void MetalGpuRhiBackend::WaitGpu()
{
    if(LastSubmittedCmdRecorder)
    {
        LastSubmittedCmdRecorder->GetCommandBuffer()->waitUntilCompleted();
        LastSubmittedCmdRecorder = nullptr;
    }
}

void MetalGpuRhiBackend::BeginFrame()
{
}

void MetalGpuRhiBackend::EndFrame()
{
	WaitGpu();
	GMtlCmdRecorderPool.Empty();
    GDeferredReleaseOneFrame.Empty();
}

TRefCountPtr<GpuTexture> MetalGpuRhiBackend::CreateTexture(const GpuTextureDesc &InTexDesc, GpuResourceState InitState)
{
	return AUX::StaticCastRefCountPtr<GpuTexture>(CreateMetalTexture2D(InTexDesc));
}

TRefCountPtr<GpuShader> MetalGpuRhiBackend::CreateShaderFromSource(const GpuShaderSourceDesc& Desc)
{
	return AUX::StaticCastRefCountPtr<GpuShader>(CreateMetalShader(Desc));
}

TRefCountPtr<GpuShader> MetalGpuRhiBackend::CreateShaderFromFile(const GpuShaderFileDesc& Desc)
{
	return AUX::StaticCastRefCountPtr<GpuShader>(CreateMetalShader(Desc));
}

TRefCountPtr<GpuBindGroup> MetalGpuRhiBackend::CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc)
{
	return AUX::StaticCastRefCountPtr<GpuBindGroup>(CreateMetalBindGroup(InBindGroupDesc));
}

TRefCountPtr<GpuBindGroupLayout> MetalGpuRhiBackend::CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc)
{
	return AUX::StaticCastRefCountPtr<GpuBindGroupLayout>(CreateMetalBindGroupLayout(InBindGroupLayoutDesc));
}

TRefCountPtr<GpuRenderPipelineState> MetalGpuRhiBackend::CreateRenderPipelineState(const GpuRenderPipelineStateDesc &InPipelineStateDesc)
{
	return AUX::StaticCastRefCountPtr<GpuRenderPipelineState>(CreateMetalRenderPipelineState(InPipelineStateDesc));
}

TRefCountPtr<GpuComputePipelineState> MetalGpuRhiBackend::CreateComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc)
{
    return AUX::StaticCastRefCountPtr<GpuComputePipelineState>(CreateMetalComputePipelineState(InPipelineStateDesc));
}

TRefCountPtr<GpuBuffer> MetalGpuRhiBackend::CreateBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState)
{
	return AUX::StaticCastRefCountPtr<GpuBuffer>(CreateMetalBuffer(InBufferDesc, InitState));
}

TRefCountPtr<GpuSampler> MetalGpuRhiBackend::CreateSampler(const GpuSamplerDesc &InSamplerDesc)
{
	return AUX::StaticCastRefCountPtr<GpuSampler>(CreateMetalSampler(InSamplerDesc));
}

void MetalGpuRhiBackend::SetResourceName(const FString& Name, GpuResource* InResource)
{
    if (InResource->GetType() == GpuResourceType::Texture)
    {
        static_cast<MetalTexture*>(InResource)->GetResource()->setLabel(FStringToNSString(Name));
    }
}

void *MetalGpuRhiBackend::MapGpuTexture(GpuTexture *InGpuTexture, GpuResourceMapMode InMapMode, uint32 &OutRowPitch)
{
	MetalTexture *Texture = static_cast<MetalTexture *>(InGpuTexture);
	if (InMapMode == GpuResourceMapMode::Read_Only) {
		const uint32 BytesPerTexel = GetTextureFormatByteSize(InGpuTexture->GetFormat());
		const uint64 UnpaddedSize = InGpuTexture->GetWidth() * InGpuTexture->GetHeight() * BytesPerTexel;
		if (!Texture->ReadBackBuffer.IsValid()) {
			Texture->ReadBackBuffer = CreateMetalBuffer({(uint32)UnpaddedSize, GpuBufferUsage::ReadBack});
		}

		// Metal does not consider the alignment when copying back?
		OutRowPitch = InGpuTexture->GetWidth() * BytesPerTexel;
        auto CmdRecorder = GMtlGpuRhi->BeginRecording();
        {
            CmdRecorder->CopyTextureToBuffer(InGpuTexture, Texture->ReadBackBuffer);
        }
        GMtlGpuRhi->EndRecording(CmdRecorder);
        GMtlGpuRhi->Submit({CmdRecorder});
		GMtlGpuRhi->WaitGpu();
		
		return Texture->ReadBackBuffer->GetContents();
	} else {
		// TODO
		check(false);
		return nullptr;
	}
}

void MetalGpuRhiBackend::UnMapGpuTexture(GpuTexture *InGpuTexture)
{
	// do nothing.
}

void *MetalGpuRhiBackend::MapGpuBuffer(GpuBuffer *InGpuBuffer, GpuResourceMapMode InMapMode)
{
	GpuBufferUsage Usage = InGpuBuffer->GetUsage();
	MetalBuffer *Buffer = static_cast<MetalBuffer *>(InGpuBuffer);
	void *Data = nullptr;
	
	if (EnumHasAnyFlags(Usage, GpuBufferUsage::DynamicMask))
	{
		Data = Buffer->GetContents();
	}
	else
	{
		if (InMapMode == GpuResourceMapMode::Read_Only)
		{
			TRefCountPtr<MetalBuffer> ReadBackBuffer = CreateMetalBuffer({Buffer->GetByteSize(), GpuBufferUsage::ReadBack});
			auto CmdRecorder = GMtlGpuRhi->BeginRecording();
			{
				CmdRecorder->CopyBufferToBuffer(Buffer, 0, ReadBackBuffer, 0, Buffer->GetByteSize());
			}
			GMtlGpuRhi->EndRecording(CmdRecorder);
			GMtlGpuRhi->Submit({CmdRecorder});
			GMtlGpuRhi->WaitGpu();
			Data = ReadBackBuffer->GetContents();
		}
	}

	return Data;
}

void MetalGpuRhiBackend::UnMapGpuBuffer(GpuBuffer *InGpuBuffer)
{
	// do nothing.
}

bool MetalGpuRhiBackend::CompileShader(GpuShader *InShader, FString &OutErrorInfo, const TArray<FString>& Definitions)
{
	return CompileShaderFromHlsl(static_cast<MetalShader *>(InShader), OutErrorInfo);
}

void MetalGpuRhiBackend::Submit(const TArray<GpuCmdRecorder*>& CmdRecorders)
{
	for(auto CmdRecorder : CmdRecorders)
    {
        MtlCmdRecorder* MetalCmdRecorder = static_cast<MtlCmdRecorder*>(CmdRecorder);
        MTL::CommandBuffer* CmdBuffer = MetalCmdRecorder->GetCommandBuffer();
        CmdBuffer->commit();
    }
    LastSubmittedCmdRecorder = static_cast<MtlCmdRecorder*>(CmdRecorders.Last());
}

TMap<FString, NS::SharedPtr<MTL::CaptureScope>> CaptureScopes;
MTL::CaptureScope* CurScope;

void MetalGpuRhiBackend::BeginGpuCapture(const FString &CaptureName)
{
#if GPU_API_CAPTURE
#if !GENERATE_XCODE_GPUTRACE
    if(!CaptureScopes.Contains(CaptureName))
    {
        NS::SharedPtr<MTL::CaptureScope> CaptureScope = NS::TransferPtr(MTL::CaptureManager::sharedCaptureManager()->newCaptureScope(GDevice));
        CaptureScope->setLabel(FStringToNSString(CaptureName));
        CaptureScopes.Add(CaptureName, MoveTemp(CaptureScope));
    }
    CurScope = CaptureScopes[CaptureName].get();
    CurScope->beginScope();
#else
    if(MTL::CaptureManager::sharedCaptureManager()->supportsDestination(MTL::CaptureDestinationGPUTraceDocument))
    {
        FString SavedFileName = PathHelper::SavedCaptureDir() / CaptureName + ".gputrace";
        NS::SharedPtr<MTL::CaptureDescriptor> CaptureDesc = NS::TransferPtr(MTL::CaptureDescriptor::alloc()->init());
        CaptureDesc->setCaptureObject((id)GDevice);
        CaptureDesc->setDestination(MTL::CaptureDestinationGPUTraceDocument);
        CaptureDesc->setOutputURL(NS::URL::fileURLWithPath(FStringToNSString(SavedFileName)));
        NS::Error* err = nullptr;
        
        IFileManager::Get().DeleteDirectory(*SavedFileName, false, true);
        MTL::CaptureManager::sharedCaptureManager()->startCapture(CaptureDesc.get(), &err);
        if(err)
        {
            SH_LOG(LogMetal, Error, TEXT("%s"), *NSStringToFString(err->localizedDescription()));
        }
    }
#endif
#endif
}

void MetalGpuRhiBackend::EndGpuCapture()
{
#if GPU_API_CAPTURE
#if !GENERATE_XCODE_GPUTRACE
    CurScope->endScope();
#else
    if(MTL::CaptureManager::sharedCaptureManager()->supportsDestination(MTL::CaptureDestinationGPUTraceDocument))
    {
        MTL::CaptureManager::sharedCaptureManager()->stopCapture();
    }
#endif
#endif
}

void *MetalGpuRhiBackend::GetSharedHandle(GpuTexture *InGpuTexture)
{
	return static_cast<MetalTexture *>(InGpuTexture)->GetSharedHandle();
}

GpuCmdRecorder* MetalGpuRhiBackend::BeginRecording(const FString& RecorderName)
{
	//mtl commandbuffer is transient(lightweight) and do not support reuse, so create new one per request.
    MTLCommandBufferPtr Cb = NS::RetainPtr(GCommandQueue->commandBuffer());
    if(!RecorderName.IsEmpty())
    {
        Cb->setLabel(FStringToNSString(RecorderName));
    }
    auto CmdRecorder = MakeUnique<MtlCmdRecorder>(MoveTemp(Cb));
    GMtlCmdRecorderPool.Add(MoveTemp(CmdRecorder));
    return GMtlCmdRecorderPool.Last().Get();
}

void MetalGpuRhiBackend::EndRecording(GpuCmdRecorder* InCmdRecorder)
{
    
}

}
