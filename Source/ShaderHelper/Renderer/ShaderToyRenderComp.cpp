#include "CommonHeader.h"
#include "ShaderToyRenderComp.h"
#include "GpuApi/GpuRhi.h"
#include "ProjectManager/ShProjectManager.h"
#include "GpuApi/GpuResourceHelper.h"
#include "AssetObject/StShader.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyKeyboardNode.h"

using namespace FW;

namespace SH
{

	void ShaderToyRenderComp::RefreshKeyboard()
	{
		TArray<uint8> RawData;
		RawData.SetNumZeroed(256 * 3);
		for (uint32 PressedKey : PressedKeys)
		{
			RawData[PressedKey] = 255;
		}
		Context.Keyboard = GGpuRhi->CreateTexture({
			.Width = 256, .Height = 3,
			.Format = GpuTextureFormat::R8_UNORM,
			.Usage = GpuTextureUsage::ShaderResource,
			.InitialData = RawData
		});
		GGpuRhi->SetResourceName("Keyboard", Context.Keyboard);
		for (auto Node : ShaderToyGraph->GetNodes())
		{
			if (Node->DynamicMetaType() == GetMetaType<ShaderToyKeyboardNode>())
			{
				Node->HasResponse = PressedKeys.Num() > 0;
			}
		}
	}

	ShaderToyRenderComp::ShaderToyRenderComp(ShaderToy* InShaderToyGraph, PreviewViewPort* InViewPort)
		: ShaderToyGraph(InShaderToyGraph)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		Context.ViewPort = InViewPort;
		ResizeHandle = Context.ViewPort->ResizeHandler.AddLambda([this](const Vector2f& InResolution) {
			Context.iResolution = { InResolution.X, InResolution.Y, InResolution.Y / InResolution.X };
		});
		FocusLostHandle = Context.ViewPort->FocusLostHandler.AddLambda([this](const FFocusEvent& InFocusEvent) {
			PressedKeys.Reset();
		});
		KeyDownHandle = Context.ViewPort->KeyDownHandler.AddLambda([this, ShEditor](const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) {
			if (!PressedKeys.Contains(InKeyEvent.GetKeyCode()))
			{
				PressedKeys.Add(InKeyEvent.GetKeyCode());
				RefreshKeyboard();
				ShEditor->ForceRender();
			}
		});
		KeyUpHandle = Context.ViewPort->KeyUpHandler.AddLambda([this, ShEditor](const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) {
			PressedKeys.Remove(InKeyEvent.GetKeyCode());
			RefreshKeyboard();
			ShEditor->ForceRender();
		});
		MouseDownHandle = Context.ViewPort->MouseDownHandler.AddLambda([this, ShEditor](const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
			Context.iMouse.xy = (Vector2f)(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()) * MyGeometry.Scale);
			Context.iMouse.zw = Context.iMouse.xy;
			ShEditor->ForceRender();
		});
		MouseUpHandle = Context.ViewPort->MouseUpHandler.AddLambda([this](const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
			if (Context.iMouse.z > 0)
			{
				Context.iMouse.z = -Context.iMouse.z;
			}
			if (Context.iMouse.w > 0)
			{
				Context.iMouse.w = -Context.iMouse.w;
			}
		});
		MouseMoveHandle = Context.ViewPort->MouseMoveHandler.AddLambda([this, ShEditor](const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
			if (Context.iMouse.z > 0)
			{
				Context.iMouse.xy = (Vector2f)(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()) * MyGeometry.Scale);
				if (Context.iMouse.w > 0)
				{
					Context.iMouse.w = -Context.iMouse.w;
				}
				ShEditor->ForceRender();
			}
		});
		RefreshKeyboard();
		Context.iResolution = { (float)Context.ViewPort->GetSize().X, (float)Context.ViewPort->GetSize().Y, (float)Context.ViewPort->GetSize().Y / Context.ViewPort->GetSize().X };
	}

    ShaderToyRenderComp::~ShaderToyRenderComp()
    {
		Context.ViewPort->ResizeHandler.Remove(ResizeHandle);
		Context.ViewPort->FocusLostHandler.Remove(FocusLostHandle);
		Context.ViewPort->KeyDownHandler.Remove(KeyDownHandle);
		Context.ViewPort->KeyUpHandler.Remove(KeyUpHandle);
		Context.ViewPort->MouseDownHandler.Remove(MouseDownHandle);
		Context.ViewPort->MouseUpHandler.Remove(MouseUpHandle);
		Context.ViewPort->MouseMoveHandler.Remove(MouseMoveHandle);
    }

	void ShaderToyRenderComp::RenderBegin()
	{
		Context.iTime = TSingleton<ShProjectManager>::Get().GetProject()->TimelineCurTime;
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
					else
					{
						Context.Ontputs[i].Pass.LoadAction = RenderTargetLoadAction::Load;
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
