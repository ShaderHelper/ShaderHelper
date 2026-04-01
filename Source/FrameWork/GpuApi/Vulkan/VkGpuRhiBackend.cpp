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
#include "VkMap.h"

using namespace FW::VK;

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

	TRefCountPtr<GpuTexture> VkGpuRhiBackend::CreateTextureInternal(const GpuTextureDesc& InTexDesc, GpuResourceState InitState)
	{
		return AUX::StaticCastRefCountPtr<GpuTexture>(CreateVulkanTexture(InTexDesc, InitState));
	}

	TRefCountPtr<GpuTextureView> VkGpuRhiBackend::CreateTextureViewInternal(const GpuTextureViewDesc& InViewDesc)
	{
		VulkanTexture* VkTex = static_cast<VulkanTexture*>(InViewDesc.Texture);
		bool bIsCube = InViewDesc.Texture->GetResourceDesc().Dimension == GpuTextureDimension::TexCube;
		bool bIs3D = InViewDesc.Texture->GetResourceDesc().Dimension == GpuTextureDimension::Tex3D;

		VkImageViewType ViewType;
		uint32 BaseArrayLayer = 0;
		uint32 LayerCount = 1;

		if (bIsCube)
		{
			if (InViewDesc.ArrayLayerCount == 6)
			{
				ViewType = VK_IMAGE_VIEW_TYPE_CUBE;
				LayerCount = 6;
			}
			else
			{
				ViewType = VK_IMAGE_VIEW_TYPE_2D;
				BaseArrayLayer = InViewDesc.BaseArrayLayer;
				LayerCount = InViewDesc.ArrayLayerCount;
			}
		}
		else if (bIs3D)
		{
			ViewType = VK_IMAGE_VIEW_TYPE_3D;
		}
		else
		{
			ViewType = VK_IMAGE_VIEW_TYPE_2D;
		}

		VkImageAspectFlags AspectMask = EnumHasAnyFlags(InViewDesc.Texture->GetResourceDesc().Usage, GpuTextureUsage::DepthStencil)
			? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

		VkImageViewCreateInfo ViewInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = VkTex->GetImage(),
			.viewType = ViewType,
			.format = MapTextureFormat(InViewDesc.Texture->GetFormat()),
			.subresourceRange = {
				.aspectMask = AspectMask,
				.baseMipLevel = InViewDesc.BaseMipLevel,
				.levelCount = InViewDesc.MipLevelCount,
				.baseArrayLayer = BaseArrayLayer,
				.layerCount = LayerCount
			}
		};
		VkImageView ImageView;
		VkCheck(vkCreateImageView(GDevice, &ViewInfo, nullptr, &ImageView));

		GpuTextureViewDesc ViewDesc = InViewDesc;
		return new VulkanTextureView(MoveTemp(ViewDesc), ImageView);
	}

	TRefCountPtr<GpuBuffer> VkGpuRhiBackend::CreateBufferInternal(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState)
	{
		return AUX::StaticCastRefCountPtr<GpuBuffer>(CreateVulkanBuffer(InBufferDesc, InitState));
	}

	TRefCountPtr<GpuShader> VkGpuRhiBackend::CreateShaderFromSourceInternal(const GpuShaderSourceDesc& Desc) const
	{
		TRefCountPtr<VulkanShader> NewShader = new VulkanShader(Desc);
		return TRefCountPtr<GpuShader>(NewShader);
	}

	TRefCountPtr<GpuShader> VkGpuRhiBackend::CreateShaderFromFileInternal(const GpuShaderFileDesc& Desc)
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

	TRefCountPtr<GpuRenderPipelineState> VkGpuRhiBackend::CreateRenderPipelineStateInternal(const GpuRenderPipelineStateDesc& InPipelineStateDesc)
	{
		return AUX::StaticCastRefCountPtr<GpuRenderPipelineState>(CreateVulkanRenderPipelineState(InPipelineStateDesc));
	}

	TRefCountPtr<GpuComputePipelineState> VkGpuRhiBackend::CreateComputePipelineStateInternal(const GpuComputePipelineStateDesc& InPipelineStateDesc)
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

	bool VkGpuRhiBackend::CompileShaderInternal(GpuShader* InShader, FString& OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs)
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

	void VkGpuRhiBackend::SubmitInternal(const TArray<GpuCmdRecorder*>& CmdRecorders)
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
		const uint32 BytesPerTexel = GetFormatByteSize(InGpuTexture->GetFormat());
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
				GpuResourceState LastState = InGpuTexture->GetSubResourceState(0, 0);
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
				GpuResourceState LastState = InGpuTexture->GetSubResourceState(0, 0);
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
				VkBuffer->ReadBackBuffer = CreateVulkanBuffer({ InGpuBuffer->GetByteSize(), GpuBufferUsage::ReadBack }, GpuResourceState::CopyDst);
				auto CmdRecorder = GVkGpuRhi->BeginRecording();
				{
					GpuResourceState LastState = InGpuBuffer->State;
					CmdRecorder->Barriers({
						{ InGpuBuffer, GpuResourceState::CopySrc },
					});
					CmdRecorder->CopyBufferToBuffer(InGpuBuffer, 0, VkBuffer->ReadBackBuffer, 0, InGpuBuffer->GetByteSize());
					CmdRecorder->Barriers({
						{ InGpuBuffer, LastState }
					});
				}
				GVkGpuRhi->EndRecording(CmdRecorder);
				GVkGpuRhi->Submit({ CmdRecorder });
				GVkGpuRhi->WaitGpu();

				VkCheck(vmaMapMemory(GAllocator, VkBuffer->ReadBackBuffer->GetAllocation(), &Data));
			}
		}

		return Data;
	}

	void VkGpuRhiBackend::UnMapGpuBuffer(GpuBuffer* InGpuBuffer)
	{
		VulkanBuffer* VkBuffer = static_cast<VulkanBuffer*>(InGpuBuffer);
		GpuBufferUsage Usage = InGpuBuffer->GetUsage();
		if (EnumHasAnyFlags(Usage, GpuBufferUsage::DynamicMask))
		{
			vmaUnmapMemory(GAllocator, VkBuffer->GetAllocation());
		}
		else if (VkBuffer->ReadBackBuffer)
		{
			vmaUnmapMemory(GAllocator, VkBuffer->ReadBackBuffer->GetAllocation());
			VkBuffer->ReadBackBuffer = nullptr;
		}
	}

	TRefCountPtr<GpuQuerySet> VkGpuRhiBackend::CreateQuerySet(uint32 Count)
	{
		return new VulkanQuerySet(Count);
	}

	VulkanQuerySet::VulkanQuerySet(uint32 InCount)
		: GpuQuerySet(InCount)
	{
		VkQueryPoolCreateInfo PoolInfo{
			.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
			.queryType = VK_QUERY_TYPE_TIMESTAMP,
			.queryCount = InCount
		};
		VkCheck(vkCreateQueryPool(GDevice, &PoolInfo, nullptr, &Pool));
		GVkDeferredReleaseManager.AddResource(this);
	}

	VulkanQuerySet::~VulkanQuerySet()
	{
		if (Pool != VK_NULL_HANDLE)
		{
			vkDestroyQueryPool(GDevice, Pool, nullptr);
		}
	}

	double VulkanQuerySet::GetTimestampPeriodNs() const
	{
		VkPhysicalDeviceProperties Props;
		vkGetPhysicalDeviceProperties(GPhysicalDevice, &Props);
		return (double)Props.limits.timestampPeriod;
	}

	void VulkanQuerySet::ResolveResults(uint32 FirstQuery, uint32 QueryCount, TArray<uint64>& OutTimestamps)
	{
		OutTimestamps.SetNum(QueryCount);
		vkGetQueryPoolResults(GDevice, Pool, FirstQuery, QueryCount,
			QueryCount * sizeof(uint64), OutTimestamps.GetData(), sizeof(uint64),
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
	}
}
