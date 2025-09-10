#include "CommonHeader.h"
#include "ShaderToyRenderComp.h"
#include "GpuApi/GpuRhi.h"
#include "ProjectManager/ShProjectManager.h"
#include "GpuApi/GpuResourceHelper.h"
#include "AssetObject/StShader.h"

using namespace FW;

namespace SH
{

	ShaderToyRenderComp::ShaderToyRenderComp(ShaderToy* InShaderToyGraph, PreviewViewPort* InViewPort)
		: ShaderToyGraph(InShaderToyGraph)
	{
		Context.ViewPort = InViewPort;
        ResizeHandle = Context.ViewPort->ViewportResize.AddRaw(this, &ShaderToyRenderComp::OnViewportResize);
		Context.iResolution = { (float)Context.ViewPort->GetSize().X, (float)Context.ViewPort->GetSize().Y, (float)Context.ViewPort->GetSize().Y / Context.ViewPort->GetSize().X };
	}

    ShaderToyRenderComp::~ShaderToyRenderComp()
    {
		Context.ViewPort->ViewportResize.Remove(ResizeHandle);
    }

	void ShaderToyRenderComp::OnViewportResize(const Vector2f& InResolution)
	{
		Context.iResolution = { InResolution.X, InResolution.Y, InResolution.Y / InResolution.X };
	}

	void ShaderToyRenderComp::RenderBegin()
	{
		Context.iTime = TSingleton<ShProjectManager>::Get().GetProject()->TimelineCurTime;
		Context.iMouse = Context.ViewPort->GetiMouse();
	}

	void ShaderToyRenderComp::RenderInternal()
	{
        if(!TSingleton<ShProjectManager>::Get().GetProject()->TimelineStop)
		{
            RenderGraph Graph;
            Context.RG = &Graph;
            {
                //Update builtin uniformbuffer value.
                StShader::GetBuiltInUb()->GetMember<Vector3f>("iResolution") = Context.iResolution;
                StShader::GetBuiltInUb()->GetMember<float>("iTime") = Context.iTime;
				StShader::GetBuiltInUb()->GetMember<Vector4f>("iMouse") = Context.iMouse;
                
                ExecRet GraphRet = ShaderToyGraph->Exec(Context);
                if(GraphRet.Terminate)
                {
                    TSingleton<ShProjectManager>::Get().GetProject()->TimelineStop = true;
                }
            }
            Graph.Execute();
        }
	}


}
