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
		Context.Ontputs.Reset();
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

				if (!Context.FinalRT.IsValid() ||
					Context.FinalRT->GetWidth() != (uint32)Context.iResolution.X || Context.FinalRT->GetHeight() != (uint32)Context.iResolution.Y)
				{
					Context.FinalRT = GGpuRhi->CreateTexture({
						.Width = (uint32)Context.iResolution.x,
						.Height = (uint32)Context.iResolution.y,
						.Format = GpuTextureFormat::B8G8R8A8_UNORM,
						.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared,
					});
					GGpuRhi->SetResourceName("FinalRT", Context.FinalRT);
					Context.ViewPort->SetViewPortRenderTexture(Context.FinalRT);
				}
                
                ExecRet GraphRet = ShaderToyGraph->Exec(Context);

				Context.Ontputs.Sort([](const ShaderToyOutputDesc& A, const ShaderToyOutputDesc& B) { return A.Layer < B.Layer; });
				for (int i = 0; i < Context.Ontputs.Num(); i++)
				{
					if (i == 0)
					{
						Context.Ontputs[i].Pass.LoadAction = RenderTargetLoadAction::Clear;
					}
					AddBlitPass(*Context.RG, Context.Ontputs[i].Pass);
				}

                if(GraphRet.Terminate)
                {
                    TSingleton<ShProjectManager>::Get().GetProject()->TimelineStop = true;
                }
            }
            Graph.Execute();
        }
	}


}
