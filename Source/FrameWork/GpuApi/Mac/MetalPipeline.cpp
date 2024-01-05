#include "CommonHeader.h"
#include "MetalPipeline.h"
#include "MetalShader.h"
#include "MetalMap.h"
#include "MetalDevice.h"

namespace FRAMEWORK
{
	TRefCountPtr<MetalPipelineState> CreateMetalPipelineState(const GpuPipelineStateDesc& InPipelineStateDesc)
    {
        MetalShader* Vs = static_cast<MetalShader*>(InPipelineStateDesc.Vs);
        MetalShader* Ps = static_cast<MetalShader*>(InPipelineStateDesc.Ps);
        
        mtlpp::RenderPipelineDescriptor PipelineDesc;
        PipelineDesc.SetVertexFunction(Vs->GetCompilationResult());
        PipelineDesc.SetFragmentFunction(Ps->GetCompilationResult());
        
        ns::AutoReleased<ns::Array<mtlpp::RenderPipelineColorAttachmentDescriptor>> ColorAttachments = PipelineDesc.GetColorAttachments();
        for(uint32 i = 0; i < InPipelineStateDesc.Targets.Num(); i++)
        {
            const PipelineTargetDesc& Target = InPipelineStateDesc.Targets[i];
            ColorAttachments[i].SetBlendingEnabled(Target.BlendEnable);
            ColorAttachments[i].SetSourceRgbBlendFactor((mtlpp::BlendFactor)MapBlendFactor(Target.SrcFactor));
            ColorAttachments[i].SetSourceAlphaBlendFactor((mtlpp::BlendFactor)MapBlendFactor(Target.SrcAlphaFactor));
            ColorAttachments[i].SetDestinationRgbBlendFactor((mtlpp::BlendFactor)MapBlendFactor(Target.DestFactor));
            ColorAttachments[i].SetDestinationAlphaBlendFactor((mtlpp::BlendFactor)MapBlendFactor(Target.DestAlphaFactor));
            ColorAttachments[i].SetRgbBlendOperation((mtlpp::BlendOperation)MapBlendOp(Target.ColorOp));
            ColorAttachments[i].SetAlphaBlendOperation((mtlpp::BlendOperation)MapBlendOp(Target.AlphaOp));
            ColorAttachments[i].SetWriteMask((mtlpp::ColorWriteMask)MapWriteMask(Target.Mask));
            
            ColorAttachments[i].SetPixelFormat((mtlpp::PixelFormat)MapTextureFormat(Target.TargetFormat));
        }

        ns::AutoReleasedError Err;
        mtlpp::RenderPipelineState PipelineState = GDevice.NewRenderPipelineState(PipelineDesc, mtlpp::PipelineOption::NoPipelineOption ,nullptr ,&Err);
        if (!PipelineState)
        {
            SH_LOG(LogMetal, Fatal, TEXT("Failed to create render pipeline: %s"), ConvertOcError(Err.GetPtr()));
            //TDOO: fallback to default pipeline.
        }
        
        return new MetalPipelineState(MoveTemp(PipelineState), MapPrimitiveType(InPipelineStateDesc.Primitive));
    }
}
