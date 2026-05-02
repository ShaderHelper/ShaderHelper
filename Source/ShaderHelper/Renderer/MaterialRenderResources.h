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
	struct MeshBuffers;
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
		MaterialUniformOverrideBytesResolver UniformOverrideBytesResolver;
	};

	FW::GpuVertexLayoutDesc BuildMaterialMeshVertexLayout(const Material& InMaterial);
	TUniquePtr<FW::UniformBuffer> BuildMaterialUniformBufferFromReflection(const TArray<FW::GpuShaderUbMemberInfo>& Members);
	FString MakeMaterialUniformBufferKey(const FString& Name, FW::BindingShaderStage Stage);

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
