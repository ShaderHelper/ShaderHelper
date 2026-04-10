#include "CommonHeader.h"
#include "TestCubeUnit.h"
#include "Common/Path/PathHelper.h"

using namespace FW;

namespace UNITTEST_GPUAPI
{
	TestCubeUnit::TestCubeUnit()
		: TestUnit(TEXT("TestCube"))
	{
		ViewCamera.Position = { 0.0f, 0.0f, -2.75f };
		ViewCamera.VerticalFov = 2.0f * FMath::Atan(1.0f / 1.4f);
		ViewCamera.AspectRatio = 1.0f;
		ViewCamera.NearPlane = 0.1f;
		ViewCamera.FarPlane = 100.0f;
	}

	void TestCubeUnit::Init()
	{
		UniformBufferBuilder CubeUbBuilder{ UniformBufferUsage::Persistant };
		CubeUbBuilder.AddMatrix4x4f("Transform");
		UniformBuffer = CubeUbBuilder.Build();

		BindGroupLayout = GpuBindGroupLayoutBuilder{ 0 }
			.AddUniformBuffer("CubeUb", CubeUbBuilder, BindingShaderStage::Vertex)
			.Build();

		BindGroup = GpuBindGroupBuilder{ BindGroupLayout }
			.SetUniformBuffer("CubeUb", UniformBuffer->GetGpuResource())
			.Build();

		PrimitiveBuffers = UploadMesh(CreateCube());

		TRefCountPtr<GpuShader> Vs = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "Test/TestCube.hlsl",
			.Type = ShaderType::VertexShader,
			.EntryPoint = "MainVS",
			.ExtraDecl = BindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL),
		});

		TRefCountPtr<GpuShader> Ps = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "Test/TestCube.hlsl",
			.Type = ShaderType::PixelShader,
			.EntryPoint = "MainPS",
			.ExtraDecl = BindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL),
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
			},
			.BindGroupLayouts = { BindGroupLayout },
			.VertexLayout = {
				{
					.ByteStride = sizeof(MeshVertex),
					.Attributes = {
						{
							.Location = 0,
							.SemanticName = TEXT("POSITION"),
							.Format = GpuFormat::R32G32B32_FLOAT,
							.ByteOffset = offsetof(MeshVertex, Position),
						},
						{
							.Location = 1,
							.SemanticName = TEXT("TEXCOORD"),
							.Format = GpuFormat::R32G32_FLOAT,
							.ByteOffset = offsetof(MeshVertex, UV),
						},
					}
				}
			},
			.RasterizerState = {
				.FillMode = RasterizerFillMode::Solid,
				.CullMode = RasterizerCullMode::Back,
			},
		};

		Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

		Viewport->MouseDownHandler.BindLambda([this](const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				bDragging = true;
				LastMousePos = (Vector2f)MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
				return FReply::Handled();
			}
			return FReply::Unhandled();
		});

		Viewport->MouseUpHandler.BindLambda([this](const FGeometry&, const FPointerEvent& MouseEvent) {
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				bDragging = false;
				return FReply::Handled();
			}
			return FReply::Unhandled();
		});

		Viewport->MouseMoveHandler.BindLambda([this](const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
			if (!bDragging || !MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
			{
				return FReply::Unhandled();
			}

			const Vector2f CurrentMousePos = (Vector2f)MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			const Vector2f MouseDelta = CurrentMousePos - LastMousePos;
			LastMousePos = CurrentMousePos;
			ModelYaw -= MouseDelta.x * 0.01f;
			ModelPitch = FMath::Clamp(ModelPitch + MouseDelta.y * 0.01f, -1.4f, 1.4f);
			return FReply::Handled();
		});
	}

	void TestCubeUnit::Render()
	{
		GpuTexture* RenderTarget = Viewport->GetRenderTarget();
		ViewCamera.AspectRatio = (float)RenderTarget->GetWidth() / (float)RenderTarget->GetHeight();

		const FMatrix44f ModelMatrix = RotationMatrix(ModelYaw, ModelPitch);
		const FMatrix44f Transform = ModelMatrix * ViewCamera.GetViewProjectionMatrix();
		UniformBuffer->GetMember<FMatrix44f>("Transform") = Transform;

		GGpuRhi->BeginGpuCapture("TestCube");

		GpuRenderPassDesc PassDesc;
		PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ RenderTarget->GetDefaultView(), RenderTargetLoadAction::Clear, RenderTargetStoreAction::Store });

		auto CmdRecorder = GGpuRhi->BeginRecording();
		{
			auto PassRecorder = CmdRecorder->BeginRenderPass(PassDesc, TEXT("TestCube"));
			{
				PassRecorder->SetRenderPipelineState(Pipeline);
				PassRecorder->SetBindGroups({ BindGroup });
				PassRecorder->SetVertexBuffer(0, PrimitiveBuffers.VertexBuffer);
				PassRecorder->SetIndexBuffer(PrimitiveBuffers.IndexBuffer);
				PassRecorder->DrawIndexed(0, PrimitiveBuffers.IndexCount);
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });

		GGpuRhi->EndGpuCapture();
	}
}
