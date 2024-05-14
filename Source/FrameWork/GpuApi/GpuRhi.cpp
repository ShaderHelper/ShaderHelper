#include "CommonHeader.h"

#include "GpuRhi.h"

#if PLATFORM_WINDOWS
#include "./Dx12/Dx12GpuRhiBackend.h"
#elif PLATFORM_MAC
#include "./Metal/MetalGpuRhiBackend.h"
#endif
#include "GpuApiValidation.h"

namespace FRAMEWORK
{
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
	void FlushGpu() override
	{
		RhiBackend->FlushGpu();
	}

	void BeginFrame() override
	{
		RhiBackend->BeginFrame();
	}
	void EndFrame() override
	{
		RhiBackend->EndFrame();
	}

	TRefCountPtr<GpuTexture> CreateTexture(const GpuTextureDesc &InTexDesc) override
	{
		return RhiBackend->CreateTexture(InTexDesc);
	}

	TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName, FString EntryPoint) override
	{
		return RhiBackend->CreateShaderFromSource(InType, InSourceText, InShaderName, EntryPoint);
	}

	TRefCountPtr<GpuShader> CreateShaderFromFile(FString FileName, ShaderType InType, FString EntryPoint, FString ExtraDeclaration) override
	{
		return RhiBackend->CreateShaderFromFile(FileName, InType, EntryPoint, ExtraDeclaration);
	}

	TRefCountPtr<GpuBindGroup> CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc) override
	{
		check(GpuApi::ValidateCreateBindGroup(InBindGroupDesc));
		return RhiBackend->CreateBindGroup(InBindGroupDesc);
	}

	TRefCountPtr<GpuBindGroupLayout> CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc) override
	{
		check(GpuApi::ValidateCreateBindGroupLayout(InBindGroupLayoutDesc));
		return RhiBackend->CreateBindGroupLayout(InBindGroupLayoutDesc);
	}

	TRefCountPtr<GpuPipelineState> CreateRenderPipelineState(const GpuPipelineStateDesc &InPipelineStateDesc) override
	{
		check(GpuApi::ValidateCreateRenderPipelineState(InPipelineStateDesc));
		return RhiBackend->CreateRenderPipelineState(InPipelineStateDesc);
	}

	TRefCountPtr<GpuBuffer> CreateBuffer(uint32 ByteSize, GpuBufferUsage Usage) override
	{
		return RhiBackend->CreateBuffer(ByteSize, Usage);
	}

	TRefCountPtr<GpuSampler> CreateSampler(const GpuSamplerDesc &InSamplerDesc) override
	{
		return RhiBackend->CreateSampler(InSamplerDesc);
	}

	void SetTextureName(const FString &TexName, GpuTexture *InTexture) override
	{
		RhiBackend->SetTextureName(TexName, InTexture);
	}

	void SetBufferName(const FString &BufferName, GpuBuffer *InBuffer) override
	{
		RhiBackend->SetBufferName(BufferName, InBuffer);
	}

	void *MapGpuTexture(GpuTexture *InGpuTexture, GpuResourceMapMode InMapMode, uint32 &OutRowPitch) override
	{
		return RhiBackend->MapGpuTexture(InGpuTexture, InMapMode, OutRowPitch);
	}

	void UnMapGpuTexture(GpuTexture *InGpuTexture) override
	{
		RhiBackend->UnMapGpuTexture(InGpuTexture);
	}

	void *MapGpuBuffer(GpuBuffer *InGpuBuffer, GpuResourceMapMode InMapMode) override
	{
		return RhiBackend->MapGpuBuffer(InGpuBuffer, InMapMode);
	}

	void UnMapGpuBuffer(GpuBuffer *InGpuBuffer) override
	{
		RhiBackend->UnMapGpuBuffer(InGpuBuffer);
	}

	bool CrossCompileShader(GpuShader *InShader, FString &OutErrorInfo) override
	{
		return RhiBackend->CrossCompileShader(InShader, OutErrorInfo);
	}

	void SetRenderPipelineState(GpuPipelineState *InPipelineState) override
	{
		RhiBackend->SetRenderPipelineState(InPipelineState);
	}

	void SetVertexBuffer(GpuBuffer *InVertexBuffer) override
	{
		RhiBackend->SetVertexBuffer(InVertexBuffer);
	}

	void SetViewPort(const GpuViewPortDesc &InViewPortDesc) override
	{
		RhiBackend->SetViewPort(InViewPortDesc);
	}

	void SetBindGroups(GpuBindGroup *BindGroup0, GpuBindGroup *BindGroup1, GpuBindGroup *BindGroup2, GpuBindGroup *BindGroup3) override
	{
		check(GpuApi::ValidateSetBindGroups(BindGroup0, BindGroup1, BindGroup2, BindGroup3));
		RhiBackend->SetBindGroups(BindGroup0, BindGroup1, BindGroup2, BindGroup3);
	}

	void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount) override
	{
		RhiBackend->DrawPrimitive(StartInstanceLocation, VertexCount, StartInstanceLocation, InstanceCount);
	}

	void Submit() override
	{
		RhiBackend->Submit();
	}

	void BeginGpuCapture(const FString &SavedFileName) override
	{
		RhiBackend->BeginGpuCapture(SavedFileName);
	}

	void EndGpuCapture() override
	{
		RhiBackend->EndGpuCapture();
	}

	void BeginCaptureEvent(const FString &EventName) override
	{
		RhiBackend->BeginCaptureEvent(EventName);
	}

	void EndCaptureEvent() override
	{
		RhiBackend->EndCaptureEvent();
	}

	void *GetSharedHandle(GpuTexture *InGpuTexture) override
	{
		return RhiBackend->GetSharedHandle(InGpuTexture);
	}

	void BeginRenderPass(const GpuRenderPassDesc &PassDesc, const FString &PassName) override
	{
		RhiBackend->BeginRenderPass(PassDesc, PassName);
	}

	void EndRenderPass() override
	{
		RhiBackend->EndRenderPass();
	}

private:
	TUniquePtr<GpuRhi> RhiBackend;
};

TUniquePtr<GpuRhi> GpuRhi::CreateGpuRhi(const GpuRhiConfig &InConfig)
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
		break;
	}

	if (InConfig.EnableValidationCheck) {
		return MakeUnique<GpuRhiValidation>(MoveTemp(RhiBackend));
	} else {
		return RhiBackend;
	}
}

}
