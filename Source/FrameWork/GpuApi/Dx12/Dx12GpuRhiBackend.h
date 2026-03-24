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
	TRefCountPtr<GpuTexture> CreateTextureInternal(const GpuTextureDesc &InTexDesc, GpuResourceState InitState) override;
	TRefCountPtr<GpuBuffer> CreateBufferInternal(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState) override;
	TRefCountPtr<GpuTextureView> CreateTextureViewInternal(const GpuTextureViewDesc& InViewDesc) override;
	TRefCountPtr<GpuShader> CreateShaderFromSourceInternal(const GpuShaderSourceDesc& Desc) const override;
	TRefCountPtr<GpuShader> CreateShaderFromFileInternal(const GpuShaderFileDesc& Desc) override;
	TRefCountPtr<GpuBindGroup> CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc) override;
	TRefCountPtr<GpuBindGroupLayout> CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc) override;
	TRefCountPtr<GpuRenderPipelineState> CreateRenderPipelineStateInternal(const GpuRenderPipelineStateDesc& InPipelineStateDesc) override;
	TRefCountPtr<GpuComputePipelineState> CreateComputePipelineStateInternal(const GpuComputePipelineStateDesc& InPipelineStateDesc) override;
	TRefCountPtr<GpuSampler> CreateSampler(const GpuSamplerDesc &InSamplerDesc) override;
	void SetResourceName(const FString& Name, GpuResource* InResource) override;
	bool CompileShader(GpuShader *InShader, FString &OutErrorInfo, FString &OutWarnInfo, const TArray<FString>& ExtraArgs) override;
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
	TRefCountPtr<GpuQuerySet> CreateQuerySet(uint32 Count) override;
};

inline Dx12GpuRhiBackend* GDx12GpuRhi;

class Dx12QuerySet : public GpuQuerySet, public Dx12DeferredDeleteObject<Dx12QuerySet>
{
public:
	Dx12QuerySet(uint32 InCount);
	double GetTimestampPeriodNs() const override;
	void ResolveResults(uint32 FirstQuery, uint32 QueryCount, TArray<uint64>& OutTimestamps) override;
	ID3D12QueryHeap* GetHeap() const { return QueryHeap; }
	ID3D12Resource* GetReadbackBuffer() const;
private:
	TRefCountPtr<ID3D12QueryHeap> QueryHeap;
	TRefCountPtr<GpuBuffer> ReadbackBuffer;
};
}
