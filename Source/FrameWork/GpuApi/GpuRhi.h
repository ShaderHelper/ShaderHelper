#pragma once

#include "GpuResource.h"

namespace FRAMEWORK
{
enum class GpuRhiBackendType
{
	Dummy,
	DX12,
	Metal,
	Vulkan,
};

struct GpuRhiConfig
{
	bool EnableValidationCheck;
	GpuRhiBackendType BackendType;
};

class FRAMEWORK_API GpuRhi
{
public:
	GpuRhi() = default;
	virtual ~GpuRhi() = default;

public:
	virtual void InitApiEnv() = 0;

public: // command apis
	// Wait for all gpu work to finish
	virtual void FlushGpu() = 0;

	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;

	virtual TRefCountPtr<GpuTexture> CreateTexture(const GpuTextureDesc &InTexDesc) = 0;
	virtual TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName, FString EntryPoint) = 0;
	// The file directory will be considered as a include dir by default.
	virtual TRefCountPtr<GpuShader> CreateShaderFromFile(FString FileName, ShaderType InType, FString EntryPoint, FString ExtraDeclaration = {}) = 0;
	virtual TRefCountPtr<GpuBindGroup> CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc) = 0;
	virtual TRefCountPtr<GpuBindGroupLayout> CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc) = 0;
	virtual TRefCountPtr<GpuPipelineState> CreateRenderPipelineState(const GpuPipelineStateDesc &InPipelineStateDesc) = 0;
	virtual TRefCountPtr<GpuBuffer> CreateBuffer(uint32 ByteSize, GpuBufferUsage Usage) = 0;
	virtual TRefCountPtr<GpuSampler> CreateSampler(const GpuSamplerDesc &InSamplerDesc) = 0;

	virtual void SetTextureName(const FString &TexName, GpuTexture *InTexture) = 0;
	virtual void SetBufferName(const FString &BufferName, GpuBuffer *InBuffer) = 0;

	// Need OutRowPitch to correctly read or write data, because the mapped buffer actually contains the *padded* texture data.
	// RowPitch != Width x ElementByteSize
	// RowPitch = align(Width x ElementByteSize, RequiredAlignValue)
	virtual void *MapGpuTexture(GpuTexture *InGpuTexture, GpuResourceMapMode InMapMode, uint32 &OutRowPitch) = 0;
	virtual void UnMapGpuTexture(GpuTexture *InGpuTexture) = 0;

	virtual void *MapGpuBuffer(GpuBuffer *InGpuBuffer, GpuResourceMapMode InMapMode) = 0;
	virtual void UnMapGpuBuffer(GpuBuffer *InGpuBuffer) = 0;

	virtual bool CrossCompileShader(GpuShader *InShader, FString &OutErrorInfo) = 0;

	virtual void SetRenderPipelineState(GpuPipelineState *InPipelineState) = 0;

	virtual void SetVertexBuffer(GpuBuffer *InVertexBuffer) = 0;

	// If omitted, it defaults to GpuApi::SetViewPort(GpuViewPortDesc{RenderTarget.Width, RenderTarget.Height}).
	virtual void SetViewPort(const GpuViewPortDesc &InViewPortDesc) = 0;

	// Only support 4 BindGroups to adapt some mobile devices.
	virtual void SetBindGroups(GpuBindGroup *BindGroup0, GpuBindGroup *BindGroup1, GpuBindGroup *BindGroup2, GpuBindGroup *BindGroup3) = 0;

	virtual void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount) = 0;

	virtual void Submit() = 0;

	virtual void BeginGpuCapture(const FString &SavedFileName) = 0;
	virtual void EndGpuCapture() = 0;
	virtual void BeginCaptureEvent(const FString &EventName) = 0;
	virtual void EndCaptureEvent() = 0;

	// The renderer for drawing ui in UE uses dx11/opengl as backend api, so need a shared resource.
	virtual void *GetSharedHandle(GpuTexture *InGpuTexture) = 0;

	virtual void BeginRenderPass(const GpuRenderPassDesc &PassDesc, const FString &PassName) = 0;
	virtual void EndRenderPass() = 0;

public:
	static TUniquePtr<GpuRhi> CreateGpuRhi(const GpuRhiConfig &InConfig);
};
}
