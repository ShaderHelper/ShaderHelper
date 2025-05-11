#pragma once

#include "GpuResource.h"

namespace FW
{
enum class GpuRhiBackendType
{
#if PLATFORM_WINDOWS
	DX12,
#elif PLATFORM_MAC
	Metal,
#endif
	Vulkan,

// default backend for different platforms.
#if PLATFORM_WINDOWS
	Default = DX12,
#elif PLATFORM_MAC
	Default = Metal,
#else
	Default = Vulkan
#endif
};

struct GpuRhiConfig
{
	bool EnableValidationCheck;
	GpuRhiBackendType BackendType;
};

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
	virtual void SetRenderPipelineState(GpuRenderPipelineState* InPipelineState) = 0;
	virtual void SetVertexBuffer(GpuBuffer* InVertexBuffer) = 0;
	// If omitted, it defaults to SetViewPort(GpuViewPortDesc{RenderTarget.Width, RenderTarget.Height}).
	virtual void SetViewPort(const GpuViewPortDesc& InViewPortDesc) = 0;
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
	GpuTrackedResource* Resource;
	GpuResourceState NewState;
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
	virtual void CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture) = 0;
	virtual void CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer) = 0;
	virtual void CopyBufferToBuffer(GpuBuffer* SrcBuffer, uint32 SrcOffset, GpuBuffer* DestBuffer, uint32 DestOffset, uint32 Size) = 0;
};

class FRAMEWORK_API GpuRhi
{
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

	//If the initial state is unknown, then the state be determined based on usage
	virtual TRefCountPtr<GpuTexture> CreateTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState = GpuResourceState::Unknown) = 0;
	virtual TRefCountPtr<GpuBuffer> CreateBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState = GpuResourceState::Unknown) = 0;

	virtual TRefCountPtr<GpuShader> CreateShaderFromSource(const GpuShaderSourceDesc& Desc) = 0;
	// The file directory will be considered as a include dir by default.
	virtual TRefCountPtr<GpuShader> CreateShaderFromFile(const GpuShaderFileDesc& Desc) = 0;
	virtual TRefCountPtr<GpuBindGroup> CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc) = 0;
	virtual TRefCountPtr<GpuBindGroupLayout> CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc) = 0;
	virtual TRefCountPtr<GpuRenderPipelineState> CreateRenderPipelineState(const GpuRenderPipelineStateDesc &InPipelineStateDesc) = 0;
	virtual TRefCountPtr<GpuComputePipelineState> CreateComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc) = 0;
	virtual TRefCountPtr<GpuSampler> CreateSampler(const GpuSamplerDesc &InSamplerDesc) = 0;

	virtual void SetResourceName(const FString &Name, GpuResource *InResource) = 0;

	virtual bool CompileShader(GpuShader* InShader, FString& OutErrorInfo, const TArray<FString>& Definitions = {}) = 0;

	// Need OutRowPitch to correctly read or write data, because the mapped buffer actually contains the *padded* texture data.
	// RowPitch != Width x ElementByteSize
	// RowPitch = align(Width x ElementByteSize, RequiredAlignValue)
	virtual void* MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode, uint32& OutRowPitch) = 0;
	virtual void UnMapGpuTexture(GpuTexture* InGpuTexture) = 0;

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

public:
	static bool InitGpuRhi(const GpuRhiConfig &InConfig);
};

FRAMEWORK_API extern TUniquePtr<GpuRhi> GGpuRhi;

}
