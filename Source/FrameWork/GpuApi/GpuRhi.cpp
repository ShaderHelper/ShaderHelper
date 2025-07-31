#include "CommonHeader.h"

#include "GpuRhi.h"

#if PLATFORM_WINDOWS
#include "./Dx12/Dx12GpuRhiBackend.h"
#elif PLATFORM_MAC
#include "./Metal/MetalGpuRhiBackend.h"
#endif
#include "GpuApiValidation.h"

namespace FW
{

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
	void SetRenderPipelineState(GpuRenderPipelineState* InPipelineState) override
	{
		checkf(!IsEnd, TEXT("Can not record the command on the pass recorder that already ended."));
		PassRecorder->SetRenderPipelineState(InPipelineState);
	}
	void SetVertexBuffer(GpuBuffer* InVertexBuffer) override
	{
		checkf(!IsEnd, TEXT("Can not record the command on the pass recorder that already ended."));
		PassRecorder->SetVertexBuffer(InVertexBuffer);
	}
	void SetViewPort(const GpuViewPortDesc& InViewPortDesc) override
	{
		checkf(!IsEnd, TEXT("Can not record the command on the pass recorder that already ended."));
		PassRecorder->SetViewPort(InViewPortDesc);
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

	void CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture) override
	{
		check(State == CmdRecorderState::Begin);
		check(EnumHasAnyFlags(InBuffer->State, GpuResourceState::CopySrc) && EnumHasAnyFlags(InTexture->State, GpuResourceState::CopyDst));
		CmdRecorder->CopyBufferToTexture(InBuffer, InTexture);
	}

	void CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer) override
	{
		check(State == CmdRecorderState::Begin);
		check(EnumHasAnyFlags(InBuffer->State, GpuResourceState::CopyDst) && EnumHasAnyFlags(InTexture->State, GpuResourceState::CopySrc));
		CmdRecorder->CopyTextureToBuffer(InTexture, InBuffer);
	}

	void CopyBufferToBuffer(GpuBuffer* SrcBuffer, uint32 SrcOffset, GpuBuffer* DestBuffer, uint32 DestOffset, uint32 Size) override
	{
		check(State == CmdRecorderState::Begin);
		check(EnumHasAnyFlags(DestBuffer->State, GpuResourceState::CopyDst) && EnumHasAnyFlags(SrcBuffer->State, GpuResourceState::CopySrc));
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

	TRefCountPtr<GpuTexture> CreateTexture(const GpuTextureDesc &InTexDesc, GpuResourceState InitState) override
	{
		check(ValidateCreateTexture(InTexDesc, InitState));
		return RhiBackend->CreateTexture(InTexDesc, InitState);
	}

	TRefCountPtr<GpuBuffer> CreateBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState) override
	{
		check(ValidateCreateBuffer(InBufferDesc, InitState));
		return RhiBackend->CreateBuffer(InBufferDesc);
	}

	TRefCountPtr<GpuShader> CreateShaderFromSource(const GpuShaderSourceDesc& Desc) const override
	{
		return RhiBackend->CreateShaderFromSource(Desc);
	}

	TRefCountPtr<GpuShader> CreateShaderFromFile(const GpuShaderFileDesc& Desc) override
	{
		return RhiBackend->CreateShaderFromFile(Desc);
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

	TRefCountPtr<GpuRenderPipelineState> CreateRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc) override
	{
		check(ValidateCreateRenderPipelineState(InPipelineStateDesc));
		return RhiBackend->CreateRenderPipelineState(InPipelineStateDesc);
	}

	TRefCountPtr<GpuComputePipelineState> CreateComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc)
	{
		return RhiBackend->CreateComputePipelineState(InPipelineStateDesc);
	}

	TRefCountPtr<GpuSampler> CreateSampler(const GpuSamplerDesc &InSamplerDesc) override
	{
		return RhiBackend->CreateSampler(InSamplerDesc);
	}

	void SetResourceName(const FString& Name, GpuResource* InResource) override
	{
		RhiBackend->SetResourceName(Name, InResource);
	}

	bool CompileShader(GpuShader *InShader, FString &OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs) override
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

	void Submit(const TArray<GpuCmdRecorder*>& CmdRecorders) override
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

private:
	TUniquePtr<GpuRhi> RhiBackend;
	TArray<TUniquePtr<GpuCmdRecorderValidation>> RequestedCmdRecorders;
};

bool GpuRhi::InitGpuRhi(const GpuRhiConfig &InConfig)
{
	TUniquePtr<GpuRhi> RhiBackend;
	switch (InConfig.BackendType) {
#if PLATFORM_WINDOWS
	case GpuRhiBackendType::DX12: RhiBackend = MakeUnique<Dx12GpuRhiBackend>(); break;
#elif PLATFORM_MAC
	case GpuRhiBackendType::Metal: RhiBackend = MakeUnique<MetalGpuRhiBackend>(); break;
#endif
	case GpuRhiBackendType::Vulkan:
		// TODO: create vulkan backends.
		unimplemented();
		break;
	default: // unreachable
		checkf(false, TEXT("Invalid GpuApiBackendType."));
		return false;
	}

	if (InConfig.EnableValidationCheck) {
		GGpuRhi = MakeUnique<GpuRhiValidation>(MoveTemp(RhiBackend));
	} else {
		GGpuRhi = MoveTemp(RhiBackend);
	}
	return true;
}

GpuCmdRecorder* GGpuCmdRecorder;
TUniquePtr<GpuRhi> GGpuRhi;
}
