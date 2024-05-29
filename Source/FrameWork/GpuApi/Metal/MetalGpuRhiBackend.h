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
	void FlushGpu() override;
	void BeginFrame() override;
	void EndFrame() override;
	TRefCountPtr<GpuTexture> CreateTexture(const GpuTextureDesc &InTexDesc) override;
	TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName, FString EntryPoint) override;
	TRefCountPtr<GpuShader> CreateShaderFromFile(FString FileName, ShaderType InType, FString EntryPoint, FString ExtraDeclaration) override;
	TRefCountPtr<GpuBindGroup> CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc) override;
	TRefCountPtr<GpuBindGroupLayout> CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc) override;
	TRefCountPtr<GpuPipelineState> CreateRenderPipelineState(const GpuRenderPipelineStateDesc &InPipelineStateDesc) override;
	TRefCountPtr<GpuBuffer> CreateBuffer(uint32 ByteSize, GpuBufferUsage Usage) override;
	TRefCountPtr<GpuSampler> CreateSampler(const GpuSamplerDesc &InSamplerDesc) override;
	void SetTextureName(const FString &TexName, GpuTexture *InTexture) override;
	void SetBufferName(const FString &BufferName, GpuBuffer *InBuffer) override;
	void *MapGpuTexture(GpuTexture *InGpuTexture, GpuResourceMapMode InMapMode, uint32 &OutRowPitch) override;
	void UnMapGpuTexture(GpuTexture *InGpuTexture) override;
	void *MapGpuBuffer(GpuBuffer *InGpuBuffer, GpuResourceMapMode InMapMode) override;
	void UnMapGpuBuffer(GpuBuffer *InGpuBuffer) override;
	bool CrossCompileShader(GpuShader *InShader, FString &OutErrorInfo) override;
	void SetRenderPipelineState(GpuPipelineState *InPipelineState) override;
	void SetVertexBuffer(GpuBuffer *InVertexBuffer) override;
	void SetViewPort(const GpuViewPortDesc &InViewPortDesc) override;
	void SetBindGroups(GpuBindGroup *BindGroup0, GpuBindGroup *BindGroup1, GpuBindGroup *BindGroup2, GpuBindGroup *BindGroup3) override;
	void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount) override;
	void Submit() override;
	void BeginGpuCapture(const FString &SavedFileName) override;
	void EndGpuCapture() override;
	void BeginCaptureEvent(const FString &EventName) override;
	void EndCaptureEvent() override;
	void *GetSharedHandle(GpuTexture *InGpuTexture) override;
	void BeginRenderPass(const GpuRenderPassDesc &PassDesc, const FString &PassName) override;
	void EndRenderPass() override;
};
}
