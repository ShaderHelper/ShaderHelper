#include "CommonHeader.h"
#include "MaterialPreviewRenderer.h"
#include "GpuApi/GpuRhi.h"
#include "GpuApi/GpuBindGroupLayout.h"
#include "GpuApi/GpuBindGroup.h"
#include "Renderer/MaterialRenderResources.h"
#include "RenderResource/Camera.h"
#include "RenderResource/UniformBuffer.h"
#include "Common/Util/Math.h"

#include <stdexcept>

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
		LinkageErrorFunc = nullptr;
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
		TArray<FText> Errors;
		if (!MaterialAsset->VertexShaderAsset)
			Errors.Add(LOCALIZATION("VsNotSpecified"));
		else if (!MaterialAsset->VertexShaderAsset->GetCompiledShader(ShaderType::Vertex))
			Errors.Add(LOCALIZATION("VsInvalidShader"));

		if (!MaterialAsset->PixelShaderAsset)
			Errors.Add(LOCALIZATION("PsNotSpecified"));
		else if (!MaterialAsset->PixelShaderAsset->GetCompiledShader(ShaderType::Pixel))
			Errors.Add(LOCALIZATION("PsInvalidShader"));

		if (LinkageErrorFunc)
			Errors.Add(LinkageErrorFunc());

		TArray<FString> ErrorStrings;
		for (const auto& E : Errors) ErrorStrings.Add(E.ToString());
		return FText::FromString(FString::Join(ErrorStrings, TEXT("\n")));
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

		if (!MaterialAsset->VertexShaderAsset || !MaterialAsset->PixelShaderAsset)
		{
			return false;
		}

		GpuShader* Vs = MaterialAsset->VertexShaderAsset->GetCompiledShader(ShaderType::Vertex);
		GpuShader* Ps = MaterialAsset->PixelShaderAsset->GetCompiledShader(ShaderType::Pixel);
		if (!Vs || !Ps)
		{
			return false;
		}

		if (Vs->GetShaderLanguage() != Ps->GetShaderLanguage())
		{
			LinkageErrorFunc = []{ return LOCALIZATION("ShaderLanguageMismatch"); };
			return false;
		}

		// Validate VS output / PS input linkage before creating pipeline
		{
			TArray<GpuShaderStageSemantic> VsOutputs = Vs->GetStageOutputSemantics();
			TArray<GpuShaderStageSemantic> PsInputs = Ps->GetStageInputSemantics();
			TArray<FString> MissingSemantics;
			for (const auto& PsInput : PsInputs)
			{
				if (!PsInput.bRead)
				{
					continue;
				}

				if(Vs->GetShaderLanguage() == GpuShaderLanguage::HLSL)
				{
					const GpuShaderStageSemantic* MatchingVsOutput = VsOutputs.FindByPredicate([&](const GpuShaderStageSemantic& VsOutput) {
						return VsOutput.SemanticName.Equals(PsInput.SemanticName, ESearchCase::IgnoreCase)
							&& VsOutput.SemanticIndex == PsInput.SemanticIndex;
					});
					if (!MatchingVsOutput || !MatchingVsOutput->bWritten)
					{
						FString SemanticStr = PsInput.SemanticIndex > 0
							? FString::Printf(TEXT("%s%d"), *PsInput.SemanticName, PsInput.SemanticIndex)
							: PsInput.SemanticName;
						MissingSemantics.Add(MoveTemp(SemanticStr));
					}
				}
				else
				{
					const GpuShaderStageSemantic* MatchingVsOutput = VsOutputs.FindByPredicate([&](const GpuShaderStageSemantic& VsOutput) {
						return VsOutput.Location == PsInput.Location;
					});
					if (!MatchingVsOutput || !MatchingVsOutput->bWritten)
					{
						FString Desc = FString::Printf(TEXT("(location %d) %s"), PsInput.Location, *PsInput.Name);
						MissingSemantics.Add(MoveTemp(Desc));
					}
					else if (!MatchingVsOutput->Type.IsEmpty() && !PsInput.Type.IsEmpty()
						&& !MatchingVsOutput->Type.Equals(PsInput.Type, ESearchCase::IgnoreCase))
					{
						MissingSemantics.Add(FString::Printf(TEXT("(location %d) %s (type mismatch: VS=%s, PS=%s)"),
							PsInput.Location, *PsInput.Name, *MatchingVsOutput->Type, *PsInput.Type));
					}
				}
			}
			if (MissingSemantics.Num() > 0)
			{
				FString Joined = FString::Join(MissingSemantics, TEXT(", "));
				LinkageErrorFunc = [Joined]{ return FText::Format(LOCALIZATION("SemanticLinkageError"), FText::FromString(Joined)); };
				return false;
			}

			LinkageErrorFunc = nullptr;
		}

		if (BindGroupLayouts.IsEmpty())
		{
			BuildBindGroupFromMaterial();
			if (LinkageErrorFunc)
			{
				return false;
			}
		}

		GpuVertexLayoutDesc VertexLayoutDesc = BuildMaterialMeshVertexLayout(*MaterialAsset);

		TArray<GpuBindGroupLayout*> BindGroupLayoutArray;
		for (auto& [_, BindGroupLayout] : BindGroupLayouts)
		{
			BindGroupLayoutArray.Add(BindGroupLayout.GetReference());
		}

		GpuRenderPipelineStateDesc PipelineDesc{
			.Vs = Vs,
			.Ps = Ps,
			.Targets = {
				{
					.TargetFormat = PreviewColorFormat,
					.BlendEnable = MaterialAsset->BlendEnable,
					.SrcFactor = MaterialAsset->SrcBlendFactor,
					.ColorOp = MaterialAsset->ColorBlendOp,
					.DestFactor = MaterialAsset->DestBlendFactor,
				}
			},
			.BindGroupLayouts = MoveTemp(BindGroupLayoutArray),
			.VertexLayout = { MoveTemp(VertexLayoutDesc) },
			.RasterizerState = {
				.FillMode = MaterialAsset->FillMode,
				.CullMode = MaterialAsset->CullMode,
			},
			.Primitive = MaterialAsset->Primitive,
			.SampleCount = PreviewSampleCount,
			.DepthStencilState = MaterialAsset->DepthTestEnable ?
				TOptional<DepthStencilStateDesc>(DepthStencilStateDesc{
					.DepthFormat = PreviewDepthFormat,
					.DepthCompare = MaterialAsset->DepthCompare,
				}) : TOptional<DepthStencilStateDesc>(),
		};

		try
		{
			Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);
		}
		catch (const std::runtime_error& e)
		{
			FString ErrorMsg = ANSI_TO_TCHAR(e.what());
			LinkageErrorFunc = [ErrorMsg]{ return FText::FromString(ErrorMsg); };
		}
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

		const FMatrix44f ModelMatrix = FMatrix44f::Identity;
		const FMatrix44f ViewMatrix = ViewCamera.GetViewMatrix();
		const FMatrix44f ProjMatrix = ViewCamera.GetProjectionMatrix();
		const FMatrix44f ViewProjMatrix = ViewMatrix * ProjMatrix;
		const FMatrix44f MVPMatrix = ModelMatrix * ViewProjMatrix;

		MaterialUniformBufferUpdateOptions Options;
		Options.ModelMatrix = ModelMatrix;
		Options.ViewMatrix = ViewMatrix;
		Options.ProjMatrix = ProjMatrix;
		Options.ViewProjMatrix = ViewProjMatrix;
		Options.MVPMatrix = MVPMatrix;
		UpdateMaterialUniformBuffers(*MaterialAsset, PreviewUniformBuffers, Options);
	}
}
