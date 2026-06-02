#include "CommonHeader.h"
#include "RenderSceneRenderComp.h"
#include "GpuApi/GpuRhi.h"
#include "Editor/ShaderHelperEditor.h"
#include "Editor/SceneViewCommands.h"
#include "UI/Widgets/Scene/SSceneView.h"
#include "UI/Widgets/Scene/SceneUndoManager.h"
#include "UI/Widgets/AssetBrowser/AssetViewItem/AssetViewItem.h"
#include "AssetObject/Render/MeshSceneObject.h"
#include "AssetObject/Render/CameraSceneObject.h"
#include "AssetObject/Model.h"
#include "AssetManager/AssetManager.h"
#include "ProjectManager/ShProjectManager.h"

using namespace FW;

namespace SH
{
	static Vector3f GetSceneObjectWorldPosition(const SceneObject* Obj)
	{
		const FVector3f Origin = Obj->GetWorldMatrix().GetOrigin();
		return Vector3f(Origin.X, Origin.Y, Origin.Z);
	}

	static FMatrix44f GetSceneObjectWorldRotationMatrix(const SceneObject* Obj)
	{
		if (!Obj)
		{
			return FMatrix44f::Identity;
		}

		FMatrix44f WorldRotation = Obj->GetWorldMatrix().GetMatrixWithoutScale();
		WorldRotation.SetOrigin(FVector3f::ZeroVector);
		return WorldRotation;
	}

	static bool HasSelectedAncestor(const TArray<SceneObject*>& SelectedObjects, const SceneObject* Obj)
	{
		for (const SceneObject* Parent = Obj ? Obj->GetParent() : nullptr; Parent; Parent = Parent->GetParent())
		{
			if (SelectedObjects.Contains(const_cast<SceneObject*>(Parent)))
			{
				return true;
			}
		}
		return false;
	}

	static TArray<SceneObject*> GetTransformSelectionRoots(const TArray<SceneObject*>& SelectedObjects)
	{
		TArray<SceneObject*> RootObjects;
		for (SceneObject* Obj : SelectedObjects)
		{
			if (!HasSelectedAncestor(SelectedObjects, Obj))
			{
				RootObjects.Add(Obj);
			}
		}
		return RootObjects;
	}

	static Vector3f WorldPositionToLocalPosition(const SceneObject* Obj, const Vector3f& WorldPosition)
	{
		if (const SceneObject* Parent = Obj->GetParent())
		{
			const FVector4f LocalPosition = Parent->GetWorldMatrix().Inverse().TransformFVector4(
				FVector4f(WorldPosition.X, WorldPosition.Y, WorldPosition.Z, 1.0f));
			return Vector3f(LocalPosition.X, LocalPosition.Y, LocalPosition.Z);
		}

		return WorldPosition;
	}

	static Vector3f WorldDirectionToParentLocalDirection(const SceneObject* Obj, const Vector3f& WorldDirection)
	{
		if (const SceneObject* Parent = Obj ? Obj->GetParent() : nullptr)
		{
			const FMatrix44f ParentWorldRotation = GetSceneObjectWorldRotationMatrix(Parent);
			const FVector4f LocalDirection = ParentWorldRotation.Inverse().TransformFVector4(
				FVector4f(WorldDirection.X, WorldDirection.Y, WorldDirection.Z, 0.0f));
			Vector3f Result(LocalDirection.X, LocalDirection.Y, LocalDirection.Z);
			const float LenSq = Result.X * Result.X + Result.Y * Result.Y + Result.Z * Result.Z;
			if (LenSq > SMALL_NUMBER)
			{
				Result = Result * (1.0f / FMath::Sqrt(LenSq));
			}
			return Result;
		}

		return WorldDirection;
	}

	RenderSceneRenderComp::RenderSceneRenderComp(Render* InRenderGraph, PreviewViewPort* InViewPort)
		: RenderGraphAsset(InRenderGraph)
		, ViewPort(InViewPort)
	{
		PreviewCamera.Position = {0, 3, -7};
		PreviewCamera.Pitch = -0.5f;
		PreviewCamera.VerticalFov = FMath::DegreesToRadians(60.0f);
		PreviewCamera.NearPlane = 0.1f;
		PreviewCamera.FarPlane = 100.0f;
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

		// Register gizmo mode switching commands
		SceneCommandList = MakeShared<FUICommandList>();
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SceneCommandList->MapAction(
			SceneViewCommands::Get().GizmoMove,
			FExecuteAction::CreateLambda([ShEditor] { ShEditor->SetGizmoMode(GizmoMode::Move); })
		);
		SceneCommandList->MapAction(
			SceneViewCommands::Get().GizmoRotate,
			FExecuteAction::CreateLambda([ShEditor] { ShEditor->SetGizmoMode(GizmoMode::Rotate); })
		);
		SceneCommandList->MapAction(
			SceneViewCommands::Get().GizmoScale,
			FExecuteAction::CreateLambda([ShEditor] { ShEditor->SetGizmoMode(GizmoMode::Scale); })
		);
		SceneCommandList->MapAction(
			SceneViewCommands::Get().DeleteObject,
			FExecuteAction::CreateLambda([ShEditor] {
				ShEditor->GetSceneView()->DeleteSelected();
			}),
			FCanExecuteAction::CreateLambda([ShEditor] {
				return !ShEditor->GetSceneView()->GetSelectedObjects().IsEmpty();
			})
		);

		ViewPort->DrawOverlayHandler = [this](const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled)
		{
			return DrawViewportOverlay(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
		};

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
		ViewPort->DrawOverlayHandler = {};
	}

	bool RenderSceneRenderComp::IsScenePreviewActive() const
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		return ShEditor->IsScenePreview();
	}

	CameraSceneObject* RenderSceneRenderComp::GetGraphPreviewCameraObject() const
	{
		return RenderGraphAsset ? RenderGraphAsset->PreviewCamera.Get() : nullptr;
	}

	bool RenderSceneRenderComp::IsCameraControlActive() const
	{
		return IsScenePreviewActive() || GetGraphPreviewCameraObject() != nullptr;
	}

	void RenderSceneRenderComp::SyncPreviewCameraFromGraphCamera()
	{
		CameraSceneObject* CamObj = GetGraphPreviewCameraObject();
		if (!CamObj)
		{
			return;
		}
		PreviewCamera = CamObj->ToCamera(PreviewCamera.AspectRatio);
	}

	void RenderSceneRenderComp::SyncGraphCameraFromPreviewCamera()
	{
		CameraSceneObject* CamObj = GetGraphPreviewCameraObject();
		if (!CamObj)
		{
			return;
		}
		CamObj->Position = PreviewCamera.Position;
		CamObj->Rotation.X = FMath::RadiansToDegrees(PreviewCamera.Pitch);
		CamObj->Rotation.Y = FMath::RadiansToDegrees(PreviewCamera.Yaw);
		CamObj->Rotation.Z = FMath::RadiansToDegrees(PreviewCamera.Roll);
	}

	void RenderSceneRenderComp::ResetScenePreviewInteractionState()
	{
		bRightMouseDown = false;
		bMiddleMouseDown = false;
		bKeyW = false;
		bKeyA = false;
		bKeyS = false;
		bKeyD = false;
		bKeyQ = false;
		bKeyE = false;
		HoveredAxis = GizmoAxis::None;
		DraggingAxis = GizmoAxis::None;
		DragAllObjectStates.Empty();
		PendingPickedObject = nullptr;
		bPendingBoxSelection = false;
		bBoxSelecting = false;
		bBoxSelectAdditive = false;
		DragLocalAxisDir = Vector3f(0, 0, 0);
		DragPivotStart = Vector3f(0, 0, 0);
		DragStartMousePos = FVector2D(0, 0);
		DragAxisScreenDir = FVector2D(0, 0);
		DragPixelsPerUnit = 1.0f;
		DragPlaneNormal = Vector3f(0, 1, 0);
		DragPlaneHitStart = Vector3f(0, 0, 0);
	}

	FReply RenderSceneRenderComp::OnMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (!IsCameraControlActive())
		{
			ResetScenePreviewInteractionState();
			return FReply::Unhandled();
		}

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
		if (IsScenePreviewActive() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
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
					DragStartMousePos = LocalPos;

					// Snapshot all selected objects for multi-object drag.
					DragAllObjectStates.Empty();
					const TArray<SceneObject*> TransformObjects = GetTransformSelectionRoots(GetSceneViewWidget()->GetSelectedObjects());
					for (SceneObject* Obj : TransformObjects)
					{
						DragObjectState State;
						State.Object = Obj;
						State.StartPos = Obj->Position;
						State.StartWorldPos = GetSceneObjectWorldPosition(Obj);
						State.StartRotation = Obj->Rotation;
						State.StartScale = Obj->Scale;
						DragAllObjectStates.Add(MoveTemp(State));
					}
					DragPivotStart = GetGizmoPivot();
					Vector3f GizmoPivot = DragPivotStart;

					if (DraggingAxis == GizmoAxis::All)
					{
						// Uniform scale: use camera-right as drag reference direction
						float GizScale = GetGizmoScale(GizmoPivot);
						FMatrix44f CamRotMat = PreviewCamera.GetWorldRotationMatrix();
						Vector3f CamRight(CamRotMat.M[0][0], CamRotMat.M[0][1], CamRotMat.M[0][2]);
						FVector2D CenterScreen = WorldToScreen(GizmoPivot, VpSize);
						FVector2D EndScreen = WorldToScreen(GizmoPivot + CamRight * GizScale, VpSize);
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
						if (RayPlaneIntersect(RayOrigin, RayDir, GizmoPivot, DragPlaneNormal, HitPt))
							DragPlaneHitStart = HitPt;
						else
							DragPlaneHitStart = GizmoPivot;
						// DragPixelsPerUnit doesn't matter for plane drag; set sentinel
						DragPixelsPerUnit = 1.0f;
					}
					else
					{
						FMatrix44f OrientMat = GetGizmoOrientationMatrix(SelObj);
						Vector3f AxisDir = GetOrientedAxisDir(DraggingAxis, OrientMat);
						DragLocalAxisDir = AxisDir;

						// Compute screen-space axis direction and pixels-per-unit
						FVector2D CenterScreen = WorldToScreen(GizmoPivot, VpSize);
						float GizScale = GetGizmoScale(GizmoPivot);
						FVector2D EndScreen = WorldToScreen(GizmoPivot + AxisDir * GizScale, VpSize);
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
									Vector3f ViewDir = GizmoPivot - PreviewCamera.Position;
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

			PendingPickedObject = PickSceneObject(LocalPos, VpSize);
			bPendingBoxSelection = true;
			bBoxSelecting = false;
			bBoxSelectAdditive = MouseEvent.IsShiftDown();
			BoxSelectionStart = LocalPos;
			BoxSelectionEnd = LocalPos;
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	FReply RenderSceneRenderComp::OnMouseUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (!IsCameraControlActive())
		{
			ResetScenePreviewInteractionState();
			return FReply::Unhandled();
		}

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
		if (IsScenePreviewActive() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			if (DraggingAxis != GizmoAxis::None)
			{
				SSceneView* SceneViewWidget = GetSceneViewWidget();
				if (SceneViewWidget->GetRender())
				{
					SceneUndoManager::ScopedTransaction Transaction(SceneViewWidget->GetUndoManager());
					for (auto& State : DragAllObjectStates)
					{
						SceneObject* Obj = State.Object.Get();
						bool bChanged = (Obj->Position != State.StartPos) ||
										(Obj->Rotation != State.StartRotation) ||
										(Obj->Scale != State.StartScale);
						if (bChanged)
						{
							SceneViewWidget->GetUndoManager().DoCommand(MakeShared<TransformCommand>(
								SceneViewWidget,
								Obj,
								State.StartPos, State.StartRotation, State.StartScale,
								Obj->Position, Obj->Rotation, Obj->Scale
							));
						}
					}
				}
				DragAllObjectStates.Empty();
				PendingPickedObject = nullptr;
			}
			else if (bPendingBoxSelection)
			{
				SSceneView* SceneViewWidget = GetSceneViewWidget();
				if (bBoxSelecting)
				{
					TArray<SceneObject*> PickedObjects = PickSceneObjectsInRect(BoxSelectionStart, BoxSelectionEnd, MyGeometry.GetLocalSize());
					SceneViewWidget->SelectObjects(PickedObjects, bBoxSelectAdditive);
				}
				else
				{
					SceneObject* Picked = PendingPickedObject.Get();
					if (bBoxSelectAdditive && Picked)
					{
						SceneViewWidget->SelectObject(Picked, /*bAdditive=*/true);
					}
					else
					{
						SceneViewWidget->SelectObject(Picked);
					}
				}

				PendingPickedObject = nullptr;
				bPendingBoxSelection = false;
				bBoxSelecting = false;
			}
			DraggingAxis = GizmoAxis::None;
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	FReply RenderSceneRenderComp::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (!IsCameraControlActive())
		{
			ResetScenePreviewInteractionState();
			return FReply::Unhandled();
		}

		if (bRightMouseDown)
		{
			FVector2D CurrentPos = MouseEvent.GetScreenSpacePosition();
			FVector2D Delta = CurrentPos - LastMousePos;
			LastMousePos = CurrentPos;

			SyncPreviewCameraFromGraphCamera();
			PreviewCamera.Yaw += (float)Delta.X * CameraRotateSpeed;
			PreviewCamera.Pitch -= (float)Delta.Y * CameraRotateSpeed;
			PreviewCamera.Pitch = FMath::Clamp(PreviewCamera.Pitch, -PI * 0.49f, PI * 0.49f);
			SyncGraphCameraFromPreviewCamera();
			return FReply::Handled();
		}

		if (bMiddleMouseDown)
		{
			FVector2D CurrentPos = MouseEvent.GetScreenSpacePosition();
			FVector2D Delta = CurrentPos - LastMousePos;
			LastMousePos = CurrentPos;

			SyncPreviewCameraFromGraphCamera();
			FMatrix44f RotMat = PreviewCamera.GetWorldRotationMatrix();
			Vector3f Right = Vector3f(RotMat.M[0][0], RotMat.M[0][1], RotMat.M[0][2]);
			Vector3f Up = Vector3f(RotMat.M[1][0], RotMat.M[1][1], RotMat.M[1][2]);

			PreviewCamera.Position -= Right * (float)Delta.X * CameraPanSpeed;
			PreviewCamera.Position += Up * (float)Delta.Y * CameraPanSpeed;
			SyncGraphCameraFromPreviewCamera();
			return FReply::Handled();
		}

		if (!IsCameraControlActive())
		{
			return FReply::Unhandled();
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
				FMatrix44f OrientMat = GetGizmoOrientationMatrix(SelObj);

				// Plane drag: use ray-plane intersection directly (bypasses screen-space projection)
				if (DraggingAxis == GizmoAxis::XY || DraggingAxis == GizmoAxis::XZ || DraggingAxis == GizmoAxis::YZ)
				{
					Vector3f RayOrigin, RayDir;
					ComputeMouseRay(LocalPos, VpSize, RayOrigin, RayDir);
					Vector3f HitPt;
					if (RayPlaneIntersect(RayOrigin, RayDir, DragPlaneHitStart, DragPlaneNormal, HitPt))
					{
						Vector3f PlaneDelta = HitPt - DragPlaneHitStart;
						GizmoAxis A, B;
						if      (DraggingAxis == GizmoAxis::XY) { A = GizmoAxis::X; B = GizmoAxis::Y; }
						else if (DraggingAxis == GizmoAxis::XZ) { A = GizmoAxis::X; B = GizmoAxis::Z; }
						else                                    { A = GizmoAxis::Y; B = GizmoAxis::Z; }
						Vector3f DirA = GetOrientedAxisDir(A, OrientMat);
						Vector3f DirB = GetOrientedAxisDir(B, OrientMat);
						float dA = PlaneDelta.X * DirA.X + PlaneDelta.Y * DirA.Y + PlaneDelta.Z * DirA.Z;
						float dB = PlaneDelta.X * DirB.X + PlaneDelta.Y * DirB.Y + PlaneDelta.Z * DirB.Z;
						Vector3f MoveOffset = DirA * dA + DirB * dB;
						for (auto& State : DragAllObjectStates)
						{
							Vector3f NewWorldPos = State.StartWorldPos + MoveOffset;
							State.Object->Position = WorldPositionToLocalPosition(State.Object.Get(), NewWorldPos);
							State.Object->GetOuterMost()->MarkDirty();
						}
					}
					return FReply::Handled();
				}

				FVector2D MouseDelta = LocalPos - DragStartMousePos;
				double Projected = MouseDelta.X * DragAxisScreenDir.X + MouseDelta.Y * DragAxisScreenDir.Y;
				float WorldDelta = (float)(Projected / DragPixelsPerUnit);

				GizmoMode Mode = GetCurrentGizmoMode();

				if (Mode == GizmoMode::Move)
				{
					Vector3f AxisDir = GetOrientedAxisDir(DraggingAxis, OrientMat);
					Vector3f MoveOffset = AxisDir * WorldDelta;
					for (auto& State : DragAllObjectStates)
					{
						Vector3f NewWorldPos = State.StartWorldPos + MoveOffset;
						State.Object->Position = WorldPositionToLocalPosition(State.Object.Get(), NewWorldPos);
						State.Object->GetOuterMost()->MarkDirty();
					}
				}
				else if (Mode == GizmoMode::Rotate)
				{
					float AngleDelta = WorldDelta * 90.0f;
					float AngleRad = FMath::DegreesToRadians(AngleDelta);
					float AxisLenSq = DragLocalAxisDir.X * DragLocalAxisDir.X +
						DragLocalAxisDir.Y * DragLocalAxisDir.Y +
						DragLocalAxisDir.Z * DragLocalAxisDir.Z;
					Vector3f AxisDir = AxisLenSq > SMALL_NUMBER
						? DragLocalAxisDir * (1.0f / FMath::Sqrt(AxisLenSq))
						: Vector3f(0, 0, 1);
					float CosAngle = FMath::Cos(AngleRad);
					float SinAngle = FMath::Sin(AngleRad);

					for (auto& State : DragAllObjectStates)
					{
						Vector3f ParentLocalAxis = WorldDirectionToParentLocalDirection(State.Object.Get(), AxisDir);
						FVector3f UeAxis(ParentLocalAxis.Z, ParentLocalAxis.X, ParentLocalAxis.Y);
						FQuat4f DeltaQuat(UeAxis, AngleRad);
						FRotator3f StartRotator(State.StartRotation.X, State.StartRotation.Y, State.StartRotation.Z);
						FQuat4f StartQuat = StartRotator.Quaternion();
						FQuat4f NewQuat = DeltaQuat * StartQuat;
						NewQuat.Normalize();
						FRotator3f NewRotator = NewQuat.Rotator();

						Vector3f StartOffset = State.StartWorldPos - DragPivotStart;
						float AxisDotOffset = StartOffset.X * AxisDir.X + StartOffset.Y * AxisDir.Y + StartOffset.Z * AxisDir.Z;
						Vector3f AxisCrossOffset(
							AxisDir.Y * StartOffset.Z - AxisDir.Z * StartOffset.Y,
							AxisDir.Z * StartOffset.X - AxisDir.X * StartOffset.Z,
							AxisDir.X * StartOffset.Y - AxisDir.Y * StartOffset.X);
						Vector3f RotatedOffset =
							StartOffset * CosAngle +
							AxisCrossOffset * SinAngle +
							AxisDir * (AxisDotOffset * (1.0f - CosAngle));

						Vector3f NewWorldPos = DragPivotStart + RotatedOffset;
						State.Object->Position = WorldPositionToLocalPosition(State.Object.Get(), NewWorldPos);
						State.Object->Rotation = Vector3f(NewRotator.Pitch, NewRotator.Yaw, NewRotator.Roll);
						State.Object->GetOuterMost()->MarkDirty();
					}
				}
				else if (Mode == GizmoMode::Scale)
				{
					float ScaleFactor = FMath::Max(1.0f + WorldDelta, 0.01f);
					Vector3f ScaleAxisDir(0, 0, 0);
					if (DraggingAxis != GizmoAxis::All)
					{
						ScaleAxisDir = GetOrientedAxisDir(DraggingAxis, OrientMat);
						float AxisLenSq = ScaleAxisDir.X * ScaleAxisDir.X +
							ScaleAxisDir.Y * ScaleAxisDir.Y +
							ScaleAxisDir.Z * ScaleAxisDir.Z;
						if (AxisLenSq > SMALL_NUMBER)
						{
							ScaleAxisDir = ScaleAxisDir * (1.0f / FMath::Sqrt(AxisLenSq));
						}
					}

					for (auto& State : DragAllObjectStates)
					{
						Vector3f NewScale = State.StartScale;
						if (DraggingAxis == GizmoAxis::All) NewScale = State.StartScale * ScaleFactor;
						else if (DraggingAxis == GizmoAxis::X) NewScale.X *= ScaleFactor;
						else if (DraggingAxis == GizmoAxis::Y) NewScale.Y *= ScaleFactor;
						else NewScale.Z *= ScaleFactor;

						Vector3f StartOffset = State.StartWorldPos - DragPivotStart;
						Vector3f ScaledOffset = StartOffset;
						if (DraggingAxis == GizmoAxis::All)
						{
							ScaledOffset = StartOffset * ScaleFactor;
						}
						else
						{
							float AxisOffset = StartOffset.X * ScaleAxisDir.X +
								StartOffset.Y * ScaleAxisDir.Y +
								StartOffset.Z * ScaleAxisDir.Z;
							ScaledOffset = StartOffset + ScaleAxisDir * (AxisOffset * (ScaleFactor - 1.0f));
						}

						Vector3f NewWorldPos = DragPivotStart + ScaledOffset;
						State.Object->Position = WorldPositionToLocalPosition(State.Object.Get(), NewWorldPos);
						State.Object->Scale = NewScale;
						State.Object->GetOuterMost()->MarkDirty();
					}
				}
			}
			return FReply::Handled();
		}

		if (bPendingBoxSelection)
		{
			BoxSelectionEnd = LocalPos;
			FVector2D DragDelta = BoxSelectionEnd - BoxSelectionStart;
			if (!bBoxSelecting)
			{
				double DragLenSq = DragDelta.X * DragDelta.X + DragDelta.Y * DragDelta.Y;
				if (DragLenSq >= BoxSelectionStartThreshold * BoxSelectionStartThreshold)
				{
					bBoxSelecting = true;
				}
			}
			return FReply::Handled();
		}

		// Update hovered axis for highlight
		HoveredAxis = HitTestGizmo(LocalPos, VpSize);
		return FReply::Unhandled();
	}

	int32 RenderSceneRenderComp::DrawViewportOverlay(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
	{
		if (!IsCameraControlActive() || !bBoxSelecting)
		{
			return LayerId;
		}

		FVector2D RectMin(FMath::Min(BoxSelectionStart.X, BoxSelectionEnd.X), FMath::Min(BoxSelectionStart.Y, BoxSelectionEnd.Y));
		FVector2D RectMax(FMath::Max(BoxSelectionStart.X, BoxSelectionEnd.X), FMath::Max(BoxSelectionStart.Y, BoxSelectionEnd.Y));
		FVector2D RectSize = RectMax - RectMin;

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(RectMin, RectSize),
			FAppStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			FLinearColor(0.15f, 0.45f, 0.95f, 0.12f));

		TArray<FVector2D> LinePoints;
		LinePoints.Add(RectMin);
		LinePoints.Add(FVector2D(RectMax.X, RectMin.Y));
		LinePoints.Add(RectMax);
		LinePoints.Add(FVector2D(RectMin.X, RectMax.Y));
		LinePoints.Add(RectMin);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry.ToPaintGeometry(),
			LinePoints,
			ESlateDrawEffect::None,
			FLinearColor(0.15f, 0.55f, 1.0f, 0.95f),
			true,
			1.0f);

		return LayerId + 2;
	}

	FReply RenderSceneRenderComp::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (!IsCameraControlActive())
		{
			ResetScenePreviewInteractionState();
			return FReply::Unhandled();
		}

		float WheelDelta = MouseEvent.GetWheelDelta();
		SyncPreviewCameraFromGraphCamera();
		FMatrix44f RotMat = PreviewCamera.GetWorldRotationMatrix();
		Vector3f Forward = Vector3f(RotMat.M[2][0], RotMat.M[2][1], RotMat.M[2][2]);
		PreviewCamera.Position += Forward * WheelDelta * CameraScrollSpeed;
		SyncGraphCameraFromPreviewCamera();
		return FReply::Handled();
	}

	void RenderSceneRenderComp::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
	{
		if (!IsCameraControlActive())
		{
			ResetScenePreviewInteractionState();
			return;
		}

		if (IsScenePreviewActive() && !bRightMouseDown)
		{
			if (SceneCommandList->ProcessCommandBindings(KeyEvent))
			{
				return;
			}
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
		if (!IsCameraControlActive())
		{
			ResetScenePreviewInteractionState();
			return;
		}

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

		SyncPreviewCameraFromGraphCamera();
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
			SyncGraphCameraFromPreviewCamera();
		}
	}

	void RenderSceneRenderComp::RenderBegin()
	{
		if (!IsScenePreviewActive())
		{
			GraphComp->RenderBegin();
		}
	}

	void RenderSceneRenderComp::RenderInternal()
	{
		double CurrentTime = FPlatformTime::Seconds();
		float DeltaTime = (float)(CurrentTime - LastFrameTime);
		LastFrameTime = CurrentTime;
		DeltaTime = FMath::Clamp(DeltaTime, 0.0f, 0.1f);

		if (IsScenePreviewActive())
		{
			UpdateCamera(DeltaTime);
			RenderPreview();
		}
		else
		{
			if (GetGraphPreviewCameraObject())
			{
				UpdateCamera(DeltaTime);
			}
			else
			{
				ResetScenePreviewInteractionState();
			}

			//Camera control
			if (bRightMouseDown)
			{
				ScopedTimelineResume TimelineResumeScope;
				RenderGraph();
			}
			else
			{
				RenderGraph();
			}
		}
	}

	void RenderSceneRenderComp::RenderEnd()
	{
		if (!IsScenePreviewActive())
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

		SyncPreviewCameraFromGraphCamera();

		PreviewRenderer.InitResources();

		FIntPoint ViewSize = ViewPort->GetSize();
		if (ViewSize.X == 0 || ViewSize.Y == 0)
		{
			return;
		}

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

		//GGpuRhi->BeginGpuCapture("RenderPreview");

		FW::RenderGraph Graph;

		// Render meshes first; this always clears color + depth even when the scene has no meshes.
		PreviewRenderer.RenderMeshes(Graph, MsaaRTV, DepthDSV, PreviewCamera, RenderGraphAsset->SceneObjects);

		// Render grid with depth test
		PreviewRenderer.RenderGrid(Graph, MsaaRTV, DepthDSV, PreviewCamera);

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
			Vector3f GizmoPivot = GetGizmoPivot();
			PreviewRenderer.RenderGizmo(Graph, MsaaRTV, PreviewCamera, GizmoPivot, Highlight, Mode, OrientMat);
		}

		// Resolve MSAA to FinalRT
		PreviewRenderer.ResolveMsaa(Graph, MsaaRTV, FinalRTV);

		// Render outlines for all selected meshes (post-process on resolved FinalRT)
		TArray<SceneObject*> SelectedObjects = GetSceneViewWidget()->GetSelectedObjects();
		if (!SelectedObjects.IsEmpty())
		{
			PreviewRenderer.RenderOutline(Graph, FinalRTV, MaskRTV, PreviewCamera, SelectedObjects);
		}

		// Render camera preview overlay when a camera is selected
		if (SelCam)
		{
			Camera SelCamera = SelCam->ToCamera((float)ViewSize.X / (float)ViewSize.Y);
			PreviewRenderer.RenderCameraPreview(Graph, FinalRTV, SelCamera,
				RenderGraphAsset->SceneObjects, (uint32)ViewSize.X, (uint32)ViewSize.Y);
		}

		Graph.Execute();

		//GGpuRhi->EndGpuCapture();
	}

	void RenderSceneRenderComp::RenderGraph()
	{
		GraphComp->RenderInternal();
	}

	SSceneView* RenderSceneRenderComp::GetSceneViewWidget() const
	{
		auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		return ShEditor->GetSceneView();
	}

	SceneObject* RenderSceneRenderComp::GetSelectedSceneObject() const
	{
		TArray<SceneObject*> RootObjects = GetTransformSelectionRoots(GetSceneViewWidget()->GetSelectedObjects());
		return RootObjects.IsEmpty() ? nullptr : RootObjects[0];
	}

	FW::Vector3f RenderSceneRenderComp::GetGizmoPivot() const
	{
		TArray<SceneObject*> Objs = GetTransformSelectionRoots(GetSceneViewWidget()->GetSelectedObjects());
		if (Objs.IsEmpty()) return Vector3f(0, 0, 0);
		Vector3f Sum(0, 0, 0);
		for (SceneObject* Obj : Objs)
		{
			Sum += GetSceneObjectWorldPosition(Obj);
		}
		return Sum * (1.0f / (float)Objs.Num());
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

		// Gizmo is suppressed for the graph's preview camera, so don't hit-test it either.
		if (CameraSceneObject* SelCam = dynamic_cast<CameraSceneObject*>(SelObj))
		{
			if (SelCam == GetGraphPreviewCameraObject())
			{
				return GizmoAxis::None;
			}
		}

		Vector3f Center = GetGizmoPivot();
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
			return GetSceneObjectWorldRotationMatrix(Obj);
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
		if (!IsScenePreviewActive())
		{
			ResetScenePreviewInteractionState();
			return;
		}

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
		if (!IsScenePreviewActive())
		{
			ResetScenePreviewInteractionState();
			return;
		}

		TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
		if (DragDropOp)
		{
			DragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());
		}
	}

	FReply RenderSceneRenderComp::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		if (!IsScenePreviewActive())
		{
			ResetScenePreviewInteractionState();
			return FReply::Unhandled();
		}

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SSceneView* SceneViewWidget = GetSceneViewWidget();

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
					MeshObj->ObjectName = ModelAsset->ObjectName;
					MeshObj->ModelAsset = MoveTemp(ModelAsset);
					int32 Index = CurRender->SceneObjects.Num() - 1;
					SceneViewWidget->GetUndoManager().PushCommand(MakeShared<AddSceneObjectCommand>(SceneViewWidget, CurRender, MeshObj, Index));
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

	TArray<SceneObject*> RenderSceneRenderComp::PickSceneObjectsInRect(const FVector2D& RectStart, const FVector2D& RectEnd, const FVector2D& ViewportSize) const
	{
		TArray<SceneObject*> Result;
		if (!RenderGraphAsset || ViewportSize.X <= 0 || ViewportSize.Y <= 0)
		{
			return Result;
		}

		CameraSceneObject* GraphPreviewCam = GetGraphPreviewCameraObject();
		FVector2D RectMin(FMath::Min(RectStart.X, RectEnd.X), FMath::Min(RectStart.Y, RectEnd.Y));
		FVector2D RectMax(FMath::Max(RectStart.X, RectEnd.X), FMath::Max(RectStart.Y, RectEnd.Y));
		for (const auto& Obj : RenderGraphAsset->SceneObjects)
		{
			if (Obj.Get() == GraphPreviewCam)
			{
				continue;
			}
			FVector2D ObjMin, ObjMax;
			if (GetSceneObjectScreenRect(Obj.Get(), ViewportSize, ObjMin, ObjMax) &&
				ObjMax.X >= RectMin.X && ObjMin.X <= RectMax.X &&
				ObjMax.Y >= RectMin.Y && ObjMin.Y <= RectMax.Y)
			{
				Result.Add(Obj.Get());
			}
		}
		return Result;
	}

	bool RenderSceneRenderComp::GetSceneObjectScreenRect(SceneObject* Obj, const FVector2D& ViewportSize, FVector2D& OutMin, FVector2D& OutMax) const
	{
		Vector3f BoundsMin, BoundsMax;
		if (MeshSceneObject* MeshObj = dynamic_cast<MeshSceneObject*>(Obj))
		{
			if (Model* Mdl = MeshObj->ModelAsset.Get())
			{
				Vector3f Center;
				float Radius;
				Mdl->ComputeBounds(Center, Radius);
				BoundsMin = Center - Vector3f(Radius);
				BoundsMax = Center + Vector3f(Radius);
			}
			else
			{
				BoundsMin = Vector3f(-0.3f);
				BoundsMax = Vector3f(0.3f);
			}
		}
		else
		{
			BoundsMin = Vector3f(-0.3f);
			BoundsMax = Vector3f(0.3f);
		}

		FMatrix44f WorldMat = Obj->GetWorldMatrix();
		OutMin = FVector2D(FLT_MAX, FLT_MAX);
		OutMax = FVector2D(-FLT_MAX, -FLT_MAX);
		bool bHasValidPoint = false;
		for (int i = 0; i < 8; ++i)
		{
			Vector3f Corner(
				(i & 1) ? BoundsMax.X : BoundsMin.X,
				(i & 2) ? BoundsMax.Y : BoundsMin.Y,
				(i & 4) ? BoundsMax.Z : BoundsMin.Z);
			FVector4f WorldCorner = WorldMat.TransformFVector4(FVector4f(Corner.X, Corner.Y, Corner.Z, 1.0f));
			FVector2D ScreenPoint = WorldToScreen(Vector3f(WorldCorner.X, WorldCorner.Y, WorldCorner.Z), ViewportSize);
			if (ScreenPoint.X <= -9999.0f || ScreenPoint.Y <= -9999.0f)
			{
				continue;
			}

			bHasValidPoint = true;
			OutMin.X = FMath::Min(OutMin.X, ScreenPoint.X);
			OutMin.Y = FMath::Min(OutMin.Y, ScreenPoint.Y);
			OutMax.X = FMath::Max(OutMax.X, ScreenPoint.X);
			OutMax.Y = FMath::Max(OutMax.Y, ScreenPoint.Y);
		}
		return bHasValidPoint;
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

		CameraSceneObject* GraphPreviewCam = GetGraphPreviewCameraObject();
		for (const auto& Obj : RenderGraphAsset->SceneObjects)
		{
			if (Obj.Get() == GraphPreviewCam)
			{
				continue;
			}

			// Compute AABB for the object
			Vector3f BoundsMin, BoundsMax;

			if (MeshSceneObject* MeshObj = dynamic_cast<MeshSceneObject*>(Obj.Get()))
			{
				if (Model* Mdl = MeshObj->ModelAsset.Get())
				{
					Vector3f Center;
					float Radius;
					Mdl->ComputeBounds(Center, Radius);
					// Use bounding sphere as AABB approximation in local space
					BoundsMin = Center - Vector3f(Radius);
					BoundsMax = Center + Vector3f(Radius);
				}
				else
				{
					// MeshSceneObject without a model (uses DrawPrimitive(VertexCount))
					BoundsMin = Vector3f(-0.3f);
					BoundsMax = Vector3f(0.3f);
				}
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
