#pragma once
#include "VkDevice.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	class VulkanCmdRecorder;
	class VulkanRenderPipelineState;
	class VulkanComputePipelineState;

	class VkRenderStateCache
	{
	public:
		VkRenderStateCache(VulkanCmdRecorder* InOwner, TArray<GpuTexture*> InRenderTargets);
		void ApplyDrawState();

		void SetPipeline(VulkanRenderPipelineState* InPipelineState);
		void SetVertexBuffer(GpuBuffer* InBuffer);
		void SetViewPort(VkViewport InViewPort);
		void SetScissorRect(VkRect2D InScissorRect);
		void SetBindGroups(GpuBindGroup* InGroup0, GpuBindGroup* InGroup1, GpuBindGroup* InGroup2, GpuBindGroup* InGroup3);

	public:
		bool IsRenderPipelineDirty : 1;
		bool IsViewportDirty : 1;
		bool IsScissorRectDirty : 1;
		bool IsVertexBufferDirty : 1;

		bool IsBindGroup0Dirty : 1;
		bool IsBindGroup1Dirty : 1;
		bool IsBindGroup2Dirty : 1;
		bool IsBindGroup3Dirty : 1;

	private:
		VulkanCmdRecorder* Owner;
		VulkanRenderPipelineState* CurrentRenderPipelineState = nullptr;
		GpuBuffer* CurrentVertexBuffer = nullptr;
		TOptional<VkViewport> CurrentViewPort;
		TOptional<VkRect2D> CurrentScissorRect;

		GpuBindGroup* CurrentBindGroup0 = nullptr;
		GpuBindGroup* CurrentBindGroup1 = nullptr;
		GpuBindGroup* CurrentBindGroup2 = nullptr;
		GpuBindGroup* CurrentBindGroup3 = nullptr;

		TArray<GpuTexture*> RenderTargets;
	};

	class VkComputeStateCache
	{
	public:
		VkComputeStateCache();
		void ApplyComputeState(VkCommandBuffer InCmdBuffer);

		void SetPipeline(VulkanComputePipelineState* InPipelineState);
		void SetBindGroups(GpuBindGroup* InGroup0, GpuBindGroup* InGroup1, GpuBindGroup* InGroup2, GpuBindGroup* InGroup3);

	public:
		bool IsComputePipelineDirty : 1;
		bool IsBindGroup0Dirty : 1;
		bool IsBindGroup1Dirty : 1;
		bool IsBindGroup2Dirty : 1;
		bool IsBindGroup3Dirty : 1;

	private:
		VulkanComputePipelineState* CurrentComputePipelineState = nullptr;
		GpuBindGroup* CurrentBindGroup0 = nullptr;
		GpuBindGroup* CurrentBindGroup1 = nullptr;
		GpuBindGroup* CurrentBindGroup2 = nullptr;
		GpuBindGroup* CurrentBindGroup3 = nullptr;
	};

	class VulkanComputePassRecorder : public GpuComputePassRecorder
	{
	public:
		VulkanComputePassRecorder(VulkanCmdRecorder* InOwner)
			: Owner(InOwner)
		{}

	public:
		void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) override;
		void SetComputePipelineState(GpuComputePipelineState* InPipelineState) override;
		void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3) override;

	private:
		VulkanCmdRecorder* Owner;
		VkComputeStateCache StateCache;
	};

	class VulkanRenderPassRecorder : public GpuRenderPassRecorder
	{
	public:
		VulkanRenderPassRecorder(VulkanCmdRecorder* InOwner, VkRenderPass InRenderPass, VkFramebuffer InFrameBuffer, TArray<GpuTexture*> InRenderTargets) 
			: Owner(InOwner)
			, RenderPass(InRenderPass), FrameBuffer(InFrameBuffer)
			, StateCache(InOwner, MoveTemp(InRenderTargets))
		{}
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
		VulkanCmdRecorder* Owner;
		VkRenderPass RenderPass;
		VkFramebuffer FrameBuffer;
		VkRenderStateCache StateCache;
	};

	class VulkanCmdRecorderPool
	{
	public:
		VulkanCmdRecorder* AcquireCmdRecorder(const FString& RecorderName);
		VkCommandPool GetCommandPool() const { return CommandPool; }
		void Empty() { CmdRecorders.Empty(); }

	private:
		VkCommandPool CommandPool = VK_NULL_HANDLE;
		TArray<TUniquePtr<VulkanCmdRecorder>> CmdRecorders;
	};

	inline VulkanCmdRecorderPool GVkCmdRecorderPool;

	class VulkanCmdRecorder : public GpuCmdRecorder
	{
	public:
		VulkanCmdRecorder(VkCommandBuffer InCmdBuffer) : CommandBuffer(InCmdBuffer) {}
		~VulkanCmdRecorder() {
			vkFreeCommandBuffers(GDevice, GVkCmdRecorderPool.GetCommandPool(), 1, &CommandBuffer);
		}
		const VkCommandBuffer& GetCommandBuffer() const { return CommandBuffer; }

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
		TArray<TUniquePtr<VulkanComputePassRecorder>> ComputePassRecorders;
	};
}
