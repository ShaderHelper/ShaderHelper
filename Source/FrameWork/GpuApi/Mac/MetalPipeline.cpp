#include "CommonHeader.h"
#include "MetalPipeline.h"
#include "MetalShader.h"
#include "MetalMap.h"
#include "MetalDevice.h"

namespace FRAMEWORK
{
    TRefCountPtr<MetalPipelineState> CreateMetalPipelineState(const PipelineStateDesc& InPipelineStateDesc)
    {
        MetalShader* Vs = static_cast<MetalShader*>(InPipelineStateDesc.Vs);
        MetalShader* Ps = static_cast<MetalShader*>(InPipelineStateDesc.Ps);
        
        mtlpp::RenderPipelineDescriptor PipelineDesc;
        PipelineDesc.SetVertexFunction(Vs->GetCompilationResult());
        PipelineDesc.SetFragmentFunction(Ps->GetCompilationResult());
        
        ns::AutoReleased<ns::Array<mtlpp::RenderPipelineColorAttachmentDescriptor>> ColorAttachments = PipelineDesc.GetColorAttachments();
        BlendStateDesc BlendDesc =InPipelineStateDesc.BlendState;
        const uint32 BlendRtNum = BlendDesc.RtDescs.Num();
        for(uint32 i = 0; i < BlendRtNum; i++)
        {
            BlendRenderTargetDesc BlendRtInfo = BlendDesc.RtDescs[i];
            ColorAttachments[i].SetBlendingEnabled(BlendRtInfo.BlendEnable);
            ColorAttachments[i].SetSourceRgbBlendFactor((mtlpp::BlendFactor)MapBlendFactor(BlendRtInfo.SrcFactor));
            ColorAttachments[i].SetSourceAlphaBlendFactor((mtlpp::BlendFactor)MapBlendFactor(BlendRtInfo.SrcAlphaFactor));
            ColorAttachments[i].SetDestinationRgbBlendFactor((mtlpp::BlendFactor)MapBlendFactor(BlendRtInfo.DestFactor));
            ColorAttachments[i].SetDestinationAlphaBlendFactor((mtlpp::BlendFactor)MapBlendFactor(BlendRtInfo.DestAlphaFactor));
            ColorAttachments[i].SetRgbBlendOperation((mtlpp::BlendOperation)MapBlendOp(BlendRtInfo.ColorOp));
            ColorAttachments[i].SetAlphaBlendOperation((mtlpp::BlendOperation)MapBlendOp(BlendRtInfo.AlphaOp));
            ColorAttachments[i].SetWriteMask((mtlpp::ColorWriteMask)MapWriteMask(BlendRtInfo.Mask));
        }
        
        const uint32 RtFormatNum = InPipelineStateDesc.RtFormats.Num();
        for(uint32 i = 0; i < RtFormatNum; i++)
        {
            ColorAttachments[i].SetPixelFormat((mtlpp::PixelFormat)MapTextureFormat(InPipelineStateDesc.RtFormats[i]));
        }

        ns::AutoReleasedError Err;
        mtlpp::RenderPipelineState PipelineState = GDevice.NewRenderPipelineState(PipelineDesc, mtlpp::PipelineOption::NoPipelineOption ,nullptr ,&Err);
        if (!PipelineState)
        {
            SH_LOG(LogMetal, Fatal, TEXT("Failed to create render pipeline: %s"), ConvertOcError(Err.GetPtr()));
            //TDOO: fallback to default pipeline.
        }
        
        return new MetalPipelineState(MoveTemp(PipelineState));
    }
}
