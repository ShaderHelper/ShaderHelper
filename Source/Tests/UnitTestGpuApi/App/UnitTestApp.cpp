#include "CommonHeader.h"
#include "UnitTestApp.h"
#include "Editor/UnitTestEditor.h"
#include "Editor/TestViewport.h"
#include "Common/Path/PathHelper.h"
#include "GpuApi/GpuFeature.h"


using namespace FW;

namespace UNITTEST_GPUAPI
{
	UnitTestApp::UnitTestApp(const Vector2D& InClientSize, const TCHAR* CommandLine)
		: App(InClientSize, CommandLine)
	{
		
	}

	void UnitTestApp::Init()
	{
		App::Init();
		
		AppEditor = MakeUnique<UnitTestEditor>(AppClientSize);
		//Add test otherwise the run loop will immediately exit.
		AddTestCast();
		AddTestTriangle();
	}

	void UnitTestApp::Update(float DeltaTime)
	{
		App::Update(DeltaTime);
	}

	void UnitTestApp::Render()
	{
		App::Render();

		for (const auto& TestUnit: TestUnits)
		{
			TestUnit.TestFunc(TestUnit.Viewport.Get());
		}
	}

	void UnitTestApp::AddTestCast()
	{
		static_cast<UnitTestEditor*>(AppEditor.Get())->AddTest("TestCast", [](TestViewport*) {
			GGpuRhi->BeginGpuCapture("TestCast");

			uint16 TestData = 0xFD5E; //NaN 64862
			TArray<uint8> RawData((uint8*)&TestData, sizeof(TestData));

			GpuTextureDesc Desc{ 1, 1, GpuTextureFormat::R16_FLOAT, GpuTextureUsage::ShaderResource , RawData };
			TRefCountPtr<GpuTexture> TestTex = GGpuRhi->CreateTexture(Desc);
			GGpuRhi->SetResourceName("TestCast_Tex", TestTex);

			TRefCountPtr<GpuShader> Vs = GGpuRhi->CreateShaderFromFile({
				.FileName = PathHelper::ShaderDir() / "Test/TestCast.hlsl",
				.Type = ShaderType::VertexShader,
				.EntryPoint = "MainVS",
			});
			if (GpuFeature::Support16bitType) {
				Vs->CompilerFlag |= GpuShaderCompilerFlag::Enable16bitType;
			}
			FString ErrorInfo, WarnInfo;
			GGpuRhi->CompileShader(Vs, ErrorInfo, WarnInfo);
			check(ErrorInfo.IsEmpty());

			TRefCountPtr<GpuBindGroupLayout> BindGroupLayout = GpuBindGroupLayoutBuilder{ 0 }
				.AddExistingBinding(0, BindingType::Texture, BindingShaderStage::Vertex)
				.Build();

			TRefCountPtr<GpuBindGroup> BindGroup = GpuBindGroupBuilder{ BindGroupLayout }
				.SetExistingBinding(0, TestTex)
				.Build();

			TRefCountPtr<GpuTexture> DummyRT = GGpuRhi->CreateTexture({ 1, 1, GpuTextureFormat::R8G8B8A8_UNORM, GpuTextureUsage::RenderTarget });

			GpuRenderPipelineStateDesc PipelineDesc{
				.CheckLayout = true,
				.Vs = Vs,
				.Targets = { {.TargetFormat = DummyRT->GetFormat()}},
				.BindGroupLayout0 = BindGroupLayout
			};
			TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);
			GpuRenderPassDesc PassDesc;
			PassDesc.ColorRenderTargets.Add({ DummyRT });

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
		});
	}

	void UnitTestApp::AddTestTriangle()
	{
		static_cast<UnitTestEditor*>(AppEditor.Get())->AddTest("TestTriangle", [](TestViewport* Viewport) {
			GpuTexture* RenderTarget = Viewport->GetRenderTarget();

			GGpuRhi->BeginGpuCapture("TestTriangle");

			TRefCountPtr<GpuShader> Vs = GGpuRhi->CreateShaderFromFile({
				.FileName = PathHelper::ShaderDir() / "Test/TestTriangle.hlsl",
				.Type = ShaderType::VertexShader,
				.EntryPoint = "MainVS",
			});

			TRefCountPtr<GpuShader> Ps = GGpuRhi->CreateShaderFromFile({
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

			TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

			GpuRenderPassDesc PassDesc;
			PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ RenderTarget, RenderTargetLoadAction::Load, RenderTargetStoreAction::Store });

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
		});
	}

}


