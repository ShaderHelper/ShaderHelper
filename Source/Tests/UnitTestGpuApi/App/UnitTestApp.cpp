#include "CommonHeader.h"
#include "UnitTestApp.h"
#include "Editor/UnitTestEditor.h"
#include "Common/Path/PathHelper.h"
#include "GpuApi/GpuFeature.h"


using namespace FRAMEWORK;

namespace UNITTEST_GPUAPI
{
	UnitTestApp::UnitTestApp(const Vector2D& InClientSize, const TCHAR* CommandLine)
		: App(InClientSize, CommandLine)
	{
		AppEditor = MakeUnique<UnitTestEditor>(AppClientSize);
	}

	void UnitTestApp::Update(double DeltaTime)
	{
		App::Update(DeltaTime);
	}

	void UnitTestApp::Render()
	{
		App::Render();

		GGpuRhi->BeginGpuCapture("TestCast");

		uint16 TestData = 0xFD5E; //NaN
		TArray<uint8> RawData((uint8*)&TestData, sizeof(TestData));

		GpuTextureDesc Desc{ 1, 1, GpuTextureFormat::R16_FLOAT, GpuTextureUsage::ShaderResource , RawData };
		TRefCountPtr<GpuTexture> TestTex = GGpuRhi->CreateTexture(Desc);

		TRefCountPtr<GpuShader> Vs = GGpuRhi->CreateShaderFromFile(
			PathHelper::ShaderDir() / "Test/TestCast.hlsl",
			ShaderType::VertexShader,
			TEXT("MainVS")
		);
		if (GpuFeature::Support16bitType) {
			Vs->AddFlag(GpuShaderFlag::Enable16bitType);
		}
	
		FString ErrorInfo;
		GGpuRhi->CrossCompileShader(Vs, ErrorInfo);
		check(ErrorInfo.IsEmpty());

		TRefCountPtr<GpuBindGroupLayout> BindGroupLayout = GpuBindGroupLayoutBuilder{ 0 }
			.AddExistingBinding(0, BindingType::Texture, BindingShaderStage::Vertex)
			.Build();

		TRefCountPtr<GpuBindGroup> BindGroup = GpuBindGrouprBuilder{ BindGroupLayout }
			.SetExistingBinding(0, TestTex)
			.Build();

		GpuPipelineStateDesc PipelineDesc{};
		PipelineDesc.Vs = Vs;
		PipelineDesc.BindGroupLayout0 = BindGroupLayout;

		TRefCountPtr<GpuPipelineState> Pipeline = GGpuRhi->CreateRenderPipelineState(PipelineDesc);

		GGpuRhi->BeginRenderPass({}, TEXT("TestCast"));
		{
			GGpuRhi->SetRenderPipelineState(Pipeline);
			GGpuRhi->SetBindGroups(BindGroup, nullptr, nullptr, nullptr);
			GGpuRhi->DrawPrimitive(0, 3, 0, 1);
		}
		GGpuRhi->EndRenderPass();

		GGpuRhi->EndGpuCapture();
	}

}


