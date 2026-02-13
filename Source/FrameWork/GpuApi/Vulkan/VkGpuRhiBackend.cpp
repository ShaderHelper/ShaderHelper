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
		GpuResourceState ValidState = InitState == GpuResourceState::Unknown ? GetTextureState(InTexDesc.Usage) : InitState;
		return AUX::StaticCastRefCountPtr<GpuTexture>(CreateVulkanTexture(InTexDesc, ValidState));
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
		return AUX::StaticCastRefCountPtr<GpuComputePipelineState>(CreateVulkanComputePipelineState(InPipelineStateDesc));
	}

	TRefCountPtr<GpuSampler> VkGpuRhiBackend::CreateSampler(const GpuSamplerDesc& InSamplerDesc)
	{
		return AUX::StaticCastRefCountPtr<GpuSampler>(CreateVulkanSampler(InSamplerDesc));
	}

	void VkGpuRhiBackend::SetResourceName(const FString& Name, GpuResource* InResource)
	{
		switch (InResource->GetType())
		{
		case GpuResourceType::Texture:
			SetVkObjectName(VK_OBJECT_TYPE_IMAGE, (uint64)static_cast<VulkanTexture*>(InResource)->GetImage(), Name);
			break;
		case GpuResourceType::Buffer:
			SetVkObjectName(VK_OBJECT_TYPE_BUFFER, (uint64)static_cast<VulkanBuffer*>(InResource)->GetBuffer(), Name);
			break;
		case GpuResourceType::Sampler:
			SetVkObjectName(VK_OBJECT_TYPE_SAMPLER, (uint64)static_cast<VulkanSampler*>(InResource)->GetSampler(), Name);
			break;
		case GpuResourceType::RenderPipelineState:
			SetVkObjectName(VK_OBJECT_TYPE_PIPELINE, (uint64)static_cast<VulkanRenderPipelineState*>(InResource)->GetPipeline(), Name);
			break;
		case GpuResourceType::ComputePipelineState:
			SetVkObjectName(VK_OBJECT_TYPE_PIPELINE, (uint64)static_cast<VulkanComputePipelineState*>(InResource)->GetPipeline(), Name);
			break;
		case GpuResourceType::Shader:
			SetVkObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64)static_cast<VulkanShader*>(InResource)->GetCompilationResult(), Name);
			break;
		default:
			break;
		}
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
		SetVkObjectName(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64)VkCmdRecorder->GetCommandBuffer(), RecorderName);
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
		VulkanTexture* Texture = static_cast<VulkanTexture*>(InGpuTexture);
		void* Data = nullptr;
		const uint32 BytesPerTexel = GetTextureFormatByteSize(InGpuTexture->GetFormat());
		const uint32 BufferSize = InGpuTexture->GetWidth() * InGpuTexture->GetHeight() * BytesPerTexel;
		OutRowPitch = InGpuTexture->GetWidth() * BytesPerTexel;

		if (InMapMode == GpuResourceMapMode::Write_Only)
		{
			if (!Texture->UploadBuffer)
			{
				Texture->UploadBuffer = AUX::StaticCastRefCountPtr<GpuBuffer>(CreateVulkanBuffer({ BufferSize, GpuBufferUsage::Upload }, GpuResourceState::CopySrc));
			}
			VulkanBuffer* VkBuffer = static_cast<VulkanBuffer*>(Texture->UploadBuffer.GetReference());
			VkCheck(vmaMapMemory(GAllocator, VkBuffer->GetAllocation(), &Data));
			Texture->bIsMappingForWriting = true;
		}
		else if (InMapMode == GpuResourceMapMode::Read_Only)
		{
			if (!Texture->ReadBackBuffer)
			{
				Texture->ReadBackBuffer = AUX::StaticCastRefCountPtr<GpuBuffer>(CreateVulkanBuffer({ BufferSize, GpuBufferUsage::ReadBack }, GpuResourceState::CopyDst));
			}
			auto CmdRecorder = GVkGpuRhi->BeginRecording();
			{
				GpuResourceState LastState = InGpuTexture->State;
				CmdRecorder->Barriers({
					{ InGpuTexture, GpuResourceState::CopySrc }
				});
				CmdRecorder->CopyTextureToBuffer(InGpuTexture, Texture->ReadBackBuffer);
				CmdRecorder->Barriers({
					{ InGpuTexture, LastState }
				});
			}
			GVkGpuRhi->EndRecording(CmdRecorder);
			GVkGpuRhi->Submit({ CmdRecorder });
			GVkGpuRhi->WaitGpu();

			VulkanBuffer* VkBuffer = static_cast<VulkanBuffer*>(Texture->ReadBackBuffer.GetReference());
			VkCheck(vmaMapMemory(GAllocator, VkBuffer->GetAllocation(), &Data));
		}

		return Data;
	}

	void VkGpuRhiBackend::UnMapGpuTexture(GpuTexture* InGpuTexture)
	{
		VulkanTexture* Texture = static_cast<VulkanTexture*>(InGpuTexture);
		if (Texture->bIsMappingForWriting)
		{
			VulkanBuffer* VkUploadBuffer = static_cast<VulkanBuffer*>(Texture->UploadBuffer.GetReference());
			vmaUnmapMemory(GAllocator, VkUploadBuffer->GetAllocation());

			auto CmdRecorder = GVkGpuRhi->BeginRecording();
			{
				GpuResourceState LastState = InGpuTexture->State;
				CmdRecorder->Barriers({
					{ InGpuTexture, GpuResourceState::CopyDst }
				});
				CmdRecorder->CopyBufferToTexture(Texture->UploadBuffer, InGpuTexture);
				CmdRecorder->Barriers({
					{ InGpuTexture, LastState }
				});
			}
			GVkGpuRhi->EndRecording(CmdRecorder);
			GVkGpuRhi->Submit({ CmdRecorder });

			Texture->bIsMappingForWriting = false;
		}
		else
		{
			VulkanBuffer* VkReadBackBuffer = static_cast<VulkanBuffer*>(Texture->ReadBackBuffer.GetReference());
			vmaUnmapMemory(GAllocator, VkReadBackBuffer->GetAllocation());
		}
	}

	void* VkGpuRhiBackend::MapGpuBuffer(GpuBuffer* InGpuBuffer, GpuResourceMapMode InMapMode)
	{
		GpuBufferUsage Usage = InGpuBuffer->GetUsage();
		VulkanBuffer* VkBuffer = static_cast<VulkanBuffer*>(InGpuBuffer);
		void* Data = nullptr;

		if (EnumHasAnyFlags(Usage, GpuBufferUsage::DynamicMask))
		{
			VkCheck(vmaMapMemory(GAllocator, VkBuffer->GetAllocation(), &Data));
		}
		else
		{
			if (InMapMode == GpuResourceMapMode::Read_Only)
			{
				TRefCountPtr<VulkanBuffer> ReadBackBuffer = CreateVulkanBuffer({ InGpuBuffer->GetByteSize(), GpuBufferUsage::ReadBack }, GpuResourceState::CopyDst);
				auto CmdRecorder = GVkGpuRhi->BeginRecording();
				{
					GpuResourceState LastState = InGpuBuffer->State;
					CmdRecorder->Barriers({
						{ InGpuBuffer, GpuResourceState::CopySrc },
					});
					CmdRecorder->CopyBufferToBuffer(InGpuBuffer, 0, ReadBackBuffer, 0, InGpuBuffer->GetByteSize());
					CmdRecorder->Barriers({
						{ InGpuBuffer, LastState }
					});
				}
				GVkGpuRhi->EndRecording(CmdRecorder);
				GVkGpuRhi->Submit({ CmdRecorder });
				GVkGpuRhi->WaitGpu();

				VkCheck(vmaMapMemory(GAllocator, ReadBackBuffer->GetAllocation(), &Data));
			}
		}

		return Data;
	}

	void VkGpuRhiBackend::UnMapGpuBuffer(GpuBuffer* InGpuBuffer)
	{
		GpuBufferUsage Usage = InGpuBuffer->GetUsage();
		if (EnumHasAnyFlags(Usage, GpuBufferUsage::DynamicMask))
		{
			VulkanBuffer* VkBuffer = static_cast<VulkanBuffer*>(InGpuBuffer);
			vmaUnmapMemory(GAllocator, VkBuffer->GetAllocation());
		}
	}
}
