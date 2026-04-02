#pragma once

#include "GpuApi/GpuRhi.h"
#include "MetalDevice.h"

namespace FW
{
class MetalGpuRhiBackend : public GpuRhi
{
public:
	MetalGpuRhiBackend();
	virtual ~MetalGpuRhiBackend();

public:
    void InitApiEnv() override;
    void WaitGpu() override;
    void BeginFrame() override;
    void EndFrame() override;
	const GpuFeature& GetFeature() const override;
    TRefCountPtr<GpuTexture> CreateTextureInternal(const GpuTextureDesc &InTexDesc, GpuResourceState InitState) override;
	TRefCountPtr<GpuTextureView> CreateTextureViewInternal(const GpuTextureViewDesc& InViewDesc) override;
	TRefCountPtr<GpuBuffer> CreateBufferInternal(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState) override;
    TRefCountPtr<GpuShader> CreateShaderFromSourceInternal(const GpuShaderSourceDesc& Desc) const override;
    TRefCountPtr<GpuShader> CreateShaderFromFileInternal(const GpuShaderFileDesc& Desc) override;
    TRefCountPtr<GpuBindGroup> CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc) override;
    TRefCountPtr<GpuBindGroupLayout> CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc) override;
    TRefCountPtr<GpuRenderPipelineState> CreateRenderPipelineStateInternal(const GpuRenderPipelineStateDesc& InPipelineStateDesc) override;
	TRefCountPtr<GpuComputePipelineState> CreateComputePipelineStateInternal(const GpuComputePipelineStateDesc& InPipelineStateDesc) override;
    TRefCountPtr<GpuSampler> CreateSampler(const GpuSamplerDesc &InSamplerDesc) override;
    void SetResourceName(const FString& Name, GpuResource* InResource) override;
    void BeginGpuCapture(const FString &CaptureName) override;
    void EndGpuCapture() override;
    void *GetSharedHandle(GpuTexture *InGpuTexture) override;
    GpuCmdRecorder* BeginRecording(const FString& RecorderName = {}) override;
    void EndRecording(GpuCmdRecorder* InCmdRecorder) override;
    void SubmitInternal(const TArray<GpuCmdRecorder*>& CmdRecorders) override;
    virtual void* MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode, uint32& OutRowPitch) override;
    virtual void UnMapGpuTexture(GpuTexture* InGpuTexture) override;
    virtual void* MapGpuBuffer(GpuBuffer* InGpuBuffer, GpuResourceMapMode InMapMode) override;
    virtual void UnMapGpuBuffer(GpuBuffer* InGpuBuffer) override;
    TRefCountPtr<GpuQuerySet> CreateQuerySet(uint32 Count) override;
	bool CompileShaderInternal(GpuShader *InShader, FString &OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs) override;
};

class MtlDeferredReleaseManager
{
public:
	void AddResource(TRefCountPtr<GpuResource> InResource)
	{
		FScopeLock Lock(&CS);
		Resources.Add(MoveTemp(InResource));
	}

	void ProcessResources()
	{
		FScopeLock Lock(&CS);
		for (auto It = Resources.CreateIterator(); It; ++It)
		{
			if ((*It).GetRefCount() == 1)
			{
				It.RemoveCurrent();
			}
		}
	}

private:
	TSparseArray<TRefCountPtr<GpuResource>> Resources;
	FCriticalSection CS;
};

inline MtlDeferredReleaseManager GMtlDeferredReleaseManager;
inline MetalGpuRhiBackend* GMtlGpuRhi;

class MetalQuerySet : public GpuQuerySet
{
public:
    MetalQuerySet(uint32 InCount, NS::SharedPtr<MTL::CounterSampleBuffer> InSampleBuffer);
    double GetTimestampPeriodNs() const override;
    void ResolveResults(uint32 FirstQuery, uint32 QueryCount, TArray<uint64>& OutTimestamps) override;
    MTL::CounterSampleBuffer* GetSampleBuffer() const { return SampleBuffer.get(); }
private:
    NS::SharedPtr<MTL::CounterSampleBuffer> SampleBuffer;
};
}
