#pragma once

#include "GpuApi/GpuRhi.h"

namespace FRAMEWORK
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
    TRefCountPtr<GpuTexture> CreateTexture(const GpuTextureDesc &InTexDesc, GpuResourceState InitState) override;
    TRefCountPtr<GpuBuffer> CreateBuffer(uint32 ByteSize, GpuBufferUsage Usage, GpuResourceState InitState) override;
    TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName, FString EntryPoint) override;
    TRefCountPtr<GpuShader> CreateShaderFromFile(FString FileName, ShaderType InType, FString EntryPoint, FString ExtraDeclaration) override;
    TRefCountPtr<GpuBindGroup> CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc) override;
    TRefCountPtr<GpuBindGroupLayout> CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc) override;
    TRefCountPtr<GpuPipelineState> CreateRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc) override;
    TRefCountPtr<GpuSampler> CreateSampler(const GpuSamplerDesc &InSamplerDesc) override;
    void SetResourceName(const FString& Name, GpuResource* InResource) override;
    bool CrossCompileShader(GpuShader *InShader, FString &OutErrorInfo) override;
    void BeginGpuCapture(const FString &SavedFileName) override;
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
}
