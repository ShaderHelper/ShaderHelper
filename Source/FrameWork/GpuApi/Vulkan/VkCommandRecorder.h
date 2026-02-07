#pragma once
#include "VkDevice.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	class VulkanComputePassRecorder : public GpuComputePassRecorder
	{
	public:
		VulkanComputePassRecorder() {}

	public:
		void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) override;
		void SetComputePipelineState(GpuComputePipelineState* InPipelineState) override;
		void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3) override;

	private:
	};

	class VulkanRenderPassRecorder : public GpuRenderPassRecorder
	{
	public:
		VulkanRenderPassRecorder(VkRenderPass InRenderPass, VkFramebuffer InFrameBuffer) : RenderPass(InRenderPass), FrameBuffer(InFrameBuffer) {}
		~VulkanRenderPassRecorder() {
			vkDestroyFramebuffer(GDevice, FrameBuffer, nullptr);
			vkDestroyRenderPass(GDevice, RenderPass, nullptr); 
		}

	public:
		void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount) override;
		void SetRenderPipelineState(GpuRenderPipelineState* InPipelineState) override;
		void SetVertexBuffer(GpuBuffer* InVertexBuffer) override;
		void SetViewPort(const GpuViewPortDesc& InViewPortDesc) override;
		void SetScissorRect(const GpuScissorRectDesc& InScissorRectDes) override;
		void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3) override;

	private:
		VkRenderPass RenderPass;
		VkFramebuffer FrameBuffer;
	};

	class VulkanCmdRecorder : public GpuCmdRecorder
	{
	public:
		VulkanCmdRecorder(VkCommandBuffer InCmdBuffer) : CommandBuffer(InCmdBuffer) {}
		VkCommandBuffer GetCommandBuffer() const { return CommandBuffer; }

	public:
		GpuComputePassRecorder* BeginComputePass(const FString& PassName) override;
		void EndComputePass(GpuComputePassRecorder* InComputePassRecorder) override;
		GpuRenderPassRecorder* BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName) override;
		void EndRenderPass(GpuRenderPassRecorder* InRenderPassRecorder) override;
		void BeginCaptureEvent(const FString& EventName) override;
		void EndCaptureEvent() override;
		void Barriers(const TArray<GpuBarrierInfo>& BarrierInfos) override;
		void CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture) override;
		void CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer) override;
		void CopyBufferToBuffer(GpuBuffer* SrcBuffer, uint32 SrcOffset, GpuBuffer* DestBuffer, uint32 DestOffset, uint32 Size) override;

	private:
		VkCommandBuffer CommandBuffer;
		TArray<TUniquePtr<VulkanRenderPassRecorder>> RenderPassRecorders;
	};

	class VulkanCmdRecorderPool
	{
	public:
		VulkanCmdRecorder* AcquireCmdRecorder(const FString& RecorderName);
		void Empty() { CmdRecorders.Empty(); }

	private:
		VkCommandPool CommandPool = VK_NULL_HANDLE;
		TArray<TUniquePtr<VulkanCmdRecorder>> CmdRecorders;
	};

	inline VulkanCmdRecorderPool GVkCmdRecorderPool;
}