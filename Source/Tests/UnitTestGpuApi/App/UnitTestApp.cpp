#include "CommonHeader.h"
#include "UnitTestApp.h"
#include "Editor/UnitTestEditor.h"
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
		//Add swindow otherwise the run loop will immediately exit.
		AppEditor = MakeUnique<UnitTestEditor>(AppClientSize);
	}

	void UnitTestApp::Update(float DeltaTime)
	{
		App::Update(DeltaTime);
	}

	void UnitTestApp::Render()
	{
		App::Render();

		//GGpuRhi->BeginGpuCapture("TestCast");

		uint16 TestData = 0xFD5E; //NaN 64862
		TArray<uint8> RawData((uint8*)&TestData, sizeof(TestData));

		GpuTextureDesc Desc{ 1, 1, GpuTextureFormat::R16_FLOAT, GpuTextureUsage::ShaderResource , RawData };
		TRefCountPtr<GpuTexture> TestTex = GGpuRhi->CreateTexture(Desc);

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

		GpuRenderPipelineStateDesc PipelineDesc{
			.Vs = Vs,
			.BindGroupLayout0 = BindGroupLayout
		};

		TRefCountPtr<GpuRenderPipelineState> Pipeline = GGpuRhi->CreateRenderPipelineState(PipelineDesc);

		auto CmdRecorder = GGpuRhi->BeginRecording();
		{
			auto PassRecorder = CmdRecorder->BeginRenderPass({}, TEXT("TestCast"));
			{
				PassRecorder->SetRenderPipelineState(Pipeline);
				PassRecorder->SetBindGroups(BindGroup, nullptr, nullptr, nullptr);
				PassRecorder->DrawPrimitive(0, 3, 0, 1);
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({CmdRecorder});

		//GGpuRhi->EndGpuCapture();
	}

}


