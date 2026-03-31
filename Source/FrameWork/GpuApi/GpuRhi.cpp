#include "CommonHeader.h"

#include "GpuRhi.h"
#include "Editor/Editor.h"

#if PLATFORM_WINDOWS
#include "./Dx12/Dx12GpuRhiBackend.h"
#include "./Vulkan/VkGpuRhiBackend.h"
#elif PLATFORM_MAC
#include "./Metal/MetalGpuRhiBackend.h"
#endif
#include "GpuApiValidation.h"
#include "GpuShader.h"
#include "GpuResourceHelper.h"

namespace FW
{

GpuResourceState GpuCmdRecorder::GetLocalTextureSubResourceState(GpuTexture* Texture, uint32 MipLevel, uint32 ArraySlice)
{
	if (auto* Found = LocalTextureStates.Find(Texture))
	{
		return (*Found)[Texture->CalcSubResourceIndex(MipLevel, ArraySlice)];
	}
	return Texture->GetSubResourceState(MipLevel, ArraySlice);
}

void GpuCmdRecorder::SetLocalTextureSubResourceState(GpuTexture* Texture, uint32 MipLevel, uint32 ArraySlice, GpuResourceState NewState)
{
	auto& States = LocalTextureStates.FindOrAdd(Texture);
	if (States.IsEmpty())
	{
		uint32 Num = Texture->GetNumSubResources();
		States.SetNum(Num);
		for (uint32 i = 0; i < Num; i++)
		{
			uint32 Mip = i % Texture->GetNumMips();
			uint32 Layer = i / Texture->GetNumMips();
			States[i] = Texture->GetSubResourceState(Mip, Layer);
		}
		// Record initial snapshot
		if (!InitialTextureStates.Contains(Texture))
		{
			InitialTextureStates.Add(Texture, States);
		}
	}
	States[Texture->CalcSubResourceIndex(MipLevel, ArraySlice)] = NewState;
}

void GpuCmdRecorder::SetLocalTextureAllSubResourceStates(GpuTexture* Texture, GpuResourceState NewState)
{
	auto& States = LocalTextureStates.FindOrAdd(Texture);
	if (States.IsEmpty() && !InitialTextureStates.Contains(Texture))
	{
		uint32 Num = Texture->GetNumSubResources();
		TArray<GpuResourceState> InitStates;
		InitStates.SetNum(Num);
		for (uint32 i = 0; i < Num; i++)
		{
			uint32 Mip = i % Texture->GetNumMips();
			uint32 Layer = i / Texture->GetNumMips();
			InitStates[i] = Texture->GetSubResourceState(Mip, Layer);
		}
		InitialTextureStates.Add(Texture, MoveTemp(InitStates));
	}
	States.SetNum(Texture->GetNumSubResources());
	for (auto& S : States) S = NewState;
}

GpuResourceState GpuCmdRecorder::GetLocalBufferState(GpuBuffer* Buffer)
{
	if (auto* Found = LocalBufferStates.Find(Buffer))
	{
		return *Found;
	}
	return Buffer->State;
}

void GpuCmdRecorder::SetLocalBufferState(GpuBuffer* Buffer, GpuResourceState NewState)
{
	if (!InitialBufferStates.Contains(Buffer))
	{
		InitialBufferStates.Add(Buffer, Buffer->State);
	}
	LocalBufferStates.FindOrAdd(Buffer) = NewState;
}

void GpuCmdRecorder::ResolveToGlobalStates()
{
	for (auto& [Texture, States] : LocalTextureStates)
	{
		uint32 Num = States.Num();
		for (uint32 i = 0; i < Num; i++)
		{
			uint32 Mip = i % Texture->GetNumMips();
			uint32 Layer = i / Texture->GetNumMips();
			Texture->SetSubResourceState(Mip, Layer, States[i]);
		}
	}
	for (auto& [Buffer, State] : LocalBufferStates)
	{
		Buffer->State = State;
	}
	LocalTextureStates.Empty();
	LocalBufferStates.Empty();
	InitialTextureStates.Empty();
	InitialBufferStates.Empty();
}

void GpuRhi::Submit(const TArray<GpuCmdRecorder*>& CmdRecorders)
{
	if (CmdRecorders.Num() <= 1)
	{
		SubmitInternal(CmdRecorders);
		CmdRecorders[0]->ResolveToGlobalStates();
		return;
	}

	// For multiple recorders, insert bridge barriers between adjacent recorders
	// to correct mismatched initial snapshots.
	TArray<GpuCmdRecorder*> FinalCmdRecorders;
	TArray<TRefCountPtr<GpuTextureView>> BridgeTextureViews;

	for (int32 i = 0; i < CmdRecorders.Num(); i++)
	{
		if (i > 0)
		{
			GpuCmdRecorder* Prev = CmdRecorders[i - 1];
			GpuCmdRecorder* Next = CmdRecorders[i];
			TArray<GpuBarrierInfo> BridgeBarriers;

			for (auto& [Texture, NextInitStates] : Next->GetInitialTextureStates())
			{
				uint32 NumMips = Texture->GetNumMips();
				uint32 Num = NextInitStates.Num();

				// Check if all initial states are uniform — if so, a single whole-texture barrier suffices
				bool bUniformInitState = true;
				for (uint32 j = 1; j < Num; j++)
				{
					if (NextInitStates[j] != NextInitStates[0]) { bUniformInitState = false; break; }
				}

				bool bHasMismatch = false;
				for (uint32 j = 0; j < Num; j++)
				{
					uint32 Mip = j % NumMips;
					uint32 Layer = j / NumMips;
					GpuResourceState PrevFinalState;
					auto* PrevLocalStates = Prev->GetLocalTextureStates().Find(Texture);
					if (PrevLocalStates)
					{
						PrevFinalState = (*PrevLocalStates)[j];
					}
					else
					{
						PrevFinalState = Texture->GetSubResourceState(Mip, Layer);
					}

					if (PrevFinalState != NextInitStates[j])
					{
						if (bUniformInitState)
						{
							BridgeBarriers.Add({.Resource = Texture, .NewState = NextInitStates[0]});
							break;
						}
						else
						{
							TRefCountPtr<GpuTextureView> BridgeView = CreateTextureView({
								.Texture = Texture, .BaseMipLevel = Mip, .MipLevelCount = 1,
								.BaseArrayLayer = Layer, .ArrayLayerCount = 1
							});
							BridgeBarriers.Add({.Resource = BridgeView.GetReference(), .NewState = NextInitStates[j]});
							BridgeTextureViews.Add(MoveTemp(BridgeView));
						}
					}
				}
			}

			for (auto& [Buffer, NextInitState] : Next->GetInitialBufferStates())
			{
				GpuResourceState PrevFinalState;
				auto* PrevLocalState = Prev->GetLocalBufferStates().Find(Buffer);
				if (PrevLocalState)
				{
					PrevFinalState = *PrevLocalState;
				}
				else
				{
					PrevFinalState = Buffer->State;
				}

				if (PrevFinalState != NextInitState)
				{
					BridgeBarriers.Add({.Resource = Buffer, .NewState = NextInitState});
				}
			}

			if (!BridgeBarriers.IsEmpty())
			{
				GpuCmdRecorder* BridgeRecorder = BeginRecording(TEXT("BridgeBarrier"));
				BridgeRecorder->Barriers(BridgeBarriers);
				EndRecording(BridgeRecorder);
				FinalCmdRecorders.Add(BridgeRecorder);
			}
		}
		FinalCmdRecorders.Add(CmdRecorders[i]);
	}

	SubmitInternal(FinalCmdRecorders);

	for (auto* Recorder : FinalCmdRecorders)
	{
		Recorder->ResolveToGlobalStates();
	}
}

TRefCountPtr<GpuTexture> GpuRhi::CreateTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState)
{
	//If the initial state is unknown, then the state be determined based on usage,
	//but it may not be possible to infer the state from some composite usage e.g. GpuTextureUsage::ShaderResource | GpuTextureUsage::RenderTarget
	GpuResourceState ValidState = InitState == GpuResourceState::Unknown ? GetTextureState(InTexDesc.Usage) : InitState;

	bool bAutoMipmap = InTexDesc.NumMips == 0;
	if (bAutoMipmap)
	{
		//InitialData for mip0
		check(!InTexDesc.InitialData.IsEmpty());
		check(EnumHasAllFlags(InTexDesc.Usage, GpuTextureUsage::ShaderResource | GpuTextureUsage::UnorderedAccess));
		GpuTextureDesc Desc = InTexDesc;
		uint32 MaxDim = FMath::Max(InTexDesc.Width, InTexDesc.Height);
		if (InTexDesc.Dimension == GpuTextureDimension::Tex3D)
			MaxDim = FMath::Max(MaxDim, InTexDesc.Depth);
		Desc.NumMips = FMath::FloorLog2(MaxDim) + 1;

		TRefCountPtr<GpuTexture> Texture = CreateTextureInternal(Desc, ValidState);
		GpuResourceHelper::GenerateMipmap(Texture);

		return Texture;
	}

	return CreateTextureInternal(InTexDesc, ValidState);
}

TRefCountPtr<GpuBuffer> GpuRhi::CreateBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState)
{
	GpuResourceState ValidState = InitState == GpuResourceState::Unknown ? GetBufferState(InBufferDesc.Usage) : InitState;
	return CreateBufferInternal(InBufferDesc, ValidState);
}

TRefCountPtr<GpuTextureView> GpuRhi::CreateTextureView(const GpuTextureViewDesc& InViewDesc)
{
	return CreateTextureViewInternal(InViewDesc);
}

TRefCountPtr<GpuShader> GpuRhi::CreateShaderFromSource(const GpuShaderSourceDesc& Desc) const
{
	return CreateShaderFromSourceInternal(Desc);
}

TRefCountPtr<GpuShader> GpuRhi::CreateShaderFromFile(const GpuShaderFileDesc& Desc)
{
	return CreateShaderFromFileInternal(Desc);
}

TRefCountPtr<GpuRenderPipelineState> GpuRhi::CreateRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc)
{
	return CreateRenderPipelineStateInternal(InPipelineStateDesc);
}

TRefCountPtr<GpuComputePipelineState> GpuRhi::CreateComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc)
{
	return CreateComputePipelineStateInternal(InPipelineStateDesc);
}

bool GpuRhi::CompileShader(GpuShader* InShader, FString& OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs)
{
	InShader->CompileExtraArgs = ExtraArgs;
	return CompileShaderInternal(InShader, OutErrorInfo, OutWarnInfo, ExtraArgs);
}

class GpuComputePassRecorderValidation : public GpuComputePassRecorder
{
public:
	GpuComputePassRecorderValidation(GpuComputePassRecorder* InRecorder) : PassRecorder(InRecorder) {}

public:
	void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) override
	{
		checkf(!IsEnd, TEXT("Can not record the command on the pass recorder that already ended."));
		PassRecorder->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	void SetComputePipelineState(GpuComputePipelineState* InPipelineState) override
	{
		checkf(!IsEnd, TEXT("Can not record the command on the pass recorder that already ended."));
		PassRecorder->SetComputePipelineState(InPipelineState);
	}

	void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3) override
	{
		checkf(!IsEnd, TEXT("Can not record the command on the pass recorder that already ended."));
		check(ValidateSetBindGroups(BindGroup0, BindGroup1, BindGroup2, BindGroup3));
		PassRecorder->SetBindGroups(BindGroup0, BindGroup1, BindGroup2, BindGroup3);
	}

	GpuComputePassRecorder* PassRecorder;
	bool IsEnd{};
};

class GpuRenderPassRecorderValidation : public GpuRenderPassRecorder
{
public:
	GpuRenderPassRecorderValidation(GpuRenderPassRecorder* InRecorder) : PassRecorder(InRecorder) {}

public:
	void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount) override
	{
		checkf(!IsEnd, TEXT("Can not record the command on the pass recorder that already ended."));
		PassRecorder->DrawPrimitive(StartVertexLocation, VertexCount, StartInstanceLocation, InstanceCount);
	}
	void DrawIndexed(uint32 StartIndexLocation, uint32 IndexCount, int32 BaseVertexLocation, uint32 StartInstanceLocation, uint32 InstanceCount) override
	{
		checkf(!IsEnd, TEXT("Can not record the command on the pass recorder that already ended."));
		PassRecorder->DrawIndexed(StartIndexLocation, IndexCount, BaseVertexLocation, StartInstanceLocation, InstanceCount);
	}
	void SetRenderPipelineState(GpuRenderPipelineState* InPipelineState) override
	{
		checkf(!IsEnd, TEXT("Can not record the command on the pass recorder that already ended."));
		PassRecorder->SetRenderPipelineState(InPipelineState);
	}
	void SetVertexBuffer(uint32 Slot, GpuBuffer* InVertexBuffer, uint32 Offset) override
	{
		checkf(!IsEnd, TEXT("Can not record the command on the pass recorder that already ended."));
		check(ValidateSetVertexBuffer(Slot, InVertexBuffer, Offset));
		PassRecorder->SetVertexBuffer(Slot, InVertexBuffer, Offset);
	}
	void SetIndexBuffer(GpuBuffer* InIndexBuffer, GpuFormat IndexFormat, uint32 Offset) override
	{
		checkf(!IsEnd, TEXT("Can not record the command on the pass recorder that already ended."));
		check(ValidateSetIndexBuffer(InIndexBuffer, IndexFormat, Offset));
		PassRecorder->SetIndexBuffer(InIndexBuffer, IndexFormat, Offset);
	}
	void SetViewPort(const GpuViewPortDesc& InViewPortDesc) override
	{
		checkf(!IsEnd, TEXT("Can not record the command on the pass recorder that already ended."));
		PassRecorder->SetViewPort(InViewPortDesc);
	}
	void SetScissorRect(const GpuScissorRectDesc& InScissorRectDes) override
	{
		checkf(!IsEnd, TEXT("Can not record the command on the pass recorder that already ended."));
		PassRecorder->SetScissorRect(InScissorRectDes);
	}
	void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3) override
	{
		checkf(!IsEnd, TEXT("Can not record the command on the pass recorder that already ended."));
		check(ValidateSetBindGroups(BindGroup0, BindGroup1, BindGroup2, BindGroup3));
		PassRecorder->SetBindGroups(BindGroup0, BindGroup1, BindGroup2, BindGroup3);
	}

	GpuRenderPassRecorder* PassRecorder;
	bool IsEnd{};
};

class GpuCmdRecorderValidation : public GpuCmdRecorder
{
public:
	GpuCmdRecorderValidation(const FString& InName, GpuCmdRecorder* InCmdRecorder) 
		: Name(InName) , CmdRecorder(InCmdRecorder) 
	{}

public:
	GpuComputePassRecorder* BeginComputePass(const FString& PassName) override
	{
		check(State == CmdRecorderState::Begin);
		auto PassRecorderValidation = MakeUnique<GpuComputePassRecorderValidation>(CmdRecorder->BeginComputePass(PassName));
		RequestedComputePassRecorders.Add(MoveTemp(PassRecorderValidation));
		return RequestedComputePassRecorders.Last().Get();
	}

	void EndComputePass(GpuComputePassRecorder* InComputePassRecorder) override
	{
		check(State == CmdRecorderState::Begin);
		GpuComputePassRecorderValidation* PassRecorderValidation = static_cast<GpuComputePassRecorderValidation*>(InComputePassRecorder);
		CmdRecorder->EndComputePass(PassRecorderValidation->PassRecorder);
		PassRecorderValidation->IsEnd = true;
	}

	GpuRenderPassRecorder* BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName) override
	{
		check(State == CmdRecorderState::Begin);
		check(ValidateBeginRenderPass(PassDesc));
		auto PassRecorderValidation = MakeUnique<GpuRenderPassRecorderValidation>(CmdRecorder->BeginRenderPass(PassDesc, PassName));
		RequestedRenderPassRecorders.Add(MoveTemp(PassRecorderValidation));
		return RequestedRenderPassRecorders.Last().Get();
	}

	void EndRenderPass(GpuRenderPassRecorder* InRenderPassRecorder) override
	{
		check(State == CmdRecorderState::Begin);
		GpuRenderPassRecorderValidation* PassRecorderValidation = static_cast<GpuRenderPassRecorderValidation*>(InRenderPassRecorder);
		CmdRecorder->EndRenderPass(PassRecorderValidation->PassRecorder);
		PassRecorderValidation->IsEnd = true;
	}

	void BeginCaptureEvent(const FString& EventName) override
	{
		CmdRecorder->BeginCaptureEvent(EventName);
	}

	void EndCaptureEvent() override
	{
		CmdRecorder->EndCaptureEvent();
	}

	void Barriers(const TArray<GpuBarrierInfo>& BarrierInfos) override
	{
		check(State == CmdRecorderState::Begin);
		check(ValidateBarriers(BarrierInfos));
		CmdRecorder->Barriers(BarrierInfos);
	}

	void CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture, uint32 ArrayLayer = 0, uint32 MipLevel = 0) override
	{
		check(State == CmdRecorderState::Begin);
		check(EnumHasAnyFlags(CmdRecorder->GetLocalBufferState(InBuffer), GpuResourceState::CopySrc) && EnumHasAnyFlags(CmdRecorder->GetLocalTextureSubResourceState(InTexture, MipLevel, ArrayLayer), GpuResourceState::CopyDst));
		CmdRecorder->CopyBufferToTexture(InBuffer, InTexture, ArrayLayer, MipLevel);
	}

	void CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer) override
	{
		check(State == CmdRecorderState::Begin);
		check(EnumHasAnyFlags(CmdRecorder->GetLocalBufferState(InBuffer), GpuResourceState::CopyDst) && EnumHasAnyFlags(CmdRecorder->GetLocalTextureSubResourceState(InTexture, 0, 0), GpuResourceState::CopySrc));
		CmdRecorder->CopyTextureToBuffer(InTexture, InBuffer);
	}

	void CopyBufferToBuffer(GpuBuffer* SrcBuffer, uint32 SrcOffset, GpuBuffer* DestBuffer, uint32 DestOffset, uint32 Size) override
	{
		check(State == CmdRecorderState::Begin);
		check(EnumHasAnyFlags(CmdRecorder->GetLocalBufferState(DestBuffer), GpuResourceState::CopyDst) && EnumHasAnyFlags(CmdRecorder->GetLocalBufferState(SrcBuffer), GpuResourceState::CopySrc));
		CmdRecorder->CopyBufferToBuffer(SrcBuffer, SrcOffset, DestBuffer, DestOffset, Size);
	}

	FString Name;
	GpuCmdRecorder* CmdRecorder;
	CmdRecorderState State;
	TArray<TUniquePtr<GpuRenderPassRecorderValidation>> RequestedRenderPassRecorders;
	TArray<TUniquePtr<GpuComputePassRecorderValidation>> RequestedComputePassRecorders;
};


// Rhi validation layer, check validation before dispatch commands to concrete api backend.
class GpuRhiValidation : public GpuRhi
{
public:
	GpuRhiValidation(TUniquePtr<GpuRhi> InRhiBackend)
		: RhiBackend(MoveTemp(InRhiBackend))
	{
	}

public:
	void InitApiEnv() override
	{
		RhiBackend->InitApiEnv();
	}

public:
	void WaitGpu() override
	{
		RhiBackend->WaitGpu();
	}

	void BeginFrame() override
	{
		RhiBackend->BeginFrame();
	}
	void EndFrame() override
	{
		RhiBackend->EndFrame();
		RequestedCmdRecorders.Empty();
	}

	TRefCountPtr<GpuTexture> CreateTextureInternal(const GpuTextureDesc &InTexDesc, GpuResourceState InitState) override
	{
		check(ValidateCreateTexture(InTexDesc, InitState));
		return RhiBackend->CreateTextureInternal(InTexDesc, InitState);
	}

	TRefCountPtr<GpuBuffer> CreateBufferInternal(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState) override
	{
		check(ValidateCreateBuffer(InBufferDesc, InitState));
		return RhiBackend->CreateBufferInternal(InBufferDesc, InitState);
	}

	TRefCountPtr<GpuTextureView> CreateTextureViewInternal(const GpuTextureViewDesc& InViewDesc) override
	{
		return RhiBackend->CreateTextureViewInternal(InViewDesc);
	}

	TRefCountPtr<GpuShader> CreateShaderFromSourceInternal(const GpuShaderSourceDesc& Desc) const override
	{
		return RhiBackend->CreateShaderFromSourceInternal(Desc);
	}

	TRefCountPtr<GpuShader> CreateShaderFromFileInternal(const GpuShaderFileDesc& Desc) override
	{
		checkf(IFileManager::Get().FileExists(*Desc.FileName), TEXT("Can not find the shader file:%s"), *Desc.FileName);
		return RhiBackend->CreateShaderFromFileInternal(Desc);
	}

	TRefCountPtr<GpuBindGroup> CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc) override
	{
		check(ValidateCreateBindGroup(InBindGroupDesc));
		return RhiBackend->CreateBindGroup(InBindGroupDesc);
	}

	TRefCountPtr<GpuBindGroupLayout> CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc) override
	{
		check(ValidateCreateBindGroupLayout(InBindGroupLayoutDesc));
		return RhiBackend->CreateBindGroupLayout(InBindGroupLayoutDesc);
	}

	TRefCountPtr<GpuRenderPipelineState> CreateRenderPipelineStateInternal(const GpuRenderPipelineStateDesc& InPipelineStateDesc) override
	{
		check(ValidateCreateRenderPipelineState(InPipelineStateDesc));
		return RhiBackend->CreateRenderPipelineStateInternal(InPipelineStateDesc);
	}

	TRefCountPtr<GpuComputePipelineState> CreateComputePipelineStateInternal(const GpuComputePipelineStateDesc& InPipelineStateDesc) override
	{
		return RhiBackend->CreateComputePipelineStateInternal(InPipelineStateDesc);
	}

	TRefCountPtr<GpuSampler> CreateSampler(const GpuSamplerDesc &InSamplerDesc) override
	{
		return RhiBackend->CreateSampler(InSamplerDesc);
	}

	void SetResourceName(const FString& Name, GpuResource* InResource) override
	{
		RhiBackend->SetResourceName(Name, InResource);
	}

	bool CompileShaderInternal(GpuShader *InShader, FString &OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs) override
	{
		return RhiBackend->CompileShader(InShader, OutErrorInfo, OutWarnInfo, ExtraArgs);
	}

	void BeginGpuCapture(const FString &CaptureName) override
	{
		RhiBackend->BeginGpuCapture(CaptureName);
	}

	void EndGpuCapture() override
	{
		RhiBackend->EndGpuCapture();
	}

	void *GetSharedHandle(GpuTexture *InGpuTexture) override
	{
		return RhiBackend->GetSharedHandle(InGpuTexture);
	}

	GpuCmdRecorder* BeginRecording(const FString& RecorderName) override
	{
		auto CmdRecorder = MakeUnique<GpuCmdRecorderValidation>(RecorderName, RhiBackend->BeginRecording(RecorderName));
		CmdRecorder->State = CmdRecorderState::Begin;
		RequestedCmdRecorders.Add(MoveTemp(CmdRecorder));
		return RequestedCmdRecorders.Last().Get();
	}

	void EndRecording(GpuCmdRecorder* InCmdRecorder) override
	{
		GpuCmdRecorderValidation* CmdRecorderValidation = static_cast<GpuCmdRecorderValidation*>(InCmdRecorder);
		RhiBackend->EndRecording(CmdRecorderValidation->CmdRecorder);
		CmdRecorderValidation->State = CmdRecorderState::End;
	}

	void SubmitInternal(const TArray<GpuCmdRecorder*>& CmdRecorders) override
	{
		TArray<GpuCmdRecorder*> RealCmdRecorders;
		for (int Index = 0; Index < CmdRecorders.Num(); Index++)
		{
			GpuCmdRecorderValidation* CmdRecorderValidation = static_cast<GpuCmdRecorderValidation*>(CmdRecorders[Index]);
			checkf(CmdRecorderValidation->State == CmdRecorderState::End, 
				*FString::Printf(TEXT("Error State:%s for the GpuCmdRecorder(%d) %s being submitted. Note that do not submit a CmdRecorder that has been submitted or not properly ended."), 
					ANSI_TO_TCHAR(magic_enum::enum_name(CmdRecorderValidation->State).data()), Index, *CmdRecorderValidation->Name)
			);
			CmdRecorderValidation->State = CmdRecorderState::Finish;
			RealCmdRecorders.Add(CmdRecorderValidation->CmdRecorder);
		}
		RhiBackend->Submit(MoveTemp(RealCmdRecorders));
	}

	void* MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode, uint32& OutRowPitch) override
	{
		return RhiBackend->MapGpuTexture(InGpuTexture, InMapMode, OutRowPitch);
	}

	void UnMapGpuTexture(GpuTexture* InGpuTexture) override
	{
		RhiBackend->UnMapGpuTexture(InGpuTexture);
	}

	void* MapGpuBuffer(GpuBuffer* InGpuBuffer, GpuResourceMapMode InMapMode) override
	{
		return RhiBackend->MapGpuBuffer(InGpuBuffer, InMapMode);
	}

	void UnMapGpuBuffer(GpuBuffer* InGpuBuffer) override
	{
		RhiBackend->UnMapGpuBuffer(InGpuBuffer);
	}

	TRefCountPtr<GpuQuerySet> CreateQuerySet(uint32 Count) override
	{
		return RhiBackend->CreateQuerySet(Count);
	}

private:
	TUniquePtr<GpuRhi> RhiBackend;
	TArray<TUniquePtr<GpuCmdRecorderValidation>> RequestedCmdRecorders;
};

void GpuRhi::InitGpuRhi(const GpuRhiConfig &InConfig)
{
	TUniquePtr<GpuRhi> RhiBackend;
	switch (InConfig.BackendType) {
#if PLATFORM_WINDOWS
	case GpuRhiBackendType::DX12: RhiBackend = MakeUnique<Dx12GpuRhiBackend>(); break;
	case GpuRhiBackendType::Vulkan: RhiBackend = MakeUnique<VkGpuRhiBackend>(); break;
#elif PLATFORM_MAC
	case GpuRhiBackendType::Metal: RhiBackend = MakeUnique<MetalGpuRhiBackend>(); break;
#endif
	default:
		AUX::Unreachable();
	}

	if (InConfig.EnableValidationCheck) {
		GGpuRhi = MakeUnique<GpuRhiValidation>(MoveTemp(RhiBackend));
	} else {
		GGpuRhi = MoveTemp(RhiBackend);
	}
}

GpuCmdRecorder* GGpuCmdRecorder;
TUniquePtr<GpuRhi> GGpuRhi;
GpuRhiBackendType GetGpuRhiBackendType()
{
	static GpuRhiBackendType BackendType = [] {
	#if PLATFORM_WINDOWS
		FString BackendName = TEXT("DX12");
	#elif PLATFORM_MAC
		FString BackendName = TEXT("Metal");
	#endif 
		auto Config = MakeUnique<FConfigFile>();
		Config->Read(EditorConfigPath());
		if (IFileManager::Get().FileExists(*EditorConfigPath()))
		{
			Config->GetString(TEXT("Environment"), TEXT("GraphicsApi"), BackendName);
		}
		
		ON_SCOPE_EXIT
		{
			Config->SetString(TEXT("Environment"), TEXT("GraphicsApi"), *BackendName);
			Config->Write(EditorConfigPath());
		};
	#if PLATFORM_WINDOWS
		if (BackendName.Equals(TEXT("DX12"), ESearchCase::IgnoreCase))
		{
			return GpuRhiBackendType::DX12;
		}
		else if (BackendName.Equals(TEXT("Vulkan"), ESearchCase::IgnoreCase))
		{
			return GpuRhiBackendType::Vulkan;
		}
	#endif
	#if PLATFORM_MAC
		if (BackendName.Equals(TEXT("Metal"), ESearchCase::IgnoreCase))
		{
			return GpuRhiBackendType::Metal;
		}
	#endif 
		else
		{
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, *FString::Printf(TEXT("Invalid graphics backend:%s"), *BackendName), TEXT("Error:"));
			std::_Exit(0);
		}
	}();
	return BackendType;
}

bool ShowTimestampMs()
{
	bool bShow = true;
	Editor::GetEditorConfig()->GetBool(TEXT("Environment"), TEXT("ShowTimestamp"), bShow);
	return bShow;
}
}
