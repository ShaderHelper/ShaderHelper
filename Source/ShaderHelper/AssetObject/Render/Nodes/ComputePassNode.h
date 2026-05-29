#pragma once
#include "AssetObject/Render/MeshRenderObject.h"
#include "AssetObject/Shader.h"
#include "Debugger/DebuggableObject.h"
#include "GpuApi/GpuBindGroup.h"
#include "GpuApi/GpuBindGroupLayout.h"
#include "GpuApi/GpuPipelineState.h"
#include "RenderResource/UniformBuffer.h"
#include "RenderResource/PrintBuffer.h"
#include "Editor/AssetEditor/ShAssetEditor.h"

namespace FW
{
	class GpuQuerySet;
	class RenderGraph;
	struct RGComputePass;
}

namespace SH
{
	enum class ComputeBuiltInFloatValue : uint8
	{
		Time,
	};

	enum class ComputeBuiltInVector2Value : uint8
	{
		ViewportSize,
		MousePos,
	};

	class ComputePassNodeOp : public ShPropertyOp
	{
		REFLECTION_TYPE(ComputePassNodeOp)
	public:
		ComputePassNodeOp() = default;
		FW::MetaType* SupportType() override;
		void OnCancelSelect(FW::ShObject* InObject) override;
		void OnSelect(FW::ShObject* InObject) override;
	};

	class ComputePassNode : public FW::GraphNode, public DebuggableObject
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

		// DebuggableObject
		TArray<DebugItem> GetSupportedDebugItems() const override { return { DebugItem::Compute }; }
		DebugTargetInfo OnStartDebugging(DebugItem Item) override;
		void OnEndDebuggging() override;
		ShaderAsset* GetShaderAsset(DebugItem /*Item*/) const override { return ShaderAsset.Get(); }
		InvocationState GetInvocationState(DebugItem Item) override;
		void OnFinalizeCompute(const FW::Vector3u& WorkGroupId, const FW::Vector3u& LocalInvocationId) override;

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
		void UpdateUniformBuffers(const struct RenderExecContext& Ctx);
		TArray<BindingBuilder> BuildDebugBindingBuilders() const;
		FW::Vector3u ReflectThreadGroupSize() const;
		// Resolve the input texture for a resource binding:
		// override pin -> slot's TextureAsset -> default texture for that BindingType.
		FW::GpuTexture* ResolveBindingTexture(const FW::GpuShaderLayoutBinding& Binding);

		FW::GpuBuffer* ResolveBindingBuffer(const FW::GpuShaderLayoutBinding& Binding);

		FW::PrintBuffer* GetPrintBuffer();
		bool FlushPrintBufferLogs(const FString& LogPrefix);
		void AddComputePassResourceAccesses(FW::RGComputePass& Pass, FW::GpuShader* Cs);

		uint32 AddAssertHighlightPass(FW::RenderGraph& RG, FW::GpuShader* Cs, const TArray<int32>& SortedGroupSlots, const TArray<FW::GpuBindGroupLayout*>& BaseLayoutPtrs);
		bool BuildAssertHighlightCs();
		void ReadbackAssertedThreads(uint32 TotalThreads);

	private:
		FDelegateHandle ShaderRefreshedHandle;
		FDelegateHandle ShaderDestroyedHandle;
		Shader* BoundShader = nullptr;

		TMap<int32, TRefCountPtr<FW::GpuBindGroupLayout>> BindGroupLayouts;
		TMap<int32, TRefCountPtr<FW::GpuBindGroup>> BindGroups;
		TMap<FString, TUniquePtr<FW::UniformBuffer>> UniformBuffers;
		FW::Vector2f CurrentViewportSize{ 0, 0 };
		TRefCountPtr<FW::GpuComputePipelineState> Pipeline;
		FW::GpuComputePipelineStateDesc CachedPipelineDesc;
		TRefCountPtr<FW::GpuQuerySet> TimestampQuerySet;
		TUniquePtr<FW::PrintBuffer> PrinterBuffer;
		bool bDebugging = false;

		// Assert-highlight resources
		TRefCountPtr<FW::GpuShader> AssertHighlightCs;
		TRefCountPtr<FW::GpuBuffer> AssertThreadBuffer;
	};
}
