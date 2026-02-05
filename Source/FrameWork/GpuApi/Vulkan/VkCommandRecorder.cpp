#include "CommonHeader.h"
#include "VkCommandRecorder.h"

namespace FW
{
	GpuComputePassRecorder* VulkanCmdRecorder::BeginComputePass(const FString& PassName)
	{
		return nullptr;
	}

	void VulkanCmdRecorder::EndComputePass(GpuComputePassRecorder* InComputePassRecorder)
	{
	}

	GpuRenderPassRecorder* VulkanCmdRecorder::BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName)
	{
		return nullptr;
	}

	void VulkanCmdRecorder::EndRenderPass(GpuRenderPassRecorder* InRenderPassRecorder)
	{
	}

	void VulkanCmdRecorder::BeginCaptureEvent(const FString& EventName)
	{
	}

	void VulkanCmdRecorder::EndCaptureEvent()
	{
	}

	void VulkanCmdRecorder::Barriers(const TArray<GpuBarrierInfo>& BarrierInfos)
	{
	}

	void VulkanCmdRecorder::CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture)
	{
	}

	void VulkanCmdRecorder::CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer)
	{
	}

	void VulkanCmdRecorder::CopyBufferToBuffer(GpuBuffer* SrcBuffer, uint32 SrcOffset, GpuBuffer* DestBuffer, uint32 DestOffset, uint32 Size)
	{
	}

	void VulkanComputePassRecorder::Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
	{
	}

	void VulkanComputePassRecorder::SetComputePipelineState(GpuComputePipelineState* InPipelineState)
	{
	}

	void VulkanComputePassRecorder::SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
	{
	}

	void VulkanRenderPassRecorder::DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount)
	{
	}

	void VulkanRenderPassRecorder::SetRenderPipelineState(GpuRenderPipelineState* InPipelineState)
	{
	}

	void VulkanRenderPassRecorder::SetVertexBuffer(GpuBuffer* InVertexBuffer)
	{
	}

	void VulkanRenderPassRecorder::SetViewPort(const GpuViewPortDesc& InViewPortDesc)
	{
	}

	void VulkanRenderPassRecorder::SetScissorRect(const GpuScissorRectDesc& InScissorRectDes)
	{
	}

	void VulkanRenderPassRecorder::SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
	{
	}
}