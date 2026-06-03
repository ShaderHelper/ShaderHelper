#pragma once
#include "Editor/PreviewViewPort.h"
#include "RenderResource/Camera.h"
#include "RenderResource/UniformBuffer.h"
#include "GpuApi/GpuRhi.h"

class SViewport;

namespace SH
{
	// Vertex-stage geometry viewer: restores captured SV_Position to world space, draws the captured
	// mesh with a green overlay, and lets the user pick exactly one vertex to step.
	class SVertexDebuggerViewport : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SVertexDebuggerViewport){}
		SLATE_END_ARGS()
		~SVertexDebuggerViewport() override;
		void Construct(const FArguments& InArgs);

		// ClipPositions: per (instance,vertex) flat clip-space xyzw (instance*VertexCount+vertex).
		// Indices: this submesh's per-instance triangle list. VertexCount/InstanceCount: capture dims.
		void SetDebugData(const TArray<FW::Vector4f>& InClipPositions, const TArray<uint32>& InIndices, 
			uint32 InVertexCount, uint32 InInstanceCount, const FMatrix44f& ClipToWorld, const TOptional<FW::Camera>& InDebugCamera, bool GlobalValidation = false,
			TSet<FW::Vector2u> InAssertedVertices = {});
		void SetFinalizedVertex(uint32 InVertexIndex, uint32 InInstanceIndex);
		void Clear();
		bool FinalizedVertex() const { return bVertexFinalized; }

	private:
		struct SubMeshRender
		{
			TArray<FW::Vector3f> WorldPositions;
			TArray<uint32> Indices;
			TRefCountPtr<FW::GpuBuffer> VertexBuffer;
			TRefCountPtr<FW::GpuBuffer> IndexBuffer;
			TRefCountPtr<FW::GpuBuffer> OverlayLineBuffer;
			TRefCountPtr<FW::GpuBuffer> OverlayPointBuffer;
			uint32 IndexCount = 0;
			uint32 TriangleCount = 0;
			uint32 OverlayLineVertexCount = 0;
			uint32 OverlayPointVertexCount = 0;
		};

		void InitRenderResources();
		void ResizeRenderTargetIfNeeded();
		void UpdateDefaultOrbitCamera(float InDistance);
		void FrameCamera(const TOptional<FW::Camera>& InDebugCamera, const TArray<FW::Vector3f>& InFrustumPositions);
		void UpdateCamera(float DeltaTime);
		void Render();
		void OnPick();
		void SetFinalizedAssertedVertex(uint32 InVertexIndex, uint32 InInstanceIndex);
		void BuildAssertedVertexBuffer(const TSet<FW::Vector2u>& InAssertedVertices);

	private:
		TSharedPtr<FW::PreviewViewPort> Preview;
		TSharedPtr<SViewport> ViewportWidget;
		TRefCountPtr<FW::GpuTexture> RenderTarget;
		TRefCountPtr<FW::GpuTexture> MsaaRenderTarget;
		TRefCountPtr<FW::GpuTexture> DepthTarget;

		TArray<SubMeshRender> SubMeshes;
		FW::Vector3f SceneCenter{ 0, 0, 0 };
		float SceneRadius = 1.0f;

		FW::Camera ViewCamera;
		float CameraDistance = 3.0f;
		float MinCameraDistance = 0.1f;
		float MaxCameraDistance = 1000.0f;
		bool bLeftMouseDown = false;
		bool bLeftDragMoved = false;
		bool bRightMouseDown = false;
		bool bMiddleMouseDown = false;
		bool bUseDebugCameraAspect = false;
		bool bKeyW = false;
		bool bKeyA = false;
		bool bKeyS = false;
		bool bKeyD = false;
		bool bKeyQ = false;
		bool bKeyE = false;
		float CameraMoveSpeed = 5.0f;
		FW::Vector2f LastMousePos{};

		// Shared pipeline state (UB: transform, color, and screen-space overlay size).
		FW::UniformBufferBuilder WireUbBuilder{ FW::UniformBufferUsage::Persistant };
		TUniquePtr<FW::UniformBuffer> WireUb;        // green wire/point overlay
		TUniquePtr<FW::UniformBuffer> FrustumUb;     // gray pass camera frustum
		TUniquePtr<FW::UniformBuffer> HighlightUb;   // blue selection
		TUniquePtr<FW::UniformBuffer> AssertedUb;    // pink assert marker
		TUniquePtr<FW::UniformBuffer> SolidUb;       // shaded solid mesh
		TRefCountPtr<FW::GpuBindGroupLayout> WireBindGroupLayout;
		TRefCountPtr<FW::GpuBindGroup> WireBindGroup;
		TRefCountPtr<FW::GpuBindGroup> FrustumBindGroup;
		TRefCountPtr<FW::GpuBindGroup> HighlightBindGroup;
		TRefCountPtr<FW::GpuBindGroup> AssertedBindGroup;
		TRefCountPtr<FW::GpuBindGroup> SolidBindGroup;
		TRefCountPtr<FW::GpuShader> WireVs;
		TRefCountPtr<FW::GpuShader> WirePs;
		TRefCountPtr<FW::GpuShader> SolidVs;
		TRefCountPtr<FW::GpuShader> SolidPs;
		TRefCountPtr<FW::GpuShader> OverlayLineVs;
		TRefCountPtr<FW::GpuShader> OverlayPointVs;
		TRefCountPtr<FW::GpuShader> OverlayPs;
		TRefCountPtr<FW::GpuShader> OverlayPointPs;
		TRefCountPtr<FW::GpuRenderPipelineState> SolidPipeline;       // TriangleList, shaded solid fill
		TRefCountPtr<FW::GpuRenderPipelineState> WirePipeline;        // TriangleList, thick screen-space edge quads
		TRefCountPtr<FW::GpuRenderPipelineState> PointPipeline;       // TriangleList, green screen-space point quads
		TRefCountPtr<FW::GpuRenderPipelineState> FrustumPipeline;     // LineList
		TRefCountPtr<FW::GpuBuffer> FrustumVertexBuffer;

		// Selection
		bool bVertexFinalized = false;
		bool bIsValidating = false;
		uint32 PerInstanceVertexCount = 0; // to decode picked flat id back into (instance, localVertex)
		TRefCountPtr<FW::GpuBuffer> HighlightVertexBuffer; // 6 verts of a selected-vertex billboard point
		TRefCountPtr<FW::GpuBuffer> AssertedVertexBuffer;  // pink billboards for assert-failing vertices
		uint32 AssertedVertexPointCount = 0;

		FDelegateHandle ResizeHandlerHandle;
		FDelegateHandle KeyDownHandlerHandle;
		FDelegateHandle KeyUpHandlerHandle;
		FDelegateHandle CameraTickerHandle;
	};
}
