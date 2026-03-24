#pragma once
#include "MetalCommon.h"
#include "MetalPipeline.h"
#include "GpuApi/GpuResource.h"
#include "MetalBuffer.h"
#include "MetalArgumentBuffer.h"
#include "MetalDevice.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
    class MtlRenderStateCache
    {
    public:
        struct VertexBufferBinding
        {
            MetalBuffer* Buffer = nullptr;
            uint32 Offset = 0;
        };

		MtlRenderStateCache(MTLRenderPassDescriptorPtr InRenderPassDesc);
        void ApplyDrawState(MTL::RenderCommandEncoder* RenderCommandEncoder);
        
        void SetPipeline(MetalRenderPipelineState* InPipelineState);
        void SetVertexBuffer(uint32 Slot, MetalBuffer* InBuffer, uint32 Offset);
		void SetIndexBuffer(MetalBuffer* InBuffer, GpuFormat InIndexFormat, uint32 Offset);
        void SetViewPort(MTL::Viewport InViewPort);
		void SetScissorRect(MTL::ScissorRect InSissorRect);
        void SetBindGroups(MetalBindGroup* InGroup0, MetalBindGroup* InGroup1, MetalBindGroup* InGroup2, MetalBindGroup* InGroup3);
        MTL::PrimitiveType GetPrimitiveType() const {
            check(CurrentRenderPipelineState);
            return (MTL::PrimitiveType)CurrentRenderPipelineState->GetPrimitiveType();
        }
		MetalBuffer* GetIndexBuffer() const { return CurrentIndexBuffer; }
        GpuFormat GetIndexFormat() const { return CurrentIndexFormat; }
		uint32 GetIndexOffset() const { return CurrentIndexOffset; }
        
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
        MetalRenderPipelineState* CurrentRenderPipelineState;
        std::array<VertexBufferBinding, GpuResourceLimit::MaxVertexBufferSlotNum> CurrentVertexBuffers{};
        MetalBuffer* CurrentIndexBuffer;
		GpuFormat CurrentIndexFormat;
        uint32 CurrentIndexOffset;
        TOptional<MTL::Viewport> CurrentViewPort;
        TOptional<MTL::ScissorRect> CurrentScissorRect;
        
        MetalBindGroup* CurrentBindGroup0;
        MetalBindGroup* CurrentBindGroup1;
        MetalBindGroup* CurrentBindGroup2;
        MetalBindGroup* CurrentBindGroup3;
        
        MTLRenderPassDescriptorPtr RenderPassDesc;
    };

    class MtlComputeStateCache
    {
    public:
        MtlComputeStateCache();
        void ApplyComputeState(MTL::ComputeCommandEncoder* ComputeCommandEncoder);
        void SetPipeline(MetalComputePipelineState* InPipelineState);
        void SetBindGroups(MetalBindGroup* InGroup0, MetalBindGroup* InGroup1, MetalBindGroup* InGroup2, MetalBindGroup* InGroup3);
        MetalComputePipelineState* GetPipeline() const { return CurrentComputePipelineState; }

    public:
        bool IsComputePipelineDirty : 1;
        bool IsBindGroup0Dirty : 1;
        bool IsBindGroup1Dirty : 1;
        bool IsBindGroup2Dirty : 1;
        bool IsBindGroup3Dirty : 1;

    private:
        MetalComputePipelineState* CurrentComputePipelineState;
        MetalBindGroup* CurrentBindGroup0;
        MetalBindGroup* CurrentBindGroup1;
        MetalBindGroup* CurrentBindGroup2;
        MetalBindGroup* CurrentBindGroup3;
    };

    class MtlRenderPassRecorder : public GpuRenderPassRecorder
    {
    public:
        MtlRenderPassRecorder(MTLRenderCommandEncoderPtr InCmdEncoder, MTLRenderPassDescriptorPtr InRenderPassDesc)
            : CmdEncoder(MoveTemp(InCmdEncoder))
            , StateCache(MoveTemp(InRenderPassDesc))
        {}
        
        MTL::RenderCommandEncoder* GetEncoder() const { return CmdEncoder.get(); }
        
    public:
        void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount) override;
		void DrawIndexed(uint32 StartIndexLocation, uint32 IndexCount, int32 BaseVertexLocation, uint32 StartInstanceLocation, uint32 InstanceCount) override;
        void SetRenderPipelineState(GpuRenderPipelineState* InPipelineState) override;
		void SetVertexBuffer(uint32 Slot, GpuBuffer* InVertexBuffer, uint32 Offset) override;
        void SetIndexBuffer(GpuBuffer* InIndexBuffer, GpuFormat IndexFormat, uint32 Offset) override;
        void SetViewPort(const GpuViewPortDesc& InViewPortDesc) override;
		void SetScissorRect(const GpuScissorRectDesc& InScissorRectDes) override;
        void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3) override;
        
    private:
        MTLRenderCommandEncoderPtr CmdEncoder;
        MtlRenderStateCache StateCache;
    };

	class MtlComputePassRecorder: public GpuComputePassRecorder
	{
	public:
		MtlComputePassRecorder(MTLComputeCommandEncoderPtr InCmdEncoder)
            : CmdEncoder(MoveTemp(InCmdEncoder))
        {}
		MTL::ComputeCommandEncoder* GetEncoder() const { return CmdEncoder.get(); }
		
	public:
		void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) override;
		void SetComputePipelineState(GpuComputePipelineState* InPipelineState) override;
		void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3) override;
		
	private:
		MTLComputeCommandEncoderPtr CmdEncoder;
        MtlComputeStateCache StateCache;
	};

    class MtlCmdRecorder : public GpuCmdRecorder
    {
    public:
        MtlCmdRecorder(MTLCommandBufferPtr InCmdBuffer)
            : CmdBuffer(MoveTemp(InCmdBuffer))
        {}
        
        MTL::CommandBuffer* GetCommandBuffer() const { return CmdBuffer.get(); }
        
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
        MTLCommandBufferPtr CmdBuffer;
        TArray<TUniquePtr<MtlRenderPassRecorder>> RenderPassRecorders;
        TArray<TUniquePtr<MtlComputePassRecorder>> ComputePassRecorders;
    };
    
    inline MtlCmdRecorder* LastSubmittedCmdRecorder;
    inline TArray<TUniquePtr<MtlCmdRecorder>> GMtlCmdRecorderPool;

}
