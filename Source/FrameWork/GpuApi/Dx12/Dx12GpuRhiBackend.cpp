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
	CleanupShaderCache();
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
	GDx12CmdRecorderPool.EndFrame();
	
	CurGpuFrame = CpuSyncGpuFence->GetCompletedValue();
	const uint64 CurLag = CurCpuFrame - CurGpuFrame;
	if (CurLag > AllowableLag) {
		// Cpu is waiting for gpu to catch up a frame.
		DxCheck(CpuSyncGpuFence->SetEventOnCompletion(CurGpuFrame + 1, CpuSyncGpuEvent));
		WaitForSingleObject(CpuSyncGpuEvent, INFINITE);
		CurGpuFrame = CurGpuFrame + 1;
	}

	GDx12DeferredReleaseManager->ProcessResources();
}

TRefCountPtr<GpuTexture> Dx12GpuRhiBackend::CreateTextureInternal(const GpuTextureDesc& InTexDesc, GpuResourceState InitState)
{
	return AUX::StaticCastRefCountPtr<GpuTexture>(CreateDx12Texture2D(InTexDesc, InitState));
}

TRefCountPtr<GpuTextureView> Dx12GpuRhiBackend::CreateTextureViewInternal(const GpuTextureViewDesc& InViewDesc)
{
	Dx12Texture* DxTexture = static_cast<Dx12Texture*>(InViewDesc.Texture);
	TUniquePtr<CpuDescriptor> SRV;
	TUniquePtr<CpuDescriptor> RTV;

	if (EnumHasAnyFlags(InViewDesc.Texture->GetResourceDesc().Usage, GpuTextureUsage::ShaderResource))
	{
		SRV = AllocCpuCbvSrvUav();
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc{};
		SrvDesc.Format = DxTexture->GetResource()->GetDesc().Format;
		SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		if (InViewDesc.Texture->GetResourceDesc().Dimension == GpuTextureDimension::TexCube)
		{
			SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			SrvDesc.TextureCube.MostDetailedMip = InViewDesc.BaseMipLevel;
			SrvDesc.TextureCube.MipLevels = InViewDesc.MipLevelCount;
			SrvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		}
		else
		{
			SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			SrvDesc.Texture2D.MostDetailedMip = InViewDesc.BaseMipLevel;
			SrvDesc.Texture2D.MipLevels = InViewDesc.MipLevelCount;
		}
		GDevice->CreateShaderResourceView(DxTexture->GetResource(), &SrvDesc, SRV->GetHandle());
	}

	if (EnumHasAnyFlags(InViewDesc.Texture->GetResourceDesc().Usage, GpuTextureUsage::RenderTarget))
	{
		RTV = AllocRtv();
		D3D12_RENDER_TARGET_VIEW_DESC RtvDesc{};
		RtvDesc.Format = DxTexture->GetResource()->GetDesc().Format;
		RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		RtvDesc.Texture2D.MipSlice = InViewDesc.BaseMipLevel;
		GDevice->CreateRenderTargetView(DxTexture->GetResource(), &RtvDesc, RTV->GetHandle());
	}

	GpuTextureViewDesc ViewDesc = InViewDesc;
	return new Dx12TextureView(MoveTemp(ViewDesc), MoveTemp(SRV), MoveTemp(RTV));
}

TRefCountPtr<GpuShader> Dx12GpuRhiBackend::CreateShaderFromSourceInternal(const GpuShaderSourceDesc& Desc) const
{
	TRefCountPtr<Dx12Shader> NewShader = new Dx12Shader(Desc);
	return AUX::StaticCastRefCountPtr<GpuShader>(NewShader);
}

TRefCountPtr<GpuShader> Dx12GpuRhiBackend::CreateShaderFromFileInternal(const GpuShaderFileDesc& Desc)
{
	TRefCountPtr<Dx12Shader> NewShader = new Dx12Shader(Desc);
	return AUX::StaticCastRefCountPtr<GpuShader>(NewShader);
}

TRefCountPtr<GpuBindGroup> Dx12GpuRhiBackend::CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc)
{
	return AUX::StaticCastRefCountPtr<GpuBindGroup>(CreateDx12BindGroup(InBindGroupDesc));
}

TRefCountPtr<GpuBindGroupLayout> Dx12GpuRhiBackend::CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc)
{
	return AUX::StaticCastRefCountPtr<GpuBindGroupLayout>(CreateDx12BindGroupLayout(InBindGroupLayoutDesc));
}

TRefCountPtr<GpuRenderPipelineState> Dx12GpuRhiBackend::CreateRenderPipelineStateInternal(const GpuRenderPipelineStateDesc& InPipelineStateDesc)
{
	return AUX::StaticCastRefCountPtr<GpuRenderPipelineState>(CreateDx12RenderPso(InPipelineStateDesc));
}

TRefCountPtr<GpuComputePipelineState> Dx12GpuRhiBackend::CreateComputePipelineStateInternal(const GpuComputePipelineStateDesc& InPipelineStateDesc)
{
	return AUX::StaticCastRefCountPtr<GpuComputePipelineState>(CreateDx12ComputePso(InPipelineStateDesc));
}

TRefCountPtr<GpuBuffer> Dx12GpuRhiBackend::CreateBufferInternal(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState)
{
	return AUX::StaticCastRefCountPtr<GpuBuffer>(CreateDx12Buffer(InBufferDesc, InitState));
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
		Dx12Buffer* Buffer = static_cast<Dx12Buffer*>(InBuffer);
		if (Buffer->GetAllocation().GetPolicy() != AllocationPolicy::SubAllocate)
		{
			Buffer->GetAllocation().GetResource()->SetName(*Name);
		}
	}
}

bool Dx12GpuRhiBackend::CompileShader(GpuShader *InShader, FString &OutErrorInfo, FString &OutWarnInfo, const TArray<FString>& ExtraArgs)
{
	return GShaderCompiler.Compile(static_cast<Dx12Shader *>(InShader), OutErrorInfo, OutWarnInfo, ExtraArgs);
}


void Dx12GpuRhiBackend::Submit(const TArray<GpuCmdRecorder*>& CmdRecorders)
{
	TArray<ID3D12CommandList*> CmdLists;
	for (auto CmdRecorder : CmdRecorders)
	{
		Dx12CmdRecorder* DxCmdRecorder = static_cast<Dx12CmdRecorder*>(CmdRecorder);
		ID3D12GraphicsCommandList* CmdList = DxCmdRecorder->GetCommandList();
		CmdLists.Add(CmdList);
	}

	GGraphicsQueue->ExecuteCommandLists(CmdLists.Num(), CmdLists.GetData());

	for (auto CmdRecorder : CmdRecorders)
	{
		static_cast<Dx12CmdRecorder*>(CmdRecorder)->MarkSubmitted();
	}
}

void* Dx12GpuRhiBackend::MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode, uint32& OutRowPitch)
{
	Dx12Texture* Texture = static_cast<Dx12Texture*>(InGpuTexture);
	void* Data{};
	if (InMapMode == GpuResourceMapMode::Write_Only) {
		const uint32 BufferSize = (uint32)GetRequiredIntermediateSize(Texture->GetResource(), 0, 1);
		if (!Texture->UploadBuffer) {
			Texture->UploadBuffer = CreateDx12Buffer({ BufferSize, GpuBufferUsage::Upload }, GpuResourceState::CopySrc);
		}
		Data = Texture->UploadBuffer->GetAllocation().GetCpuAddr();
		Texture->bIsMappingForWriting = true;
	}
	else if (InMapMode == GpuResourceMapMode::Read_Only) {
		const uint32 BufferSize = (uint32)GetRequiredIntermediateSize(Texture->GetResource(), 0, 1);
		if (!Texture->ReadBackBuffer) {
			Texture->ReadBackBuffer = CreateDx12Buffer({ BufferSize, GpuBufferUsage::ReadBack }, GpuResourceState::CopyDst);
		}
		Data = Texture->ReadBackBuffer->GetAllocation().GetCpuAddr();
		auto CmdRecorder = GDx12GpuRhi->BeginRecording();
		{
			GpuResourceState LastState = InGpuTexture->GetSubResourceState(0, 0);
			CmdRecorder->Barriers({
				{ InGpuTexture, GpuResourceState::CopySrc } 
			});
			CmdRecorder->CopyTextureToBuffer(InGpuTexture, Texture->ReadBackBuffer);
			CmdRecorder->Barriers({
				{ InGpuTexture, LastState } 
			});
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
			GpuResourceState LastState = InGpuTexture->GetSubResourceState(0, 0);
			CmdRecorder->Barriers({
				{InGpuTexture, GpuResourceState::CopyDst} 
			});
			CmdRecorder->CopyBufferToTexture(Texture->UploadBuffer, Texture);
			CmdRecorder->Barriers({ 
				{InGpuTexture, LastState} 
			});
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

	if (EnumHasAnyFlags(Usage, GpuBufferUsage::DynamicMask))
	{
		Data = Buffer->GetAllocation().GetCpuAddr();
	}
	else
	{
		if (InMapMode == GpuResourceMapMode::Read_Only)
		{
			Buffer->ReadBackBuffer = CreateDx12Buffer({ Buffer->GetByteSize(), GpuBufferUsage::ReadBack}, GpuResourceState::CopyDst);
			auto CmdRecorder = GDx12GpuRhi->BeginRecording();
			{
				GpuResourceState LastState = InGpuBuffer->State;
				CmdRecorder->Barriers({
					{ InGpuBuffer, GpuResourceState::CopySrc },
				});
				CmdRecorder->CopyBufferToBuffer(InGpuBuffer, 0, Buffer->ReadBackBuffer, 0, Buffer->GetByteSize());
				CmdRecorder->Barriers({
					{ InGpuBuffer, LastState }
				});
			}
			GDx12GpuRhi->EndRecording(CmdRecorder);
			GDx12GpuRhi->Submit({ CmdRecorder });
			GDx12GpuRhi->WaitGpu();

			Data = Buffer->ReadBackBuffer->GetAllocation().GetCpuAddr();
		}
	}

	return Data;
}

void Dx12GpuRhiBackend::UnMapGpuBuffer(GpuBuffer* InGpuBuffer)
{
	Dx12Buffer* Buffer = static_cast<Dx12Buffer*>(InGpuBuffer);
	Buffer->ReadBackBuffer = nullptr;
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
	//The commandLists must be closed before executing them.
	Dx12CmdRecorder* DxCmdRecorder = static_cast<Dx12CmdRecorder*>(InCmdRecorder);
	DxCmdRecorder->End();
}

}
