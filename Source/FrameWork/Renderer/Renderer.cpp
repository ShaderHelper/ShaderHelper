#include "CommonHeader.h"
#include "Renderer.h"
#include "GpuApi/GpuApiInterface.h"
#include "Renderer/RenderResource/RenderResourceUtil.h"

namespace FRAMEWORK
{

    Renderer::Renderer()
    {
        GpuApi::InitApiEnv();
		RenderResourceUtil::Init();
    }

    void Renderer::Render()
    {
        RenderBegin();
        
        RenderInternal();
        
        RenderEnd();
    }

    void Renderer::RenderBegin()
    {
        GpuApi::StartRenderFrame();
    }

    void Renderer::RenderEnd()
    {
        GpuApi::EndRenderFrame();
    }

}


