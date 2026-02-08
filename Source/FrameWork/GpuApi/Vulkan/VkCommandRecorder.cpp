#include "CommonHeader.h"
#include "VkCommandRecorder.h"
#include "VkMap.h"
#include "VkTexture.h"

namespace FW
{
	GpuComputePassRecorder* VulkanCmdRecorder::BeginComputePass(const FString& PassName)
	{
		return nullptr;
	}

	void VulkanCmdRecorder::EndComputePass(GpuComputePassRecorder* InComputePassRecorder)
	{
	}

	GpuRenderPassRecorder* VulkanCmdRecorder::BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName)
	{
		TArray<VkClearValue> ClearValues;
		TArray<VkImageView> Attachments;
		TArray<VkAttachmentDescription> AttachmentDescs;
		TArray<VkAttachmentReference> AttachmentRefs;
		if (PassDesc.ColorRenderTargets.Num() == 0)
		{
			AttachmentRefs.Emplace(VK_ATTACHMENT_UNUSED);
		}

		for (int32 Index = 0; Index < PassDesc.ColorRenderTargets.Num(); Index ++)
		{
			const GpuRenderTargetInfo& RenderTargetInfo = PassDesc.ColorRenderTargets[Index];
			VulkanTexture* RT = static_cast<VulkanTexture*>(RenderTargetInfo.Texture);
			if (RenderTargetInfo.LoadAction == RenderTargetLoadAction::Clear)
			{
				ClearValues.Add({ .color = {RenderTargetInfo.ClearColor.X, RenderTargetInfo.ClearColor.Y, RenderTargetInfo.ClearColor.Z, RenderTargetInfo.ClearColor.W} });
			}
			Attachments.Add(RT->GetView());
			AttachmentDescs.Add({
				.format = MapTextureFormat(RenderTargetInfo.Texture->GetFormat()),
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = MapLoadAction(RenderTargetInfo.LoadAction),
				.storeOp = MapStoreAction(RenderTargetInfo.StoreAction),
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_GENERAL
			});
			AttachmentRefs.Emplace(Index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		}

		VkSubpassDescription SubpassDesc{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = (uint32_t)PassDesc.ColorRenderTargets.Num(),
			.pColorAttachments = AttachmentRefs.GetData()
		};
		VkRenderPassCreateInfo RenderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = (uint32_t)PassDesc.ColorRenderTargets.Num(),
			.pAttachments = AttachmentDescs.GetData(),
			.subpassCount = 1,
			.pSubpasses = &SubpassDesc
		};
		VkRenderPass RenderPass;
		VkCheck(vkCreateRenderPass(GDevice, &RenderPassInfo, nullptr, &RenderPass));

		VkFramebufferCreateInfo FrameBufferInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = RenderPass,
			.attachmentCount = (uint32_t)PassDesc.ColorRenderTargets.Num(),
			.pAttachments = Attachments.GetData(),
			.width = PassDesc.ColorRenderTargets.Num() > 0 ? PassDesc.ColorRenderTargets[0].Texture->GetWidth() : 1,
			.height = PassDesc.ColorRenderTargets.Num() > 0 ? PassDesc.ColorRenderTargets[0].Texture->GetHeight() : 1,
			.layers = 1
		};
		VkFramebuffer FrameBuffer;
		VkCheck(vkCreateFramebuffer(GDevice, &FrameBufferInfo, nullptr, &FrameBuffer));
		RenderPassRecorders.Add(MakeUnique<VulkanRenderPassRecorder>(RenderPass, FrameBuffer));

		VkRenderPassBeginInfo PassBeginInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = RenderPass,
			.framebuffer = FrameBuffer,
			.renderArea = {
				.offset = {0, 0},
				.extent = {FrameBufferInfo.width, FrameBufferInfo.height}
			},
			.clearValueCount = (uint32_t)ClearValues.Num(),
			.pClearValues = ClearValues.GetData()
		};
		vkCmdBeginRenderPass(CommandBuffer, &PassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		return RenderPassRecorders.Last().Get();
	}

	void VulkanCmdRecorder::EndRenderPass(GpuRenderPassRecorder* InRenderPassRecorder)
	{
		vkCmdEndRenderPass(CommandBuffer);
	}

	void VulkanCmdRecorder::BeginCaptureEvent(const FString& EventName)
	{
	}

	void VulkanCmdRecorder::EndCaptureEvent()
	{
	}

	void VulkanCmdRecorder::Barriers(const TArray<GpuBarrierInfo>& BarrierInfos)
	{
	}

	void VulkanCmdRecorder::CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture)
	{
	}

	void VulkanCmdRecorder::CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer)
	{
	}

	void VulkanCmdRecorder::CopyBufferToBuffer(GpuBuffer* SrcBuffer, uint32 SrcOffset, GpuBuffer* DestBuffer, uint32 DestOffset, uint32 Size)
	{
	}

	void VulkanComputePassRecorder::Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
	{
	}

	void VulkanComputePassRecorder::SetComputePipelineState(GpuComputePipelineState* InPipelineState)
	{
	}

	void VulkanComputePassRecorder::SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
	{
	}

	void VulkanRenderPassRecorder::DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount)
	{
	}

	void VulkanRenderPassRecorder::SetRenderPipelineState(GpuRenderPipelineState* InPipelineState)
	{
	}

	void VulkanRenderPassRecorder::SetVertexBuffer(GpuBuffer* InVertexBuffer)
	{
	}

	void VulkanRenderPassRecorder::SetViewPort(const GpuViewPortDesc& InViewPortDesc)
	{
	}

	void VulkanRenderPassRecorder::SetScissorRect(const GpuScissorRectDesc& InScissorRectDes)
	{
	}

	void VulkanRenderPassRecorder::SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
	{

	}

	VulkanCmdRecorder* VulkanCmdRecorderPool::AcquireCmdRecorder(const FString& RecorderName)
	{
		if (CommandPool == VK_NULL_HANDLE)
		{
			VkCommandPoolCreateInfo PoolCreateInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
				.queueFamilyIndex = GraphicsQueueIndex
			};
			VkCheck(vkCreateCommandPool(GDevice, &PoolCreateInfo, nullptr, &CommandPool));
		}

		VkCommandBufferAllocateInfo CmdBufAllocInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = CommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		VkCommandBuffer CommandBuffer;
		VkCheck(vkAllocateCommandBuffers(GDevice, &CmdBufAllocInfo, &CommandBuffer));
		CmdRecorders.Add(MakeUnique<VulkanCmdRecorder>(CommandBuffer));
		return CmdRecorders.Last().Get();
	}
}
