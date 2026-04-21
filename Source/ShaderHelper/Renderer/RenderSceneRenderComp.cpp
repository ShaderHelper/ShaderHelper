#include "CommonHeader.h"
#include "RenderSceneRenderComp.h"
#include "GpuApi/GpuRhi.h"
#include "GpuApi/GpuResourceHelper.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/Scene/SSceneView.h"
#include "UI/Widgets/Scene/SceneUndoManager.h"
#include "UI/Widgets/AssetBrowser/AssetViewItem/AssetViewItem.h"
#include "Common/Util/Math.h"
#include "AssetObject/Render/MeshSceneObject.h"
#include "AssetObject/Render/CameraSceneObject.h"
#include "AssetObject/Model.h"
#include "AssetManager/AssetManager.h"

using namespace FW;

namespace SH
{
	RenderSceneRenderComp::RenderSceneRenderComp(Render* InRenderGraph, PreviewViewPort* InViewPort)
		: RenderGraphAsset(InRenderGraph)
		, ViewPort(InViewPort)
	{
		PreviewCamera.Position = {0, 3, -7};
		PreviewCamera.Pitch = -0.5f;
		PreviewCamera.VerticalFov = FMath::DegreesToRadians(60.0f);
		PreviewCamera.NearPlane = 0.1f;
		PreviewCamera.FarPlane = 1000.0f;
		LastFrameTime = FPlatformTime::Seconds();

		// Setup input delegates
		ViewPort->MouseDownHandler.BindRaw(this, &RenderSceneRenderComp::OnMouseDown);
		ViewPort->MouseUpHandler.BindRaw(this, &RenderSceneRenderComp::OnMouseUp);
		ViewPort->MouseMoveHandler.BindRaw(this, &RenderSceneRenderComp::OnMouseMove);
		ViewPort->MouseWheelHandler.BindRaw(this, &RenderSceneRenderComp::OnMouseWheel);
		KeyDownHandle = ViewPort->KeyDownHandler.AddRaw(this, &RenderSceneRenderComp::OnKeyDown);
		KeyUpHandle = ViewPort->KeyUpHandler.AddRaw(this, &RenderSceneRenderComp::OnKeyUp);
		ViewPort->DragEnterHandler.BindRaw(this, &RenderSceneRenderComp::OnDragEnter);
		ViewPort->DragLeaveHandler.BindRaw(this, &RenderSceneRenderComp::OnDragLeave);
		ViewPort->DropHandler.BindRaw(this, &RenderSceneRenderComp::OnDrop);

		// Create graph comp for custom mode
		GraphComp = MakeUnique<RenderRenderComp>(InRenderGraph, InViewPort);
	}

	RenderSceneRenderComp::~RenderSceneRenderComp()
	{
		ViewPort->MouseDownHandler.Unbind();
		ViewPort->MouseUpHandler.Unbind();
		ViewPort->MouseMoveHandler.Unbind();
		ViewPort->MouseWheelHandler.Unbind();
		ViewPort->KeyDownHandler.Remove(KeyDownHandle);
		ViewPort->KeyUpHandler.Remove(KeyUpHandle);
		ViewPort->DragEnterHandler.Unbind();
		ViewPort->DragLeaveHandler.Unbind();
		ViewPort->DropHandler.Unbind();
	}

	FReply RenderSceneRenderComp::OnMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			bRightMouseDown = true;
			LastMousePos = MouseEvent.GetScreenSpacePosition();
			return FReply::Handled();
		}
		if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
		{
			bMiddleMouseDown = true;
			LastMousePos = MouseEvent.GetScreenSpacePosition();
			return FReply::Handled();
		}
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			FVector2D VpSize = MyGeometry.GetLocalSize();
			if (VpSize.X > 0 && VpSize.Y > 0)
				PreviewCamera.AspectRatio = (float)VpSize.X / (float)VpSize.Y;

			GizmoAxis HitAxis = HitTestGizmo(LocalPos, VpSize);
			if (HitAxis != GizmoAxis::None)
			{
				SceneObject* SelObj = GetSelectedSceneObject();
				if (SelObj)
				{
					DraggingAxis = HitAxis;
					DragStartObjectPos = SelObj->Position;
					DragStartObjectRotation = SelObj->Rotation;
					DragStartObjectScale = SelObj->Scale;
					DragStartMousePos = LocalPos;

					if (DraggingAxis == GizmoAxis::All)
					{
						// Uniform scale: use camera-right as drag reference direction
						float GizScale = GetGizmoScale(SelObj->Position);
						FMatrix44f CamRotMat = PreviewCamera.GetWorldRotationMatrix();
						Vector3f CamRight(CamRotMat.M[0][0], CamRotMat.M[0][1], CamRotMat.M[0][2]);
						FVector2D CenterScreen = WorldToScreen(SelObj->Position, VpSize);
						FVector2D EndScreen = WorldToScreen(SelObj->Position + CamRight * GizScale, VpSize);
						FVector2D ScreenDelta = EndScreen - CenterScreen;
						float Len = (float)FMath::Sqrt(ScreenDelta.X * ScreenDelta.X + ScreenDelta.Y * ScreenDelta.Y);
						DragLocalAxisDir = Vector3f(0, 0, 0);
						DragPixelsPerUnit = (Len > 0.001f) ? (Len / GizScale) : 100.0f;
						DragAxisScreenDir = (Len > 0.001f) ? ScreenDelta * (1.0f / Len) : FVector2D(1, 0);
					}
					else if (DraggingAxis == GizmoAxis::XY || DraggingAxis == GizmoAxis::XZ || DraggingAxis == GizmoAxis::YZ)
					{
						// Plane drag: ray-cast to world plane to get start hit point
						FMatrix44f OrientMat = GetGizmoOrientationMatrix(SelObj);
						DragPlaneNormal = GetPlaneNormal(DraggingAxis, OrientMat);
						Vector3f RayOrigin, RayDir;
						ComputeMouseRay(LocalPos, VpSize, RayOrigin, RayDir);
						Vector3f HitPt;
						if (RayPlaneIntersect(RayOrigin, RayDir, SelObj->Position, DragPlaneNormal, HitPt))
							DragPlaneHitStart = HitPt;
						else
							DragPlaneHitStart = SelObj->Position;
						// DragPixelsPerUnit doesn't matter for plane drag; set sentinel
						DragPixelsPerUnit = 1.0f;
					}
					else
					{
						FMatrix44f OrientMat = GetGizmoOrientationMatrix(SelObj);
						Vector3f AxisDir = GetOrientedAxisDir(DraggingAxis, OrientMat);
						DragLocalAxisDir = AxisDir;

						// Compute screen-space axis direction and pixels-per-unit
						FVector2D CenterScreen = WorldToScreen(SelObj->Position, VpSize);
						float GizScale = GetGizmoScale(SelObj->Position);
						FVector2D EndScreen = WorldToScreen(SelObj->Position + AxisDir * GizScale, VpSize);
						FVector2D ScreenDelta = EndScreen - CenterScreen;
						DragPixelsPerUnit = (float)FMath::Sqrt(ScreenDelta.X * ScreenDelta.X + ScreenDelta.Y * ScreenDelta.Y);
						if (DragPixelsPerUnit > 0.001f)
						{
							DragAxisScreenDir = ScreenDelta * (1.0f / DragPixelsPerUnit);
							DragPixelsPerUnit /= GizScale;

							if (GetCurrentGizmoMode() == GizmoMode::Rotate)
							{
								// Screen-space tangent: perpendicular to radial direction from center to grab point
								FVector2D RadialDir = LocalPos - CenterScreen;
								float RadialLen = (float)FMath::Sqrt(RadialDir.X * RadialDir.X + RadialDir.Y * RadialDir.Y);
								if (RadialLen > 0.001f)
								{
									DragAxisScreenDir = FVector2D(-RadialDir.Y, RadialDir.X) * (1.0f / RadialLen);
									// Flip when viewing from the back side of the rotation plane
									Vector3f ViewDir = SelObj->Position - PreviewCamera.Position;
									float AxisDot = ViewDir.X * AxisDir.X + ViewDir.Y * AxisDir.Y + ViewDir.Z * AxisDir.Z;
									if (AxisDot > 0)
									{
										DragAxisScreenDir = DragAxisScreenDir * -1.0f;
									}
								}
							}
						}
					}
					return FReply::Handled();
				}
			}

			// No gizmo hit — try picking a scene object
			SceneObject* Picked = PickSceneObject(LocalPos, VpSize);
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			SSceneView* SceneViewWidget = ShEditor->GetSceneView();
			if (SceneViewWidget)
			{
				SceneViewWidget->SelectObject(Picked);
			}
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	FReply RenderSceneRenderComp::OnMouseUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			bRightMouseDown = false;
			return FReply::Handled();
		}
		if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
		{
			bMiddleMouseDown = false;
			return FReply::Handled();
		}
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			if (DraggingAxis != GizmoAxis::None)
			{
				SceneObject* SelObj = GetSelectedSceneObject();
				if (SelObj)
				{
					bool bChanged = (SelObj->Position != DragStartObjectPos) ||
									(SelObj->Rotation != DragStartObjectRotation) ||
									(SelObj->Scale != DragStartObjectScale);
					if (bChanged)
					{
						auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
						SSceneView* SceneViewWidget = ShEditor->GetSceneView();
						if (SceneViewWidget)
						{
							if (auto* Mgr = SceneViewWidget->GetUndoManager())
							{
								Mgr->PushCommand(MakeShared<TransformCommand>(
									SceneViewWidget,
									SelObj,
									DragStartObjectPos, DragStartObjectRotation, DragStartObjectScale,
									SelObj->Position, SelObj->Rotation, SelObj->Scale
								));
							}
						}
					}
				}
			}
			DraggingAxis = GizmoAxis::None;
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	FReply RenderSceneRenderComp::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (bRightMouseDown)
		{
			FVector2D CurrentPos = MouseEvent.GetScreenSpacePosition();
			FVector2D Delta = CurrentPos - LastMousePos;
			LastMousePos = CurrentPos;

			PreviewCamera.Yaw += (float)Delta.X * CameraRotateSpeed;
			PreviewCamera.Pitch -= (float)Delta.Y * CameraRotateSpeed;
			PreviewCamera.Pitch = FMath::Clamp(PreviewCamera.Pitch, -PI * 0.49f, PI * 0.49f);
			return FReply::Handled();
		}

		if (bMiddleMouseDown)
		{
			FVector2D CurrentPos = MouseEvent.GetScreenSpacePosition();
			FVector2D Delta = CurrentPos - LastMousePos;
			LastMousePos = CurrentPos;

			FMatrix44f RotMat = PreviewCamera.GetWorldRotationMatrix();
			Vector3f Right = Vector3f(RotMat.M[0][0], RotMat.M[0][1], RotMat.M[0][2]);
			Vector3f Up = Vector3f(RotMat.M[1][0], RotMat.M[1][1], RotMat.M[1][2]);

			PreviewCamera.Position -= Right * (float)Delta.X * CameraPanSpeed;
			PreviewCamera.Position += Up * (float)Delta.Y * CameraPanSpeed;
			return FReply::Handled();
		}

		FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		FVector2D VpSize = MyGeometry.GetLocalSize();
		if (VpSize.X > 0 && VpSize.Y > 0)
			PreviewCamera.AspectRatio = (float)VpSize.X / (float)VpSize.Y;

		if (DraggingAxis != GizmoAxis::None)
		{
			SceneObject* SelObj = GetSelectedSceneObject();
			if (SelObj && DragPixelsPerUnit > 0.001f)
			{
				// Plane drag: use ray-plane intersection directly (bypasses screen-space projection)
				if (DraggingAxis == GizmoAxis::XY || DraggingAxis == GizmoAxis::XZ || DraggingAxis == GizmoAxis::YZ)
				{
					Vector3f RayOrigin, RayDir;
					ComputeMouseRay(LocalPos, VpSize, RayOrigin, RayDir);
					Vector3f HitPt;
					if (RayPlaneIntersect(RayOrigin, RayDir, SelObj->Position, DragPlaneNormal, HitPt))
					{
						Vector3f PlaneDelta = HitPt - DragPlaneHitStart;
						// Project PlaneDelta onto allowed axes
						FMatrix44f OrientMat = GetGizmoOrientationMatrix(SelObj);
						GizmoAxis A, B;
						if      (DraggingAxis == GizmoAxis::XY) { A = GizmoAxis::X; B = GizmoAxis::Y; }
						else if (DraggingAxis == GizmoAxis::XZ) { A = GizmoAxis::X; B = GizmoAxis::Z; }
						else                                    { A = GizmoAxis::Y; B = GizmoAxis::Z; }
						Vector3f DirA = GetOrientedAxisDir(A, OrientMat);
						Vector3f DirB = GetOrientedAxisDir(B, OrientMat);
						float dA = PlaneDelta.X * DirA.X + PlaneDelta.Y * DirA.Y + PlaneDelta.Z * DirA.Z;
						float dB = PlaneDelta.X * DirB.X + PlaneDelta.Y * DirB.Y + PlaneDelta.Z * DirB.Z;
						SelObj->Position = DragStartObjectPos + DirA * dA + DirB * dB;
						SelObj->GetOuterMost()->MarkDirty();
					}
					return FReply::Handled();
				}

				FVector2D MouseDelta = LocalPos - DragStartMousePos;
				double Projected = MouseDelta.X * DragAxisScreenDir.X + MouseDelta.Y * DragAxisScreenDir.Y;
				float WorldDelta = (float)(Projected / DragPixelsPerUnit);

				GizmoMode Mode = GetCurrentGizmoMode();
				GizmoSpace Space = GetCurrentGizmoSpace();

				if (Mode == GizmoMode::Move)
				{
					FMatrix44f OrientMat = GetGizmoOrientationMatrix(SelObj);
					Vector3f AxisDir = GetOrientedAxisDir(DraggingAxis, OrientMat);
					SelObj->Position = DragStartObjectPos + AxisDir * WorldDelta;
					SelObj->GetOuterMost()->MarkDirty();
				}
				else if (Mode == GizmoMode::Rotate)
				{
					float AngleDelta = WorldDelta * 90.0f; // 90 degrees per gizmo-scale unit

					// Build quaternion for the start rotation
					FRotator3f StartRotator(DragStartObjectRotation.X, DragStartObjectRotation.Y, DragStartObjectRotation.Z);
					FQuat4f StartQuat = StartRotator.Quaternion();

					// Convert axis from project space (X-right,Y-up,Z-forward) to UE space (X-forward,Y-right,Z-up)
					FVector3f UeAxis(DragLocalAxisDir.Z, DragLocalAxisDir.X, DragLocalAxisDir.Y);
					FQuat4f DeltaQuat(UeAxis, FMath::DegreesToRadians(AngleDelta));

					FQuat4f NewQuat = DeltaQuat * StartQuat;
					NewQuat.Normalize();
					FRotator3f NewRotator = NewQuat.Rotator();
					SelObj->Rotation = Vector3f(NewRotator.Pitch, NewRotator.Yaw, NewRotator.Roll);
					SelObj->GetOuterMost()->MarkDirty();
				}
				else if (Mode == GizmoMode::Scale)
				{
					float ScaleFactor = 1.0f + WorldDelta;
					ScaleFactor = FMath::Max(ScaleFactor, 0.01f);
					Vector3f NewScale = DragStartObjectScale;
					if (DraggingAxis == GizmoAxis::All) NewScale = DragStartObjectScale * ScaleFactor;
					else if (DraggingAxis == GizmoAxis::X) NewScale.X *= ScaleFactor;
					else if (DraggingAxis == GizmoAxis::Y) NewScale.Y *= ScaleFactor;
					else NewScale.Z *= ScaleFactor;
					SelObj->Scale = NewScale;
					SelObj->GetOuterMost()->MarkDirty();
				}
			}
			return FReply::Handled();
		}

		// Update hovered axis for highlight
		HoveredAxis = HitTestGizmo(LocalPos, VpSize);
		return FReply::Unhandled();
	}

	FReply RenderSceneRenderComp::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		float WheelDelta = MouseEvent.GetWheelDelta();
		FMatrix44f RotMat = PreviewCamera.GetWorldRotationMatrix();
		Vector3f Forward = Vector3f(RotMat.M[2][0], RotMat.M[2][1], RotMat.M[2][2]);
		PreviewCamera.Position += Forward * WheelDelta * CameraScrollSpeed;
		return FReply::Handled();
	}

	void RenderSceneRenderComp::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
	{
		if (!bRightMouseDown)
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			if (ShEditor->GetUICommandList()->ProcessCommandBindings(KeyEvent))
			{
				return;
			}
		}

		FKey Key = KeyEvent.GetKey();
		if (Key == EKeys::W) bKeyW = true;
		else if (Key == EKeys::A) bKeyA = true;
		else if (Key == EKeys::S) bKeyS = true;
		else if (Key == EKeys::D) bKeyD = true;
		else if (Key == EKeys::Q) bKeyQ = true;
		else if (Key == EKeys::E) bKeyE = true;
	}

	void RenderSceneRenderComp::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
	{
		FKey Key = KeyEvent.GetKey();
		if (Key == EKeys::W) bKeyW = false;
		else if (Key == EKeys::A) bKeyA = false;
		else if (Key == EKeys::S) bKeyS = false;
		else if (Key == EKeys::D) bKeyD = false;
		else if (Key == EKeys::Q) bKeyQ = false;
		else if (Key == EKeys::E) bKeyE = false;
	}

	void RenderSceneRenderComp::UpdateCamera(float DeltaTime)
	{
		if (!bRightMouseDown)
		{
			return;
		}

		FMatrix44f RotMat = PreviewCamera.GetWorldRotationMatrix();
		// Left-handed: X-right, Y-up, Z-forward
		Vector3f Forward = Vector3f(RotMat.M[2][0], RotMat.M[2][1], RotMat.M[2][2]);
		Vector3f Right = Vector3f(RotMat.M[0][0], RotMat.M[0][1], RotMat.M[0][2]);
		Vector3f Up = Vector3f(0, 1, 0);

		Vector3f MoveDir(0);
		if (bKeyW) MoveDir += Forward;
		if (bKeyS) MoveDir -= Forward;
		if (bKeyD) MoveDir += Right;
		if (bKeyA) MoveDir -= Right;
		if (bKeyE) MoveDir += Up;
		if (bKeyQ) MoveDir -= Up;

		if (MoveDir.X != 0 || MoveDir.Y != 0 || MoveDir.Z != 0)
		{
			float Len = FMath::Sqrt(MoveDir.X * MoveDir.X + MoveDir.Y * MoveDir.Y + MoveDir.Z * MoveDir.Z);
			MoveDir = MoveDir * (1.0f / Len);
			PreviewCamera.Position += MoveDir * CameraMoveSpeed * DeltaTime;
		}
	}

	void RenderSceneRenderComp::RenderBegin()
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if (!ShEditor->IsScenePreview())
		{
			GraphComp->RenderBegin();
		}
	}

	void RenderSceneRenderComp::RenderInternal()
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if (ShEditor->IsScenePreview())
		{
			RenderPreview();
		}
		else
		{
			RenderGraph();
		}
	}

	void RenderSceneRenderComp::RenderEnd()
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if (!ShEditor->IsScenePreview())
		{
			GraphComp->RenderEnd();
		}
	}

	void RenderSceneRenderComp::RenderPreview()
	{
		if (!RenderGraphAsset || !ViewPort)
		{
			return;
		}

		PreviewRenderer.InitResources();

		FIntPoint ViewSize = ViewPort->GetSize();
		if (ViewSize.X == 0 || ViewSize.Y == 0)
		{
			return;
		}

		double CurrentTime = FPlatformTime::Seconds();
		float DeltaTime = (float)(CurrentTime - LastFrameTime);
		LastFrameTime = CurrentTime;
		DeltaTime = FMath::Clamp(DeltaTime, 0.0f, 0.1f);

		UpdateCamera(DeltaTime);

		PreviewCamera.AspectRatio = (float)ViewSize.X / (float)ViewSize.Y;

		// Create/resize render targets
		if (!FinalRT.IsValid() || FinalRT->GetWidth() != (uint32)ViewSize.X || FinalRT->GetHeight() != (uint32)ViewSize.Y)
		{
			FinalRT = GGpuRhi->CreateTexture({
				.Width = (uint32)ViewSize.X,
				.Height = (uint32)ViewSize.Y,
				.Format = GpuFormat::B8G8R8A8_UNORM,
				.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared,
				.ClearValues = Vector4f(0.18f, 0.18f, 0.18f, 1.0f),
			});
			GGpuRhi->SetResourceName("ScenePreviewFinalRT", FinalRT);
			FinalRTV = GGpuRhi->CreateTextureView({.Texture = FinalRT});

			MsaaRT = GGpuRhi->CreateTexture({
				.Width = (uint32)ViewSize.X,
				.Height = (uint32)ViewSize.Y,
				.Format = GpuFormat::B8G8R8A8_UNORM,
				.Usage = GpuTextureUsage::RenderTarget,
				.ClearValues = Vector4f(0.18f, 0.18f, 0.18f, 1.0f),
				.SampleCount = MsaaSampleCount,
			});
			GGpuRhi->SetResourceName("ScenePreviewMsaaRT", MsaaRT);
			MsaaRTV = GGpuRhi->CreateTextureView({.Texture = MsaaRT});

			DepthRT = GGpuRhi->CreateTexture({
				.Width = (uint32)ViewSize.X,
				.Height = (uint32)ViewSize.Y,
				.Format = GpuFormat::D32_FLOAT,
				.Usage = GpuTextureUsage::DepthStencil,
				.SampleCount = MsaaSampleCount,
			});
			GGpuRhi->SetResourceName("ScenePreviewDepthRT", DepthRT);
			DepthDSV = GGpuRhi->CreateTextureView({.Texture = DepthRT});

			MaskRT = GGpuRhi->CreateTexture({
				.Width = (uint32)ViewSize.X,
				.Height = (uint32)ViewSize.Y,
				.Format = GpuFormat::R8_UNORM,
				.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::ShaderResource,
				.ClearValues = Vector4f(0, 0, 0, 0),
			}, GpuResourceState::RenderTargetWrite);
			GGpuRhi->SetResourceName("ScenePreviewMaskRT", MaskRT);
			MaskRTV = GGpuRhi->CreateTextureView({.Texture = MaskRT});
		}
		ViewPort->SetViewPortRenderTexture(FinalRT);

		FW::RenderGraph Graph;

		// Render meshes first (clears color + depth on first draw)
		bool bMeshesDrawn = PreviewRenderer.RenderMeshes(Graph, MsaaRTV, DepthDSV, PreviewCamera, RenderGraphAsset->SceneObjects, true);

		// Render grid with depth test (clears if no meshes were drawn)
		PreviewRenderer.RenderGrid(Graph, MsaaRTV, DepthDSV, PreviewCamera, !bMeshesDrawn);

		// Render camera billboard icons
		SceneObject* SelObj = GetSelectedSceneObject();
		PreviewRenderer.RenderBillboards(Graph, MsaaRTV, DepthDSV, PreviewCamera, RenderGraphAsset->SceneObjects);

		// Render camera wireframes only when a camera is selected
		CameraSceneObject* SelCam = dynamic_cast<CameraSceneObject*>(SelObj);
		if (SelCam)
		{
			PreviewRenderer.RenderCameraWireframes(Graph, MsaaRTV, PreviewCamera, RenderGraphAsset->SceneObjects, SelObj);
		}

		// Render gizmo for selected object
		if (SelObj)
		{
			int32 Highlight = static_cast<int32>(DraggingAxis != GizmoAxis::None ? DraggingAxis : HoveredAxis);
			GizmoMode Mode = GetCurrentGizmoMode();
			FMatrix44f OrientMat = GetGizmoOrientationMatrix(SelObj);
			PreviewRenderer.RenderGizmo(Graph, MsaaRTV, PreviewCamera, SelObj->Position, Highlight, Mode, OrientMat);
		}

		// Resolve MSAA to FinalRT
		PreviewRenderer.ResolveMsaa(Graph, MsaaRTV, FinalRTV);

		// Render outline for selected mesh (post-process on resolved FinalRT)
		if (SelObj)
		{
			PreviewRenderer.RenderOutline(Graph, FinalRTV, MaskRTV, PreviewCamera, SelObj);
		}

		// Render camera preview overlay when a camera is selected
		if (SelCam)
		{
			Camera SelCamera = SelCam->ToCamera((float)ViewSize.X / (float)ViewSize.Y);
			PreviewRenderer.RenderCameraPreview(Graph, FinalRTV, SelCamera,
				RenderGraphAsset->SceneObjects, (uint32)ViewSize.X, (uint32)ViewSize.Y);
		}

		Graph.Execute();
	}

	void RenderSceneRenderComp::RenderGraph()
	{
		GraphComp->RenderInternal();
	}

	SceneObject* RenderSceneRenderComp::GetSelectedSceneObject() const
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SSceneView* SceneViewWidget = ShEditor->GetSceneView();
		if (SceneViewWidget)
		{
			return SceneViewWidget->GetSelectedObject();
		}
		return nullptr;
	}

	FVector2D RenderSceneRenderComp::WorldToScreen(const Vector3f& WorldPos, const FVector2D& ViewportSize) const
	{
		FMatrix44f VP = PreviewCamera.GetViewProjectionMatrix();
		FVector4f Clip = VP.TransformFVector4(FVector4f(WorldPos.X, WorldPos.Y, WorldPos.Z, 1.0f));
		if (Clip.W > 0.0001f)
		{
			float InvW = 1.0f / Clip.W;
			float ScreenX = (Clip.X * InvW * 0.5f + 0.5f) * (float)ViewportSize.X;
			float ScreenY = (0.5f - Clip.Y * InvW * 0.5f) * (float)ViewportSize.Y;
			return FVector2D(ScreenX, ScreenY);
		}
		return FVector2D(-10000, -10000);
	}

	float RenderSceneRenderComp::PointToSegmentDistSq(const FVector2D& P, const FVector2D& A, const FVector2D& B) const
	{
		FVector2D AB = B - A;
		FVector2D AP = P - A;
		double LenSq = AB.X * AB.X + AB.Y * AB.Y;
		if (LenSq < 0.0001)
		{
			return (float)(AP.X * AP.X + AP.Y * AP.Y);
		}
		double t = FMath::Clamp((AP.X * AB.X + AP.Y * AB.Y) / LenSq, 0.0, 1.0);
		FVector2D Closest = A + AB * t;
		FVector2D Diff = P - Closest;
		return (float)(Diff.X * Diff.X + Diff.Y * Diff.Y);
	}

	float RenderSceneRenderComp::GetGizmoScale(const Vector3f& GizmoPos) const
	{
		Vector3f Delta = GizmoPos - PreviewCamera.Position;
		float Dist = FMath::Sqrt(Delta.X * Delta.X + Delta.Y * Delta.Y + Delta.Z * Delta.Z);
		return FMath::Max(Dist * 0.15f, 0.01f);
	}

	GizmoAxis RenderSceneRenderComp::HitTestGizmo(const FVector2D& LocalMousePos, const FVector2D& ViewportSize) const
	{
		SceneObject* SelObj = GetSelectedSceneObject();
		if (!SelObj)
		{
			return GizmoAxis::None;
		}

		Vector3f Center = SelObj->Position;
		float GizScale = GetGizmoScale(Center);
		FVector2D CenterScreen = WorldToScreen(Center, ViewportSize);

		const float HitThresholdSq = 10.0f * 10.0f; // 10 pixels
		GizmoAxis BestAxis = GizmoAxis::None;
		float BestDistSq = HitThresholdSq;

		FMatrix44f OrientMat = GetGizmoOrientationMatrix(SelObj);
		GizmoAxis AxisEnums[3] = { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z };

		GizmoMode Mode = GetCurrentGizmoMode();
		float AxisLen = GizScale;

		// In Scale mode: check center cube first
		if (Mode == GizmoMode::Scale)
		{
			const float CenterHitRadiusSq = 12.0f * 12.0f;
			FVector2D Diff = LocalMousePos - CenterScreen;
			float DistSq = (float)(Diff.X * Diff.X + Diff.Y * Diff.Y);
			if (DistSq < CenterHitRadiusSq)
			{
				return GizmoAxis::All;
			}
		}

		for (int i = 0; i < 3; i++)
		{
			Vector3f AxisDir = GetOrientedAxisDir(AxisEnums[i], OrientMat);

			if (Mode == GizmoMode::Rotate)
			{
				// Get perpendicular plane vectors
				Vector3f Perp1, Perp2;
				if (FMath::Abs(AxisDir.Y) < 0.9f)
				{
					Vector3f Up(0, 1, 0);
					Perp1 = Vector3f(
						AxisDir.Y * Up.Z - AxisDir.Z * Up.Y,
						AxisDir.Z * Up.X - AxisDir.X * Up.Z,
						AxisDir.X * Up.Y - AxisDir.Y * Up.X);
				}
				else
				{
					Vector3f Right(1, 0, 0);
					Perp1 = Vector3f(
						AxisDir.Y * Right.Z - AxisDir.Z * Right.Y,
						AxisDir.Z * Right.X - AxisDir.X * Right.Z,
						AxisDir.X * Right.Y - AxisDir.Y * Right.X);
				}
				float PLen = FMath::Sqrt(Perp1.X * Perp1.X + Perp1.Y * Perp1.Y + Perp1.Z * Perp1.Z);
				if (PLen > 0.0001f) Perp1 = Perp1 * (1.0f / PLen);
				Perp2 = Vector3f(
					AxisDir.Y * Perp1.Z - AxisDir.Z * Perp1.Y,
					AxisDir.Z * Perp1.X - AxisDir.X * Perp1.Z,
					AxisDir.X * Perp1.Y - AxisDir.Y * Perp1.X);

				// Test against circle arc segments
				const int NumSamples = 32;
				for (int s = 0; s < NumSamples; s++)
				{
					float Angle0 = s * (2.0f * PI / NumSamples);
					float Angle1 = (s + 1) * (2.0f * PI / NumSamples);

					Vector3f P0 = Center + (Perp1 * FMath::Cos(Angle0) + Perp2 * FMath::Sin(Angle0)) * AxisLen;
					Vector3f P1 = Center + (Perp1 * FMath::Cos(Angle1) + Perp2 * FMath::Sin(Angle1)) * AxisLen;

					FVector2D S0 = WorldToScreen(P0, ViewportSize);
					FVector2D S1 = WorldToScreen(P1, ViewportSize);
					float DistSq = PointToSegmentDistSq(LocalMousePos, S0, S1);
					if (DistSq < BestDistSq)
					{
						BestDistSq = DistSq;
						BestAxis = AxisEnums[i];
					}
				}
			}
			else
			{
				FVector2D EndScreen = WorldToScreen(Center + AxisDir * AxisLen, ViewportSize);
				float DistSq = PointToSegmentDistSq(LocalMousePos, CenterScreen, EndScreen);
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					BestAxis = AxisEnums[i];
				}
			}
		}

		// In Move mode: check plane quads
		if (Mode == GizmoMode::Move && BestAxis == GizmoAxis::None)
		{
			float PanelOffset = GizScale * 0.2f;
			float PanelSize   = GizScale * 0.2f;
			// PlaneIndex 0=XY, 1=XZ, 2=YZ
			GizmoAxis PlaneAxes[3] = { GizmoAxis::XY, GizmoAxis::XZ, GizmoAxis::YZ };
			// axis pairs: (A, B) for each plane
			GizmoAxis PairA[3] = { GizmoAxis::X, GizmoAxis::X, GizmoAxis::Y };
			GizmoAxis PairB[3] = { GizmoAxis::Y, GizmoAxis::Z, GizmoAxis::Z };
			for (int p = 0; p < 3; p++)
			{
				Vector3f AxisA = GetOrientedAxisDir(PairA[p], OrientMat);
				Vector3f AxisB = GetOrientedAxisDir(PairB[p], OrientMat);
				// 4 corners of the quad
				Vector3f C0 = Center + AxisA * PanelOffset           + AxisB * PanelOffset;
				Vector3f C1 = Center + AxisA * (PanelOffset + PanelSize) + AxisB * PanelOffset;
				Vector3f C2 = Center + AxisA * (PanelOffset + PanelSize) + AxisB * (PanelOffset + PanelSize);
				Vector3f C3 = Center + AxisA * PanelOffset           + AxisB * (PanelOffset + PanelSize);
				FVector2D S0 = WorldToScreen(C0, ViewportSize);
				FVector2D S1 = WorldToScreen(C1, ViewportSize);
				FVector2D S2 = WorldToScreen(C2, ViewportSize);
				FVector2D S3 = WorldToScreen(C3, ViewportSize);
				// Point-in-quad: check both triangles
				auto PointInTri = [](FVector2D P, FVector2D A, FVector2D B, FVector2D C) -> bool {
					auto Sign = [](FVector2D P, FVector2D A, FVector2D B) {
						return (P.X - B.X) * (A.Y - B.Y) - (A.X - B.X) * (P.Y - B.Y);
					};
					double d1 = Sign(P, A, B), d2 = Sign(P, B, C), d3 = Sign(P, C, A);
					bool HasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
					bool HasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);
					return !(HasNeg && HasPos);
				};
				if (PointInTri(LocalMousePos, S0, S1, S2) || PointInTri(LocalMousePos, S0, S2, S3))
				{
					BestAxis = PlaneAxes[p];
					break;
				}
			}
		}

		return BestAxis;
	}

	FMatrix44f RenderSceneRenderComp::GetGizmoOrientationMatrix(SceneObject* Obj) const
	{
		GizmoSpace Space = GetCurrentGizmoSpace();
		if (Space == GizmoSpace::Local && Obj)
		{
			float PitchRad = FMath::DegreesToRadians(Obj->Rotation.X);
			float YawRad = FMath::DegreesToRadians(Obj->Rotation.Y);
			float RollRad = FMath::DegreesToRadians(Obj->Rotation.Z);
			return RotationMatrix(YawRad, PitchRad, RollRad);
		}
		return FMatrix44f::Identity;
	}

	Vector3f RenderSceneRenderComp::GetOrientedAxisDir(GizmoAxis Axis, const FMatrix44f& Orientation) const
	{
		int ColIdx = 0;
		if (Axis == GizmoAxis::Y) ColIdx = 1;
		else if (Axis == GizmoAxis::Z) ColIdx = 2;

		// Extract column from orientation matrix (row-major: row i, col ColIdx)
		return Vector3f(Orientation.M[ColIdx][0], Orientation.M[ColIdx][1], Orientation.M[ColIdx][2]);
	}

	Vector3f RenderSceneRenderComp::GetPlaneNormal(GizmoAxis PlaneAxis, const FMatrix44f& Orientation) const
	{
		// Normal of the plane is the axis perpendicular to the two plane axes
		if (PlaneAxis == GizmoAxis::XY) return GetOrientedAxisDir(GizmoAxis::Z, Orientation);
		if (PlaneAxis == GizmoAxis::XZ) return GetOrientedAxisDir(GizmoAxis::Y, Orientation);
		/* YZ */                        return GetOrientedAxisDir(GizmoAxis::X, Orientation);
	}

	void RenderSceneRenderComp::ComputeMouseRay(const FVector2D& LocalPos, const FVector2D& ViewportSize, Vector3f& OutOrigin, Vector3f& OutDir) const
	{
		float NdcX = (float)LocalPos.X / (float)ViewportSize.X * 2.0f - 1.0f;
		float NdcY = 1.0f - (float)LocalPos.Y / (float)ViewportSize.Y * 2.0f;
		FMatrix44f InvVP = PreviewCamera.GetViewProjectionMatrix().Inverse();
		FVector4f Near = InvVP.TransformFVector4(FVector4f(NdcX, NdcY, 0.0f, 1.0f));
		FVector4f Far  = InvVP.TransformFVector4(FVector4f(NdcX, NdcY, 1.0f, 1.0f));
		Near = Near * (1.0f / Near.W);
		Far  = Far  * (1.0f / Far.W);
		OutOrigin = Vector3f(Near.X, Near.Y, Near.Z);
		Vector3f RayDir(Far.X - Near.X, Far.Y - Near.Y, Far.Z - Near.Z);
		float Len = FMath::Sqrt(RayDir.X * RayDir.X + RayDir.Y * RayDir.Y + RayDir.Z * RayDir.Z);
		OutDir = (Len > SMALL_NUMBER) ? RayDir * (1.0f / Len) : Vector3f(0, 0, 1);
	}

	bool RenderSceneRenderComp::RayPlaneIntersect(const Vector3f& RayOrigin, const Vector3f& RayDir,
		const Vector3f& PlanePoint, const Vector3f& PlaneNormal, Vector3f& HitPoint)
	{
		float Denom = PlaneNormal.X * RayDir.X + PlaneNormal.Y * RayDir.Y + PlaneNormal.Z * RayDir.Z;
		if (FMath::Abs(Denom) < 0.0001f) return false;
		Vector3f Diff(PlanePoint.X - RayOrigin.X, PlanePoint.Y - RayOrigin.Y, PlanePoint.Z - RayOrigin.Z);
		float T = (PlaneNormal.X * Diff.X + PlaneNormal.Y * Diff.Y + PlaneNormal.Z * Diff.Z) / Denom;
		if (T < 0.0f) return false;
		HitPoint = RayOrigin + RayDir * T;
		return true;
	}

	void RenderSceneRenderComp::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
		if (DragDropOp && DragDropOp->IsOfType<AssetViewItemDragDropOp>())
		{
			TArray<FString> DropFilePaths = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Paths;
			for (const auto& DropFilePath : DropFilePaths)
			{
				MetaType* DropAssetMetaType = GetAssetMetaType(DropFilePath);
				if (DropAssetMetaType && DropAssetMetaType->IsType<Model>())
				{
					DragDropOp->SetCursorOverride(EMouseCursor::GrabHand);
					return;
				}
			}
			DragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);
		}
	}

	void RenderSceneRenderComp::OnDragLeave(const FDragDropEvent& DragDropEvent)
	{
		TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
		if (DragDropOp)
		{
			DragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());
		}
	}

	FReply RenderSceneRenderComp::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SSceneView* SceneViewWidget = ShEditor->GetSceneView();
		if (!SceneViewWidget) return FReply::Unhandled();

		Render* CurRender = SceneViewWidget->GetRender();
		if (!CurRender) return FReply::Unhandled();

		TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
		if (DragDropOp && DragDropOp->IsOfType<AssetViewItemDragDropOp>())
		{
			TArray<FString> DropFilePaths = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Paths;
			bool bDropped = false;
			for (const auto& DropFilePath : DropFilePaths)
			{
				MetaType* DropAssetMetaType = GetAssetMetaType(DropFilePath);
				if (DropAssetMetaType && DropAssetMetaType->IsType<Model>())
				{
					AssetPtr<Model> ModelAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<Model>(DropFilePath);
					auto MeshObj = CurRender->AddSceneObject<MeshSceneObject>();
					MeshObj->ModelAsset = MoveTemp(ModelAsset);
					int32 Index = CurRender->SceneObjects.Num() - 1;
					if (auto* Mgr = SceneViewWidget->GetUndoManager())
					{
						Mgr->PushCommand(MakeShared<AddSceneObjectCommand>(SceneViewWidget, CurRender, MeshObj, Index));
					}
					bDropped = true;
				}
			}
			if (bDropped)
			{
				SceneViewWidget->RefreshSceneItems();
				ShEditor->ForceRender();
				return FReply::Handled();
			}
		}
		return FReply::Unhandled();
	}

	GizmoMode RenderSceneRenderComp::GetCurrentGizmoMode() const
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		return ShEditor->GetGizmoMode();
	}

	GizmoSpace RenderSceneRenderComp::GetCurrentGizmoSpace() const
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		return ShEditor->GetGizmoSpace();
	}

	SceneObject* RenderSceneRenderComp::PickSceneObject(const FVector2D& LocalPos, const FVector2D& ViewportSize) const
	{
		if (!RenderGraphAsset || !ViewPort)
		{
			return nullptr;
		}

		if (ViewportSize.X <= 0 || ViewportSize.Y <= 0)
		{
			return nullptr;
		}

		// Compute ray from camera through pixel
		Vector3f RayOrigin, RayDir;
		ComputeMouseRay(LocalPos, ViewportSize, RayOrigin, RayDir);

		SceneObject* ClosestObj = nullptr;
		float ClosestT = FLT_MAX;

		for (const auto& Obj : RenderGraphAsset->SceneObjects)
		{
			// Compute AABB for the object
			Vector3f BoundsMin, BoundsMax;

			if (MeshSceneObject* MeshObj = dynamic_cast<MeshSceneObject*>(Obj.Get()))
			{
				Model* Mdl = MeshObj->ModelAsset.Get();
				if (!Mdl)
				{
					continue;
				}
				Vector3f Center;
				float Radius;
				Mdl->ComputeBounds(Center, Radius);
				// Use bounding sphere as AABB approximation in local space
				BoundsMin = Center - Vector3f(Radius);
				BoundsMax = Center + Vector3f(Radius);
			}
			else
			{
				// For non-mesh objects (Camera etc.), use a small default AABB
				BoundsMin = Vector3f(-0.3f);
				BoundsMax = Vector3f(0.3f);
			}

			// Transform AABB to world space (compute world-space AABB from 8 corners)
			FMatrix44f WorldMat = Obj->GetWorldMatrix();
			Vector3f WorldMin(FLT_MAX);
			Vector3f WorldMax(-FLT_MAX);
			for (int i = 0; i < 8; ++i)
			{
				Vector3f Corner(
					(i & 1) ? BoundsMax.X : BoundsMin.X,
					(i & 2) ? BoundsMax.Y : BoundsMin.Y,
					(i & 4) ? BoundsMax.Z : BoundsMin.Z
				);
				FVector4f WC = WorldMat.TransformFVector4(FVector4f(Corner.X, Corner.Y, Corner.Z, 1.0f));
				WorldMin.X = FMath::Min(WorldMin.X, WC.X);
				WorldMin.Y = FMath::Min(WorldMin.Y, WC.Y);
				WorldMin.Z = FMath::Min(WorldMin.Z, WC.Z);
				WorldMax.X = FMath::Max(WorldMax.X, WC.X);
				WorldMax.Y = FMath::Max(WorldMax.Y, WC.Y);
				WorldMax.Z = FMath::Max(WorldMax.Z, WC.Z);
			}

			// Ray-AABB intersection (slab method)
			float tMin = -FLT_MAX;
			float tMax = FLT_MAX;
			Vector3f BoxMin = WorldMin;
			Vector3f BoxMax = WorldMax;
			float* RayO = &RayOrigin.X;
			float* RayD = &RayDir.X;
			float* BMin = &BoxMin.X;
			float* BMax = &BoxMax.X;

			bool Hit = true;
			for (int i = 0; i < 3; ++i)
			{
				if (FMath::Abs(RayD[i]) < SMALL_NUMBER)
				{
					if (RayO[i] < BMin[i] || RayO[i] > BMax[i])
					{
						Hit = false;
						break;
					}
				}
				else
				{
					float InvD = 1.0f / RayD[i];
					float t1 = (BMin[i] - RayO[i]) * InvD;
					float t2 = (BMax[i] - RayO[i]) * InvD;
					if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
					tMin = FMath::Max(tMin, t1);
					tMax = FMath::Min(tMax, t2);
					if (tMin > tMax)
					{
						Hit = false;
						break;
					}
				}
			}

			if (Hit && tMax >= 0 && tMin < ClosestT)
			{
				ClosestT = tMin >= 0 ? tMin : tMax;
				ClosestObj = Obj.Get();
			}
		}

		return ClosestObj;
	}
}
