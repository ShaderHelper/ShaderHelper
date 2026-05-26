#pragma once
#include "AssetObject/Material.h"
#include "GpuApi/GpuBindGroup.h"
#include "GpuApi/GpuBindGroupLayout.h"
#include "GpuApi/GpuPipelineState.h"
#include "RenderResource/UniformBuffer.h"

namespace FW
{
	class GpuRenderPassRecorder;
	class GpuBuffer;
	class AssetObject;
	struct MeshBuffers;
	struct Camera;
}

namespace SH
{
	using MaterialTextureOverrideResolver = TFunction<FW::GpuTexture*(const FW::GpuShaderLayoutBinding&)>;
	using MaterialUniformOverrideBytesResolver = TFunction<const uint8*(const MaterialBindingMemberDefault&)>;

	struct MaterialErrorRenderResources
	{
		TRefCountPtr<FW::GpuBindGroupLayout> BindGroupLayout;
		TRefCountPtr<FW::GpuBindGroup> BindGroup;
		TRefCountPtr<FW::GpuRenderPipelineState> Pipeline;
		TUniquePtr<FW::UniformBuffer> UniformBuffer;
		TArray<FW::GpuFormat> ColorFormats;
		FW::GpuFormat DepthFormat = FW::GpuFormat::NUM;
		uint32 SampleCount = 0;
	};

	struct MaterialBindGroupBuildOptions
	{
		MaterialTextureOverrideResolver TextureOverrideResolver;
		TArray<MaterialBindingResourceDefault>* BindingResourceDefaults = nullptr;
		FW::Vector2f DefaultResourceViewportSize = FW::Vector2f{ 0, 0 };
		FW::GpuBuffer* PrinterBuffer = nullptr;
		bool bRebuildLayouts = true;
		bool bRebuildUniformBuffers = true;
	};

	struct MaterialUniformBufferUpdateOptions
	{
		FMatrix44f ModelMatrix = FMatrix44f::Identity;
		FMatrix44f ViewMatrix = FMatrix44f::Identity;
		FMatrix44f ProjMatrix = FMatrix44f::Identity;
		FMatrix44f ViewProjMatrix = FMatrix44f::Identity;
		FMatrix44f MVPMatrix = FMatrix44f::Identity;
		FW::Vector2f ViewportSize = FW::Vector2f(0, 0);
		FW::Vector2f MousePos = FW::Vector2f(0, 0);
		FW::Vector3f CameraPos = FW::Vector3f(0, 0, 0);
		FW::Vector3f CameraDir = FW::Vector3f(0, 0, 1);
		float Time = 0.0f;
		MaterialUniformOverrideBytesResolver UniformOverrideBytesResolver;
	};

	TOptional<FW::GpuVertexLayoutDesc> BuildMaterialMeshVertexLayout(const Material& InMaterial);
	TUniquePtr<FW::UniformBuffer> BuildMaterialUniformBufferFromReflection(const TArray<FW::GpuShaderUbMemberInfo>& Members);
	FString MakeMaterialUniformBufferKey(const FString& Name, FW::BindingShaderStage Stage);

	FW::GpuTexture* ResolveTextureAssetGpu(FW::AssetObject* TextureAsset);
	FW::Vector3f GetCameraForwardDir(const FW::Camera& InCamera);

	MaterialUniformBufferUpdateOptions MakeMaterialUniformOptions(
		const FMatrix44f& ModelMatrix,
		const FMatrix44f& ViewMatrix,
		const FMatrix44f& ProjMatrix,
		FW::Vector2f ViewportSize,
		FW::Vector2f MousePos,
		FW::Vector3f CameraPos,
		FW::Vector3f CameraDir,
		float Time);

	struct MaterialPipelineBuildOptions
	{
		TArray<FW::GpuFormat> ColorFormats;
		FW::GpuFormat DepthFormat = FW::GpuFormat::NUM;
		uint32 SampleCount = 1;
		TArray<FW::GpuBindGroupLayout*> BindGroupLayouts;
	};

	struct MaterialPipelineBuildResult
	{
		TRefCountPtr<FW::GpuRenderPipelineState> Pipeline;
		FW::GpuRenderPipelineStateDesc Desc;
		FText Error;
	};

	bool BuildMaterialPipeline(
		const Material& InMaterial,
		const MaterialPipelineBuildOptions& Options,
		MaterialPipelineBuildResult& OutResult);

	void BuildMaterialBindGroups(
		Material& InMaterial,
		TMap<int32, TRefCountPtr<FW::GpuBindGroupLayout>>& OutBindGroupLayouts,
		TMap<int32, TRefCountPtr<FW::GpuBindGroup>>& OutBindGroups,
		TMap<FString, TUniquePtr<FW::UniformBuffer>>& OutUniformBuffers,
		const MaterialBindGroupBuildOptions& Options = {});

	void UpdateMaterialUniformBuffers(
		const Material& InMaterial,
		TMap<FString, TUniquePtr<FW::UniformBuffer>>& InOutUniformBuffers,
		const MaterialUniformBufferUpdateOptions& Options);

	bool EnsureMaterialErrorRenderResources(
		MaterialErrorRenderResources& InOutResources,
		const TArray<FW::GpuFormat>& ColorFormats,
		FW::GpuFormat DepthFormat,
		uint32 SampleCount);

	void DrawMaterialErrorMesh(
		FW::GpuRenderPassRecorder* Recorder,
		MaterialErrorRenderResources& Resources,
		const FW::MeshBuffers& MeshBuffers,
		const FMatrix44f& Transform);
}
