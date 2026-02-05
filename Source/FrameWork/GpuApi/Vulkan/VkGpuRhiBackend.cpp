#include "CommonHeader.h"
#include "VkGpuRhiBackend.h"
#include "VkDevice.h"
#include "VkTexture.h"

namespace FW
{
	VkGpuRhiBackend::VkGpuRhiBackend() { GVkGpuRhi = this; }

	void VkGpuRhiBackend::InitApiEnv()
	{
		InitVulkanCore();
	}

	void VkGpuRhiBackend::WaitGpu()
	{
		vkDeviceWaitIdle(GDevice);
	}

	void VkGpuRhiBackend::BeginFrame()
	{

	}

	void VkGpuRhiBackend::EndFrame()
	{

	}

	TRefCountPtr<GpuTexture> VkGpuRhiBackend::CreateTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState)
	{
		return AUX::StaticCastRefCountPtr<GpuTexture>(CreateVulkanTexture(InTexDesc, InitState));
	}

	TRefCountPtr<GpuBuffer> VkGpuRhiBackend::CreateBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState)
	{
		return TRefCountPtr<GpuBuffer>();
	}

	TRefCountPtr<GpuShader> VkGpuRhiBackend::CreateShaderFromSource(const GpuShaderSourceDesc& Desc) const
	{
		return TRefCountPtr<GpuShader>();
	}

	TRefCountPtr<GpuShader> VkGpuRhiBackend::CreateShaderFromFile(const GpuShaderFileDesc& Desc)
	{
		return TRefCountPtr<GpuShader>();
	}

	TRefCountPtr<GpuBindGroup> VkGpuRhiBackend::CreateBindGroup(const GpuBindGroupDesc& InBindGroupDesc)
	{
		return TRefCountPtr<GpuBindGroup>();
	}

	TRefCountPtr<GpuBindGroupLayout> VkGpuRhiBackend::CreateBindGroupLayout(const GpuBindGroupLayoutDesc& InBindGroupLayoutDesc)
	{
		return TRefCountPtr<GpuBindGroupLayout>();
	}

	TRefCountPtr<GpuRenderPipelineState> VkGpuRhiBackend::CreateRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc)
	{
		return TRefCountPtr<GpuRenderPipelineState>();
	}

	TRefCountPtr<GpuComputePipelineState> VkGpuRhiBackend::CreateComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc)
	{
		return TRefCountPtr<GpuComputePipelineState>();
	}

	TRefCountPtr<GpuSampler> VkGpuRhiBackend::CreateSampler(const GpuSamplerDesc& InSamplerDesc)
	{
		return TRefCountPtr<GpuSampler>();
	}

	void VkGpuRhiBackend::SetResourceName(const FString& Name, GpuResource* InResource)
	{

	}

	bool VkGpuRhiBackend::CompileShader(GpuShader* InShader, FString& OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs)
	{
		return false;
	}

	void VkGpuRhiBackend::BeginGpuCapture(const FString& CaptureName)
	{
	}

	void VkGpuRhiBackend::EndGpuCapture()
	{
	}

	void* VkGpuRhiBackend::GetSharedHandle(GpuTexture* InGpuTexture)
	{
		return static_cast<VulkanTexture*>(InGpuTexture)->GetSharedHandle();
	}

	GpuCmdRecorder* VkGpuRhiBackend::BeginRecording(const FString& RecorderName)
	{
		return nullptr;
	}

	void VkGpuRhiBackend::EndRecording(GpuCmdRecorder* InCmdRecorder)
	{

	}

	void VkGpuRhiBackend::Submit(const TArray<GpuCmdRecorder*>& CmdRecorders)
	{

	}

	void* VkGpuRhiBackend::MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode, uint32& OutRowPitch)
	{
		return nullptr;
	}

	void VkGpuRhiBackend::UnMapGpuTexture(GpuTexture* InGpuTexture)
	{

	}

	void* VkGpuRhiBackend::MapGpuBuffer(GpuBuffer* InGpuBuffer, GpuResourceMapMode InMapMode)
	{
		return nullptr;
	}

	void VkGpuRhiBackend::UnMapGpuBuffer(GpuBuffer* InGpuBuffer)
	{

	}
}
