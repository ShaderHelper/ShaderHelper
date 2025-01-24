#include "CommonHeader.h"
#include "ShaderToyRenderComp.h"
#include "GpuApi/GpuRhi.h"

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
		static double RenderTimeStart = FPlatformTime::Seconds();
		Context.iTime = float(FPlatformTime::Seconds() - RenderTimeStart);
	}

	void ShaderToyRenderComp::RenderInternal()
	{
		RenderGraph Graph;
		Context.RG = &Graph;
		{
			ShaderToyGraph->Exec(Context);
		}
		Graph.Execute();
	}


}
