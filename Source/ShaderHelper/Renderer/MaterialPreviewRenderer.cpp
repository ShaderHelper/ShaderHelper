#include "CommonHeader.h"
#include "MaterialPreviewRenderer.h"
#include "GpuApi/GpuRhi.h"
#include "GpuApi/GpuBindGroup.h"
#include "Renderer/MaterialRenderCommon.h"
#include "RenderResource/Camera.h"
#include "RenderResource/UniformBuffer.h"
#include "Common/Util/Math.h"
#include "ProjectManager/ShProjectManager.h"

using namespace FW;

namespace SH
{
	namespace
	{
		constexpr uint32 PreviewSampleCount = 4;
		constexpr GpuFormat PreviewColorFormat = GpuFormat::B8G8R8A8_UNORM;
		constexpr GpuFormat PreviewDepthFormat = GpuFormat::D32_FLOAT;
		const Vector4f PreviewClearColor(0.08f, 0.08f, 0.08f, 1.0f);
	}

	MaterialPreviewRenderer::MaterialPreviewRenderer(Material* InMaterial)
		: MaterialAsset(InMaterial)
	{
		check(MaterialAsset);
	}

	void MaterialPreviewRenderer::ResetRenderContext()
	{
		Pipeline = nullptr;
		BindGroupLayouts.Empty();
		BindGroups.Empty();
		PreviewUniformBuffers.Empty();
		PipelineErrorFunc = nullptr;
	}

	void MaterialPreviewRenderer::SetPreviewPrimitive(MaterialPreviewPrimitive InPreviewPrimitive)
	{
		if (PreviewPrimitive == InPreviewPrimitive)
		{
			return;
		}

		PreviewPrimitive = InPreviewPrimitive;
		bPreviewMeshInitialized = false;
	}

	void MaterialPreviewRenderer::SetOrbit(float InYaw, float InPitch)
	{
		if (OrbitYaw == InYaw && OrbitPitch == InPitch)
		{
			return;
		}

		OrbitYaw = InYaw;
		OrbitPitch = InPitch;
	}

	void MaterialPreviewRenderer::SetCameraDistance(float InDistance)
	{
		CameraDistance = FMath::Clamp(InDistance, 1.0f, 10.0f);
	}

	bool MaterialPreviewRenderer::Render(uint32 InWidth, uint32 InHeight)
	{
		ResizeRenderTargetsIfNeeded(InWidth, InHeight);
		if (!EnsurePipeline())
		{
			return false;
		}

		if (!EnsurePreviewMesh())
		{
			return false;
		}

		UpdatePreviewTransform(InWidth, InHeight);

		GpuRenderPassDesc PassDesc;
		PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{
			MsaaRenderTarget->GetDefaultView(),
			RenderTargetLoadAction::Clear,
			RenderTargetStoreAction::DontCare,
			PreviewClearColor,
			RenderTarget->GetDefaultView()
		});
		PassDesc.DepthStencilTarget = GpuDepthStencilTargetInfo{
			DepthTarget->GetDefaultView(),
			RenderTargetLoadAction::Clear,
			RenderTargetStoreAction::DontCare,
			1.0f
		};

		auto CmdRecorder = GGpuRhi->BeginRecording("MaterialPreview");
		{
			auto PassRecorder = CmdRecorder->BeginRenderPass(PassDesc, "MaterialPreview");
			{
				TArray<GpuBindGroup*> BindGroupArray;
				for (auto& [_, BindGroup] : BindGroups)
				{
					BindGroupArray.Add(BindGroup.GetReference());
				}

				PassRecorder->SetRenderPipelineState(Pipeline);
				PassRecorder->SetBindGroups(BindGroupArray);
				PassRecorder->SetVertexBuffer(0, PreviewMeshBuffers.VertexBuffer);
				PassRecorder->SetIndexBuffer(PreviewMeshBuffers.IndexBuffer);
				PassRecorder->DrawIndexed(0, PreviewMeshBuffers.IndexCount);
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });
		return true;
	}

	void MaterialPreviewRenderer::RenderErrorColor(uint32 InWidth, uint32 InHeight)
	{
		ResizeRenderTargetsIfNeeded(InWidth, InHeight);
		if (!EnsureErrorPipeline())
		{
			return;
		}
		if (!EnsurePreviewMesh())
		{
			return;
		}

		Camera ViewCamera;
		ViewCamera.VerticalFov = FMath::DegreesToRadians(45.0f);
		ViewCamera.AspectRatio = (float)InWidth / (float)InHeight;
		ViewCamera.NearPlane = 0.1f;
		ViewCamera.FarPlane = 10.0f;
		ViewCamera.Yaw = OrbitYaw;
		ViewCamera.Pitch = OrbitPitch;

		const FMatrix44f OrbitRotation = RotationMatrix(ViewCamera.Yaw, ViewCamera.Pitch);
		const Vector4f OrbitPos = OrbitRotation.TransformFVector4(FVector4f(0.0f, 0.0f, -CameraDistance, 1.0f));
		ViewCamera.Position = Vector3f(OrbitPos.X, OrbitPos.Y, OrbitPos.Z);
		const FMatrix44f Transform = ViewCamera.GetViewProjectionMatrix();

		GpuRenderPassDesc PassDesc;
		PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{
			MsaaRenderTarget->GetDefaultView(),
			RenderTargetLoadAction::Clear,
			RenderTargetStoreAction::DontCare,
			PreviewClearColor,
			RenderTarget->GetDefaultView()
		});
		PassDesc.DepthStencilTarget = GpuDepthStencilTargetInfo{
			DepthTarget->GetDefaultView(),
			RenderTargetLoadAction::Clear,
			RenderTargetStoreAction::DontCare,
			1.0f
		};

		auto CmdRecorder = GGpuRhi->BeginRecording("MaterialPreviewError");
		{
			auto PassRecorder = CmdRecorder->BeginRenderPass(PassDesc, "MaterialPreviewError");
			{
				DrawMaterialErrorMesh(PassRecorder, ErrorResources, PreviewMeshBuffers, Transform);
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });
	}

	FText MaterialPreviewRenderer::GetErrorReason() const
	{
		return PipelineErrorFunc ? PipelineErrorFunc() : FText::GetEmpty();
	}

	TRefCountPtr<GpuTexture> MaterialPreviewRenderer::RenderThumbnail(const Material* InMaterial, uint32 InSize, MaterialPreviewPrimitive InPreviewPrimitive)
	{
		MaterialPreviewRenderer Renderer(const_cast<Material*>(InMaterial));
		Renderer.SetPreviewPrimitive(InPreviewPrimitive);
		Renderer.SetOrbit(0.0f, 0.0f);
		if (!Renderer.Render(InSize, InSize))
		{
			Renderer.RenderErrorColor(InSize, InSize);
		}
		return Renderer.RenderTarget;
	}

	bool MaterialPreviewRenderer::EnsurePipeline()
	{
		if (Pipeline.IsValid())
		{
			return true;
		}

		if (BindGroupLayouts.IsEmpty())
		{
			BuildBindGroupFromMaterial();
		}

		MaterialPipelineBuildOptions Options;
		Options.ColorFormats.Add(PreviewColorFormat);
		Options.DepthFormat = PreviewDepthFormat;
		Options.SampleCount = PreviewSampleCount;
		for (auto& [_, BindGroupLayout] : BindGroupLayouts)
		{
			Options.BindGroupLayouts.Add(BindGroupLayout.GetReference());
		}

		MaterialPipelineBuildResult Result;
		if (!BuildMaterialPipeline(*MaterialAsset, Options, Result))
		{
			FText ErrorText = Result.Error;
			PipelineErrorFunc = [ErrorText]{ return ErrorText; };
			return false;
		}

		PipelineErrorFunc = nullptr;
		Pipeline = MoveTemp(Result.Pipeline);
		return Pipeline.IsValid();
	}

	bool MaterialPreviewRenderer::EnsureErrorPipeline()
	{
		TArray<GpuFormat> ColorFormats;
		ColorFormats.Add(PreviewColorFormat);
		return EnsureMaterialErrorRenderResources(ErrorResources, ColorFormats, PreviewDepthFormat, PreviewSampleCount);
	}

	void MaterialPreviewRenderer::BuildBindGroupFromMaterial()
	{
		BuildMaterialBindGroups(*MaterialAsset, BindGroupLayouts, BindGroups, PreviewUniformBuffers);
	}

	bool MaterialPreviewRenderer::EnsurePreviewMesh()
	{
		if (bPreviewMeshInitialized)
		{
			return true;
		}

		if (PreviewPrimitive == MaterialPreviewPrimitive::Sphere)
		{
			PreviewMeshBuffers = UploadMesh(SphereMesh);
		}
		else if(PreviewPrimitive == MaterialPreviewPrimitive::Cube)
		{
			PreviewMeshBuffers = UploadMesh(CubeMesh);
		}
		else
		{
			PreviewMeshBuffers = UploadMesh(QuadMesh);
		}
		bPreviewMeshInitialized = true;
		return PreviewMeshBuffers.VertexBuffer.IsValid() && PreviewMeshBuffers.IndexBuffer.IsValid();
	}

	void MaterialPreviewRenderer::ResizeRenderTargetsIfNeeded(uint32 InWidth, uint32 InHeight)
	{
		if (RenderTarget.IsValid() && RenderTarget->GetWidth() == InWidth && RenderTarget->GetHeight() == InHeight)
		{
			return;
		}

		GpuTextureDesc TargetDesc{
			.Width = InWidth,
			.Height = InHeight,
			.Format = PreviewColorFormat,
			.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared,
			.ClearValues = PreviewClearColor,
		};
		RenderTarget = GGpuRhi->CreateTexture(TargetDesc);

		GpuTextureDesc MsaaDesc = TargetDesc;
		MsaaDesc.Usage = GpuTextureUsage::RenderTarget;
		MsaaDesc.SampleCount = PreviewSampleCount;
		MsaaRenderTarget = GGpuRhi->CreateTexture(MsaaDesc);

		GpuTextureDesc DepthDesc{
			.Width = InWidth,
			.Height = InHeight,
			.Format = PreviewDepthFormat,
			.Usage = GpuTextureUsage::DepthStencil,
			.SampleCount = PreviewSampleCount,
		};
		DepthTarget = GGpuRhi->CreateTexture(DepthDesc);
	}

	void MaterialPreviewRenderer::UpdatePreviewTransform(uint32 InWidth, uint32 InHeight)
	{
		Camera ViewCamera;
		ViewCamera.VerticalFov = FMath::DegreesToRadians(45.0f);
		ViewCamera.AspectRatio = (float)InWidth / (float)InHeight;
		ViewCamera.NearPlane = 0.1f;
		ViewCamera.FarPlane = 10.0f;
		ViewCamera.Yaw = OrbitYaw;
		ViewCamera.Pitch = OrbitPitch;

		const FMatrix44f OrbitRotation = RotationMatrix(ViewCamera.Yaw, ViewCamera.Pitch);
		const Vector4f OrbitPos = OrbitRotation.TransformFVector4(FVector4f(0.0f, 0.0f, -CameraDistance, 1.0f));
		ViewCamera.Position = Vector3f(OrbitPos.X, OrbitPos.Y, OrbitPos.Z);

		MaterialUniformBufferUpdateOptions Options = MakeMaterialUniformOptions(
			FMatrix44f::Identity,
			ViewCamera.GetViewMatrix(),
			ViewCamera.GetProjectionMatrix(),
			Vector2f((float)InWidth, (float)InHeight),
			Vector2f(0, 0),
			ViewCamera.Position,
			GetCameraForwardDir(ViewCamera),
			TSingleton<ShProjectManager>::Get().GetProject()->TimelineCurTime);
		UpdateMaterialUniformBuffers(*MaterialAsset, PreviewUniformBuffers, Options);
	}
}
