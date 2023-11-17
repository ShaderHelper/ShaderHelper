#pragma once
#include "MetalCommon.h"
#include "MetalPipeline.h"
#include "Common/Util/Singleton.h"
#include "GpuApi/GpuResource.h"
#include "MetalBuffer.h"
#include "MetalArgumentBuffer.h"

namespace FRAMEWORK
{
    class CommandListContext
    {
    public:
        CommandListContext();
        
    public:
        id<MTLRenderCommandEncoder> GetRenderCommandEncoder() const { return CurrentRenderCommandEncoder.GetPtr(); }
        mtlpp::BlitCommandEncoder GetBlitCommandEncoder() {
            //Return a new BlitCommandEncoder when we want to copy something every time.
            return CurrentCommandBuffer.BlitCommandEncoder();
        }
        id<MTLCommandBuffer> GetCommandBuffer() const { return CurrentCommandBuffer.GetPtr(); }
 
        void SetCommandBuffer(mtlpp::CommandBuffer InCommandBuffer) {
            CurrentCommandBuffer = MoveTemp(InCommandBuffer);
        }
        void SetRenderPassDesc(mtlpp::RenderPassDescriptor InPassDesc, const FString& PassName) {
            CurrentRenderPassDesc = MoveTemp(InPassDesc);
            CurrentRenderCommandEncoder = CurrentCommandBuffer.RenderCommandEncoder(CurrentRenderPassDesc);
            CurrentRenderCommandEncoder.GetPtr().label = [NSString stringWithUTF8String:TCHAR_TO_ANSI(*PassName)];
        }

        void SetPipeline(MetalPipelineState* InPipelineState) 
		{
			if (InPipelineState != CurrentPipelineState)
			{
				CurrentPipelineState = InPipelineState;
				MarkPipelineDirty(true);
			}
		}

        void SetVertexBuffer(MetalBuffer* InBuffer) 
		{ 
			if (CurrentVertexBuffer != InBuffer)
			{
				CurrentVertexBuffer = InBuffer;
				MarkVertexBufferDirty(true);
			}
		}

        void SetViewPort(mtlpp::Viewport InViewPort, mtlpp::ScissorRect InSissorRect) 
		{
			if (!CurrentViewport || FMemory::Memcmp(CurrentViewPort.Get() , &InViewPort, sizeof(mtlpp::Viewport))
			{
				CurrentViewport = MakeUnique<mtlpp::Viewport>(MoveTemp(InViewPort));
				CurrentScissorRect = MakeUnique<mtlpp::ScissorRect>(MoveTemp(InSissorRect));
				MarkViewportDirty(true);
			}
           
        }

        void SetBindGroups(MetalBindGroup* InGroup0, MetalBindGroup* InGroup1, MetalBindGroup* InGroup2, MetalBindGroup* InGroup3) 
		{
			if (InGroup0 != CurrentBindGroup0) {
				CurrentBindGroup0 = InGroup0;
				MarkBindGroup0Dirty(true);
			}

			if (InGroup1 != CurrentBindGroup1) {
				CurrentBindGroup1 = InGroup1;
				MarkBindGroup1Dirty(true);
			}

			if (InGroup2 != CurrentBindGroup2) {
				CurrentBindGroup2 = InGroup2;
				MarkBindGroup2Dirty(true);
			}

			if (InGroup3 != CurrentBindGroup3) {
				CurrentBindGroup3 = InGroup3;
				MarkBindGroup3Dirty(true);
			}
        }
        
        void PrepareDrawingEnv();
        void ClearBinding();
        
        void MarkPipelineDirty(bool IsDirty) { IsPipelineDirty = IsDirty; }
        void MarkViewportDirty(bool IsDirty) { IsViewportDirty = IsDirty; }
        void MarkVertexBufferDirty(bool IsDirty) { IsVertexBufferDirty = IsDirty; }

		void MarkBindGroup0Dirty(bool IsDirty) { IsBindGroup0Dirty = IsDirty; }
		void MarkBindGroup1Dirty(bool IsDirty) { IsBindGroup1Dirty = IsDirty; }
		void MarkBindGroup2Dirty(bool IsDirty) { IsBindGroup2Dirty = IsDirty; }
		void MarkBindGroup3Dirty(bool IsDirty) { IsBindGroup3Dirty = IsDirty; }
        
    private:
        MetalPipelineState* CurrentPipelineState;
        MetalBuffer* CurrentVertexBuffer;
        TUniquePtr<mtlpp::Viewport> CurrentViewport;
        TUniquePtr<mtlpp::ScissorRect> CurrentScissorRect;
        
        mtlpp::RenderPassDescriptor  CurrentRenderPassDesc;
        mtlpp::RenderCommandEncoder CurrentRenderCommandEncoder;
        mtlpp::CommandBuffer CurrentCommandBuffer;
        
        MetalBindGroup* CurrentBindGroup0;
        MetalBindGroup* CurrentBindGroup1;
        MetalBindGroup* CurrentBindGroup2;
        MetalBindGroup* CurrentBindGroup3;
        
        bool IsPipelineDirty : 1;
        bool IsViewportDirty : 1;
        bool IsVertexBufferDirty : 1;
		
		bool IsBindGroup0Dirty : 1;
		bool IsBindGroup1Dirty : 1;
		bool IsBindGroup2Dirty : 1;
		bool IsBindGroup3Dirty : 1;
    };

    inline CommandListContext* GetCommandListContext() {
        return &TSingleton<CommandListContext>::Get();
    }
}
