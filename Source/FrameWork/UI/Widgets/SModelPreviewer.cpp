#include "CommonHeader.h"
#include "SModelPreviewer.h"
#include "Common/Path/PathHelper.h"
#include "GpuApi/GpuRhi.h"
#include "Common/Util/Math.h"

#include <Widgets/SViewport.h>

namespace FW
{
	static Vector3f GetOrbitCameraPosition(float InDistance, float InYaw, float InPitch)
	{
		const FMatrix44f OrbitRotation = RotationMatrix(InYaw, InPitch);
		const Vector4f OrbitPosition = OrbitRotation.TransformFVector4(FVector4f(0.0f, 0.0f, -InDistance, 1.0f));
		return Vector3f(OrbitPosition.X, OrbitPosition.Y, OrbitPosition.Z);
	}

	void SModelPreviewer::Construct(const FArguments& InArgs)
	{
		MouseButtonDownHandler = InArgs._OnMouseButtonDown;
		ModelAsset = InArgs._InModel;
		Preview = MakeShared<PreviewViewPort>();

		ViewCamera.Position = { 0.0f, 0.0f, -1.0f };
		ViewCamera.Yaw = -(PI + PI / 8);
		ViewCamera.Pitch = 0.0f;
		ViewCamera.VerticalFov = FMath::DegreesToRadians(45.0f);
		ViewCamera.AspectRatio = 1.0f;
		ViewCamera.NearPlane = 0.01f;
		ViewCamera.FarPlane = 1000.0f;

		InitRenderResources();

		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
			[
				SNew(SViewport)
				.ViewportInterface(Preview)
			]
		];

		ResizeHandlerHandle = Preview->ResizeHandler.AddLambda([this](const Vector2f&) {
			Render();
		});

		Preview->MouseDownHandler.BindLambda([this](const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				bDragging = true;
				LastMousePos = (Vector2f)MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			}
			return FReply::Unhandled();
		});

		Preview->MouseUpHandler.BindLambda([this](const FGeometry&, const FPointerEvent& MouseEvent) {
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				bDragging = false;
				return FReply::Handled();
			}
			return FReply::Unhandled();
		});

		Preview->MouseMoveHandler.BindLambda([this](const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
			if (!bDragging || !MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
			{
				return FReply::Unhandled();
			}

			const Vector2f CurrentMousePos = (Vector2f)MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			const Vector2f MouseDelta = CurrentMousePos - LastMousePos;
			LastMousePos = CurrentMousePos;
			ViewCamera.Yaw += MouseDelta.x * 0.01f;
			ViewCamera.Pitch = FMath::Clamp(ViewCamera.Pitch - MouseDelta.y * 0.01f, -1.4f, 1.4f);
			UpdateCameraDistance(CameraDistance);
			Render();
			return FReply::Handled();
		});

		Preview->MouseWheelHandler.BindLambda([this](const FGeometry&, const FPointerEvent& MouseEvent) {
			const float ZoomFactor = FMath::Pow(0.9f, MouseEvent.GetWheelDelta());
			UpdateCameraDistance(CameraDistance * ZoomFactor);
			Render();
			return FReply::Handled();
		});

		Render();
	}

	void SModelPreviewer::InitRenderResources()
	{
		Vector3f Center;
		float Radius;
		ModelAsset->ComputeBounds(Center, Radius);

		ModelCenter = Center;
		ModelRadius = FMath::Max(Radius, 0.01f);
		CameraDistance = ModelRadius / FMath::Tan(ViewCamera.VerticalFov * 0.5f) * 1.3f;
		MinCameraDistance = FMath::Max(ModelRadius * 0.2f, 0.02f);
		MaxCameraDistance = FMath::Max(CameraDistance * 8.0f, MinCameraDistance + 1.0f);
		UpdateCameraDistance(CameraDistance);

		UniformBufferBuilder UbBuilder{ UniformBufferUsage::Persistant };
		UbBuilder.AddMatrix4x4f("Transform");
		UbBuilder.AddMatrix4x4f("NormalMatrix");
		UbBuilder.AddVector3f("LightDir");
		ModelUniformBuffer = UbBuilder.Build();

		BindGroupLayout = GpuBindGroupLayoutBuilder{0}
			.AddUniformBuffer("ModelUb", UbBuilder, BindingShaderStage::Vertex | BindingShaderStage::Pixel)
			.Build();

		BindGroup = GpuBindGroupBuilder{BindGroupLayout}
			.SetUniformBuffer("ModelUb", ModelUniformBuffer->GetGpuResource())
			.Build();

		FString ExtraDecl = BindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL);

		TRefCountPtr<GpuShader> Vs = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "ModelPreview.hlsl",
			.Type = ShaderType::Vertex,
			.EntryPoint = "MainVS",
			.ExtraDecl = ExtraDecl,
		});
		TRefCountPtr<GpuShader> Ps = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "ModelPreview.hlsl",
			.Type = ShaderType::Pixel,
			.EntryPoint = "MainPS",
			.ExtraDecl = ExtraDecl,
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
				{ .TargetFormat = GpuFormat::B8G8R8A8_UNORM }
			},
			.BindGroupLayouts = { BindGroupLayout },
			.VertexLayout = {
				{
					.ByteStride = sizeof(MeshVertex),
					.Attributes = {
						{
							.Location = 0,
							.SemanticName = "POSITION",
							.Format = GpuFormat::R32G32B32_FLOAT,
							.ByteOffset = offsetof(MeshVertex, Position),
						},
						{
							.Location = 1,
							.SemanticName = "NORMAL",
							.Format = GpuFormat::R32G32B32_FLOAT,
							.ByteOffset = offsetof(MeshVertex, Normal),
						},
						{
							.Location = 2,
							.SemanticName = "TEXCOORD",
							.Format = GpuFormat::R32G32_FLOAT,
							.ByteOffset = offsetof(MeshVertex, UVs[0]),
						},
					}
				}
			},
			.RasterizerState = {
				.FillMode = RasterizerFillMode::Solid,
				.CullMode = RasterizerCullMode::Back,
			},
			.SampleCount = 4,
			.DepthStencilState = DepthStencilStateDesc {
				.DepthFormat = GpuFormat::D32_FLOAT,
			},
		};

		Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);
	}

	void SModelPreviewer::UpdateCameraDistance(float InDistance)
	{
		CameraDistance = FMath::Clamp(InDistance, MinCameraDistance, MaxCameraDistance);
		ViewCamera.Position = GetOrbitCameraPosition(CameraDistance, ViewCamera.Yaw, ViewCamera.Pitch);
		ViewCamera.NearPlane = FMath::Max(0.01f, FMath::Min(ModelRadius * 0.05f, CameraDistance * 0.5f));
		ViewCamera.FarPlane = CameraDistance + ModelRadius * 4.0f;
	}

	void SModelPreviewer::ResizeRenderTargetIfNeeded()
	{
		const FIntPoint ViewportSize = Preview->GetSize();
		if (RenderTarget.IsValid()
			&& RenderTarget->GetWidth() == static_cast<uint32>(ViewportSize.X)
			&& RenderTarget->GetHeight() == static_cast<uint32>(ViewportSize.Y))
		{
			return;
		}

		GpuTextureDesc Desc{
			.Width = static_cast<uint32>(ViewportSize.X),
			.Height = static_cast<uint32>(ViewportSize.Y),
			.Format = GpuFormat::B8G8R8A8_UNORM,
			.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared,
			.ClearValues = Vector4f(0.08f, 0.08f, 0.08f, 1.0f),
		};
		RenderTarget = GGpuRhi->CreateTexture(Desc);

		GpuTextureDesc MsaaDesc = Desc;
		MsaaDesc.Usage = GpuTextureUsage::RenderTarget;
		MsaaDesc.SampleCount = 4;
		MsaaRenderTarget = GGpuRhi->CreateTexture(MsaaDesc);

		GpuTextureDesc DepthDesc{
			.Width = static_cast<uint32>(ViewportSize.X),
			.Height = static_cast<uint32>(ViewportSize.Y),
			.Format = GpuFormat::D32_FLOAT,
			.Usage = GpuTextureUsage::DepthStencil,
			.SampleCount = 4,
		};
		DepthTarget = GGpuRhi->CreateTexture(DepthDesc);

		ViewCamera.AspectRatio = (float)RenderTarget->GetWidth() / (float)RenderTarget->GetHeight();
	}

	void SModelPreviewer::Render()
	{
		ResizeRenderTargetIfNeeded();

		const FMatrix44f ModelMatrix = FTranslationMatrix44f(-ModelCenter);
		const FMatrix44f Transform = ModelMatrix * ViewCamera.GetViewProjectionMatrix();
		const FMatrix44f NormalMatrix = ModelMatrix.Inverse().GetTransposed();
		ModelUniformBuffer->GetMember<FMatrix44f>("Transform") = Transform;
		ModelUniformBuffer->GetMember<FMatrix44f>("NormalMatrix") = NormalMatrix;
		ModelUniformBuffer->GetMember<Vector3f>("LightDir") = FVector3f(ViewCamera.Position).GetSafeNormal();

		GpuRenderPassDesc PassDesc;
		PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{
			MsaaRenderTarget->GetDefaultView(),
			RenderTargetLoadAction::Clear,
			RenderTargetStoreAction::DontCare,
			Vector4f(0.08f, 0.08f, 0.08f, 1.0f),
			RenderTarget->GetDefaultView()
		});
		PassDesc.DepthStencilTarget = GpuDepthStencilTargetInfo{
			DepthTarget->GetDefaultView(),
			RenderTargetLoadAction::Clear,
			RenderTargetStoreAction::DontCare,
			1.0f
		};

		auto CmdRecorder = GGpuRhi->BeginRecording("ModelPreview");
		{
			auto PassRecorder = CmdRecorder->BeginRenderPass(PassDesc, "ModelPreview");
			{
				PassRecorder->SetRenderPipelineState(Pipeline);
				PassRecorder->SetBindGroups({ BindGroup });
				for (const MeshBuffers& Buffers : ModelAsset->GetGpuMeshes())
				{
					PassRecorder->SetVertexBuffer(0, Buffers.VertexBuffer);
					PassRecorder->SetIndexBuffer(Buffers.IndexBuffer);
					PassRecorder->DrawIndexed(0, Buffers.IndexCount);
				}
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });

		Preview->SetViewPortRenderTexture(RenderTarget);
	}

	FReply SModelPreviewer::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseButtonDownHandler.IsBound())
		{
			return MouseButtonDownHandler.Execute(MyGeometry, MouseEvent);
		}
		return FReply::Handled();
	}

	void SModelPreviewer::OpenModelPreviewer(AssetPtr<Model> InModel, const FPointerEventHandler& InOnMouseButtonDown, TSharedPtr<SWindow> InParentWindow)
	{
		if (TSharedPtr<SModelPreviewer> ExistingPreviewer = Instance.Pin())
		{
			if (TSharedPtr<SWindow> ExistingWindow = FSlateApplication::Get().FindWidgetWindow(ExistingPreviewer.ToSharedRef()))
			{
				ExistingWindow->RequestDestroyWindow();
			}
			Instance.Reset();
		}

		TSharedRef<SWindow> PreviewWindow = SNew(SWindow)
			.Title(LOCALIZATION("ModelPreview"))
			.ClientSize(FVector2D(512, 512))
			.AutoCenter(EAutoCenter::PreferredWorkArea)
			.SupportsMinimize(true)
			.SupportsMaximize(true);

		TSharedRef<SModelPreviewer> Previewer = SNew(SModelPreviewer)
			.InModel(InModel)
			.OnMouseButtonDown(InOnMouseButtonDown);

		PreviewWindow->SetContent(Previewer);

		if (InParentWindow.IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(PreviewWindow, InParentWindow.ToSharedRef());
		}
		else
		{
			FSlateApplication::Get().AddWindow(PreviewWindow);
		}

		Instance = Previewer;
	}
}
