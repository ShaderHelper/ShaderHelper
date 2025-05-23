#include "CommonHeader.h"
#include "ShaderToyRenderComp.h"
#include "GpuApi/GpuRhi.h"
#include "ProjectManager/ShProjectManager.h"
#include "GpuApi/GpuResourceHelper.h"
#include "RenderResource/PrintBuffer.h"

using namespace FW;

namespace SH
{

	ShaderToyRenderComp::ShaderToyRenderComp(ShaderToy* InShaderToyGraph, PreviewViewPort* InViewPort)
		: ShaderToyGraph(InShaderToyGraph)
		, ViewPort(InViewPort)
	{
        ResizeHandle = ViewPort->ViewportResize.AddRaw(this, &ShaderToyRenderComp::OnViewportResize);
		Context.iResolution = { (float)ViewPort->GetSize().X, (float)ViewPort->GetSize().Y };
	}

    ShaderToyRenderComp::~ShaderToyRenderComp()
    {
        ViewPort->ViewportResize.Remove(ResizeHandle);
    }

	void ShaderToyRenderComp::OnViewportResize(const Vector2f& InResolution)
	{
		Context.iResolution = InResolution;
	}

	void ShaderToyRenderComp::RenderBegin()
	{
		Context.iTime = TSingleton<ShProjectManager>::Get().GetProject()->TimelineCurTime;
		Context.iMouse = ViewPort->GetiMouse();
	}

	void ShaderToyRenderComp::RenderInternal()
	{
        if(!TSingleton<ShProjectManager>::Get().GetProject()->TimelineStop)
		{
			if(!Context.FinalRT.IsValid() ||
				Context.FinalRT->GetWidth() != (uint32)Context.iResolution.X || Context.FinalRT->GetHeight() != (uint32)Context.iResolution.Y )
			{
				TArray<uint8> Datas;
				Datas.SetNumZeroed(4 * (uint32)Context.iResolution.x * (uint32)Context.iResolution.y);
				//Note: BGRA8_UNORM is default framebuffer format in ue standalone renderer framework.
				Context.FinalRT = GGpuRhi->CreateTexture({
					.Width = (uint32)Context.iResolution.x,
					.Height = (uint32)Context.iResolution.y,
					.Format = GpuTextureFormat::B8G8R8A8_UNORM,
					.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared,
					.InitialData = MoveTemp(Datas)
				});
				GGpuRhi->SetResourceName("FinalRT", Context.FinalRT);
				ViewPort->SetViewPortRenderTexture(Context.FinalRT);
			}
		
            RenderGraph Graph;
            Context.RG = &Graph;
            {
                //Update builtin uniformbuffer value.
                StShader::GetBuiltInUb()->GetMember<Vector2f>("iResolution") = Context.iResolution;
                StShader::GetBuiltInUb()->GetMember<float>("iTime") = Context.iTime;
				StShader::GetBuiltInUb()->GetMember<Vector4f>("iMouse") = Context.iMouse;
                
                ExecRet GraphRet = ShaderToyGraph->Exec(Context);
                if(GraphRet.Terminate)
                {
                    TSingleton<ShProjectManager>::Get().GetProject()->TimelineStop = true;
                }
            }
            Graph.Execute();

			TArray<FString> ShaderPrintLogs = TSingleton<PrintBuffer>::Get().GetPrintStrings();
			for (const FString& PrintLog : ShaderPrintLogs)
			{
				SH_LOG(LogShaderPrint, Display, TEXT("%s"), *PrintLog);
			}
			TSingleton<PrintBuffer>::Get().Clear();
        }
	}


}
