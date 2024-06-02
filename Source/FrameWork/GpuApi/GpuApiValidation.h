#pragma once
#include "GpuResource.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRhiValidation , Log, All);
inline DEFINE_LOG_CATEGORY(LogRhiValidation);

namespace FRAMEWORK
{
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
	bool ValidateCreateBuffer(uint32 ByteSize, GpuBufferUsage Usage, GpuResourceState InitState);
	bool ValidateBarrier(GpuTrackedResource* InResource, GpuResourceState NewState);
	bool ValidateGpuResourceState(GpuResourceState InState);
}
