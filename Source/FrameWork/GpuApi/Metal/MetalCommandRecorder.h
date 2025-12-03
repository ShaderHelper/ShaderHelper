#pragma once
#include "MetalCommon.h"
#include "MetalPipeline.h"
#include "GpuApi/GpuResource.h"
#include "MetalBuffer.h"
#include "MetalArgumentBuffer.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
    class MtlRenderStateCache
    {
    public:
		MtlRenderStateCache(const FString& InName, MTLCommandBufferPtr InCmdBuffer, MTLRenderPassDescriptorPtr InRenderPassDesc);
        void ApplyDrawState();
        
        void SetPipeline(MetalRenderPipelineState* InPipelineState);
        void SetVertexBuffer(MetalBuffer* InBuffer);
        void SetViewPort(MTL::Viewport InViewPort);
		void SetScissorRect(MTL::ScissorRect InSissorRect);
        void SetBindGroups(MetalBindGroup* InGroup0, MetalBindGroup* InGroup1, MetalBindGroup* InGroup2, MetalBindGroup* InGroup3);
        MTL::PrimitiveType GetPrimitiveType() const {
            check(CurrentRenderPipelineState);
            return (MTL::PrimitiveType)CurrentRenderPipelineState->GetPrimitiveType();
        }
		MTL::RenderCommandEncoder* GetEncoder() const { return RenderCommandEncoder.get(); }
        
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
        MetalBuffer* CurrentVertexBuffer;
        TOptional<MTL::Viewport> CurrentViewPort;
        TOptional<MTL::ScissorRect> CurrentScissorRect;
        
        MetalBindGroup* CurrentBindGroup0;
        MetalBindGroup* CurrentBindGroup1;
        MetalBindGroup* CurrentBindGroup2;
        MetalBindGroup* CurrentBindGroup3;
        
		FString Name;
		MTLCommandBufferPtr CmdBuffer;
		MTLRenderCommandEncoderPtr RenderCommandEncoder;
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
        MtlRenderPassRecorder(const FString& InName, MTLCommandBufferPtr InCmdBuffer, MTLRenderPassDescriptorPtr InRenderPassDesc)
            : StateCache(InName, MoveTemp(InCmdBuffer), MoveTemp(InRenderPassDesc))
        {}
        
    public:
		void End();
        void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount) override;
        void SetRenderPipelineState(GpuRenderPipelineState* InPipelineState) override;
        void SetVertexBuffer(GpuBuffer* InVertexBuffer) override;
        void SetViewPort(const GpuViewPortDesc& InViewPortDesc) override;
		void SetScissorRect(const GpuScissorRectDesc& InScissorRectDes) override;
        void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3) override;
        
    private:
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
        void CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture) override;
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
