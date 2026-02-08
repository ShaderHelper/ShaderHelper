#include "CommonHeader.h"
#include "VkPipeline.h"
#include "VkShader.h"
#include "VkMap.h"
#include "VkGpuRhiBackend.h"

namespace FW
{
	VulkanRenderPipelineState::VulkanRenderPipelineState(GpuRenderPipelineStateDesc InDesc, VkPipeline InPipeline, VkPipelineLayout InPipelineLayout, VkRenderPass InRednedrPass)
		: GpuRenderPipelineState(InDesc)
		, Pipeline(InPipeline), PipelineLayout(InPipelineLayout), RenderPass(InRednedrPass)
	{
		GDeferredReleaseOneFrame.Add(this);
	}

	TRefCountPtr<VulkanRenderPipelineState> CreateVulkanRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc)
	{
		const VkDynamicState DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo DynamicStateInfo = { 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO ,
			.dynamicStateCount = UE_ARRAY_COUNT(DynamicStates),
			.pDynamicStates = DynamicStates
		};

		TArray<VkPipelineShaderStageCreateInfo> Stages;
		VulkanShader* Vs = static_cast<VulkanShader*>(InPipelineStateDesc.Vs);
		auto VsEntryPoint = StringCast<UTF8CHAR>(*Vs->GetEntryPoint());
		VulkanShader* Ps = static_cast<VulkanShader*>(InPipelineStateDesc.Ps);
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

		VkPipelineVertexInputStateCreateInfo VertexInputInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
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
		
		VkPipelineLayout PipelineLayout;
		VkPipelineLayoutCreateInfo PipelineLayoutInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
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
		VkSubpassDescription SubpassDesc = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = (uint32_t)AttachmentRefs.Num(),
			.pColorAttachments = AttachmentRefs.GetData()
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

		VkGraphicsPipelineCreateInfo PipelineInfo = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = (uint32_t)Stages.Num(),
			.pStages = Stages.GetData(),
			.pVertexInputState = &VertexInputInfo,
			.pInputAssemblyState = &InputAssemblyInfo,
			.pViewportState = &ViewportStateInfo,
			.pRasterizationState = &RasterizationInfo,
			.pMultisampleState = &MultisampleInfo,
			.pColorBlendState = &ColorBlendInfo,
			.pDynamicState = &DynamicStateInfo,
			.layout = PipelineLayout,
			.renderPass = RenderPass,
		};
		VkPipeline Pipeline;
		VkCheck(vkCreateGraphicsPipelines(GDevice, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Pipeline));
		return new VulkanRenderPipelineState(InPipelineStateDesc, Pipeline, PipelineLayout, RenderPass);
	}
}
