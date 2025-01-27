#include "CommonHeader.h"
#include "ShaderToyRenderComp.h"
#include "GpuApi/GpuRhi.h"
#include "ProjectManager/ShProjectManager.h"

using namespace FW;

namespace SH
{

	ShaderToyRenderComp::ShaderToyRenderComp(ShaderToy* InShaderToyGraph, PreviewViewPort* InViewPort)
		: ShaderToyGraph(InShaderToyGraph)
		, ViewPort(InViewPort)
	{
        ResizeHandle = ViewPort->OnViewportResize.AddRaw(this, &ShaderToyRenderComp::OnViewportResize);
	}

    ShaderToyRenderComp::~ShaderToyRenderComp()
    {
        ViewPort->OnViewportResize.Remove(ResizeHandle);
    }

	void ShaderToyRenderComp::OnViewportResize(const Vector2f& InResolution)
	{
		Context.iResolution = InResolution;

		//Note: BGRA8_UNORM is default framebuffer format in ue standalone renderer framework.
		GpuTextureDesc Desc{ (uint32)InResolution.x, (uint32)InResolution.y, GpuTextureFormat::B8G8R8A8_UNORM, GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared };
		Context.FinalRT = GGpuRhi->CreateTexture(Desc);
		GGpuRhi->SetResourceName("FinalRT", Context.FinalRT);

		ViewPort->SetViewPortRenderTexture(Context.FinalRT);
		RenderInternal();
	}

	void ShaderToyRenderComp::RenderBegin()
	{
		Context.iTime = TSingleton<ShProjectManager>::Get().GetProject()->TimelineCurTime;
	}

	void ShaderToyRenderComp::RenderInternal()
	{
        if(!TSingleton<ShProjectManager>::Get().GetProject()->TimelineStop)
        {
            RenderGraph Graph;
            Context.RG = &Graph;
            {
                bool AnyError = ShaderToyGraph->Exec(Context);
                if(AnyError)
                {
                    TSingleton<ShProjectManager>::Get().GetProject()->TimelineStop = true;
                }
            }
            Graph.Execute();
        }
	}


}
