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
	GMtlDeferredReleaseManager.ProcessResources();
}

TRefCountPtr<GpuTexture> MetalGpuRhiBackend::CreateTextureInternal(const GpuTextureDesc &InTexDesc, GpuResourceState InitState)
{
	return AUX::StaticCastRefCountPtr<GpuTexture>(CreateMetalTexture2D(InTexDesc, InitState));
}

TRefCountPtr<GpuTextureView> MetalGpuRhiBackend::CreateTextureViewInternal(const GpuTextureViewDesc& InViewDesc)
{
	MetalTexture* MtlTexture = static_cast<MetalTexture*>(InViewDesc.Texture);
	MTL::PixelFormat PixelFormat = MtlTexture->GetResource()->pixelFormat();
	MTL::TextureType TexType = MtlTexture->GetResource()->textureType();
	NS::Range LevelRange(InViewDesc.BaseMipLevel, InViewDesc.MipLevelCount);

	MTLTexturePtr MipView;
	if (TexType == MTL::TextureType3D)
	{
		MipView = NS::RetainPtr(MtlTexture->GetResource()->newTextureView(PixelFormat, TexType, LevelRange, NS::Range(0, 1)));
	}
	else if (TexType == MTL::TextureTypeCube)
	{
		if (InViewDesc.ArrayLayerCount == 6)
		{
			MipView = NS::RetainPtr(MtlTexture->GetResource()->newTextureView(PixelFormat, TexType, LevelRange, NS::Range(0, 6)));
		}
		else
		{
			MipView = NS::RetainPtr(MtlTexture->GetResource()->newTextureView(PixelFormat, MTL::TextureType2D, LevelRange, NS::Range(InViewDesc.BaseArrayLayer, InViewDesc.ArrayLayerCount)));
		}
	}
	else
	{
		MipView = NS::RetainPtr(MtlTexture->GetResource()->newTextureView(PixelFormat, TexType, LevelRange, NS::Range(0, 1)));
	}
	GpuTextureViewDesc Desc = InViewDesc;
	return new MetalTextureView(MoveTemp(Desc), MoveTemp(MipView));
}

TRefCountPtr<GpuShader> MetalGpuRhiBackend::CreateShaderFromSourceInternal(const GpuShaderSourceDesc& Desc) const
{
	TRefCountPtr<MetalShader> NewShader = new MetalShader(Desc);
	return AUX::StaticCastRefCountPtr<GpuShader>(NewShader);
}

TRefCountPtr<GpuShader> MetalGpuRhiBackend::CreateShaderFromFileInternal(const GpuShaderFileDesc& Desc)
{
	TRefCountPtr<MetalShader> NewShader = new MetalShader(Desc);
	return AUX::StaticCastRefCountPtr<GpuShader>(NewShader);
}

TRefCountPtr<GpuBindGroup> MetalGpuRhiBackend::CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc)
{
	return AUX::StaticCastRefCountPtr<GpuBindGroup>(CreateMetalBindGroup(InBindGroupDesc));
}

TRefCountPtr<GpuBindGroupLayout> MetalGpuRhiBackend::CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc)
{
	return AUX::StaticCastRefCountPtr<GpuBindGroupLayout>(CreateMetalBindGroupLayout(InBindGroupLayoutDesc));
}

TRefCountPtr<GpuRenderPipelineState> MetalGpuRhiBackend::CreateRenderPipelineStateInternal(const GpuRenderPipelineStateDesc &InPipelineStateDesc)
{
	return AUX::StaticCastRefCountPtr<GpuRenderPipelineState>(CreateMetalRenderPipelineState(InPipelineStateDesc));
}

TRefCountPtr<GpuComputePipelineState> MetalGpuRhiBackend::CreateComputePipelineStateInternal(const GpuComputePipelineStateDesc& InPipelineStateDesc)
{
    return AUX::StaticCastRefCountPtr<GpuComputePipelineState>(CreateMetalComputePipelineState(InPipelineStateDesc));
}

TRefCountPtr<GpuBuffer> MetalGpuRhiBackend::CreateBufferInternal(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState)
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
		const uint32 BytesPerTexel = GetFormatByteSize(InGpuTexture->GetFormat());
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
			Buffer->ReadBackBuffer = CreateMetalBuffer({Buffer->GetByteSize(), GpuBufferUsage::ReadBack});
			auto CmdRecorder = GMtlGpuRhi->BeginRecording();
			{
				CmdRecorder->CopyBufferToBuffer(Buffer, 0, Buffer->ReadBackBuffer, 0, Buffer->GetByteSize());
			}
			GMtlGpuRhi->EndRecording(CmdRecorder);
			GMtlGpuRhi->Submit({CmdRecorder});
			GMtlGpuRhi->WaitGpu();
			Data = Buffer->ReadBackBuffer->GetContents();
		}
	}

	return Data;
}

void MetalGpuRhiBackend::UnMapGpuBuffer(GpuBuffer *InGpuBuffer)
{
	MetalBuffer* Buffer = static_cast<MetalBuffer*>(InGpuBuffer);
	Buffer->ReadBackBuffer = nullptr;
}

bool MetalGpuRhiBackend::CompileShaderInternal(GpuShader *InShader, FString &OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs)
{
	return CompileMetalShader(static_cast<MetalShader *>(InShader), OutErrorInfo, OutWarnInfo, ExtraArgs);
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
    MTLCommandBufferPtr Cb = NS::RetainPtr(GCommandQueue->commandBufferWithUnretainedReferences());
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

TRefCountPtr<GpuQuerySet> MetalGpuRhiBackend::CreateQuerySet(uint32 Count)
{
    auto Desc = NS::TransferPtr(MTL::CounterSampleBufferDescriptor::alloc()->init());
    Desc->setCounterSet(GTimestampCounterSet);
    Desc->setSampleCount(Count);
    Desc->setStorageMode(MTL::StorageModeShared);

    NS::Error* Error = nullptr;
    MTL::CounterSampleBuffer* SampleBuffer = GDevice->newCounterSampleBuffer(Desc.get(), &Error);
    return new MetalQuerySet(Count, NS::TransferPtr(SampleBuffer));
}

MetalQuerySet::MetalQuerySet(uint32 InCount, NS::SharedPtr<MTL::CounterSampleBuffer> InSampleBuffer)
    : GpuQuerySet(InCount), SampleBuffer(std::move(InSampleBuffer))
{
    GMtlDeferredReleaseManager.AddResource(this);
}

double MetalQuerySet::GetTimestampPeriodNs() const
{
    return 1.0;
}

void MetalQuerySet::ResolveResults(uint32 FirstQuery, uint32 QueryCount, TArray<uint64>& OutTimestamps)
{
    GMtlGpuRhi->WaitGpu();
    OutTimestamps.SetNum(QueryCount);
    NS::Range Range = NS::Range::Make(FirstQuery, QueryCount);
    NS::Data* Data = SampleBuffer->resolveCounterRange(Range);

    const MTL::CounterResultTimestamp* Results = static_cast<const MTL::CounterResultTimestamp*>(Data->mutableBytes());
    for (uint32 i = 0; i < QueryCount; i++)
    {
        OutTimestamps[i] = Results[i].timestamp;
    }
}

}
