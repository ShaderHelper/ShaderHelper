#include "CommonHeader.h"

#include "Dx12GpuRhiBackend.h"

#include "Dx12Allocation.h"
#include "Dx12CommandRecorder.h"
#include "Dx12Device.h"
#include "Dx12Map.h"
#include "Dx12PSO.h"
#include "Dx12RS.h"
#include "Dx12Shader.h"
#include "Dx12Texture.h"
#include "Common/Path/PathHelper.h"

namespace FW
{
Dx12GpuRhiBackend::Dx12GpuRhiBackend() { 
	GDx12GpuRhi = this;
}

Dx12GpuRhiBackend::~Dx12GpuRhiBackend() { }

void Dx12GpuRhiBackend::InitApiEnv()
{
	InitDx12Core();
	InitBufferAllocator();
	InitDescriptorAllocator();
}

void Dx12GpuRhiBackend::WaitGpu()
{
	DxCheck(GGraphicsQueue->Signal(CpuSyncGpuFence, CurCpuFrame + 114514));
	DxCheck(CpuSyncGpuFence->SetEventOnCompletion(CurCpuFrame + 114514, CpuSyncGpuEvent));
	WaitForSingleObject(CpuSyncGpuEvent, INFINITE);
	CpuSyncGpuFence->Signal(CurCpuFrame);
	CurGpuFrame = CurCpuFrame;
}

void Dx12GpuRhiBackend::BeginFrame()
{
	uint32 FrameResourceIndex = GetCurFrameSourceIndex();
	GTempUniformBufferAllocator[FrameResourceIndex]->Flush();
}

void Dx12GpuRhiBackend::EndFrame()
{
	check(CurCpuFrame >= CurGpuFrame);
	CurCpuFrame++;
	DxCheck(GGraphicsQueue->Signal(CpuSyncGpuFence, CurCpuFrame));

	CurGpuFrame = CpuSyncGpuFence->GetCompletedValue();
	const uint64 CurLag = CurCpuFrame - CurGpuFrame;
	if (CurLag > AllowableLag) {
		// Cpu is waiting for gpu to catch up a frame.
		DxCheck(CpuSyncGpuFence->SetEventOnCompletion(CurGpuFrame + 1, CpuSyncGpuEvent));
		WaitForSingleObject(CpuSyncGpuEvent, INFINITE);
		CurGpuFrame = CurGpuFrame + 1;
	}

	GDeferredReleaseManager->ProcessResources();
}

TRefCountPtr<GpuTexture> Dx12GpuRhiBackend::CreateTexture(const GpuTextureDesc &InTexDesc, GpuResourceState InitState)
{
	return AUX::StaticCastRefCountPtr<GpuTexture>(CreateDx12Texture2D(InTexDesc, InitState));
}

TRefCountPtr<GpuShader> Dx12GpuRhiBackend::CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName, FString EntryPoint)
{
	return AUX::StaticCastRefCountPtr<GpuShader>(CreateDx12Shader(InType, MoveTemp(InSourceText), MoveTemp(InShaderName), MoveTemp(EntryPoint)));
}

TRefCountPtr<GpuShader> Dx12GpuRhiBackend::CreateShaderFromFile(FString FileName, ShaderType InType, FString EntryPoint, FString ExtraDeclaration)
{
	return AUX::StaticCastRefCountPtr<GpuShader>(CreateDx12Shader(MoveTemp(FileName), InType, MoveTemp(ExtraDeclaration), MoveTemp(EntryPoint)));
}

TRefCountPtr<GpuBindGroup> Dx12GpuRhiBackend::CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc)
{
	return AUX::StaticCastRefCountPtr<GpuBindGroup>(CreateDx12BindGroup(InBindGroupDesc));
}

TRefCountPtr<GpuBindGroupLayout> Dx12GpuRhiBackend::CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc)
{
	return AUX::StaticCastRefCountPtr<GpuBindGroupLayout>(CreateDx12BindGroupLayout(InBindGroupLayoutDesc));
}

TRefCountPtr<GpuPipelineState> Dx12GpuRhiBackend::CreateRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc)
{
	return AUX::StaticCastRefCountPtr<GpuPipelineState>(CreateDx12RenderPso(InPipelineStateDesc));
}

TRefCountPtr<GpuBuffer> Dx12GpuRhiBackend::CreateBuffer(uint32 ByteSize, GpuBufferUsage Usage, GpuResourceState InitState)
{
	if (EnumHasAnyFlags(Usage, GpuBufferUsage::PersistentUniform) || EnumHasAnyFlags(Usage, GpuBufferUsage::TemporaryUniform))
	{
		return AUX::StaticCastRefCountPtr<GpuBuffer>(CreateDx12ConstantBuffer(ByteSize, Usage));
	}
	else
	{
		return AUX::StaticCastRefCountPtr<GpuBuffer>(CreateDx12Buffer(MapResourceState(InitState), ByteSize, Usage));
	}
	
}

TRefCountPtr<GpuSampler> Dx12GpuRhiBackend::CreateSampler(const GpuSamplerDesc &InSamplerDesc)
{
	return AUX::StaticCastRefCountPtr<GpuSampler>(CreateDx12Sampler(InSamplerDesc));
}

void Dx12GpuRhiBackend::SetResourceName(const FString& Name, GpuResource* InResource)
{
	if (InResource->GetType() == GpuResourceType::Texture)
	{
		static_cast<Dx12Texture*>(InResource)->GetResource()->SetName(*Name);
	}
	else if (InResource->GetType() == GpuResourceType::Buffer)
	{
		GpuBuffer* InBuffer = static_cast<GpuBuffer*>(InResource);
		GpuBufferUsage BufferUsage = InBuffer->GetUsage();
		if (BufferUsage == GpuBufferUsage::Static || BufferUsage == GpuBufferUsage::Dynamic || BufferUsage == GpuBufferUsage::Staging) {
			Dx12Buffer* Buffer = static_cast<Dx12Buffer*>(InBuffer);
			Buffer->GetAllocation().GetAllocationData().Get<CommonAllocationData>().UnderlyResource->SetName(*Name);
		}
	}
}

bool Dx12GpuRhiBackend::CrossCompileShader(GpuShader *InShader, FString &OutErrorInfo)
{
	return GShaderCompiler.Compile(static_cast<Dx12Shader *>(InShader), OutErrorInfo);
}


void Dx12GpuRhiBackend::Submit(const TArray<GpuCmdRecorder*>& CmdRecorders)
{
	TArray<ID3D12CommandList*> CmdLists;
	//The commandLists must be closed before executing them.
	for (auto CmdRecorder : CmdRecorders)
	{
		Dx12CmdRecorder* DxCmdRecorder = static_cast<Dx12CmdRecorder*>(CmdRecorder);
		ID3D12GraphicsCommandList* CmdList = DxCmdRecorder->GetCommandList();
		DxCheck(CmdList->Close());
		CmdLists.Add(CmdList);
	}

	GGraphicsQueue->ExecuteCommandLists(CmdLists.Num(), CmdLists.GetData());

	for (auto CmdRecorder : CmdRecorders)
	{
		static_cast<Dx12CmdRecorder*>(CmdRecorder)->MarkFinished();
	}
}

void* Dx12GpuRhiBackend::MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode, uint32& OutRowPitch)
{
	Dx12Texture* Texture = static_cast<Dx12Texture*>(InGpuTexture);
	void* Data{};
	if (InMapMode == GpuResourceMapMode::Write_Only) {
		const uint32 BufferSize = (uint32)GetRequiredIntermediateSize(Texture->GetResource(), 0, 1);
		if (!Texture->UploadBuffer) {
			Texture->UploadBuffer = CreateDx12Buffer(D3D12_RESOURCE_STATE_COPY_SOURCE, BufferSize, GpuBufferUsage::Dynamic);
		}
		Data = Texture->UploadBuffer->GetAllocation().GetCpuAddr();
		Texture->bIsMappingForWriting = true;
	}
	else if (InMapMode == GpuResourceMapMode::Read_Only) {
		const uint32 BufferSize = (uint32)GetRequiredIntermediateSize(Texture->GetResource(), 0, 1);
		if (!Texture->ReadBackBuffer) {
			Texture->ReadBackBuffer = CreateDx12Buffer(D3D12_RESOURCE_STATE_COPY_DEST, BufferSize, GpuBufferUsage::Staging);
		}
		Data = Texture->ReadBackBuffer->GetAllocation().GetCpuAddr();
		auto CmdRecorder = GDx12GpuRhi->BeginRecording();
		{
			GpuResourceState LastState = InGpuTexture->State;
			CmdRecorder->Barrier(InGpuTexture, GpuResourceState::CopySrc);
			CmdRecorder->CopyTextureToBuffer(InGpuTexture, Texture->ReadBackBuffer);
			//TODO: batch barriers on the api backend.
			CmdRecorder->Barrier(InGpuTexture, LastState);
		}
		GDx12GpuRhi->EndRecording(CmdRecorder);
		GDx12GpuRhi->Submit({ CmdRecorder });
		// To make sure ReadBackBuffer finished copying, so cpu can read the mapped data.
		GDx12GpuRhi->WaitGpu();
	}
	OutRowPitch = Align(InGpuTexture->GetWidth() * GetTextureFormatByteSize(InGpuTexture->GetFormat()), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	return Data;
}

void Dx12GpuRhiBackend::UnMapGpuTexture(GpuTexture* InGpuTexture)
{
	Dx12Texture* Texture = static_cast<Dx12Texture*>(InGpuTexture);
	if (Texture->bIsMappingForWriting) {
		auto CmdRecorder = GDx12GpuRhi->BeginRecording();
		{
			GpuResourceState LastState = InGpuTexture->State;
			CmdRecorder->Barrier(InGpuTexture, GpuResourceState::CopyDst);
			CmdRecorder->CopyBufferToTexture(Texture->UploadBuffer, Texture);
			CmdRecorder->Barrier(InGpuTexture, LastState);
		}
		GDx12GpuRhi->EndRecording(CmdRecorder);
		GDx12GpuRhi->Submit({ CmdRecorder });

		Texture->bIsMappingForWriting = false;
	}
}

void* Dx12GpuRhiBackend::MapGpuBuffer(GpuBuffer* InGpuBuffer, GpuResourceMapMode InMapMode)
{
	GpuBufferUsage Usage = InGpuBuffer->GetUsage();
	Dx12Buffer* Buffer = static_cast<Dx12Buffer*>(InGpuBuffer);
	void* Data = nullptr;

	if (EnumHasAnyFlags(Usage, GpuBufferUsage::Dynamic)) {
		check(InMapMode == GpuResourceMapMode::Write_Only);
		Data = Buffer->GetAllocation().GetCpuAddr();
	}
	else {
	}

	return Data;
}

void Dx12GpuRhiBackend::UnMapGpuBuffer(GpuBuffer* InGpuBuffer)
{
	// do nothing.
}

void Dx12GpuRhiBackend::BeginGpuCapture(const FString &CaptureName)
{
#if GPU_API_CAPTURE && USE_PIX
	if (GCanPIXCapture) {
        IFileManager::Get().MakeDirectory(*PathHelper::SavedCaptureDir());
		PIXCaptureParameters params = {};
		FString FileName = FString::Format(TEXT("{0}.wpix"), { PathHelper::SavedCaptureDir() / CaptureName });
		params.GpuCaptureParameters.FileName = *FileName;
		PIXBeginCapture(PIX_CAPTURE_GPU, &params);
	}
#endif
}

void Dx12GpuRhiBackend::EndGpuCapture()
{
#if GPU_API_CAPTURE && USE_PIX
	if (GCanPIXCapture) {
		PIXEndCapture(false);
	}
#endif
}

void *Dx12GpuRhiBackend::GetSharedHandle(GpuTexture *InGpuTexture)
{
	return static_cast<Dx12Texture *>(InGpuTexture)->GetSharedHandle();
}


GpuCmdRecorder* Dx12GpuRhiBackend::BeginRecording(const FString& RecorderName)
{
	return GDx12CmdRecorderPool.ObtainDxCmdRecorder(RecorderName);
}

void Dx12GpuRhiBackend::EndRecording(GpuCmdRecorder* InCmdRecorder)
{

}

}
