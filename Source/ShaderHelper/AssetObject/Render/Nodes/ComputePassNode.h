#pragma once
#include "AssetObject/Render/MeshRenderObject.h"
#include "AssetObject/Shader.h"
#include "GpuApi/GpuBindGroup.h"
#include "GpuApi/GpuBindGroupLayout.h"
#include "GpuApi/GpuPipelineState.h"
#include "RenderResource/UniformBuffer.h"
#include "RenderResource/PrintBuffer.h"
#include "Editor/AssetEditor/ShAssetEditor.h"

namespace FW
{
	class GpuQuerySet;
}

namespace SH
{
	class ComputePassNodeOp : public ShPropertyOp
	{
		REFLECTION_TYPE(ComputePassNodeOp)
	public:
		ComputePassNodeOp() = default;
		FW::MetaType* SupportType() override;
	};

	class ComputePassNode : public FW::GraphNode
	{
		REFLECTION_TYPE(ComputePassNode)
	public:
		ComputePassNode();
		ComputePassNode(FW::AssetPtr<Shader> InShader);
		~ComputePassNode();

		void Init() override;
		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		FW::ExecRet Exec(FW::GraphExecContext& Context) override;
		FSlateColor GetNodeColor() const override { return FLinearColor{ 0.13f, 0.20f, 0.32f }; }
		TArray<TSharedRef<FW::PropertyData>> GeneratePropertyDatas() override;
		bool CanChangeProperty(FW::PropertyData* InProperty) override;
		void PostPropertyChanged(FW::PropertyData* InProperty) override;

	public:
		FW::AssetPtr<Shader> ShaderAsset;
		FW::Vector3u ThreadGroupCount{ 1, 1, 1 };
		TArray<ShaderOverrideSlot> OverrideSlots;

	private:
		void BindShaderDelegates();
		void UnbindShaderDelegates();
		void OnShaderRefreshed();

		void SyncOverridesFromReflection();
		void RefreshNodeWidget();

		void InvalidateRenderResources();
		bool BuildBindGroups();
		void UpdateUniformBuffers();

		FW::PrintBuffer* GetPrintBuffer();
		bool FlushPrintBufferLogs(const FString& LogPrefix);

	private:
		FDelegateHandle ShaderRefreshedHandle;
		FDelegateHandle ShaderDestroyedHandle;
		Shader* BoundShader = nullptr;

		TMap<int32, TRefCountPtr<FW::GpuBindGroupLayout>> BindGroupLayouts;
		TMap<int32, TRefCountPtr<FW::GpuBindGroup>> BindGroups;
		TMap<FString, TUniquePtr<FW::UniformBuffer>> UniformBuffers;
		TRefCountPtr<FW::GpuComputePipelineState> Pipeline;
		TRefCountPtr<FW::GpuQuerySet> TimestampQuerySet;
		TUniquePtr<FW::PrintBuffer> PrinterBuffer;
	};
}
