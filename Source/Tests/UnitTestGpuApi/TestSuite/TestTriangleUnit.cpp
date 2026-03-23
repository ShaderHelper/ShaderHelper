#include "CommonHeader.h"
#include "TestTriangleUnit.h"
#include "Common/Path/PathHelper.h"

using namespace FW;

namespace UNITTEST_GPUAPI
{
	TestTriangleUnit::TestTriangleUnit()
		: TestUnit(TEXT("TestTriangle"))
	{
	}

	void TestTriangleUnit::Init()
	{
		Vs = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "Test/TestTriangle.hlsl",
			.Type = ShaderType::VertexShader,
			.EntryPoint = "MainVS",
		});

		Ps = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "Test/TestTriangle.hlsl",
			.Type = ShaderType::PixelShader,
			.EntryPoint = "MainPS",
		});

		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(Vs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
		GGpuRhi->CompileShader(Ps, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());

		GpuRenderPipelineStateDesc PipelineDesc{
			.Vs = Vs,
			.Ps = Ps,
			.Targets = {
				{.TargetFormat = Viewport->GetRenderTargetFormat() }
			}
		};

		Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);
	}

	void TestTriangleUnit::Render()
	{
		GpuTexture* RenderTarget = Viewport->GetRenderTarget();

		GGpuRhi->BeginGpuCapture("TestTriangle");

		GpuRenderPassDesc PassDesc;
		PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ RenderTarget->GetDefaultView(), RenderTargetLoadAction::Load, RenderTargetStoreAction::Store });

		auto CmdRecorder = GGpuRhi->BeginRecording();
		{
			auto PassRecorder = CmdRecorder->BeginRenderPass(PassDesc, TEXT("TestTriangle"));
			{
				PassRecorder->SetRenderPipelineState(Pipeline);
				PassRecorder->DrawPrimitive(0, 3, 0, 1);
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });

		GGpuRhi->EndGpuCapture();
	}
}
