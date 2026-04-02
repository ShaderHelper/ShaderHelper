#include "CommonHeader.h"
#include "VkCommandRecorder.h"
#include "VkGpuRhiBackend.h"
#include "VkMap.h"
#include "VkTexture.h"
#include "VkBuffer.h"
#include "VkPipeline.h"
#include "VkDescriptorSet.h"
#include "VkUtil.h"

namespace FW::VK
{
	VkRenderStateCache::VkRenderStateCache(VulkanCmdRecorder* InOwner, TArray<GpuTextureView*> InRenderTargetViews)
		: IsRenderPipelineDirty(false)
		, IsViewportDirty(false)
		, IsScissorRectDirty(false)
		, IsVertexBufferDirty(false)
		, IsIndexBufferDirty(false)
		, IsBindGroup0Dirty(false)
		, IsBindGroup1Dirty(false)
		, IsBindGroup2Dirty(false)
		, IsBindGroup3Dirty(false)
		, Owner(InOwner)
		, RenderTargetViews(MoveTemp(InRenderTargetViews))
	{
	}

	void VkRenderStateCache::SetPipeline(VulkanRenderPipelineState* InPipelineState)
	{
		if (InPipelineState != CurrentRenderPipelineState)
		{
			CurrentRenderPipelineState = InPipelineState;
			IsRenderPipelineDirty = true;
		}
	}

	void VkRenderStateCache::SetVertexBuffer(uint32 Slot, GpuBuffer* InBuffer, uint32 Offset)
	{
		if (CurrentVertexBuffers[Slot].Buffer != InBuffer || CurrentVertexBuffers[Slot].Offset != Offset)
		{
			CurrentVertexBuffers[Slot] = { InBuffer, Offset };
			IsVertexBufferDirty = true;
		}
	}

	void VkRenderStateCache::SetIndexBuffer(GpuBuffer* InBuffer, GpuFormat InIndexFormat, uint32 Offset)
	{
		if (CurrentIndexBuffer != InBuffer || CurrentIndexFormat != InIndexFormat || CurrentIndexOffset != Offset)
		{
			CurrentIndexBuffer = InBuffer;
			CurrentIndexFormat = InIndexFormat;
			CurrentIndexOffset = Offset;
			IsIndexBufferDirty = true;
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
			VkViewport DefaultViewPort{};
			DefaultViewPort.x = 0.0f;
			DefaultViewPort.y = 0.0f;
			DefaultViewPort.width = (float)RenderTargetViews[0]->GetWidth();
			DefaultViewPort.height = (float)RenderTargetViews[0]->GetHeight();
			DefaultViewPort.minDepth = 0.0f;
			DefaultViewPort.maxDepth = 1.0f;
			SetViewPort(MoveTemp(DefaultViewPort));
		}
		// Default scissor rect from viewport if user hasn't set one
		if (!CurrentScissorRect.IsSet() && CurrentViewPort)
		{
			VkRect2D DefaultScissorRect{};
			const int32 ScissorLeft = (int32)(*CurrentViewPort).x;
			const int32 ScissorTop = (int32)(*CurrentViewPort).y;
			const int32 ScissorRight = (int32)((*CurrentViewPort).x + (*CurrentViewPort).width);
			const int32 ScissorBottom = (int32)((*CurrentViewPort).y + (*CurrentViewPort).height);
			DefaultScissorRect.offset = { ScissorLeft, ScissorTop };
			DefaultScissorRect.extent = { (uint32)(ScissorRight - ScissorLeft), (uint32)(ScissorBottom - ScissorTop) };
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

		if (IsVertexBufferDirty && CurrentRenderPipelineState)
		{
			for (int32 BufferSlot = 0; BufferSlot < CurrentRenderPipelineState->GetDesc().VertexLayout.Num(); ++BufferSlot)
			{
				const VertexBufferBinding& Binding = CurrentVertexBuffers[BufferSlot];
				if (!Binding.Buffer)
				{
					continue;
				}
				VkBuffer VkBuffers[] = { static_cast<VulkanBuffer*>(Binding.Buffer)->GetBuffer() };
				VkDeviceSize Offsets[] = { Binding.Offset };
				vkCmdBindVertexBuffers(CmdBuffer, BufferSlot, 1, VkBuffers, Offsets);
			}
			IsVertexBufferDirty = false;
		}

		if (IsIndexBufferDirty)
		{
			if (CurrentIndexBuffer)
			{
				VulkanBuffer* VkBuffer = static_cast<VulkanBuffer*>(CurrentIndexBuffer);
				vkCmdBindIndexBuffer(CmdBuffer, VkBuffer->GetBuffer(), CurrentIndexOffset, MapIndexFormat(CurrentIndexFormat));
			}
			IsIndexBufferDirty = false;
		}

		auto ApplyBindGroup = [&, this](bool& IsDirty, GpuBindGroup* InBindGroup)
		{
			if (!IsDirty || !InBindGroup) return;
			VulkanBindGroup* VkBindGroup = static_cast<VulkanBindGroup*>(InBindGroup);
			VkDescriptorSet Set = VkBindGroup->GetDescriptorSet();
			uint32 SetIndex = InBindGroup->GetLayout()->GetGroupNumber();
			vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				CurrentRenderPipelineState->GetPipelineLayout(), SetIndex, 1, &Set, 0, nullptr);
			IsDirty = false;
		};
		ApplyBindGroup(IsBindGroup0Dirty, CurrentBindGroup0);
		ApplyBindGroup(IsBindGroup1Dirty, CurrentBindGroup1);
		ApplyBindGroup(IsBindGroup2Dirty, CurrentBindGroup2);
		ApplyBindGroup(IsBindGroup3Dirty, CurrentBindGroup3);
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
		if (IsComputePipelineDirty)
		{
			check(CurrentComputePipelineState);
			vkCmdBindPipeline(InCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, CurrentComputePipelineState->GetPipeline());
			IsComputePipelineDirty = false;
		}

		auto ApplyBindGroup = [&](bool& IsDirty, GpuBindGroup* InBindGroup)
		{
			if (!IsDirty || !InBindGroup) return;
			VulkanBindGroup* VkBindGroup = static_cast<VulkanBindGroup*>(InBindGroup);
			VkDescriptorSet Set = VkBindGroup->GetDescriptorSet();
			uint32 SetIndex = InBindGroup->GetLayout()->GetGroupNumber();
			vkCmdBindDescriptorSets(InCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
				CurrentComputePipelineState->GetPipelineLayout(), SetIndex, 1, &Set, 0, nullptr);
			IsDirty = false;
		};
		ApplyBindGroup(IsBindGroup0Dirty, CurrentBindGroup0);
		ApplyBindGroup(IsBindGroup1Dirty, CurrentBindGroup1);
		ApplyBindGroup(IsBindGroup2Dirty, CurrentBindGroup2);
		ApplyBindGroup(IsBindGroup3Dirty, CurrentBindGroup3);
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
		TArray<VkAttachmentReference> ResolveAttachmentRefs;

		for (int32 Index = 0; Index < PassDesc.ColorRenderTargets.Num(); Index ++)
		{
			const GpuRenderTargetInfo& RenderTargetInfo = PassDesc.ColorRenderTargets[Index];
			VulkanTextureView* RtView = static_cast<VulkanTextureView*>(RenderTargetInfo.View);
			const uint32 SampleCount = RtView->GetTexture()->GetSampleCount();
			const uint32 AttachmentIndex = static_cast<uint32>(AttachmentDescs.Num());
			Attachments.Add(RtView->GetView());
			AttachmentDescs.Add({
				.format = MapTextureFormat(RtView->GetTexture()->GetFormat()),
				.samples = MapSampleCount(SampleCount),
				.loadOp = MapLoadAction(RenderTargetInfo.LoadAction),
				.storeOp = MapStoreAction(RenderTargetInfo.StoreAction),
				.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			});
			VkClearValue& ColorClearValue = ClearValues.AddDefaulted_GetRef();
			ColorClearValue.color = { RenderTargetInfo.ClearColor.X, RenderTargetInfo.ClearColor.Y, RenderTargetInfo.ClearColor.Z, RenderTargetInfo.ClearColor.W };
			AttachmentRefs.Emplace(AttachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			if (RenderTargetInfo.ResolveTarget)
			{
				VulkanTextureView* ResolveView = static_cast<VulkanTextureView*>(RenderTargetInfo.ResolveTarget);
				ResolveAttachmentRefs.Add({
					.attachment = uint32_t(AttachmentDescs.Num()),
					.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				});
				Attachments.Add(ResolveView->GetView());
				AttachmentDescs.Add({
					.format = MapTextureFormat(ResolveView->GetTexture()->GetFormat()),
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				});
				ClearValues.AddDefaulted();
			}
			else if (SampleCount > 1)
			{
				ResolveAttachmentRefs.Add({
					.attachment = VK_ATTACHMENT_UNUSED,
					.layout = VK_IMAGE_LAYOUT_UNDEFINED,
				});
			}
		}

		VkAttachmentReference DepthAttachmentRef{};
		bool bHasDepth = PassDesc.DepthStencilTarget.IsSet();
		if (bHasDepth)
		{
			const GpuDepthStencilTargetInfo& DepthInfo = *PassDesc.DepthStencilTarget;
			VulkanTextureView* DsView = static_cast<VulkanTextureView*>(DepthInfo.View);
			DepthAttachmentRef = {
				.attachment = uint32_t(AttachmentDescs.Num()),
				.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			};
			Attachments.Add(DsView->GetView());
			AttachmentDescs.Add({
				.format = MapTextureFormat(DsView->GetTexture()->GetFormat()),
				.samples = MapSampleCount(DsView->GetTexture()->GetSampleCount()),
				.loadOp = MapLoadAction(DepthInfo.LoadAction),
				.storeOp = MapStoreAction(DepthInfo.StoreAction),
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			});
			VkClearValue& DepthClearValue = ClearValues.AddDefaulted_GetRef();
			DepthClearValue.depthStencil = { DepthInfo.ClearDepth, 0 };
		}

		VkSubpassDescription SubpassDesc{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = (uint32_t)PassDesc.ColorRenderTargets.Num(),
			.pColorAttachments = AttachmentRefs.GetData(),
			.pResolveAttachments = ResolveAttachmentRefs.Num() > 0 ? ResolveAttachmentRefs.GetData() : nullptr,
			.pDepthStencilAttachment = bHasDepth ? &DepthAttachmentRef : nullptr
		};
		VkRenderPassCreateInfo RenderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = (uint32_t)AttachmentDescs.Num(),
			.pAttachments = AttachmentDescs.GetData(),
			.subpassCount = 1,
			.pSubpasses = &SubpassDesc
		};
		VkRenderPass RenderPass;
		VkCheck(vkCreateRenderPass(GDevice, &RenderPassInfo, nullptr, &RenderPass));
		SetVkObjectName(VK_OBJECT_TYPE_RENDER_PASS, (uint64)RenderPass, PassName);

		VkFramebufferCreateInfo FrameBufferInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = RenderPass,
			.attachmentCount = (uint32_t)Attachments.Num(),
			.pAttachments = Attachments.GetData(),
			.width = PassDesc.ColorRenderTargets[0].View->GetWidth(),
			.height = PassDesc.ColorRenderTargets[0].View->GetHeight(),
			.layers = 1
		};
		VkFramebuffer FrameBuffer;
		VkCheck(vkCreateFramebuffer(GDevice, &FrameBufferInfo, nullptr, &FrameBuffer));

		TArray<GpuTextureView*> RTViews;
		for (int32 Index = 0; Index < PassDesc.ColorRenderTargets.Num(); Index++)
		{
			RTViews.Add(PassDesc.ColorRenderTargets[Index].View);
		}
		RenderPassRecorders.Add(MakeUnique<VulkanRenderPassRecorder>(this, RenderPass, FrameBuffer, MoveTemp(RTViews), PassDesc.TimestampWrites));

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

		if (PassDesc.TimestampWrites)
		{
			const auto& TsWrites = *PassDesc.TimestampWrites;
			VulkanQuerySet* VkQuerySet = static_cast<VulkanQuerySet*>(TsWrites.QuerySet);
			vkCmdResetQueryPool(CommandBuffer, VkQuerySet->GetPool(), TsWrites.BeginningOfPassWriteIndex, 1);
			vkCmdWriteTimestamp(CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VkQuerySet->GetPool(), TsWrites.BeginningOfPassWriteIndex);
		}

		vkCmdBeginRenderPass(CommandBuffer, &PassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		return RenderPassRecorders.Last().Get();
	}

	void VulkanCmdRecorder::EndRenderPass(GpuRenderPassRecorder* InRenderPassRecorder)
	{
		vkCmdEndRenderPass(CommandBuffer);

		VulkanRenderPassRecorder* VkRecorder = static_cast<VulkanRenderPassRecorder*>(InRenderPassRecorder);
		if (VkRecorder->TimestampWrites)
		{
			const auto& TsWrites = *VkRecorder->TimestampWrites;
			VulkanQuerySet* VkQuerySet = static_cast<VulkanQuerySet*>(TsWrites.QuerySet);
			vkCmdResetQueryPool(CommandBuffer, VkQuerySet->GetPool(), TsWrites.EndOfPassWriteIndex, 1);
			vkCmdWriteTimestamp(CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkQuerySet->GetPool(), TsWrites.EndOfPassWriteIndex);
		}
	}

	void VulkanCmdRecorder::BeginCaptureEvent(const FString& EventName)
	{
		FTCHARToUTF8 Utf8Name(*EventName);
		VkDebugUtilsLabelEXT Label{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
			.pLabelName = Utf8Name.Get(),
			.color = {1.0f, 1.0f, 1.0f, 1.0f}
		};
		vkCmdBeginDebugUtilsLabelEXT(CommandBuffer, &Label);
	}

	void VulkanCmdRecorder::EndCaptureEvent()
	{
		vkCmdEndDebugUtilsLabelEXT(CommandBuffer);
	}

	void VulkanCmdRecorder::Barriers(const TArray<GpuBarrierInfo>& BarrierInfos)
	{
		if (BarrierInfos.IsEmpty())
		{
			return;
		}

		TArray<VkImageMemoryBarrier> ImageBarriers;
		TArray<VkBufferMemoryBarrier> BufferBarriers;
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
			if (EnumHasAnyFlags(InState, GpuResourceState::DepthStencilWrite))
			{
				OutAccess |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				OutStage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			}
		};

		for (const GpuBarrierInfo& Info : BarrierInfos)
		{
			check(Info.NewState != GpuResourceState::Unknown);

			VkAccessFlags DstAccess{};
			VkPipelineStageFlags DstStage{};
			GetAccessAndStage(Info.NewState, DstAccess, DstStage);

			if (Info.Resource->GetType() == GpuResourceType::Texture)
			{
				VulkanTexture* VkTex = static_cast<VulkanTexture*>(Info.Resource);
				uint32 NumMips = VkTex->GetNumMips();
				uint32 ArrayLayers = VkTex->GetArrayLayerCount();
				VkImageLayout NewLayout = MapImageLayout(Info.NewState);
				VkImageAspectFlags AspectMask = EnumHasAnyFlags(VkTex->GetResourceDesc().Usage, GpuTextureUsage::DepthStencil)
					? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

				for (uint32 Mip = 0; Mip < NumMips; Mip++)
				{
					for (uint32 Layer = 0; Layer < ArrayLayers; Layer++)
					{
						GpuResourceState SubOldState = GetLocalTextureSubResourceState(VkTex, Mip, Layer);
						if (SubOldState == Info.NewState && Info.NewState != GpuResourceState::UnorderedAccess) continue;

						VkAccessFlags SrcAccess{};
						VkPipelineStageFlags SrcStage{};
						GetAccessAndStage(SubOldState, SrcAccess, SrcStage);

						ImageBarriers.Add({
							.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
							.srcAccessMask = SrcAccess,
							.dstAccessMask = DstAccess,
							.oldLayout = MapImageLayout(SubOldState),
							.newLayout = NewLayout,
							.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
							.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
							.image = VkTex->GetImage(),
							.subresourceRange = {
								.aspectMask = AspectMask,
								.baseMipLevel = Mip,
								.levelCount = 1,
								.baseArrayLayer = Layer,
								.layerCount = 1
							},
						});
						SrcStageMask |= SrcStage;
					}
				}
				SetLocalTextureAllSubResourceStates(VkTex, Info.NewState);
			}
			else if (Info.Resource->GetType() == GpuResourceType::TextureView)
			{
				GpuTextureView* View = static_cast<GpuTextureView*>(Info.Resource);
				VulkanTexture* VkTex = static_cast<VulkanTexture*>(View->GetTexture());
				uint32 ArrayLayers = VkTex->GetArrayLayerCount();
				VkImageLayout NewLayout = MapImageLayout(Info.NewState);
				VkImageAspectFlags AspectMask = EnumHasAnyFlags(VkTex->GetResourceDesc().Usage, GpuTextureUsage::DepthStencil)
					? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

				for (uint32 i = 0; i < View->GetMipLevelCount(); i++)
				{
					uint32 Mip = View->GetBaseMipLevel() + i;
					for (uint32 Layer = 0; Layer < ArrayLayers; Layer++)
					{
						GpuResourceState SubOldState = GetLocalTextureSubResourceState(VkTex, Mip, Layer);
						if (SubOldState == Info.NewState && Info.NewState != GpuResourceState::UnorderedAccess) continue;

						VkAccessFlags SrcAccess{};
						VkPipelineStageFlags SrcStage{};
						GetAccessAndStage(SubOldState, SrcAccess, SrcStage);

						ImageBarriers.Add({
							.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
							.srcAccessMask = SrcAccess,
							.dstAccessMask = DstAccess,
							.oldLayout = MapImageLayout(SubOldState),
							.newLayout = NewLayout,
							.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
							.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
							.image = VkTex->GetImage(),
							.subresourceRange = {
								.aspectMask = AspectMask,
								.baseMipLevel = Mip,
								.levelCount = 1,
								.baseArrayLayer = Layer,
								.layerCount = 1
							},
						});
						SrcStageMask |= SrcStage;
						SetLocalTextureSubResourceState(VkTex, Mip, Layer, Info.NewState);
					}
				}
			}
			else if (Info.Resource->GetType() == GpuResourceType::Buffer)
			{
				VulkanBuffer* VkBuffer = static_cast<VulkanBuffer*>(Info.Resource);
				GpuResourceState OldState = GetLocalBufferState(VkBuffer);
				if (OldState == Info.NewState && Info.NewState != GpuResourceState::UnorderedAccess)
				{
					continue;
				}

				VkAccessFlags SrcAccess{};
				VkPipelineStageFlags SrcStage{};
				GetAccessAndStage(OldState, SrcAccess, SrcStage);

				BufferBarriers.Add({
					.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
					.srcAccessMask = SrcAccess,
					.dstAccessMask = DstAccess,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.buffer = VkBuffer->GetBuffer(),
					.offset = 0,
					.size = VK_WHOLE_SIZE
				});
				SrcStageMask |= SrcStage;
				SetLocalBufferState(VkBuffer, Info.NewState);
			}
			else
			{
				AUX::Unreachable();
			}

			DstStageMask |= DstStage;
		}

		if (SrcStageMask == 0) SrcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		if (DstStageMask == 0) DstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

		vkCmdPipelineBarrier(CommandBuffer, SrcStageMask, DstStageMask, 0,
			0, nullptr,
			BufferBarriers.Num(), BufferBarriers.GetData(),
			ImageBarriers.Num(), ImageBarriers.GetData());
	}

	void VulkanCmdRecorder::CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture, uint32 ArrayLayer, uint32 MipLevel)
	{
		VulkanBuffer* SrcBuffer = static_cast<VulkanBuffer*>(InBuffer);
		VulkanTexture* DstTexture = static_cast<VulkanTexture*>(InTexture);

		VkBufferImageCopy Region{
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = MipLevel,
				.baseArrayLayer = ArrayLayer,
				.layerCount = 1
			},
			.imageOffset = {0, 0, 0},
			.imageExtent = {InTexture->GetWidth(), InTexture->GetHeight(), InTexture->GetDepth()}
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
		StateCache.ApplyComputeState(Owner->GetCommandBuffer());
		vkCmdDispatch(Owner->GetCommandBuffer(), ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	void VulkanComputePassRecorder::SetComputePipelineState(GpuComputePipelineState* InPipelineState)
	{
		StateCache.SetPipeline(static_cast<VulkanComputePipelineState*>(InPipelineState));
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

	void VulkanRenderPassRecorder::DrawIndexed(uint32 StartIndexLocation, uint32 IndexCount, int32 BaseVertexLocation, uint32 StartInstanceLocation, uint32 InstanceCount)
	{
		StateCache.ApplyDrawState();
		vkCmdDrawIndexed(Owner->GetCommandBuffer(), IndexCount, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
	}

	void VulkanRenderPassRecorder::SetRenderPipelineState(GpuRenderPipelineState* InPipelineState)
	{
		StateCache.SetPipeline(static_cast<VulkanRenderPipelineState*>(InPipelineState));
	}

	void VulkanRenderPassRecorder::SetVertexBuffer(uint32 Slot, GpuBuffer* InVertexBuffer, uint32 Offset)
	{
		StateCache.SetVertexBuffer(Slot, InVertexBuffer, Offset);
	}

	void VulkanRenderPassRecorder::SetIndexBuffer(GpuBuffer* InIndexBuffer, GpuFormat IndexFormat, uint32 Offset)
	{
		StateCache.SetIndexBuffer(InIndexBuffer, IndexFormat, Offset);
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
