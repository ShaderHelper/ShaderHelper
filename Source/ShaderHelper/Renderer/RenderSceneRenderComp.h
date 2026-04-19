#pragma once
#include "RenderRenderComp.h"
#include "ScenePreviewRenderer.h"
#include "RenderResource/Camera.h"
#include "UI/Widgets/Scene/SSceneView.h"

namespace SH
{
	enum class GizmoAxis : int32
	{
		None = 0,
		X = 1,
		Y = 2,
		Z = 3,
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

		FReply OnMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
		FReply OnMouseUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
		FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
		FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
		void OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent);
		void OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent);
		void UpdateCamera(float DeltaTime);

		// Gizmo helpers
		SceneObject* GetSelectedSceneObject() const;
		FVector2D WorldToScreen(const FW::Vector3f& WorldPos, const FVector2D& ViewportSize) const;
		float PointToSegmentDistSq(const FVector2D& P, const FVector2D& A, const FVector2D& B) const;
		GizmoAxis HitTestGizmo(const FVector2D& LocalMousePos, const FVector2D& ViewportSize) const;
		float GetGizmoScale(const FW::Vector3f& GizmoPos) const;
		FMatrix44f GetGizmoOrientationMatrix(SceneObject* Obj) const;
		FW::Vector3f GetOrientedAxisDir(GizmoAxis Axis, const FMatrix44f& Orientation) const;
		GizmoMode GetCurrentGizmoMode() const;
		GizmoSpace GetCurrentGizmoSpace() const;

		// Picking
		SceneObject* PickSceneObject(const FVector2D& LocalPos, const FVector2D& ViewportSize) const;

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

		// Gizmo state
		GizmoAxis HoveredAxis = GizmoAxis::None;
		GizmoAxis DraggingAxis = GizmoAxis::None;
		FW::Vector3f DragStartObjectPos{0, 0, 0};
		FW::Vector3f DragStartObjectRotation{0, 0, 0};
		FW::Vector3f DragStartObjectScale{1, 1, 1};
		FW::Vector3f DragLocalAxisDir{0, 0, 0};
		FVector2D DragStartMousePos{0, 0};
		FVector2D DragAxisScreenDir{0, 0};
		float DragPixelsPerUnit = 1.0f;
	};
}
