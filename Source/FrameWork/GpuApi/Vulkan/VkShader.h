#pragma once
#include "GpuApi/GpuResource.h"
#include "VkDevice.h"

namespace FW
{
	class VulkanShader : public GpuShader
	{
	public:
		VulkanShader(const GpuShaderFileDesc& Desc);
		VulkanShader(const GpuShaderSourceDesc& Desc);
		~VulkanShader() {
			vkDestroyShaderModule(GDevice, ShaderModule, nullptr);
		}

	public:
		TArray<GpuShaderLayoutBinding> GetLayout() const override;
		void SetCompilationResult(VkShaderModule InShaderModule) { ShaderModule = InShaderModule; }
		VkShaderModule GetCompilationResult() const { return ShaderModule; }
		virtual bool IsCompiled() const override { return ShaderModule != VK_NULL_HANDLE; }

	private:
		VkShaderModule ShaderModule = VK_NULL_HANDLE;
	};

	bool CompileVulkanShader(TRefCountPtr<VulkanShader> InShader, FString& OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs);
}