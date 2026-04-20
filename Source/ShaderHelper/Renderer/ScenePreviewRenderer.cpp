#include "CommonHeader.h"
#include "ScenePreviewRenderer.h"
#include "RenderResource/SceneGridShader.h"
#include "RenderResource/ScenePreviewShader.h"
#include "RenderResource/OutlineShader.h"
#include "RenderResource/GizmoShader.h"
#include "RenderResource/CameraWireframeShader.h"
#include "RenderResource/BillboardShader.h"
#include "RenderResource/RenderPass/BlitPass.h"
#include "AssetObject/Render/MeshSceneObject.h"
#include "AssetObject/Render/CameraSceneObject.h"
#include "AssetObject/Model.h"
#include "GpuApi/GpuRhi.h"
#include "GpuApi/GpuResourceHelper.h"
#include "RenderResource/Mesh.h"
#include "Common/Path/PathHelper.h"

#include <IImageWrapperModule.h>
#include <IImageWrapper.h>
#include <ImageWrapperHelper.h>
#include <Misc/FileHelper.h>

using namespace FW;

namespace SH
{
	ScenePreviewRenderer::ScenePreviewRenderer()
	{
	}

	void ScenePreviewRenderer::InitResources()
	{
		if (CameraIconTex.IsValid())
		{
			return;
		}

		FString IconPath = PathHelper::ResourceDir() / TEXT("CustomSlate/Camera.png");
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
		TArray<uint8> CompressedData;
		if (FFileHelper::LoadFileToArray(CompressedData, *IconPath))
		{
			if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(CompressedData.GetData(), CompressedData.Num()))
			{
				TArray<uint8> UnCompressedData;
				ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UnCompressedData);
				CameraIconTex = GGpuRhi->CreateTexture({
					.Width = (uint32)ImageWrapper->GetWidth(),
					.Height = (uint32)ImageWrapper->GetHeight(),
					.Format = GpuFormat::B8G8R8A8_UNORM,
					.Usage = GpuTextureUsage::ShaderResource,
					.InitialData = UnCompressedData,
				}, GpuResourceState::ShaderResourceRead);
				GGpuRhi->SetResourceName("CameraIconTex", CameraIconTex);
				CameraIconTexView = GGpuRhi->CreateTextureView({.Texture = CameraIconTex});
			}
		}
	}

	void ScenePreviewRenderer::RenderGrid(RenderGraph& Graph, GpuTextureView* OutputView,
		GpuTextureView* DepthView,
		const Camera& Camera, bool bClear, float GridSize, float GridSpacing)
	{
		SceneGridShader* GridShader = GetShader<SceneGridShader>();

		uint32 SampleCount = OutputView->GetTexture()->GetSampleCount();
		FMatrix44f VP = Camera.GetViewProjectionMatrix();
		TRefCountPtr<GpuBindGroup> BindGroup = GridShader->GetBindGroup(VP, Camera.Position, GridSize, GridSpacing);

		BindingContext Bindings;
		Bindings.SetPassBindGroup(BindGroup);
		Bindings.SetPassBindGroupLayout(GridShader->GetBindGroupLayout());

		// Grid ground plane (analytical grid in fragment shader)
		{
			GpuRenderPassDesc PassDesc;
			PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{OutputView, 
				bClear ? RenderTargetLoadAction::Clear : RenderTargetLoadAction::Load, 
				RenderTargetStoreAction::Store});
			PassDesc.DepthStencilTarget = GpuDepthStencilTargetInfo{DepthView, 
				bClear ? RenderTargetLoadAction::Clear : RenderTargetLoadAction::Load, 
				RenderTargetStoreAction::Store};

			GpuRenderPipelineStateDesc PipelineDesc{
				.Vs = GridShader->GetGridVS(),
				.Ps = GridShader->GetPS(),
				.Targets = {{
					.TargetFormat = OutputView->GetTexture()->GetFormat(),
					.BlendEnable = true,
					.SrcFactor = BlendFactor::SrcAlpha,
					.ColorOp = BlendOp::Add,
					.DestFactor = BlendFactor::InvSrcAlpha,
				}},
				.RasterizerState = {RasterizerFillMode::Solid, RasterizerCullMode::None},
				.Primitive = PrimitiveType::TriangleList,
				.SampleCount = SampleCount,
				.DepthStencilState = DepthStencilStateDesc{
					.DepthFormat = DepthView->GetTexture()->GetFormat(),
					.DepthWriteEnable = false,
					.DepthCompare = CompareMode::LessEqual,
				},
			};
			Bindings.ApplyBindGroupLayout(PipelineDesc);
			TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

			Graph.AddRenderPass("SceneGrid", MoveTemp(PassDesc), Bindings,
				[Pipeline](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
					Bindings.ApplyBindGroup(PassRecorder);
					PassRecorder->SetRenderPipelineState(Pipeline);
					PassRecorder->DrawPrimitive(0, 6, 0, 1);
				}
			);
		}


	}

	bool ScenePreviewRenderer::RenderMeshes(RenderGraph& Graph, GpuTextureView* OutputView,
		GpuTextureView* DepthView,
		const Camera& Camera, const TArray<ObjectPtr<SceneObject>>& SceneObjects, bool bClear)
	{
		ScenePreviewShader* MeshShader = GetShader<ScenePreviewShader>();
		uint32 SampleCount = OutputView->GetTexture()->GetSampleCount();
		FMatrix44f VP = Camera.GetViewProjectionMatrix();
		Vector3f LightDir = Vector3f(0.3f, -0.7f, 0.5f);
		{
			float Len = FMath::Sqrt(LightDir.X * LightDir.X + LightDir.Y * LightDir.Y + LightDir.Z * LightDir.Z);
			LightDir = LightDir * (1.0f / Len);
		}

		bool bFirstDraw = true;
		for (const auto& Obj : SceneObjects)
		{
			MeshSceneObject* MeshObj = dynamic_cast<MeshSceneObject*>(Obj.Get());
			if (!MeshObj || !MeshObj->ModelAsset)
			{
				continue;
			}

			Model* ModelAsset = MeshObj->ModelAsset.Get();
			if (!ModelAsset)
			{
				continue;
			}

			FMatrix44f WorldMat = MeshObj->GetWorldMatrix();
			TRefCountPtr<GpuBindGroup> BindGroup = MeshShader->GetBindGroup(VP, WorldMat, Camera.Position, LightDir);

			BindingContext Bindings;
			Bindings.SetPassBindGroup(BindGroup);
			Bindings.SetPassBindGroupLayout(MeshShader->GetBindGroupLayout());

			const TArray<MeshBuffers>& GpuMeshes = ModelAsset->GetGpuMeshes();
			for (int32 SubIdx = 0; SubIdx < GpuMeshes.Num(); SubIdx++)
			{
				const MeshBuffers& Buffers = GpuMeshes[SubIdx];
				if (!Buffers.VertexBuffer || !Buffers.IndexBuffer)
				{
					continue;
				}

				GpuRenderPassDesc PassDesc;
				PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{OutputView, 
					(bClear && bFirstDraw) ? RenderTargetLoadAction::Clear : RenderTargetLoadAction::Load, 
					RenderTargetStoreAction::Store});
				PassDesc.DepthStencilTarget = GpuDepthStencilTargetInfo{DepthView, 
					(bClear && bFirstDraw) ? RenderTargetLoadAction::Clear : RenderTargetLoadAction::Load, 
					RenderTargetStoreAction::Store};
				bFirstDraw = false;

				GpuRenderPipelineStateDesc PipelineDesc{
					.Vs = MeshShader->GetVertexShader(),
					.Ps = MeshShader->GetPixelShader(),
					.Targets = {{ .TargetFormat = OutputView->GetTexture()->GetFormat() }},
					.VertexLayout = {{
						.ByteStride = sizeof(MeshVertex),
						.Attributes = {
							{0, "POSITION", 0, GpuFormat::R32G32B32_FLOAT, offsetof(MeshVertex, Position)},
							{1, "NORMAL", 0, GpuFormat::R32G32B32_FLOAT, offsetof(MeshVertex, Normal)},
							{2, "TEXCOORD", 0, GpuFormat::R32G32_FLOAT, offsetof(MeshVertex, UVs)},
							{3, "COLOR", 0, GpuFormat::R32G32B32A32_FLOAT, offsetof(MeshVertex, Color)},
						},
					}},
					.RasterizerState = {RasterizerFillMode::Solid, RasterizerCullMode::Back},
					.SampleCount = SampleCount,
					.DepthStencilState = DepthStencilStateDesc{
						.DepthFormat = DepthView->GetTexture()->GetFormat(),
						.DepthWriteEnable = true,
						.DepthCompare = CompareMode::Less,
					},
				};
				Bindings.ApplyBindGroupLayout(PipelineDesc);
				TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

				Graph.AddRenderPass("SceneMesh", MoveTemp(PassDesc), Bindings,
					[Pipeline, VB = Buffers.VertexBuffer, IB = Buffers.IndexBuffer, IdxCount = Buffers.IndexCount]
					(GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
						Bindings.ApplyBindGroup(PassRecorder);
						PassRecorder->SetRenderPipelineState(Pipeline);
						PassRecorder->SetVertexBuffer(0, VB);
						PassRecorder->SetIndexBuffer(IB);
						PassRecorder->DrawIndexed(0, IdxCount);
					}
				);
			}
		}
		return !bFirstDraw;
	}

	void ScenePreviewRenderer::RenderOutline(RenderGraph& Graph, GpuTextureView* OutputView,
		GpuTextureView* MaskView,
		const Camera& Camera, SceneObject* SelectedObject)
	{
		MeshSceneObject* MeshObj = dynamic_cast<MeshSceneObject*>(SelectedObject);
		if (!MeshObj || !MeshObj->ModelAsset)
		{
			return;
		}

		Model* ModelAsset = MeshObj->ModelAsset.Get();
		if (!ModelAsset)
		{
			return;
		}

		OutlineShader* Shader = GetShader<OutlineShader>();
		FMatrix44f VP = Camera.GetViewProjectionMatrix();
		FMatrix44f WorldMat = MeshObj->GetWorldMatrix();

		// Pass 1: Render selected object silhouette into mask
		{
			TRefCountPtr<GpuBindGroup> MaskBindGroup = Shader->GetMaskBindGroup(VP, WorldMat);

			BindingContext Bindings;
			Bindings.SetPassBindGroup(MaskBindGroup);
			Bindings.SetPassBindGroupLayout(Shader->GetMaskBindGroupLayout());

			const TArray<MeshBuffers>& GpuMeshes = ModelAsset->GetGpuMeshes();
			for (int32 SubIdx = 0; SubIdx < GpuMeshes.Num(); SubIdx++)
			{
				const MeshBuffers& Buffers = GpuMeshes[SubIdx];
				if (!Buffers.VertexBuffer || !Buffers.IndexBuffer)
				{
					continue;
				}

				GpuRenderPassDesc PassDesc;
				PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{MaskView,
					SubIdx == 0 ? RenderTargetLoadAction::Clear : RenderTargetLoadAction::Load,
					RenderTargetStoreAction::Store});

				GpuRenderPipelineStateDesc PipelineDesc{
					.Vs = Shader->GetMaskVS(),
					.Ps = Shader->GetMaskPS(),
					.Targets = {{ .TargetFormat = MaskView->GetTexture()->GetFormat() }},
					.VertexLayout = {{
						.ByteStride = sizeof(MeshVertex),
						.Attributes = {
							{0, "POSITION", 0, GpuFormat::R32G32B32_FLOAT, offsetof(MeshVertex, Position)},
							{1, "NORMAL", 0, GpuFormat::R32G32B32_FLOAT, offsetof(MeshVertex, Normal)},
							{2, "TEXCOORD", 0, GpuFormat::R32G32_FLOAT, offsetof(MeshVertex, UVs)},
							{3, "COLOR", 0, GpuFormat::R32G32B32A32_FLOAT, offsetof(MeshVertex, Color)},
						},
					}},
					.RasterizerState = {RasterizerFillMode::Solid, RasterizerCullMode::Back},
				};
				Bindings.ApplyBindGroupLayout(PipelineDesc);
				TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

				Graph.AddRenderPass("OutlineMask", MoveTemp(PassDesc), Bindings,
					[Pipeline, VB = Buffers.VertexBuffer, IB = Buffers.IndexBuffer, IdxCount = Buffers.IndexCount]
					(GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
						Bindings.ApplyBindGroup(PassRecorder);
						PassRecorder->SetRenderPipelineState(Pipeline);
						PassRecorder->SetVertexBuffer(0, VB);
						PassRecorder->SetIndexBuffer(IB);
						PassRecorder->DrawIndexed(0, IdxCount);
					}
				);
			}
		}

		// Pass 2: Post-process edge detection
		{
			GpuTexture* MaskTex = MaskView->GetTexture();
			const float OutlineWidthInPixels = 2.0f;
			Vector2f TexelSize(OutlineWidthInPixels / MaskTex->GetWidth(), OutlineWidthInPixels / MaskTex->GetHeight());
			Vector4f OutlineColor(1.0f, 0.6f, 0.0f, 1.0f); // Orange

			TRefCountPtr<GpuBindGroup> PostBindGroup = Shader->GetPostBindGroup(MaskView, OutlineColor, TexelSize);

			BindingContext Bindings;
			Bindings.SetPassBindGroup(PostBindGroup);
			Bindings.SetPassBindGroupLayout(Shader->GetPostBindGroupLayout());

			GpuRenderPassDesc PassDesc;
			PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{OutputView, RenderTargetLoadAction::Load, RenderTargetStoreAction::Store});

			GpuRenderPipelineStateDesc PipelineDesc{
				.Vs = Shader->GetPostVS(),
				.Ps = Shader->GetPostPS(),
				.Targets = {{
					.TargetFormat = OutputView->GetTexture()->GetFormat(),
					.BlendEnable = true,
					.SrcFactor = BlendFactor::SrcAlpha,
					.ColorOp = BlendOp::Add,
					.DestFactor = BlendFactor::InvSrcAlpha,
				}},
				.RasterizerState = {RasterizerFillMode::Solid, RasterizerCullMode::None},
			};
			Bindings.ApplyBindGroupLayout(PipelineDesc);
			TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

			Graph.AddRenderPass("OutlinePost", MoveTemp(PassDesc), Bindings,
				[Pipeline](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
					Bindings.ApplyBindGroup(PassRecorder);
					PassRecorder->SetRenderPipelineState(Pipeline);
					PassRecorder->DrawPrimitive(0, 3, 0, 1);
				}
			);
		}
	}

	void ScenePreviewRenderer::RenderGizmo(RenderGraph& Graph, GpuTextureView* OutputView,
		const Camera& Camera, const Vector3f& GizmoPosition, int32 HighlightAxis,
		GizmoMode Mode, const FMatrix44f& Orientation)
	{
		GizmoShader* Shader = GetShader<GizmoShader>();

		FMatrix44f VP = Camera.GetViewProjectionMatrix();

		// Constant screen size: scale by distance from camera
		Vector3f Delta = GizmoPosition - Camera.Position;
		float Dist = FMath::Sqrt(Delta.X * Delta.X + Delta.Y * Delta.Y + Delta.Z * Delta.Z);
		float GizmoScale = FMath::Max(Dist * 0.15f, 0.01f);

		TRefCountPtr<GpuBindGroup> BindGroup = Shader->GetBindGroup(VP, GizmoPosition, GizmoScale, HighlightAxis, Orientation, Camera.Position,
			FVector2f((float)OutputView->GetTexture()->GetWidth(), (float)OutputView->GetTexture()->GetHeight()));

		BindingContext Bindings;
		Bindings.SetPassBindGroup(BindGroup);
		Bindings.SetPassBindGroupLayout(Shader->GetBindGroupLayout());

		GpuFormat TargetFormat = OutputView->GetTexture()->GetFormat();
		uint32 SampleCount = OutputView->GetTexture()->GetSampleCount();

		if (Mode == GizmoMode::Move)
		{
			// Shafts: TriangleList (screen-space quads), 3 axes × 6 verts = 18 verts
			{
				GpuRenderPassDesc PassDesc;
				PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{OutputView, RenderTargetLoadAction::Load, RenderTargetStoreAction::Store});

				GpuRenderPipelineStateDesc PipelineDesc{
					.Vs = Shader->GetMoveVS(),
					.Ps = Shader->GetPixelShader(),
					.Targets = {{.TargetFormat = TargetFormat}},
					.RasterizerState = {RasterizerFillMode::Solid, RasterizerCullMode::None},
					.Primitive = PrimitiveType::TriangleList,
					.SampleCount = SampleCount,
				};
				Bindings.ApplyBindGroupLayout(PipelineDesc);
				TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

				Graph.AddRenderPass("GizmoShafts", MoveTemp(PassDesc), Bindings,
					[Pipeline](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
						Bindings.ApplyBindGroup(PassRecorder);
						PassRecorder->SetRenderPipelineState(Pipeline);
						PassRecorder->DrawPrimitive(0, 18, 0, 1);
					}
				);
			}

			// Cone arrowheads: TriangleList, 3 axes × 12 segments × 6 verts = 216 verts
			{
				GpuRenderPassDesc PassDesc;
				PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{OutputView, RenderTargetLoadAction::Load, RenderTargetStoreAction::Store});

				GpuRenderPipelineStateDesc PipelineDesc{
					.Vs = Shader->GetMoveArrowVS(),
					.Ps = Shader->GetPixelShader(),
					.Targets = {{.TargetFormat = TargetFormat}},
					.RasterizerState = {RasterizerFillMode::Solid, RasterizerCullMode::None},
					.Primitive = PrimitiveType::TriangleList,
					.SampleCount = SampleCount,
				};
				Bindings.ApplyBindGroupLayout(PipelineDesc);
				TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

				Graph.AddRenderPass("GizmoArrows", MoveTemp(PassDesc), Bindings,
					[Pipeline](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
						Bindings.ApplyBindGroup(PassRecorder);
						PassRecorder->SetRenderPipelineState(Pipeline);
						PassRecorder->DrawPrimitive(0, 216, 0, 1);
					}
				);
			}
		}
		else if (Mode == GizmoMode::Rotate)
		{
			// Rotate circles: TriangleList (screen-space quads), 4 circles × 64 seg × 6 verts = 1536 verts
			GpuRenderPassDesc PassDesc;
			PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{OutputView, RenderTargetLoadAction::Load, RenderTargetStoreAction::Store});

			GpuRenderPipelineStateDesc PipelineDesc{
				.Vs = Shader->GetRotateVS(),
				.Ps = Shader->GetPixelShader(),
				.Targets = {{.TargetFormat = TargetFormat}},
				.RasterizerState = {RasterizerFillMode::Solid, RasterizerCullMode::None},
				.Primitive = PrimitiveType::TriangleList,
				.SampleCount = SampleCount,
			};
			Bindings.ApplyBindGroupLayout(PipelineDesc);
			TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

			Graph.AddRenderPass("GizmoRotate", MoveTemp(PassDesc), Bindings,
				[Pipeline](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
					Bindings.ApplyBindGroup(PassRecorder);
					PassRecorder->SetRenderPipelineState(Pipeline);
					PassRecorder->DrawPrimitive(0, 1536, 0, 1);
				}
			);
		}
		else if (Mode == GizmoMode::Scale)
		{
			// Scale shafts: TriangleList (screen-space quads), 3 axes × 6 verts = 18 verts
			{
				GpuRenderPassDesc PassDesc;
				PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{OutputView, RenderTargetLoadAction::Load, RenderTargetStoreAction::Store});

				GpuRenderPipelineStateDesc PipelineDesc{
					.Vs = Shader->GetScaleVS(),
					.Ps = Shader->GetPixelShader(),
					.Targets = {{.TargetFormat = TargetFormat}},
					.RasterizerState = {RasterizerFillMode::Solid, RasterizerCullMode::None},
					.Primitive = PrimitiveType::TriangleList,
					.SampleCount = SampleCount,
				};
				Bindings.ApplyBindGroupLayout(PipelineDesc);
				TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

				Graph.AddRenderPass("GizmoScaleShafts", MoveTemp(PassDesc), Bindings,
					[Pipeline](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
						Bindings.ApplyBindGroup(PassRecorder);
						PassRecorder->SetRenderPipelineState(Pipeline);
						PassRecorder->DrawPrimitive(0, 18, 0, 1);
					}
				);
			}

			// Scale cube tips: TriangleList, 3 axes × 36 verts = 108 verts
			{
				GpuRenderPassDesc PassDesc;
				PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{OutputView, RenderTargetLoadAction::Load, RenderTargetStoreAction::Store});

				GpuRenderPipelineStateDesc PipelineDesc{
					.Vs = Shader->GetScaleCubeVS(),
					.Ps = Shader->GetPixelShader(),
					.Targets = {{.TargetFormat = TargetFormat}},
					.RasterizerState = {RasterizerFillMode::Solid, RasterizerCullMode::None},
					.Primitive = PrimitiveType::TriangleList,
					.SampleCount = SampleCount,
				};
				Bindings.ApplyBindGroupLayout(PipelineDesc);
				TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

				Graph.AddRenderPass("GizmoScaleCubes", MoveTemp(PassDesc), Bindings,
					[Pipeline](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
						Bindings.ApplyBindGroup(PassRecorder);
						PassRecorder->SetRenderPipelineState(Pipeline);
						PassRecorder->DrawPrimitive(0, 108, 0, 1);
					}
				);
			}
		}
	}

	void ScenePreviewRenderer::RenderCameraWireframes(RenderGraph& Graph, GpuTextureView* OutputView,
		const Camera& SceneCamera, const TArray<ObjectPtr<SceneObject>>& SceneObjects,
		SceneObject* SelectedObject)
	{
		CameraWireframeShader* Shader = GetShader<CameraWireframeShader>();

		uint32 SampleCount = OutputView->GetTexture()->GetSampleCount();
		FMatrix44f SceneVP = SceneCamera.GetViewProjectionMatrix();

		for (const auto& Obj : SceneObjects)
		{
			CameraSceneObject* CamObj = dynamic_cast<CameraSceneObject*>(Obj.Get());
			if (!CamObj)
			{
				continue;
			}

			// Use a default aspect ratio for the wireframe visualization
			Camera Cam = CamObj->ToCamera(16.0f / 9.0f);
			// Clamp far plane for wireframe visualization to avoid extremely long frustum
			Cam.FarPlane = FMath::Min(Cam.FarPlane, 20.0f);
			FMatrix44f CamVP = Cam.GetViewProjectionMatrix();
			FMatrix44f InvCamVP = CamVP.Inverse();

			FVector4f WireColor = (CamObj == SelectedObject) 
				? FVector4f(1.0f, 0.8f, 0.2f, 1.0f) 
				: FVector4f(0.8f, 0.8f, 0.8f, 1.0f);

			TRefCountPtr<GpuBindGroup> BindGroup = Shader->GetBindGroup(SceneVP, InvCamVP, WireColor);

			BindingContext Bindings;
			Bindings.SetPassBindGroup(BindGroup);
			Bindings.SetPassBindGroupLayout(Shader->GetBindGroupLayout());

			GpuRenderPassDesc PassDesc;
			PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{OutputView, RenderTargetLoadAction::Load, RenderTargetStoreAction::Store});

			GpuRenderPipelineStateDesc PipelineDesc{
				.Vs = Shader->GetVertexShader(),
				.Ps = Shader->GetPixelShader(),
				.Targets = {{
					.TargetFormat = OutputView->GetTexture()->GetFormat(),
				}},
				.RasterizerState = {RasterizerFillMode::Solid, RasterizerCullMode::None},
				.Primitive = PrimitiveType::LineList,
				.SampleCount = SampleCount,
			};
			Bindings.ApplyBindGroupLayout(PipelineDesc);
			TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

			Graph.AddRenderPass("CameraWireframe", MoveTemp(PassDesc), Bindings,
				[Pipeline](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
					Bindings.ApplyBindGroup(PassRecorder);
					PassRecorder->SetRenderPipelineState(Pipeline);
					PassRecorder->DrawPrimitive(0, 24, 0, 1); // 12 edges × 2 verts
				}
			);
		}
	}

	void ScenePreviewRenderer::RenderBillboards(RenderGraph& Graph, GpuTextureView* OutputView,
		GpuTextureView* DepthView,
		const Camera& SceneCamera, const TArray<ObjectPtr<SceneObject>>& SceneObjects)
	{
		if (!CameraIconTexView.IsValid())
		{
			return;
		}

		BillboardShader* Shader = GetShader<BillboardShader>();

		uint32 SampleCount = OutputView->GetTexture()->GetSampleCount();
		FMatrix44f VP = SceneCamera.GetViewProjectionMatrix();

		// Compute camera right and up vectors for billboarding
		FMatrix44f ViewMat = SceneCamera.GetViewMatrix();
		Vector3f CamRight(ViewMat.M[0][0], ViewMat.M[1][0], ViewMat.M[2][0]);
		Vector3f CamUp(ViewMat.M[0][1], ViewMat.M[1][1], ViewMat.M[2][1]);

		for (const auto& Obj : SceneObjects)
		{
			CameraSceneObject* CamObj = dynamic_cast<CameraSceneObject*>(Obj.Get());
			if (!CamObj)
			{
				continue;
			}

			// Constant screen size: scale by distance from scene camera
			Vector3f Delta = CamObj->Position - SceneCamera.Position;
			float Dist = FMath::Sqrt(Delta.X * Delta.X + Delta.Y * Delta.Y + Delta.Z * Delta.Z);
			float BillboardScale = FMath::Max(Dist * 0.06f, 0.01f);

			TRefCountPtr<GpuBindGroup> BindGroup = Shader->GetBindGroup(VP, CamObj->Position, BillboardScale,
				CamRight, CamUp, CameraIconTexView);

			BindingContext Bindings;
			Bindings.SetPassBindGroup(BindGroup);
			Bindings.SetPassBindGroupLayout(Shader->GetBindGroupLayout());

			GpuRenderPassDesc PassDesc;
			PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{OutputView, RenderTargetLoadAction::Load, RenderTargetStoreAction::Store});
			PassDesc.DepthStencilTarget = GpuDepthStencilTargetInfo{DepthView, RenderTargetLoadAction::Load, RenderTargetStoreAction::Store};

			GpuRenderPipelineStateDesc PipelineDesc{
				.Vs = Shader->GetVertexShader(),
				.Ps = Shader->GetPixelShader(),
				.Targets = {{
					.TargetFormat = OutputView->GetTexture()->GetFormat(),
					.BlendEnable = true,
					.SrcFactor = BlendFactor::SrcAlpha,
					.ColorOp = BlendOp::Add,
					.DestFactor = BlendFactor::InvSrcAlpha,
				}},
				.RasterizerState = {RasterizerFillMode::Solid, RasterizerCullMode::None},
				.Primitive = PrimitiveType::TriangleList,
				.SampleCount = SampleCount,
				.DepthStencilState = DepthStencilStateDesc{
					.DepthFormat = DepthView->GetTexture()->GetFormat(),
					.DepthWriteEnable = false,
					.DepthCompare = CompareMode::LessEqual,
				},
			};
			Bindings.ApplyBindGroupLayout(PipelineDesc);
			TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

			Graph.AddRenderPass("Billboard", MoveTemp(PassDesc), Bindings,
				[Pipeline](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
					Bindings.ApplyBindGroup(PassRecorder);
					PassRecorder->SetRenderPipelineState(Pipeline);
					PassRecorder->DrawPrimitive(0, 6, 0, 1);
				}
			);
		}
	}

	void ScenePreviewRenderer::RenderCameraPreview(RenderGraph& Graph, GpuTextureView* OutputView,
		const Camera& SelectedCamera, const TArray<ObjectPtr<SceneObject>>& SceneObjects,
		uint32 ViewWidth, uint32 ViewHeight)
	{
		uint32 PreviewWidth = FMath::Max(ViewWidth / 4, 1u);
		uint32 PreviewHeight = FMath::Max(ViewHeight / 4, 1u);

		// Create/resize preview render targets
		if (!CamPreviewRT.IsValid() || CamPreviewRT->GetWidth() != PreviewWidth || CamPreviewRT->GetHeight() != PreviewHeight)
		{
			CamPreviewRT = GGpuRhi->CreateTexture({
				.Width = PreviewWidth,
				.Height = PreviewHeight,
				.Format = GpuFormat::B8G8R8A8_UNORM,
				.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::ShaderResource,
				.ClearValues = Vector4f(0.1f, 0.1f, 0.1f, 1.0f),
			}, GpuResourceState::RenderTargetWrite);
			GGpuRhi->SetResourceName("CamPreviewRT", CamPreviewRT);
			CamPreviewRTV = GGpuRhi->CreateTextureView({.Texture = CamPreviewRT});

			CamPreviewDepthRT = GGpuRhi->CreateTexture({
				.Width = PreviewWidth,
				.Height = PreviewHeight,
				.Format = GpuFormat::D32_FLOAT,
				.Usage = GpuTextureUsage::DepthStencil,
			});
			GGpuRhi->SetResourceName("CamPreviewDepthRT", CamPreviewDepthRT);
			CamPreviewDepthDSV = GGpuRhi->CreateTextureView({.Texture = CamPreviewDepthRT});
		}

		// Render meshes from the selected camera's perspective
		RenderMeshes(Graph, CamPreviewRTV, CamPreviewDepthDSV, SelectedCamera, SceneObjects, true);

		// Blit preview onto the bottom-right corner of the output
		float OverlayX = (float)(ViewWidth - PreviewWidth);
		float OverlayY = (float)(ViewHeight - PreviewHeight);

		BlitPassInput BlitInput;
		BlitInput.InputView = CamPreviewRTV;
		BlitInput.InputTexSampler = GpuResourceHelper::GetSampler({ .Filter = SamplerFilter::Bilinear });
		BlitInput.OutputView = OutputView;
		BlitInput.LoadAction = RenderTargetLoadAction::Load;
		BlitInput.Viewport = GpuViewPortDesc{
			.Width = (float)PreviewWidth,
			.Height = (float)PreviewHeight,
			.TopLeftX = OverlayX,
			.TopLeftY = OverlayY,
		};
		BlitInput.Scissor = GpuScissorRectDesc{
			.Left = (uint32)OverlayX,
			.Top = (uint32)OverlayY,
			.Right = ViewWidth,
			.Bottom = ViewHeight,
		};
		AddBlitPass(Graph, BlitInput);
	}

	void ScenePreviewRenderer::ResolveMsaa(RenderGraph& Graph, GpuTextureView* MsaaView, GpuTextureView* ResolveView)
	{
		GpuRenderPassDesc PassDesc;
		PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{
			MsaaView,
			RenderTargetLoadAction::Load,
			RenderTargetStoreAction::DontCare,
			Vector4f(0, 0, 0, 0),
			ResolveView
		});

		Graph.AddRenderPass("ResolveMsaa", MoveTemp(PassDesc), {},
			[](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
			}
		);
	}
}
