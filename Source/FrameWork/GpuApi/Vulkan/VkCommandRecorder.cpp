#include "CommonHeader.h"
#include "VkCommandRecorder.h"
#include "VkMap.h"
#include "VkTexture.h"
#include "VkBuffer.h"
#include "VkPipeline.h"

namespace FW
{
	VkRenderStateCache::VkRenderStateCache(VulkanCmdRecorder* InOwner, TArray<GpuTexture*> InRenderTargets)
		: IsRenderPipelineDirty(false)
		, IsViewportDirty(false)
		, IsScissorRectDirty(false)
		, IsVertexBufferDirty(false)
		, IsBindGroup0Dirty(false)
		, IsBindGroup1Dirty(false)
		, IsBindGroup2Dirty(false)
		, IsBindGroup3Dirty(false)
		, Owner(InOwner)
		, RenderTargets(MoveTemp(InRenderTargets))
	{}

	void VkRenderStateCache::SetPipeline(VulkanRenderPipelineState* InPipelineState)
	{
		if (InPipelineState != CurrentRenderPipelineState)
		{
			CurrentRenderPipelineState = InPipelineState;
			IsRenderPipelineDirty = true;
		}
	}

	void VkRenderStateCache::SetVertexBuffer(GpuBuffer* InBuffer)
	{
		if (CurrentVertexBuffer != InBuffer)
		{
			CurrentVertexBuffer = InBuffer;
			IsVertexBufferDirty = true;
		}
	}

	void VkRenderStateCache::SetViewPort(VkViewport InViewPort)
	{
		if (!CurrentViewPort || FMemory::Memcmp(&*CurrentViewPort, &InViewPort, sizeof(VkViewport)))
		{
			CurrentViewPort = MoveTemp(InViewPort);
			IsViewportDirty = true;
		}
	}

	void VkRenderStateCache::SetScissorRect(VkRect2D InScissorRect)
	{
		if (!CurrentScissorRect || FMemory::Memcmp(&*CurrentScissorRect, &InScissorRect, sizeof(VkRect2D)))
		{
			CurrentScissorRect = MoveTemp(InScissorRect);
			IsScissorRectDirty = true;
		}
	}

	void VkRenderStateCache::SetBindGroups(GpuBindGroup* InGroup0, GpuBindGroup* InGroup1, GpuBindGroup* InGroup2, GpuBindGroup* InGroup3)
	{
		if (InGroup0 != CurrentBindGroup0) {
			CurrentBindGroup0 = InGroup0;
			IsBindGroup0Dirty = true;
		}
		if (InGroup1 != CurrentBindGroup1) {
			CurrentBindGroup1 = InGroup1;
			IsBindGroup1Dirty = true;
		}
		if (InGroup2 != CurrentBindGroup2) {
			CurrentBindGroup2 = InGroup2;
			IsBindGroup2Dirty = true;
		}
		if (InGroup3 != CurrentBindGroup3) {
			CurrentBindGroup3 = InGroup3;
			IsBindGroup3Dirty = true;
		}
	}

	void VkRenderStateCache::ApplyDrawState()
	{
		VkCommandBuffer CmdBuffer = Owner->GetCommandBuffer();

		// Default viewport from render target if user hasn't set one
		if (!CurrentViewPort.IsSet())
		{
			if (RenderTargets.Num() > 0)
			{
				VkViewport DefaultViewPort{};
				DefaultViewPort.x = 0.0f;
				DefaultViewPort.y = 0.0f;
				DefaultViewPort.width = (float)RenderTargets[0]->GetWidth();
				DefaultViewPort.height = (float)RenderTargets[0]->GetHeight();
				DefaultViewPort.minDepth = 0.0f;
				DefaultViewPort.maxDepth = 1.0f;
				SetViewPort(MoveTemp(DefaultViewPort));
			}
		}
		// Default scissor rect from viewport if user hasn't set one
		if (!CurrentScissorRect.IsSet() && CurrentViewPort)
		{
			VkRect2D DefaultScissorRect{};
			DefaultScissorRect.offset = { 0, 0 };
			DefaultScissorRect.extent = { (uint32)(*CurrentViewPort).width, (uint32)(*CurrentViewPort).height };
			SetScissorRect(MoveTemp(DefaultScissorRect));
		}

		if (IsRenderPipelineDirty)
		{
			check(CurrentRenderPipelineState);
			vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, CurrentRenderPipelineState->GetPipeline());
			IsRenderPipelineDirty = false;
		}

		if (IsViewportDirty && CurrentViewPort)
		{
			vkCmdSetViewport(CmdBuffer, 0, 1, &*CurrentViewPort);
			IsViewportDirty = false;
		}

		if (IsScissorRectDirty && CurrentScissorRect)
		{
			vkCmdSetScissor(CmdBuffer, 0, 1, &*CurrentScissorRect);
			IsScissorRectDirty = false;
		}

		// TODO: Apply vertex buffer binding
		// TODO: Apply bind groups (descriptor sets)
	}

	VkComputeStateCache::VkComputeStateCache()
		: IsComputePipelineDirty(false)
		, IsBindGroup0Dirty(false)
		, IsBindGroup1Dirty(false)
		, IsBindGroup2Dirty(false)
		, IsBindGroup3Dirty(false)
	{}

	void VkComputeStateCache::SetPipeline(VulkanComputePipelineState* InPipelineState)
	{
		if (InPipelineState != CurrentComputePipelineState)
		{
			CurrentComputePipelineState = InPipelineState;
			IsComputePipelineDirty = true;
		}
	}

	void VkComputeStateCache::SetBindGroups(GpuBindGroup* InGroup0, GpuBindGroup* InGroup1, GpuBindGroup* InGroup2, GpuBindGroup* InGroup3)
	{
		if (InGroup0 != CurrentBindGroup0) {
			CurrentBindGroup0 = InGroup0;
			IsBindGroup0Dirty = true;
		}
		if (InGroup1 != CurrentBindGroup1) {
			CurrentBindGroup1 = InGroup1;
			IsBindGroup1Dirty = true;
		}
		if (InGroup2 != CurrentBindGroup2) {
			CurrentBindGroup2 = InGroup2;
			IsBindGroup2Dirty = true;
		}
		if (InGroup3 != CurrentBindGroup3) {
			CurrentBindGroup3 = InGroup3;
			IsBindGroup3Dirty = true;
		}
	}

	void VkComputeStateCache::ApplyComputeState(VkCommandBuffer InCmdBuffer)
	{
		// TODO: Apply compute pipeline and descriptor sets
	}
	GpuComputePassRecorder* VulkanCmdRecorder::BeginComputePass(const FString& PassName)
	{
		ComputePassRecorders.Add(MakeUnique<VulkanComputePassRecorder>(this));
		return ComputePassRecorders.Last().Get();
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
				.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
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

		TArray<GpuTexture*> RTs;
		for (int32 Index = 0; Index < PassDesc.ColorRenderTargets.Num(); Index++)
		{
			RTs.Add(PassDesc.ColorRenderTargets[Index].Texture);
		}
		RenderPassRecorders.Add(MakeUnique<VulkanRenderPassRecorder>(this, RenderPass, FrameBuffer, MoveTemp(RTs)));

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
		TArray<VkImageMemoryBarrier> ImageBarriers;
		VkPipelineStageFlags SrcStageMask = 0;
		VkPipelineStageFlags DstStageMask = 0;

		auto GetAccessAndStage = [](GpuResourceState InState, VkAccessFlags& OutAccess, VkPipelineStageFlags& OutStage)
		{
			if (InState == GpuResourceState::Unknown)
			{
				OutStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				return;
			}
			if (EnumHasAnyFlags(InState, GpuResourceState::UniformBuffer))
			{
				OutAccess |= VK_ACCESS_UNIFORM_READ_BIT;
				OutStage |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			}
			if (EnumHasAnyFlags(InState, GpuResourceState::ShaderResourceRead))
			{
				OutAccess |= VK_ACCESS_SHADER_READ_BIT;
				OutStage |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			}
			if (EnumHasAnyFlags(InState, GpuResourceState::CopySrc))
			{
				OutAccess |= VK_ACCESS_TRANSFER_READ_BIT;
				OutStage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			if (EnumHasAnyFlags(InState, GpuResourceState::RenderTargetWrite))
			{
				OutAccess |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				OutStage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			}
			if (EnumHasAnyFlags(InState, GpuResourceState::CopyDst))
			{
				OutAccess |= VK_ACCESS_TRANSFER_WRITE_BIT;
				OutStage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			if (EnumHasAnyFlags(InState, GpuResourceState::UnorderedAccess))
			{
				OutAccess |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
				OutStage |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			}
		};

		for (const GpuBarrierInfo& Info : BarrierInfos)
		{
			check(Info.NewState != GpuResourceState::Unknown);

			VkAccessFlags SrcAccess{};
			VkAccessFlags DstAccess{};
			VkPipelineStageFlags SrcStage{};
			VkPipelineStageFlags DstStage{};
			GetAccessAndStage(Info.Resource->State, SrcAccess, SrcStage);
			GetAccessAndStage(Info.NewState, DstAccess, DstStage);

			if (Info.Resource->GetType() == GpuResourceType::Texture)
			{
				VulkanTexture* VkTex = static_cast<VulkanTexture*>(Info.Resource);
				VkImageLayout OldLayout = MapImageLayout(Info.Resource->State);
				VkImageLayout NewLayout = MapImageLayout(Info.NewState);

				ImageBarriers.Add({
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = SrcAccess,
					.dstAccessMask = DstAccess,
					.oldLayout = OldLayout,
					.newLayout = NewLayout,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = VkTex->GetImage(),
					.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS },
				});
			}
			else
			{
				AUX::Unreachable();
			}

			SrcStageMask |= SrcStage;
			DstStageMask |= DstStage;
			Info.Resource->State = Info.NewState;
		}

		check(SrcStageMask != 0 && DstStageMask != 0);

		vkCmdPipelineBarrier(CommandBuffer, SrcStageMask, DstStageMask, 0,
			0, nullptr,
			0, nullptr,
			ImageBarriers.Num(), ImageBarriers.GetData());
	}

	void VulkanCmdRecorder::CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture)
	{
		VulkanBuffer* SrcBuffer = static_cast<VulkanBuffer*>(InBuffer);
		VulkanTexture* DstTexture = static_cast<VulkanTexture*>(InTexture);

		VkBufferImageCopy Region{
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
			.imageOffset = {0, 0, 0},
			.imageExtent = {InTexture->GetWidth(), InTexture->GetHeight(), 1}
		};

		vkCmdCopyBufferToImage(CommandBuffer, SrcBuffer->GetBuffer(), DstTexture->GetImage(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
	}

	void VulkanCmdRecorder::CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer)
	{
		VulkanTexture* SrcTexture = static_cast<VulkanTexture*>(InTexture);
		VulkanBuffer* DstBuffer = static_cast<VulkanBuffer*>(InBuffer);

		VkBufferImageCopy Region{
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
			.imageOffset = {0, 0, 0},
			.imageExtent = {InTexture->GetWidth(), InTexture->GetHeight(), 1}
		};

		vkCmdCopyImageToBuffer(CommandBuffer, SrcTexture->GetImage(),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DstBuffer->GetBuffer(), 1, &Region);
	}

	void VulkanCmdRecorder::CopyBufferToBuffer(GpuBuffer* SrcBuffer, uint32 SrcOffset, GpuBuffer* DestBuffer, uint32 DestOffset, uint32 Size)
	{
		VulkanBuffer* VkSrcBuffer = static_cast<VulkanBuffer*>(SrcBuffer);
		VulkanBuffer* VkDstBuffer = static_cast<VulkanBuffer*>(DestBuffer);

		VkBufferCopy CopyRegion{
			.srcOffset = SrcOffset,
			.dstOffset = DestOffset,
			.size = Size
		};

		vkCmdCopyBuffer(CommandBuffer, VkSrcBuffer->GetBuffer(), VkDstBuffer->GetBuffer(), 1, &CopyRegion);
	}

	void VulkanComputePassRecorder::Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
	{
	}

	void VulkanComputePassRecorder::SetComputePipelineState(GpuComputePipelineState* InPipelineState)
	{
		// TODO: StateCache.SetPipeline(static_cast<VulkanComputePipelineState*>(InPipelineState));
	}

	void VulkanComputePassRecorder::SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
	{
		StateCache.SetBindGroups(BindGroup0, BindGroup1, BindGroup2, BindGroup3);
	}

	void VulkanRenderPassRecorder::DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount)
	{
		StateCache.ApplyDrawState();
		vkCmdDraw(Owner->GetCommandBuffer(), VertexCount, InstanceCount, StartVertexLocation, StartInstanceLocation);
	}

	void VulkanRenderPassRecorder::SetRenderPipelineState(GpuRenderPipelineState* InPipelineState)
	{
		StateCache.SetPipeline(static_cast<VulkanRenderPipelineState*>(InPipelineState));
	}

	void VulkanRenderPassRecorder::SetVertexBuffer(GpuBuffer* InVertexBuffer)
	{
		StateCache.SetVertexBuffer(InVertexBuffer);
	}

	void VulkanRenderPassRecorder::SetViewPort(const GpuViewPortDesc& InViewPortDesc)
	{
		VkViewport ViewPort{};
		ViewPort.x = InViewPortDesc.TopLeftX;
		ViewPort.y = InViewPortDesc.TopLeftY;
		ViewPort.width = InViewPortDesc.Width;
		ViewPort.height = InViewPortDesc.Height;
		ViewPort.minDepth = InViewPortDesc.ZMin;
		ViewPort.maxDepth = InViewPortDesc.ZMax;
		StateCache.SetViewPort(MoveTemp(ViewPort));
	}

	void VulkanRenderPassRecorder::SetScissorRect(const GpuScissorRectDesc& InScissorRectDes)
	{
		VkRect2D ScissorRect{};
		ScissorRect.offset = { (int32)InScissorRectDes.Left, (int32)InScissorRectDes.Top };
		ScissorRect.extent = { InScissorRectDes.Right - InScissorRectDes.Left, InScissorRectDes.Bottom - InScissorRectDes.Top };
		StateCache.SetScissorRect(MoveTemp(ScissorRect));
	}

	void VulkanRenderPassRecorder::SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
	{
		StateCache.SetBindGroups(BindGroup0, BindGroup1, BindGroup2, BindGroup3);
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
