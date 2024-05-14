#pragma once
#include "GpuResource.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGpuApi, Log, All);
inline DEFINE_LOG_CATEGORY(LogGpuApi);

namespace FRAMEWORK
{
	bool ValidateSetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3);
	bool ValidateCreateBindGroup(const GpuBindGroupDesc& InBindGroupDesc);
    bool ValidateCreateBindGroupLayout(const GpuBindGroupLayoutDesc& InBindGroupLayoutDesc);
	bool ValidateCreateRenderPipelineState(const GpuPipelineStateDesc& InPipelineStateDesc);
}
