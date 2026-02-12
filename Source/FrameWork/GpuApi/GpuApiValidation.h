#pragma once
#include "GpuResource.h"
#include <stdexcept>

DECLARE_LOG_CATEGORY_EXTERN(LogRhiValidation , Log, All);
inline DEFINE_LOG_CATEGORY(LogRhiValidation);

namespace FW
{
	struct GpuBarrierInfo;

	enum class CmdRecorderState
	{
		Begin,
		End,
		Finish,
	};

	bool ValidateSetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3);
	bool ValidateCreateBindGroup(const GpuBindGroupDesc& InBindGroupDesc);
    bool ValidateCreateBindGroupLayout(const GpuBindGroupLayoutDesc& InBindGroupLayoutDesc);
	bool ValidateCreateRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc);
	bool ValidateBeginRenderPass(const GpuRenderPassDesc& InPassDesc);
	bool ValidateCreateBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState);
	bool ValidateBarriers(const TArray<GpuBarrierInfo>& BarrierInfos);
	bool ValidateGpuResourceState(GpuResourceState InState);
	bool ValidateCreateTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState);

	template<typename PipelineStateDesc>
	void CheckShaderLayoutBinding(const PipelineStateDesc& InPipelineStateDesc, const TArray<GpuShaderLayoutBinding>& InShaderLayoutBindings)
	{
		auto CheckGroup = [&](const GpuShaderLayoutBinding& ShaderLayoutBinding, GpuBindGroupLayout* InLayout)
			{
				if (!InLayout)
				{
					return true;
				}
				else if (auto* LayoutBinding = InLayout->GetDesc().Layouts.Find(ShaderLayoutBinding.Slot))
				{
					if (LayoutBinding->Type != ShaderLayoutBinding.Type)
					{
						return true;
					}
				}
				else
				{
					return true;
				}
				return false;
			};

		for (const auto& ShaderLayoutBinding : InShaderLayoutBindings)
		{
			bool HasError{};
			if (ShaderLayoutBinding.Group == 0)
			{
				HasError = CheckGroup(ShaderLayoutBinding, InPipelineStateDesc.BindGroupLayout0);
			}
			else if (ShaderLayoutBinding.Group == 1)
			{
				HasError = CheckGroup(ShaderLayoutBinding, InPipelineStateDesc.BindGroupLayout1);
			}
			else if (ShaderLayoutBinding.Group == 2)
			{
				HasError = CheckGroup(ShaderLayoutBinding, InPipelineStateDesc.BindGroupLayout2);
			}
			else if (ShaderLayoutBinding.Group == 3)
			{
				HasError = CheckGroup(ShaderLayoutBinding, InPipelineStateDesc.BindGroupLayout3);
			}
			else
			{
				HasError = true;
			}

			if (HasError)
			{
				FString ErrorInfo = FString::Printf(TEXT("Binding layouts do not match shaders:\ncontain a binding %s(Slot:%d, Group:%d, Type:%s), but it cannot be found in provided layouts."),
					*ShaderLayoutBinding.Name, ShaderLayoutBinding.Slot, ShaderLayoutBinding.Group, ANSI_TO_TCHAR(magic_enum::enum_name(ShaderLayoutBinding.Type).data()));
				throw std::runtime_error(TCHAR_TO_ANSI(*ErrorInfo));
			}
		}

	};
}
