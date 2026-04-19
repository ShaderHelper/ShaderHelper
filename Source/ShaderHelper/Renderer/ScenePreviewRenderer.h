#pragma once
#include "Renderer/RenderGraph.h"
#include "RenderResource/Camera.h"
#include "AssetObject/Render/SceneObject.h"
#include "UI/Widgets/Scene/SSceneView.h"

namespace SH
{
	class ScenePreviewRenderer
	{
	public:
		ScenePreviewRenderer();

		void RenderGrid(FW::RenderGraph& Graph, FW::GpuTextureView* OutputView, FW::GpuTextureView* DepthView,
			const FW::Camera& Camera, bool bClear = false, float GridSize = 50.0f, float GridSpacing = 1.0f);

		bool RenderMeshes(FW::RenderGraph& Graph, FW::GpuTextureView* OutputView,
			FW::GpuTextureView* DepthView,
			const FW::Camera& Camera, const TArray<FW::ObjectPtr<SceneObject>>& SceneObjects, bool bClear = false);

		void RenderOutline(FW::RenderGraph& Graph, FW::GpuTextureView* OutputView,
			FW::GpuTextureView* MaskView,
			const FW::Camera& Camera, SceneObject* SelectedObject);

		void RenderGizmo(FW::RenderGraph& Graph, FW::GpuTextureView* OutputView,
			const FW::Camera& Camera, const FW::Vector3f& GizmoPosition, int32 HighlightAxis,
			GizmoMode Mode, const FMatrix44f& Orientation);

		void RenderCameraWireframes(FW::RenderGraph& Graph, FW::GpuTextureView* OutputView,
			const FW::Camera& SceneCamera, const TArray<FW::ObjectPtr<SceneObject>>& SceneObjects,
			SceneObject* SelectedObject);

		void ResolveMsaa(FW::RenderGraph& Graph, FW::GpuTextureView* MsaaView, FW::GpuTextureView* ResolveView);
	};
}
