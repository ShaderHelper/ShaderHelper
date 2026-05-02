#pragma once
#include "VkDevice.h"

namespace FW::VK
{
	class VulkanCmdRecorder;
	class VulkanRenderPipelineState;
	class VulkanComputePipelineState;

	class VkRenderStateCache
	{
	public:
		struct VertexBufferBinding
		{
			GpuBuffer* Buffer = nullptr;
			uint32 Offset = 0;
		};

		VkRenderStateCache(VulkanCmdRecorder* InOwner, TArray<GpuTextureView*> InRenderTargetViews);
		void ApplyDrawState();

		void SetPipeline(VulkanRenderPipelineState* InPipelineState);
		void SetVertexBuffer(uint32 Slot, GpuBuffer* InBuffer, uint32 Offset);
		void SetIndexBuffer(GpuBuffer* InBuffer, GpuFormat InIndexFormat, uint32 Offset);
		void SetViewPort(VkViewport InViewPort);
		void SetScissorRect(VkRect2D InScissorRect);
		void SetBindGroups(const TArray<GpuBindGroup*>& InGroups);

	public:
		bool IsRenderPipelineDirty;
		bool IsViewportDirty;
		bool IsScissorRectDirty;
		bool IsVertexBufferDirty;
		bool IsIndexBufferDirty;

		bool IsBindGroupDirty[GpuResourceLimit::MaxBindableBingGroupNum];

	private:
		VulkanCmdRecorder* Owner;
		VulkanRenderPipelineState* CurrentRenderPipelineState = nullptr;
		std::array<VertexBufferBinding, GpuResourceLimit::MaxVertexBufferSlotNum> CurrentVertexBuffers{};
		GpuBuffer* CurrentIndexBuffer = nullptr;
		GpuFormat CurrentIndexFormat = GpuFormat::R16_UINT;
		uint32 CurrentIndexOffset = 0;
		TOptional<VkViewport> CurrentViewPort;
		TOptional<VkRect2D> CurrentScissorRect;

		GpuBindGroup* CurrentBindGroups[GpuResourceLimit::MaxBindableBingGroupNum] = {};

		TArray<GpuTextureView*> RenderTargetViews;
	};

	class VkComputeStateCache
	{
	public:
		VkComputeStateCache();
		void ApplyComputeState(VkCommandBuffer InCmdBuffer);

		void SetPipeline(VulkanComputePipelineState* InPipelineState);
		void SetBindGroups(const TArray<GpuBindGroup*>& InGroups);

	public:
		bool IsComputePipelineDirty;
		bool IsBindGroupDirty[GpuResourceLimit::MaxBindableBingGroupNum];

	private:
		VulkanComputePipelineState* CurrentComputePipelineState = nullptr;
		GpuBindGroup* CurrentBindGroups[GpuResourceLimit::MaxBindableBingGroupNum] = {};
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
		void SetBindGroups(const TArray<GpuBindGroup*>& BindGroups) override;

	private:
		VulkanCmdRecorder* Owner;
		VkComputeStateCache StateCache;
	};

	class VulkanRenderPassRecorder : public GpuRenderPassRecorder
	{
	public:
		VulkanRenderPassRecorder(VulkanCmdRecorder* InOwner, VkRenderPass InRenderPass, VkFramebuffer InFrameBuffer, TArray<GpuTextureView*> InRenderTargetViews,
			TOptional<GpuRenderPassTimestampWrites> InTimestampWrites = {})
			: Owner(InOwner)
			, RenderPass(InRenderPass), FrameBuffer(InFrameBuffer)
			, StateCache(InOwner, MoveTemp(InRenderTargetViews))
			, TimestampWrites(MoveTemp(InTimestampWrites))
		{}
		~VulkanRenderPassRecorder() {
			vkDestroyFramebuffer(GDevice, FrameBuffer, nullptr);
			vkDestroyRenderPass(GDevice, RenderPass, nullptr);
		}

	public:
		void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount) override;
		void DrawIndexed(uint32 StartIndexLocation, uint32 IndexCount, int32 BaseVertexLocation, uint32 StartInstanceLocation, uint32 InstanceCount) override;
		void SetRenderPipelineState(GpuRenderPipelineState* InPipelineState) override;
		void SetVertexBuffer(uint32 Slot, GpuBuffer* InVertexBuffer, uint32 Offset) override;
		void SetIndexBuffer(GpuBuffer* InIndexBuffer, GpuFormat IndexFormat, uint32 Offset) override;
		void SetViewPort(const GpuViewPortDesc& InViewPortDesc) override;
		void SetScissorRect(const GpuScissorRectDesc& InScissorRectDes) override;
		void SetBindGroups(const TArray<GpuBindGroup*>& BindGroups) override;

		TOptional<GpuRenderPassTimestampWrites> TimestampWrites;

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
		void CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture, uint32 ArrayLayer = 0, uint32 MipLevel = 0) override;
		void CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer) override;
		void CopyBufferToBuffer(GpuBuffer* SrcBuffer, uint32 SrcOffset, GpuBuffer* DestBuffer, uint32 DestOffset, uint32 Size) override;

	private:
		VkCommandBuffer CommandBuffer;
		TArray<TUniquePtr<VulkanRenderPassRecorder>> RenderPassRecorders;
		TArray<TUniquePtr<VulkanComputePassRecorder>> ComputePassRecorders;
	};
}
