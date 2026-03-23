#include "CommonHeader.h"
#include "TestCastUnit.h"
#include "Common/Path/PathHelper.h"
#include "GpuApi/GpuFeature.h"

using namespace FW;

namespace UNITTEST_GPUAPI
{
	TestCastUnit::TestCastUnit()
		: TestUnit(TEXT("TestCast"))
	{
	}

	void TestCastUnit::Init()
	{
		uint16 TestData = 0xFD5E;
		TArray<uint8> RawData((uint8*)&TestData, sizeof(TestData));

		TestTex = GGpuRhi->CreateTexture({ 1, 1, GpuTextureFormat::R16_FLOAT, GpuTextureUsage::ShaderResource, RawData });
		GGpuRhi->SetResourceName("TestCast_Tex", TestTex);

		Vs = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "Test/TestCast.hlsl",
			.Type = ShaderType::VertexShader,
			.EntryPoint = "MainVS",
		});
		if (GpuFeature::Support16bitType)
		{
			Vs->CompilerFlag |= GpuShaderCompilerFlag::Enable16bitType;
		}

		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(Vs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());

		BindGroupLayout = GpuBindGroupLayoutBuilder{ 0 }
			.AddExistingBinding(0, BindingType::Texture, BindingShaderStage::Vertex)
			.Build();

		BindGroup = GpuBindGroupBuilder{ BindGroupLayout }
			.SetExistingBinding(0, TestTex)
			.Build();

		DummyRT = GGpuRhi->CreateTexture({ 1, 1, GpuTextureFormat::R8G8B8A8_UNORM, GpuTextureUsage::RenderTarget });

		GpuRenderPipelineStateDesc PipelineDesc{
			.CheckLayout = true,
			.Vs = Vs,
			.Targets = { {.TargetFormat = DummyRT->GetFormat()} },
			.BindGroupLayout0 = BindGroupLayout,
		};
		Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);
	}

	void TestCastUnit::Render()
	{
		GGpuRhi->BeginGpuCapture("TestCast");

		GpuRenderPassDesc PassDesc;
		PassDesc.ColorRenderTargets.Add({ DummyRT->GetDefaultView() });

		auto CmdRecorder = GGpuRhi->BeginRecording();
		{
			auto PassRecorder = CmdRecorder->BeginRenderPass(PassDesc, TEXT("TestCast"));
			{
				PassRecorder->SetRenderPipelineState(Pipeline);
				PassRecorder->SetBindGroups(BindGroup, nullptr, nullptr, nullptr);
				PassRecorder->DrawPrimitive(0, 3, 0, 1);
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });

		GGpuRhi->EndGpuCapture();
	}
}
