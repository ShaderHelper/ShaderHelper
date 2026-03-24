#pragma once

#include "GpuResource.h"

namespace FW
{
enum class GpuRhiBackendType
{
#if PLATFORM_WINDOWS
	DX12,
	Vulkan,
#elif PLATFORM_MAC
	Metal,
#endif
	Num,
};

struct GpuRhiConfig
{
	bool EnableValidationCheck;
	GpuRhiBackendType BackendType;
};

//Note: doesn't support state inheritance across pass
class GpuComputePassRecorder
{
public:
	GpuComputePassRecorder() = default;
	virtual ~GpuComputePassRecorder() = default;

public:
	virtual void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) = 0;
	virtual void SetComputePipelineState(GpuComputePipelineState* InPipelineState) = 0;
	virtual void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3) = 0;
};

class GpuRenderPassRecorder
{
public:
	GpuRenderPassRecorder() = default;
	virtual ~GpuRenderPassRecorder() = default;

public:
	virtual void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount) = 0;
	virtual void DrawIndexed(uint32 StartIndexLocation, uint32 IndexCount, int32 BaseVertexLocation = 0, uint32 StartInstanceLocation = 0, uint32 InstanceCount = 1) = 0;
	virtual void SetRenderPipelineState(GpuRenderPipelineState* InPipelineState) = 0;
	virtual void SetVertexBuffer(uint32 Slot, GpuBuffer* InVertexBuffer, uint32 Offset = 0) = 0;
	virtual void SetIndexBuffer(GpuBuffer* InIndexBuffer, GpuFormat IndexFormat = GpuFormat::R32_UINT, uint32 Offset = 0) = 0;
	// If omitted, it defaults to SetViewPort(GpuViewPortDesc{RenderTarget.Width, RenderTarget.Height}).
	virtual void SetViewPort(const GpuViewPortDesc& InViewPortDesc) = 0;
	// If omitted, it defaults to SetScissorRect(GpuScissorRectDesc{0, 0, Viewport.Width, Viewport.Height}).
	virtual void SetScissorRect(const GpuScissorRectDesc& InScissorRectDes) = 0;
	// Only support 4 BindGroups to adapt some mobile devices.
	virtual void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3) = 0;
};

class GpuComputeCmdRecorder
{
public:
	GpuComputeCmdRecorder() = default;
	virtual ~GpuComputeCmdRecorder() = default;

public:
	virtual GpuComputePassRecorder* BeginComputePass(const FString& PassName) = 0;
	virtual void EndComputePass(GpuComputePassRecorder* InComputePassRecorder) = 0;
};

struct GpuBarrierInfo
{
	GpuResource* Resource;
	GpuResourceState NewState;
};

class FRAMEWORK_API GpuQuerySet : public GpuResource
{
public:
	GpuQuerySet(uint32 InCount) : GpuResource(GpuResourceType::QuerySet), Count(InCount) {}
	virtual ~GpuQuerySet() = default;
	uint32 GetCount() const { return Count; }
	virtual double GetTimestampPeriodNs() const = 0;
	virtual void ResolveResults(uint32 FirstQuery, uint32 QueryCount, TArray<uint64>& OutTimestamps) = 0;
private:
	uint32 Count;
};

//Recorder is designed as transient within one frame, can not reuse it
class GpuCmdRecorder : public GpuComputeCmdRecorder
{
public:
	virtual GpuRenderPassRecorder* BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName) = 0;
	virtual void EndRenderPass(GpuRenderPassRecorder* InRenderPassRecorder) = 0;
	virtual void BeginCaptureEvent(const FString& EventName) = 0;
	virtual void EndCaptureEvent() = 0;
	virtual void Barriers(const TArray<GpuBarrierInfo>& BarrierInfos) = 0;
	virtual void CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture, uint32 ArrayLayer = 0, uint32 MipLevel = 0) = 0;
	virtual void CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer) = 0;
	virtual void CopyBufferToBuffer(GpuBuffer* SrcBuffer, uint32 SrcOffset, GpuBuffer* DestBuffer, uint32 DestOffset, uint32 Size) = 0;
};

class FRAMEWORK_API GpuRhi
{
	friend class GpuRhiValidation;
public:
	GpuRhi() = default;
	virtual ~GpuRhi() = default;

public:
	virtual void InitApiEnv() = 0;

public:
	// Wait for all gpu works that have already been submitted to finish
	virtual void WaitGpu() = 0;

	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;

	TRefCountPtr<GpuTexture> CreateTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState = GpuResourceState::Unknown);
	TRefCountPtr<GpuBuffer> CreateBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState = GpuResourceState::Unknown);
	TRefCountPtr<GpuTextureView> CreateTextureView(const GpuTextureViewDesc& InViewDesc);

	TRefCountPtr<GpuShader> CreateShaderFromSource(const GpuShaderSourceDesc& Desc) const;
	// The file directory will be considered as a include dir by default.
	TRefCountPtr<GpuShader> CreateShaderFromFile(const GpuShaderFileDesc& Desc);
	virtual TRefCountPtr<GpuBindGroup> CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc) = 0;
	virtual TRefCountPtr<GpuBindGroupLayout> CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc) = 0;
	TRefCountPtr<GpuRenderPipelineState> CreateRenderPipelineState(const GpuRenderPipelineStateDesc &InPipelineStateDesc);
	TRefCountPtr<GpuComputePipelineState> CreateComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc);
	virtual TRefCountPtr<GpuSampler> CreateSampler(const GpuSamplerDesc &InSamplerDesc) = 0;

	virtual void SetResourceName(const FString &Name, GpuResource *InResource) = 0;

	virtual bool CompileShader(GpuShader* InShader, FString& OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs = {}) = 0;

	// Need OutRowPitch to correctly read or write data, because the mapped buffer actually contains the *padded* texture data.
	// RowPitch != Width x ElementByteSize
	// RowPitch = align(Width x ElementByteSize, RequiredAlignValue)
	virtual void* MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode, uint32& OutRowPitch) = 0;
	virtual void UnMapGpuTexture(GpuTexture* InGpuTexture) = 0;

	//The results of Map* interfaces are transient within one frame, which should not be used across multiple frames.
	virtual void* MapGpuBuffer(GpuBuffer* InGpuBuffer, GpuResourceMapMode InMapMode) = 0;
	virtual void UnMapGpuBuffer(GpuBuffer* InGpuBuffer) = 0;

	//Capture command recorders that are requested and submitted within the BeginGpuCapture/EndGpuCapture scope.
	virtual void BeginGpuCapture(const FString &CaptureName) = 0;
	virtual void EndGpuCapture() = 0;

	// The renderer for drawing ui in UE uses dx11/opengl as backend api, so need a shared resource.
	virtual void *GetSharedHandle(GpuTexture *InGpuTexture) = 0;

	//TODO: virtual GpuComputeCmdRecorder* BeginAsyncComputeRecording() = 0;
	//virtual void EndRecordingAndSubmit(GpuComputeCmdRecorder* InCmdRecorder) = 0;
	virtual GpuCmdRecorder* BeginRecording(const FString& RecorderName = {}) = 0;
	virtual void EndRecording(GpuCmdRecorder* InCmdRecorder) = 0;
	virtual void Submit(const TArray<GpuCmdRecorder*>& CmdRecorders) = 0;

	virtual TRefCountPtr<GpuQuerySet> CreateQuerySet(uint32 Count) = 0;

public:
	static void InitGpuRhi(const GpuRhiConfig &InConfig);

protected:
	virtual TRefCountPtr<GpuTexture> CreateTextureInternal(const GpuTextureDesc& InTexDesc, GpuResourceState InitState) = 0;
	virtual TRefCountPtr<GpuBuffer> CreateBufferInternal(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState) = 0;
	virtual TRefCountPtr<GpuTextureView> CreateTextureViewInternal(const GpuTextureViewDesc& InViewDesc) = 0;
	virtual TRefCountPtr<GpuRenderPipelineState> CreateRenderPipelineStateInternal(const GpuRenderPipelineStateDesc &InPipelineStateDesc) = 0;
	virtual TRefCountPtr<GpuComputePipelineState> CreateComputePipelineStateInternal(const GpuComputePipelineStateDesc& InPipelineStateDesc) = 0;
	virtual TRefCountPtr<GpuShader> CreateShaderFromSourceInternal(const GpuShaderSourceDesc& Desc) const = 0;
	virtual TRefCountPtr<GpuShader> CreateShaderFromFileInternal(const GpuShaderFileDesc& Desc) = 0;
};

FRAMEWORK_API extern TUniquePtr<GpuRhi> GGpuRhi;
FRAMEWORK_API GpuRhiBackendType GetGpuRhiBackendType();
}
