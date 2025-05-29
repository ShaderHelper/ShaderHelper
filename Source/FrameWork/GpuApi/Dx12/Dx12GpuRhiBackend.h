#pragma once

#include "GpuApi/GpuRhi.h"
#include "Dx12CommandRecorder.h"

namespace FW
{
class Dx12GpuRhiBackend : public GpuRhi
{
public:
	Dx12GpuRhiBackend();
	virtual ~Dx12GpuRhiBackend();

public:
	void InitApiEnv() override;
	void WaitGpu() override;
	void BeginFrame() override;
	void EndFrame() override;
	TRefCountPtr<GpuTexture> CreateTexture(const GpuTextureDesc &InTexDesc, GpuResourceState InitState) override;
	TRefCountPtr<GpuBuffer> CreateBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState) override;
	TRefCountPtr<GpuShader> CreateShaderFromSource(const GpuShaderSourceDesc& Desc) const override;
	TRefCountPtr<GpuShader> CreateShaderFromFile(const GpuShaderFileDesc& Desc) override;
	TRefCountPtr<GpuBindGroup> CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc) override;
	TRefCountPtr<GpuBindGroupLayout> CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc) override;
	TRefCountPtr<GpuRenderPipelineState> CreateRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc) override;
	TRefCountPtr<GpuComputePipelineState> CreateComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc) override;
	TRefCountPtr<GpuSampler> CreateSampler(const GpuSamplerDesc &InSamplerDesc) override;
	void SetResourceName(const FString& Name, GpuResource* InResource) override;
	bool CompileShader(GpuShader *InShader, FString &OutErrorInfo, const TArray<FString>& Definitions) override;
	void BeginGpuCapture(const FString &CaptureName) override;
	void EndGpuCapture() override;
	void *GetSharedHandle(GpuTexture *InGpuTexture) override;
	GpuCmdRecorder* BeginRecording(const FString& RecorderName = {}) override;
	void EndRecording(GpuCmdRecorder* InCmdRecorder) override;
	void Submit(const TArray<GpuCmdRecorder*>& CmdRecorders) override;
	virtual void* MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode, uint32& OutRowPitch) override;
	virtual void UnMapGpuTexture(GpuTexture* InGpuTexture) override;
	virtual void* MapGpuBuffer(GpuBuffer* InGpuBuffer, GpuResourceMapMode InMapMode) override;
	virtual void UnMapGpuBuffer(GpuBuffer* InGpuBuffer) override;
};

inline Dx12GpuRhiBackend* GDx12GpuRhi;
}
