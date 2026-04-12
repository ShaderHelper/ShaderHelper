#pragma once
#include "AssetObject/Material.h"
#include "RenderResource/Mesh.h"

namespace FW
{
	class GpuBindGroup;
	class GpuBindGroupLayout;
	class GpuRenderPipelineState;
	class GpuTexture;
	class UniformBuffer;
}

namespace SH
{
	enum class MaterialPreviewPrimitive
	{
		Sphere,
		Quad,
	};

	class MaterialPreviewRenderer
	{
	public:
		explicit MaterialPreviewRenderer(Material* InMaterial = nullptr);

		void SetMaterial(Material* InMaterial);
		void SetPreviewPrimitive(MaterialPreviewPrimitive InPreviewPrimitive);
		void SetOrbit(float InYaw, float InPitch);
		void SetCameraDistance(float InDistance);
		bool Render(uint32 InWidth, uint32 InHeight);
		void RenderErrorColor(uint32 InWidth, uint32 InHeight);
		FText GetErrorReason() const;
		FW::GpuTexture* GetRenderTarget() const { return RenderTarget.GetReference(); }

		static TRefCountPtr<FW::GpuTexture> RenderThumbnail(const Material* InMaterial, uint32 InSize = 128, MaterialPreviewPrimitive InPreviewPrimitive = MaterialPreviewPrimitive::Sphere);

	private:
		bool EnsurePipeline();
		bool EnsureErrorPipeline();
		bool EnsurePreviewMesh();
		void ResizeRenderTargetsIfNeeded(uint32 InWidth, uint32 InHeight);
		void UpdatePreviewTransform(uint32 InWidth, uint32 InHeight);
		void BuildBindGroupFromMaterial();

	private:
		Material* MaterialAsset = nullptr;
		FW::MeshData QuadMesh = FW::CreateQuad();
		FW::MeshData SphereMesh = FW::CreateSphere();
		TMap<int32, TRefCountPtr<FW::GpuBindGroupLayout>> BindGroupLayouts;
		TMap<int32, TRefCountPtr<FW::GpuBindGroup>> BindGroups;
		TRefCountPtr<FW::GpuRenderPipelineState> Pipeline;
		TMap<FString, TUniquePtr<FW::UniformBuffer>> PreviewUniformBuffers;

		TRefCountPtr<FW::GpuBindGroupLayout> ErrorBindGroupLayout;
		TRefCountPtr<FW::GpuBindGroup> ErrorBindGroup;
		TRefCountPtr<FW::GpuRenderPipelineState> ErrorPipeline;
		TUniquePtr<FW::UniformBuffer> ErrorUniformBuffer;

		FW::MeshBuffers PreviewMeshBuffers;
		TRefCountPtr<FW::GpuTexture> RenderTarget;
		TRefCountPtr<FW::GpuTexture> MsaaRenderTarget;
		TRefCountPtr<FW::GpuTexture> DepthTarget;
		float OrbitYaw = PI;
		float OrbitPitch = 0.0f;
		float CameraDistance = 2.0f;
		MaterialPreviewPrimitive PreviewPrimitive = MaterialPreviewPrimitive::Sphere;
		bool bPreviewMeshInitialized = false;
		TFunction<FText()> LinkageErrorFunc;
	};
}
