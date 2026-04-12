#pragma once
#include "GpuApi/GpuResource.h"
#include "VkDevice.h"

namespace FW::VK
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
		void SetCompilationResult(VkShaderModule InShaderModule) { ShaderModule = InShaderModule; }
		VkShaderModule GetCompilationResult() const { return ShaderModule; }
		virtual bool IsCompiled() const override { return ShaderModule != VK_NULL_HANDLE; }
		virtual TArray<GpuShaderLayoutBinding> GetLayout() const override
		{
			TArray<GpuShaderLayoutBinding> Bindings = GpuShader::GetLayout();
			for (auto& Binding : Bindings)
			{
				Binding.Slot -= GetBindingShift(Binding.Type) + GetStageBindingOffset(Binding.Stage);
			}
			return Bindings;
		}

	private:
		VkShaderModule ShaderModule = VK_NULL_HANDLE;
	};

	bool CompileVulkanShader(TRefCountPtr<VulkanShader> InShader, FString& OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs);
}
