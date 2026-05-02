#pragma once
#include "RenderRenderComp.h"
#include "ScenePreviewRenderer.h"
#include "RenderResource/Camera.h"

namespace SH
{
	class SceneObject;
	class SSceneView;
	enum class GizmoMode : int32;
	enum class GizmoSpace : int32;

	enum class GizmoAxis : int32
	{
		None = 0,
		X = 1,
		Y = 2,
		Z = 3,
		All = 4, // Uniform scale
		XY = 5,  // Move on XY plane
		XZ = 6,  // Move on XZ plane
		YZ = 7,  // Move on YZ plane
	};

	class RenderSceneRenderComp : public FW::RenderComponent
	{
	public:
		RenderSceneRenderComp(Render* InRenderGraph, FW::PreviewViewPort* InViewPort);
		~RenderSceneRenderComp() override;

	public:
		void RenderBegin() override;
		void RenderInternal() override;
		void RenderEnd() override;

	private:
		void RenderPreview();
		void RenderGraph();
		bool IsScenePreviewActive() const;
		void ResetScenePreviewInteractionState();

		FReply OnMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
		FReply OnMouseUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
		FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
		FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
		void OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent);
		void OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent);
		void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent);
		void OnDragLeave(const FDragDropEvent& DragDropEvent);
		FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent);
		void UpdateCamera(float DeltaTime);
		int32 DrawViewportOverlay(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const;

		// Gizmo helpers
		SSceneView* GetSceneViewWidget() const;
		SceneObject* GetSelectedSceneObject() const;
		FW::Vector3f GetGizmoPivot() const;
		FVector2D WorldToScreen(const FW::Vector3f& WorldPos, const FVector2D& ViewportSize) const;
		float PointToSegmentDistSq(const FVector2D& P, const FVector2D& A, const FVector2D& B) const;
		GizmoAxis HitTestGizmo(const FVector2D& LocalMousePos, const FVector2D& ViewportSize) const;
		float GetGizmoScale(const FW::Vector3f& GizmoPos) const;
		FMatrix44f GetGizmoOrientationMatrix(SceneObject* Obj) const;
		FW::Vector3f GetOrientedAxisDir(GizmoAxis Axis, const FMatrix44f& Orientation) const;
		FW::Vector3f GetPlaneNormal(GizmoAxis PlaneAxis, const FMatrix44f& Orientation) const;
		void ComputeMouseRay(const FVector2D& LocalPos, const FVector2D& ViewportSize, FW::Vector3f& OutOrigin, FW::Vector3f& OutDir) const;
		static bool RayPlaneIntersect(const FW::Vector3f& RayOrigin, const FW::Vector3f& RayDir, const FW::Vector3f& PlanePoint, const FW::Vector3f& PlaneNormal, FW::Vector3f& HitPoint);
		GizmoMode GetCurrentGizmoMode() const;
		GizmoSpace GetCurrentGizmoSpace() const;

		// Picking
		SceneObject* PickSceneObject(const FVector2D& LocalPos, const FVector2D& ViewportSize) const;
		TArray<SceneObject*> PickSceneObjectsInRect(const FVector2D& RectStart, const FVector2D& RectEnd, const FVector2D& ViewportSize) const;
		bool GetSceneObjectScreenRect(SceneObject* Obj, const FVector2D& ViewportSize, FVector2D& OutMin, FVector2D& OutMax) const;

	private:
		Render* RenderGraphAsset;
		FW::PreviewViewPort* ViewPort;

		// Preview camera
		FW::Camera PreviewCamera;
		bool bRightMouseDown = false;
		bool bMiddleMouseDown = false;
		FVector2D LastMousePos{0, 0};

		// WASD movement keys
		bool bKeyW = false, bKeyA = false, bKeyS = false, bKeyD = false;
		bool bKeyQ = false, bKeyE = false;
		float CameraMoveSpeed = 5.0f;
		float CameraRotateSpeed = 0.003f;
		float CameraScrollSpeed = 1.0f;
		float CameraPanSpeed = 0.01f;

		double LastFrameTime = 0.0;

		// Preview resources
		static const uint32 MsaaSampleCount = 4;
		ScenePreviewRenderer PreviewRenderer;
		TRefCountPtr<FW::GpuTexture> FinalRT;
		TRefCountPtr<FW::GpuTexture> MsaaRT;
		TRefCountPtr<FW::GpuTexture> DepthRT;
		TRefCountPtr<FW::GpuTexture> MaskRT;
		TRefCountPtr<FW::GpuTextureView> FinalRTV;
		TRefCountPtr<FW::GpuTextureView> MsaaRTV;
		TRefCountPtr<FW::GpuTextureView> DepthDSV;
		TRefCountPtr<FW::GpuTextureView> MaskRTV;

		// Graph render comp for play mode
		TUniquePtr<RenderRenderComp> GraphComp;

		// Viewport delegates
		FDelegateHandle KeyDownHandle;
		FDelegateHandle KeyUpHandle;
		TSharedPtr<FUICommandList> SceneCommandList;

		// Gizmo state
		GizmoAxis HoveredAxis = GizmoAxis::None;
		GizmoAxis DraggingAxis = GizmoAxis::None;

		struct DragObjectState
		{
			FW::ObjectPtr<SceneObject> Object;
			FW::Vector3f StartPos;
			FW::Vector3f StartWorldPos;
			FW::Vector3f StartRotation;
			FW::Vector3f StartScale;
		};
		TArray<DragObjectState> DragAllObjectStates; // multi-object drag snapshots
		FW::Vector3f DragLocalAxisDir{0, 0, 0};
		FW::Vector3f DragPivotStart{0, 0, 0};
		FVector2D DragStartMousePos{0, 0};
		FVector2D DragAxisScreenDir{0, 0};
		float DragPixelsPerUnit = 1.0f;
		FW::ObjectPtr<SceneObject> PendingPickedObject;
		bool bPendingBoxSelection = false;
		bool bBoxSelecting = false;
		bool bBoxSelectAdditive = false;
		FVector2D BoxSelectionStart{0, 0};
		FVector2D BoxSelectionEnd{0, 0};
		static constexpr float BoxSelectionStartThreshold = 4.0f;
		// Plane drag state
		FW::Vector3f DragPlaneNormal{0, 1, 0};
		FW::Vector3f DragPlaneHitStart{0, 0, 0};
	};
}
