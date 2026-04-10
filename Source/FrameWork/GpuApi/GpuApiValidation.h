#pragma once
#include "GpuResource.h"
#include "GpuShader.h"
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

	bool ValidateSetBindGroups(const TArray<GpuBindGroup*>& BindGroups);
	bool ValidateCreateBindGroup(const GpuBindGroupDesc& InBindGroupDesc);
    bool ValidateCreateBindGroupLayout(const GpuBindGroupLayoutDesc& InBindGroupLayoutDesc);
	bool ValidateCreateRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc);
	bool ValidateBeginRenderPass(const GpuRenderPassDesc& InPassDesc);
	bool ValidateCreateBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState);
	bool ValidateSetVertexBuffer(uint32 Slot, GpuBuffer* InVertexBuffer, uint32 Offset);
	bool ValidateSetIndexBuffer(GpuBuffer* InIndexBuffer, GpuFormat IndexFormat, uint32 Offset);
	bool ValidateBarriers(const TArray<GpuBarrierInfo>& BarrierInfos);
	bool ValidateGpuResourceState(GpuResourceState InState);
	bool ValidateCreateTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState);

	template<typename PipelineStateDesc>
	void CheckShaderLayoutBinding(const PipelineStateDesc& InPipelineStateDesc, GpuShader* InShader)
	{
		const TArray<GpuShaderLayoutBinding> ShaderLayoutBindings = InShader->GetLayout();
		auto CheckGroup = [&](const GpuShaderLayoutBinding& ShaderLayoutBinding, GpuBindGroupLayout* InLayout)
			{
				if (!InLayout)
				{
					return true;
				}
				else if (auto* LayoutBinding = InLayout->GetDesc().Layouts.Find(BindingSlot{ShaderLayoutBinding.Slot, ShaderLayoutBinding.Type}))
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

		for (const auto& ShaderLayoutBinding : ShaderLayoutBindings)
		{
			bool HasError{};
			GpuBindGroupLayout* MatchedLayout = nullptr;
			for (GpuBindGroupLayout* Layout : InPipelineStateDesc.BindGroupLayouts)
			{
				if (Layout && Layout->GetGroupNumber() == ShaderLayoutBinding.Group)
				{
					MatchedLayout = Layout;
					break;
				}
			}
			HasError = CheckGroup(ShaderLayoutBinding, MatchedLayout);

			if (HasError)
			{
				FString ErrorInfo = FString::Printf(TEXT("The %s[%s] contains a binding %s(Slot:%d, Group:%d, Type:%s), but it cannot be found in provided binding layouts."),
					ANSI_TO_TCHAR(magic_enum::enum_name(InShader->GetShaderType()).data()), *InShader->GetShaderName(), *ShaderLayoutBinding.Name, ShaderLayoutBinding.Slot, ShaderLayoutBinding.Group, ANSI_TO_TCHAR(magic_enum::enum_name(ShaderLayoutBinding.Type).data()));
				throw std::runtime_error(TCHAR_TO_ANSI(*ErrorInfo));
			}
		}

	};
}
