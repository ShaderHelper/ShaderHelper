#pragma once
#include "VkDevice.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
	class VulkanComputePipelineState : public GpuComputePipelineState
	{
	public:
		VulkanComputePipelineState(GpuComputePipelineStateDesc InDesc, VkPipeline InPipeline, VkPipelineLayout InPipelineLayout);
		~VulkanComputePipelineState() {
			vkDestroyPipeline(GDevice, Pipeline, nullptr);
			vkDestroyPipelineLayout(GDevice, PipelineLayout, nullptr);
		}
	public:
		VkPipeline GetPipeline() const { return Pipeline; }
		VkPipelineLayout GetPipelineLayout() const { return PipelineLayout; }

	private:
		VkPipeline Pipeline;
		VkPipelineLayout PipelineLayout;
	};

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
		VkPipeline GetPipeline() const { return Pipeline; }
		VkPipelineLayout GetPipelineLayout() const { return PipelineLayout; }

	private:
		VkPipeline Pipeline;
		VkPipelineLayout PipelineLayout;
		VkRenderPass RenderPass;
	};

	TRefCountPtr<VulkanRenderPipelineState> CreateVulkanRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc);
	TRefCountPtr<VulkanComputePipelineState> CreateVulkanComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc);
}
