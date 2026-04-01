#include "CommonHeader.h"
#include "VkPipeline.h"
#include "VkShader.h"
#include "VkMap.h"
#include "VkGpuRhiBackend.h"
#include "VkUtil.h"
#include "VkDescriptorSet.h"
#include "GpuApi/GpuApiValidation.h"

namespace FW::VK
{
	VulkanComputePipelineState::VulkanComputePipelineState(GpuComputePipelineStateDesc InDesc, VkPipeline InPipeline, VkPipelineLayout InPipelineLayout)
		: GpuComputePipelineState(InDesc)
		, Pipeline(InPipeline), PipelineLayout(InPipelineLayout)
	{
		GVkDeferredReleaseManager.AddResource(this);
	}

	VulkanRenderPipelineState::VulkanRenderPipelineState(GpuRenderPipelineStateDesc InDesc, VkPipeline InPipeline, VkPipelineLayout InPipelineLayout, VkRenderPass InRednedrPass)
		: GpuRenderPipelineState(InDesc)
		, Pipeline(InPipeline), PipelineLayout(InPipelineLayout), RenderPass(InRednedrPass)
	{
		GVkDeferredReleaseManager.AddResource(this);
	}

	TRefCountPtr<VulkanRenderPipelineState> CreateVulkanRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc)
	{
		const VkDynamicState DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo DynamicStateInfo = { 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO ,
			.dynamicStateCount = UE_ARRAY_COUNT(DynamicStates),
			.pDynamicStates = DynamicStates
		};

		VulkanShader* Vs = static_cast<VulkanShader*>(InPipelineStateDesc.Vs);
		VulkanShader* Ps = static_cast<VulkanShader*>(InPipelineStateDesc.Ps);

		if (InPipelineStateDesc.CheckLayout)
		{
			CheckShaderLayoutBinding(InPipelineStateDesc, Vs);
			if (Ps)
			{
				CheckShaderLayoutBinding(InPipelineStateDesc, Ps);
			}
		}

		TArray<VkPipelineShaderStageCreateInfo> Stages;
		auto VsEntryPoint = StringCast<UTF8CHAR>(*Vs->GetEntryPoint());
		Stages.Add({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = Vs->GetCompilationResult(),
			.pName = (char*)VsEntryPoint.Get()
		});
		if (Ps)
		{
			auto PsEntryPoint = StringCast<UTF8CHAR>(*Ps->GetEntryPoint());
			Stages.Add({
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = Ps->GetCompilationResult(),
				.pName = (char*)PsEntryPoint.Get()
			});
		}

		TArray<VkVertexInputBindingDescription> BindingDescriptions;
		TArray<VkVertexInputAttributeDescription> AttributeDescriptions;
		for (int32 BufferSlot = 0; BufferSlot < InPipelineStateDesc.VertexLayout.Num(); ++BufferSlot)
		{
			const GpuVertexLayoutDesc& BufferLayout = InPipelineStateDesc.VertexLayout[BufferSlot];
			BindingDescriptions.Add({
				.binding = static_cast<uint32>(BufferSlot),
				.stride = BufferLayout.ByteStride,
				.inputRate = MapVertexStepMode(BufferLayout.StepMode)
			});
			for (const GpuVertexAttributeDesc& Attribute : BufferLayout.Attributes)
			{
				AttributeDescriptions.Add({
					.location = Attribute.Location,
					.binding = static_cast<uint32>(BufferSlot),
					.format = MapTextureFormat(Attribute.Format),
					.offset = Attribute.ByteOffset
				});
			}
		}

		VkPipelineVertexInputStateCreateInfo VertexInputInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = (uint32)BindingDescriptions.Num(),
			.pVertexBindingDescriptions = BindingDescriptions.GetData(),
			.vertexAttributeDescriptionCount = (uint32)AttributeDescriptions.Num(),
			.pVertexAttributeDescriptions = AttributeDescriptions.GetData(),
		};
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = MapPrimitiveType(InPipelineStateDesc.Primitive)
		};
		VkPipelineViewportStateCreateInfo ViewportStateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1
		};
		VkPipelineRasterizationStateCreateInfo RasterizationInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.polygonMode = MapRasterizerFillMode(InPipelineStateDesc.RasterizerState.FillMode),
			.cullMode = MapRasterizerCullMode(InPipelineStateDesc.RasterizerState.CullMode),
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.lineWidth = 1.0f
		};
		VkPipelineMultisampleStateCreateInfo MultisampleInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
		};
		TArray<VkPipelineColorBlendAttachmentState> BlendAttachments;
		for (const auto& Target : InPipelineStateDesc.Targets)
		{
			BlendAttachments.Add({
				.blendEnable = Target.BlendEnable ? VK_TRUE : VK_FALSE,
				.srcColorBlendFactor = MapBlendFactor(Target.SrcFactor),
				.dstColorBlendFactor = MapBlendFactor(Target.DestFactor),
				.colorBlendOp = MapBlendOp(Target.ColorOp),
				.srcAlphaBlendFactor = MapBlendFactor(Target.SrcAlphaFactor),
				.dstAlphaBlendFactor = MapBlendFactor(Target.DestAlphaFactor),
				.alphaBlendOp = MapBlendOp(Target.AlphaOp),
				.colorWriteMask = MapColorWriteMask(Target.Mask)
			});
		}
		VkPipelineColorBlendStateCreateInfo ColorBlendInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.attachmentCount = (uint32_t)BlendAttachments.Num(),
			.pAttachments = BlendAttachments.GetData()
		};
		
		static VkDescriptorSetLayout DummyLayout = [&] {
			VkDescriptorSetLayoutCreateInfo LayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
			VkDescriptorSetLayout DummyLayout;
			VkCheck(vkCreateDescriptorSetLayout(GDevice, &LayoutInfo, nullptr, &DummyLayout));
			return DummyLayout;
		}();
		TArray<VkDescriptorSetLayout, TFixedAllocator<GpuResourceLimit::MaxBindableBingGroupNum>> SetLayouts;
		SetLayouts.Init(DummyLayout, GpuResourceLimit::MaxBindableBingGroupNum);
		auto AddLayout = [&](GpuBindGroupLayout* InLayout)
		{
			if (!InLayout) { return; }
			VulkanBindGroupLayout* BindGroupLayout = static_cast<VulkanBindGroupLayout*>(InLayout);
			BindingGroupSlot GroupNumber = BindGroupLayout->GetGroupNumber();
			SetLayouts[GroupNumber] = BindGroupLayout->GetLayout();
		};
		AddLayout(InPipelineStateDesc.BindGroupLayout0);
		AddLayout(InPipelineStateDesc.BindGroupLayout1);
		AddLayout(InPipelineStateDesc.BindGroupLayout2);
		AddLayout(InPipelineStateDesc.BindGroupLayout3);

		VkPipelineLayout PipelineLayout;
		VkPipelineLayoutCreateInfo PipelineLayoutInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = (uint32_t)SetLayouts.Num(),
			.pSetLayouts = SetLayouts.GetData(),
		};
		VkCheck(vkCreatePipelineLayout(GDevice, &PipelineLayoutInfo, nullptr, &PipelineLayout));

		//Compatible dummy renderpass
		TArray<VkAttachmentDescription> AttachmentDescs;
		TArray<VkAttachmentReference> AttachmentRefs;
		for (int32 i = 0; i < InPipelineStateDesc.Targets.Num(); i++)
		{
			AttachmentDescs.Add({
				.format = MapTextureFormat(InPipelineStateDesc.Targets[i].TargetFormat),
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			});
			AttachmentRefs.Add({
				.attachment = uint32_t(i),
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			});
		}

		VkAttachmentReference DepthAttachmentRef{};
		bool bHasDepth = InPipelineStateDesc.DepthStencilState.IsSet();
		if (bHasDepth)
		{
			DepthAttachmentRef = {
				.attachment = uint32_t(AttachmentDescs.Num()),
				.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			};
			AttachmentDescs.Add({
				.format = MapTextureFormat(InPipelineStateDesc.DepthStencilState->DepthFormat),
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			});
		}

		VkSubpassDescription SubpassDesc = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = (uint32_t)AttachmentRefs.Num(),
			.pColorAttachments = AttachmentRefs.GetData(),
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

		VkPipelineDepthStencilStateCreateInfo DepthStencilInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = bHasDepth ? VK_TRUE : VK_FALSE,
			.depthWriteEnable = bHasDepth && InPipelineStateDesc.DepthStencilState->DepthWriteEnable ? VK_TRUE : VK_FALSE,
			.depthCompareOp = bHasDepth ? MapCompareOp(InPipelineStateDesc.DepthStencilState->DepthCompare) : VK_COMPARE_OP_NEVER,
		};

		VkGraphicsPipelineCreateInfo PipelineInfo = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = (uint32_t)Stages.Num(),
			.pStages = Stages.GetData(),
			.pVertexInputState = &VertexInputInfo,
			.pInputAssemblyState = &InputAssemblyInfo,
			.pViewportState = &ViewportStateInfo,
			.pRasterizationState = &RasterizationInfo,
			.pMultisampleState = &MultisampleInfo,
			.pDepthStencilState = &DepthStencilInfo,
			.pColorBlendState = &ColorBlendInfo,
			.pDynamicState = &DynamicStateInfo,
			.layout = PipelineLayout,
			.renderPass = RenderPass,
		};
		VkPipeline Pipeline;
		VkCheck(vkCreateGraphicsPipelines(GDevice, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Pipeline));
		return new VulkanRenderPipelineState(InPipelineStateDesc, Pipeline, PipelineLayout, RenderPass);
	}

	TRefCountPtr<VulkanComputePipelineState> CreateVulkanComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc)
	{
		VulkanShader* Cs = static_cast<VulkanShader*>(InPipelineStateDesc.Cs);
		if (InPipelineStateDesc.CheckLayout)
		{
			CheckShaderLayoutBinding(InPipelineStateDesc, Cs);
		}

		auto CsEntryPoint = StringCast<UTF8CHAR>(*Cs->GetEntryPoint());

		VkPipelineShaderStageCreateInfo StageInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = Cs->GetCompilationResult(),
			.pName = (char*)CsEntryPoint.Get()
		};

		static VkDescriptorSetLayout DummyLayout = [] {
			VkDescriptorSetLayoutCreateInfo LayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
			VkDescriptorSetLayout Layout;
			VkCheck(vkCreateDescriptorSetLayout(GDevice, &LayoutInfo, nullptr, &Layout));
			return Layout;
		}();
		TArray<VkDescriptorSetLayout, TFixedAllocator<GpuResourceLimit::MaxBindableBingGroupNum>> SetLayouts;
		SetLayouts.Init(DummyLayout, GpuResourceLimit::MaxBindableBingGroupNum);
		auto AddLayout = [&](GpuBindGroupLayout* InLayout)
		{
			if (!InLayout) { return; }
			VulkanBindGroupLayout* BindGroupLayout = static_cast<VulkanBindGroupLayout*>(InLayout);
			BindingGroupSlot GroupNumber = BindGroupLayout->GetGroupNumber();
			SetLayouts[GroupNumber] = BindGroupLayout->GetLayout();
		};
		AddLayout(InPipelineStateDesc.BindGroupLayout0);
		AddLayout(InPipelineStateDesc.BindGroupLayout1);
		AddLayout(InPipelineStateDesc.BindGroupLayout2);
		AddLayout(InPipelineStateDesc.BindGroupLayout3);

		VkPipelineLayout PipelineLayout;
		VkPipelineLayoutCreateInfo PipelineLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = (uint32_t)SetLayouts.Num(),
			.pSetLayouts = SetLayouts.GetData(),
		};
		VkCheck(vkCreatePipelineLayout(GDevice, &PipelineLayoutInfo, nullptr, &PipelineLayout));

		VkComputePipelineCreateInfo PipelineInfo{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = StageInfo,
			.layout = PipelineLayout,
		};
		VkPipeline Pipeline;
		VkCheck(vkCreateComputePipelines(GDevice, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Pipeline));
		return new VulkanComputePipelineState(InPipelineStateDesc, Pipeline, PipelineLayout);
	}
}
