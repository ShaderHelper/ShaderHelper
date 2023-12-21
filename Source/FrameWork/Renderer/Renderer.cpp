#include "CommonHeader.h"
#include "Renderer.h"
#include "GpuApi/GpuApiInterface.h"

namespace FRAMEWORK
{

	Renderer::Renderer()
	{

	}

    void Renderer::Render()
    {
        RenderBegin();
        
        RenderInternal();
        
        RenderEnd();
    }

    void Renderer::RenderBegin()
    {
        
    }

    void Renderer::RenderEnd()
    {
        
    }

}


