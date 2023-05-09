#include "CommonHeader.h"
#include "Renderer.h"
#include "GpuApi/GpuApiInterface.h"
namespace FRAMEWORK
{

    Renderer::Renderer()
    {
        GpuApi::InitApiEnv();
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


