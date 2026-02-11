#include "CommonHeader.h"
#include "VkGpuRhiBackend.h"
#include "VkDevice.h"
#include "VkTexture.h"
#include "VkCommandRecorder.h"
#include "VkShader.h"
#include "VkPipeline.h"
#include "VkUtil.h"
#include "VkBuffer.h"
#include "VkDescriptorSet.h"

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
		WaitGpu();
		GVkCmdRecorderPool.Empty();
		GVkDeferredReleaseManager.ProcessResources();
	}

	TRefCountPtr<GpuTexture> VkGpuRhiBackend::CreateTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState)
	{
		return AUX::StaticCastRefCountPtr<GpuTexture>(CreateVulkanTexture(InTexDesc, InitState));
	}

	TRefCountPtr<GpuBuffer> VkGpuRhiBackend::CreateBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState)
	{
		GpuResourceState ValidState = InitState == GpuResourceState::Unknown ? GetBufferState(InBufferDesc.Usage) : InitState;
		return AUX::StaticCastRefCountPtr<GpuBuffer>(CreateVulkanBuffer(InBufferDesc, ValidState));
	}

	TRefCountPtr<GpuShader> VkGpuRhiBackend::CreateShaderFromSource(const GpuShaderSourceDesc& Desc) const
	{
		TRefCountPtr<VulkanShader> NewShader = new VulkanShader(Desc);
		return TRefCountPtr<GpuShader>(NewShader);
	}

	TRefCountPtr<GpuShader> VkGpuRhiBackend::CreateShaderFromFile(const GpuShaderFileDesc& Desc)
	{
		TRefCountPtr<VulkanShader> NewShader = new VulkanShader(Desc);
		return TRefCountPtr<GpuShader>(NewShader);
	}

	TRefCountPtr<GpuBindGroup> VkGpuRhiBackend::CreateBindGroup(const GpuBindGroupDesc& InBindGroupDesc)
	{
		return AUX::StaticCastRefCountPtr<GpuBindGroup>(CreateVulkanBindGroup(InBindGroupDesc));
	}

	TRefCountPtr<GpuBindGroupLayout> VkGpuRhiBackend::CreateBindGroupLayout(const GpuBindGroupLayoutDesc& InBindGroupLayoutDesc)
	{
		return AUX::StaticCastRefCountPtr<GpuBindGroupLayout>(CreateVulkanBindGroupLayout(InBindGroupLayoutDesc));
	}

	TRefCountPtr<GpuRenderPipelineState> VkGpuRhiBackend::CreateRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc)
	{
		return AUX::StaticCastRefCountPtr<GpuRenderPipelineState>(CreateVulkanRenderPipelineState(InPipelineStateDesc));
	}

	TRefCountPtr<GpuComputePipelineState> VkGpuRhiBackend::CreateComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc)
	{
		return TRefCountPtr<GpuComputePipelineState>();
	}

	TRefCountPtr<GpuSampler> VkGpuRhiBackend::CreateSampler(const GpuSamplerDesc& InSamplerDesc)
	{
		return AUX::StaticCastRefCountPtr<GpuSampler>(CreateVulkanSampler(InSamplerDesc));
	}

	void VkGpuRhiBackend::SetResourceName(const FString& Name, GpuResource* InResource)
	{

	}

	bool VkGpuRhiBackend::CompileShader(GpuShader* InShader, FString& OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs)
	{
		return CompileVulkanShader(static_cast<VulkanShader*>(InShader), OutErrorInfo, OutWarnInfo, ExtraArgs);
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
		VulkanCmdRecorder* VkCmdRecorder = GVkCmdRecorderPool.AcquireCmdRecorder(RecorderName);
		VkCommandBufferBeginInfo BeginInfo{ 
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};
		VkCheck(vkBeginCommandBuffer(VkCmdRecorder->GetCommandBuffer(), &BeginInfo));
		return VkCmdRecorder;
	}

	void VkGpuRhiBackend::EndRecording(GpuCmdRecorder* InCmdRecorder)
	{
		VulkanCmdRecorder* VkCmdRecorder = static_cast<VulkanCmdRecorder*>(InCmdRecorder);
		VkCheck(vkEndCommandBuffer(VkCmdRecorder->GetCommandBuffer()));
	}

	void VkGpuRhiBackend::Submit(const TArray<GpuCmdRecorder*>& CmdRecorders)
	{
		TArray<VkCommandBuffer> CommamdBuffers;
		for (auto* Recorder : CmdRecorders)
		{
			CommamdBuffers.Add(static_cast<VulkanCmdRecorder*>(Recorder)->GetCommandBuffer());
		}
		VkSubmitInfo SubmitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = (uint32_t)CmdRecorders.Num(),
			.pCommandBuffers = CommamdBuffers.GetData()
		};
		VkCheck(vkQueueSubmit(GGraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE));
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
		VulkanBuffer* VkBuffer = static_cast<VulkanBuffer*>(InGpuBuffer);
		void* MappedData = nullptr;
		VkCheck(vmaMapMemory(GAllocator, VkBuffer->GetAllocation(), &MappedData));
		return MappedData;
	}

	void VkGpuRhiBackend::UnMapGpuBuffer(GpuBuffer* InGpuBuffer)
	{
		VulkanBuffer* VkBuffer = static_cast<VulkanBuffer*>(InGpuBuffer);
		vmaUnmapMemory(GAllocator, VkBuffer->GetAllocation());
	}
}
