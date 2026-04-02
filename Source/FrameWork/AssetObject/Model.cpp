#include "CommonHeader.h"
#include "Model.h"
#include "Common/Util/Reflection.h"
#include "Common/Path/PathHelper.h"
#include "GpuApi/GpuRhi.h"
#include "RenderResource/Camera.h"
#include "RenderResource/UniformBuffer.h"

namespace FW
{
	static uint32 CountVertices(const TArray<MeshData>& InSubMeshes)
	{
		uint32 VertexCount = 0;
		for (const MeshData& SubMesh : InSubMeshes)
		{
			VertexCount += SubMesh.Vertices.Num();
		}
		return VertexCount;
	}

	static uint32 CountFaces(const TArray<MeshData>& InSubMeshes)
	{
		uint32 FaceCount = 0;
		for (const MeshData& SubMesh : InSubMeshes)
		{
			FaceCount += SubMesh.Indices.Num() / 3;
		}
		return FaceCount;
	}

	REFLECTION_REGISTER(AddClass<Model>("Model")
								.BaseClass<AssetObject>()
								.Data<&Model::VertexCount, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("VertexCount"))
								.Data<&Model::TriangleFaceCount, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("TriangleFaceCount"))
	)

	Model::Model()
		: VertexCount(0)
		, TriangleFaceCount(0)
	{
	}

	Model::Model(TArray<MeshData> InSubMeshes)
		: SubMeshes(MoveTemp(InSubMeshes))
	{
		VertexCount = CountVertices(SubMeshes);
		TriangleFaceCount = CountFaces(SubMeshes);
		InitGpuData();
	}

	void Model::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);
		Ar << SubMeshes;

		VertexCount = CountVertices(SubMeshes);
		TriangleFaceCount = CountFaces(SubMeshes);
	}

	void Model::PostLoad()
	{
		AssetObject::PostLoad();
		InitGpuData();
	}

	void Model::InitGpuData()
	{
		GpuMeshes.Empty();
		for (const MeshData& SM : SubMeshes)
		{
			GpuMeshes.Add(UploadMesh(SM));
		}
	}

	FString Model::FileExtension() const
	{
		return "model";
	}

	void Model::ComputeBounds(Vector3f& OutCenter, float& OutRadius) const
	{
		FVector3f MinBound(FLT_MAX, FLT_MAX, FLT_MAX);
		FVector3f MaxBound(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		for (const MeshData& SM : SubMeshes)
		{
			for (const MeshVertex& V : SM.Vertices)
			{
				MinBound.X = FMath::Min(MinBound.X, V.Position.X);
				MinBound.Y = FMath::Min(MinBound.Y, V.Position.Y);
				MinBound.Z = FMath::Min(MinBound.Z, V.Position.Z);
				MaxBound.X = FMath::Max(MaxBound.X, V.Position.X);
				MaxBound.Y = FMath::Max(MaxBound.Y, V.Position.Y);
				MaxBound.Z = FMath::Max(MaxBound.Z, V.Position.Z);
			}
		}
		OutCenter = (MinBound + MaxBound) * 0.5f;
		OutRadius = (MaxBound - MinBound).Size() * 0.5f;
		if (OutRadius < SMALL_NUMBER)
		{
			OutRadius = 1.0f;
		}
	}

	TRefCountPtr<GpuTexture> Model::GenerateThumbnail() const
	{
		if (GpuMeshes.Num() == 0)
		{
			return nullptr;
		}

		const uint32 ThumbnailSize = 128;
		const GpuFormat ThumbnailFormat = GpuFormat::B8G8R8A8_UNORM;
		const GpuFormat DepthFormat = GpuFormat::D32_FLOAT;
		const uint32 SampleCount = 4;

		GpuTextureDesc RtDesc{
			.Width = ThumbnailSize,
			.Height = ThumbnailSize,
			.Format = ThumbnailFormat,
			.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared,
			.ClearValues = Vector4f(0.08f, 0.08f, 0.08f, 1.0f),
		};
		TRefCountPtr<GpuTexture> Thumbnail = GGpuRhi->CreateTexture(MoveTemp(RtDesc));

		TRefCountPtr<GpuTexture> MsaaThumbnail;
		GpuTextureDesc MsaaRtDesc{
			.Width = ThumbnailSize,
			.Height = ThumbnailSize,
			.Format = ThumbnailFormat,
			.Usage = GpuTextureUsage::RenderTarget,
			.ClearValues = Vector4f(0.08f, 0.08f, 0.08f, 1.0f),
			.SampleCount = SampleCount,
		};
		MsaaThumbnail = GGpuRhi->CreateTexture(MoveTemp(MsaaRtDesc));

		GpuTextureDesc DepthDesc{
			.Width = ThumbnailSize,
			.Height = ThumbnailSize,
			.Format = DepthFormat,
			.Usage = GpuTextureUsage::DepthStencil,
			.SampleCount = SampleCount,
		};
		TRefCountPtr<GpuTexture> DepthTarget = GGpuRhi->CreateTexture(MoveTemp(DepthDesc));

		Vector3f Center;
		float Radius;
		ComputeBounds(Center, Radius);

		Camera ViewCamera;
		ViewCamera.VerticalFov = FMath::DegreesToRadians(45.0f);
		ViewCamera.AspectRatio = 1.0f;
		ViewCamera.NearPlane = FMath::Max(0.01f, Radius * 0.05f);
		const float Distance = Radius / FMath::Tan(ViewCamera.VerticalFov * 0.5f) * 1.6f;
		ViewCamera.FarPlane = Distance + Radius * 4.0f;
		ViewCamera.Position = Vector3f(0.0f, 0.0f, -Distance);

		const FMatrix44f ViewProj = ViewCamera.GetViewProjectionMatrix();
		const FMatrix44f ModelMatrix = RotationMatrix(PI + PI / 8, 0);
		const FMatrix44f Transform = ModelMatrix * ViewProj;
		const FMatrix44f NormalMatrix = ModelMatrix.Inverse().GetTransposed();

		UniformBufferBuilder UbBuilder{UniformBufferUsage::Temp};
		UbBuilder.AddMatrix4x4f("Transform");
		UbBuilder.AddMatrix4x4f("NormalMatrix");
		UbBuilder.AddVector3f("LightDir");
		TUniquePtr<UniformBuffer> Ub = UbBuilder.Build();
		Ub->GetMember<FMatrix44f>("Transform") = Transform;
		Ub->GetMember<FMatrix44f>("NormalMatrix") = NormalMatrix;
		Ub->GetMember<Vector3f>("LightDir") = Vector3f(0.0f, 0.0f, -1.0f);

		TRefCountPtr<GpuBindGroupLayout> BindGroupLayout = GpuBindGroupLayoutBuilder{0}
			.AddUniformBuffer("ModelUb", UbBuilder, BindingShaderStage::Vertex | BindingShaderStage::Pixel)
			.Build();

		TRefCountPtr<GpuBindGroup> BindGroup = GpuBindGroupBuilder{BindGroupLayout}
			.SetUniformBuffer("ModelUb", Ub->GetGpuResource())
			.Build();

		FString ExtraDecl = BindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL);

		TRefCountPtr<GpuShader> Vs = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "ModelPreview.hlsl",
			.Type = ShaderType::VertexShader,
			.EntryPoint = "MainVS",
			.ExtraDecl = ExtraDecl,
		});
		TRefCountPtr<GpuShader> Ps = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "ModelPreview.hlsl",
			.Type = ShaderType::PixelShader,
			.EntryPoint = "MainPS",
			.ExtraDecl = ExtraDecl,
		});

		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(Vs, ErrorInfo, WarnInfo);
		if (!ErrorInfo.IsEmpty()) { return nullptr; }
		GGpuRhi->CompileShader(Ps, ErrorInfo, WarnInfo);
		if (!ErrorInfo.IsEmpty()) { return nullptr; }

		GpuRenderPipelineStateDesc PipelineDesc{
			.Vs = Vs,
			.Ps = Ps,
			.Targets = {
				{.TargetFormat = ThumbnailFormat}
			},
			.BindGroupLayout0 = BindGroupLayout,
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
							.ByteOffset = offsetof(MeshVertex, UV),
						},
					}
				}
			},
			.RasterizerState = {
				.FillMode = RasterizerFillMode::Solid,
				.CullMode = RasterizerCullMode::Back,
			},
			.SampleCount = SampleCount,
			.DepthStencilState = DepthStencilStateDesc{
				.DepthFormat = DepthFormat,
			},
		};
		TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

		GpuRenderPassDesc PassDesc;
		PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{
			MsaaThumbnail->GetDefaultView(),
			RenderTargetLoadAction::Clear,
			RenderTargetStoreAction::DontCare,
			Vector4f(0.08f, 0.08f, 0.08f, 1.0f),
			Thumbnail->GetDefaultView()
		});
		PassDesc.DepthStencilTarget = GpuDepthStencilTargetInfo{
			DepthTarget->GetDefaultView(),
			RenderTargetLoadAction::Clear,
			RenderTargetStoreAction::DontCare,
			1.0f
		};

		auto CmdRecorder = GGpuRhi->BeginRecording("ModelThumbnail");
		{
			auto PassRecorder = CmdRecorder->BeginRenderPass(PassDesc, "ModelThumbnail");
			{
				PassRecorder->SetRenderPipelineState(Pipeline);
				PassRecorder->SetBindGroups(BindGroup, nullptr, nullptr, nullptr);
				for (const MeshBuffers& Buffers : GpuMeshes)
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

		return Thumbnail;
	}
}
