#pragma once
#include "GpuResource.h"

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
	bool ValidateCreateBuffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InitState);
	bool ValidateBarriers(const TArray<GpuBarrierInfo>& BarrierInfos);
	bool ValidateGpuResourceState(GpuResourceState InState);
	bool ValidateCreateTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState);
}
