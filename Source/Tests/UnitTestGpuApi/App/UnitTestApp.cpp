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
		Editor = MakeUnique<UnitTestEditor>(AppClientSize);

		// Create Rhi backend.
		GpuRhiConfig Config;
		Config.EnableValidationCheck = false;
		Config.BackendType = GpuRhiBackendType::Default;

		if (FParse::Param(FCommandLine::Get(), TEXT("validation")))
		{
			Config.EnableValidationCheck = true;
		}
		FString BackendName;
		if (FParse::Value(FCommandLine::Get(), TEXT("backend="), BackendName))
		{
			if (BackendName.Equals(TEXT("Vulkan")))
			{
				Config.BackendType = GpuRhiBackendType::Vulkan;
			}
#if PLATFORM_MAC
			else if (BackendName.Equals(TEXT("Metal")))
			{
				Config.BackendType = GpuRhiBackendType::Metal;
			}
#elif PLATFORM_WINDOWS
			else if (BackendName.Equals(TEXT("DX12")))
			{
				Config.BackendType = GpuRhiBackendType::DX12;
			}
#endif
			else {
				// invalid backend name, use default backend type.
			}
		}
		Rhi = GpuRhi::CreateGpuRhi(Config);
	}

	void UnitTestApp::Update(double DeltaTime)
	{
		App::Update(DeltaTime);
	}

	void UnitTestApp::Render()
	{
		App::Render();

		 Rhi->BeginGpuCapture("TestCast");

		uint16 TestData = 0xFD5E; //NaN
		TArray<uint8> RawData((uint8*)&TestData, sizeof(TestData));

		GpuTextureDesc Desc{ 1, 1, GpuTextureFormat::R16_FLOAT, GpuTextureUsage::ShaderResource , RawData };
		TRefCountPtr<GpuTexture> TestTex =  Rhi->CreateTexture(Desc);

		TRefCountPtr<GpuShader> Vs =  Rhi->CreateShaderFromFile(
			PathHelper::ShaderDir() / "Test/TestCast.hlsl",
			ShaderType::VertexShader,
			TEXT("MainVS")
		);
		if (GpuFeature::Support16bitType) {
			Vs->AddFlag(GpuShaderFlag::Enable16bitType);
		}
	
		FString ErrorInfo;
		 Rhi->CrossCompileShader(Vs, ErrorInfo);
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

		TRefCountPtr<GpuPipelineState> Pipeline =  Rhi->CreateRenderPipelineState(PipelineDesc);

		 Rhi->BeginRenderPass({}, TEXT("TestCast"));
		{
			 Rhi->SetRenderPipelineState(Pipeline);
			 Rhi->SetBindGroups(BindGroup, nullptr, nullptr, nullptr);
			 Rhi->DrawPrimitive(0, 3, 0, 1);
		}
		 Rhi->EndRenderPass();

		 Rhi->EndGpuCapture();
	}

}


