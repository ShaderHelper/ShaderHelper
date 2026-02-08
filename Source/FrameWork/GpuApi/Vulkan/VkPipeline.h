#pragma once
#include "VkDevice.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
	class VulkanRenderPipelineState : public GpuRenderPipelineState
	{
	public:
		VulkanRenderPipelineState(GpuRenderPipelineStateDesc InDesc, VkPipeline InPipeline, VkPipelineLayout InPipelineLayout, VkRenderPass InRednedrPass);
		~VulkanRenderPipelineState() {
			vkDestroyPipeline(GDevice, Pipeline, nullptr);
			vkDestroyPipelineLayout(GDevice, PipelineLayout, nullptr);
			vkDestroyRenderPass(GDevice, RenderPass, nullptr);
		}

	public:


	private:
		VkPipeline Pipeline;
		VkPipelineLayout PipelineLayout;
		VkRenderPass RenderPass;
	};

	TRefCountPtr<VulkanRenderPipelineState> CreateVulkanRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc);
}