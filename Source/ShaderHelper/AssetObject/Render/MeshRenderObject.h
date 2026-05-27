#pragma once
#include "AssetObject/Graph.h"
#include "AssetObject/Material.h"
#include "AssetObject/Render/ShaderOverrideHelper.h"
#include "AssetObject/Render/MeshSceneObject.h"
#include "RenderResource/UniformBuffer.h"
#include "RenderResource/Camera.h"
#include "GpuApi/GpuBindGroupLayout.h"
#include "GpuApi/GpuBindGroup.h"
#include "Renderer/MaterialRenderCommon.h"
#include "RenderResource/PrintBuffer.h"
#include "Debugger/DebuggableObject.h"

namespace FW
{
	class RenderGraph;
	struct RGRenderPass;
}

namespace SH
{
	class MeshRenderObject : public FW::ShObject, public DebuggableObject
	{
		REFLECTION_TYPE(MeshRenderObject)
	public:
		MeshRenderObject();
		~MeshRenderObject();

		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		TArray<TSharedRef<FW::PropertyData>> GeneratePropertyDatas() override;
		void PostPropertyChanged(FW::PropertyData* InProperty) override;

		// Ensure pipeline + bind groups are valid for the given RT formats / depth / sample count.
		bool EnsureRenderResources(const TArray<FW::GpuFormat>& ColorFormats, FW::GpuFormat DepthFormat, uint32 SampleCount);

		// Update UB members with built-in + default values, apply overrides, then draw every submesh.
		void Draw(FW::GpuRenderPassRecorder* Recorder, const FW::Camera* InCamera, const FMatrix44f& ModelMatrix, const FW::Vector2f& ViewportSize, const FW::Vector2f& MousePos, float Time);

		// Called by property panel: add/remove override entries.
		void AddOverride(const FString& BindingName, const FString& MemberName, const FString& Type, FW::BindingShaderStage Stage, bool bIsResource);
		void RemoveOverride(int32 SlotIndex);

		// Invalidate material-derived render resources.
		void InvalidateRenderResources();
		bool UsesTextureAsShaderInput(FW::GpuTexture* Texture, FString& OutBindingName) const;
		FW::PrintBuffer* GetPrintBuffer();
		bool FlushPrintBufferLogs(const FString& LogPrefix);

		void AddRWInputBlitPasses(FW::RenderGraph& RG);
		void AddRenderPassResourceAccesses(FW::RGRenderPass& Pass);

		// DebuggableObject
		TArray<DebugItem> GetSupportedDebugItems() const override { return { DebugItem::Vertex, DebugItem::Pixel }; }
		DebugTargetInfo OnStartDebugging(DebugItem Item) override;
		void OnEndDebuggging() override { bDebugging = false; }
		ShaderAsset* GetShaderAsset(DebugItem Item) const override;
		InvocationState GetInvocationState(DebugItem Item) override;
		void OnFinalizePixel(const FW::Vector2u& PixelCoord) override;

		TRefCountPtr<FW::GpuTexture> BuildCoverageMask();

		// Re-renders the mesh with a SPIR-V-patched PS that writes pink (1,0,1,1) wherever
		// GPrivate_AssertResult != 1. Skipped when bDrawMaterialError or when no assert was
		// reported on the previous frame.
		void AddAssertHighlightPass(FW::RenderGraph& RG, const TArray<TRefCountPtr<FW::GpuTexture>>& ColorRTs, TRefCountPtr<FW::GpuTexture> DepthRT, const FW::Vector2f& ViewportSize, const FW::Camera* Cam);
		bool bHadAssertError = false;

	public:
		FW::ObserverObjectPtr<MeshSceneObject> MeshSceneObjectRef;
		FW::AssetPtr<Material> MaterialAsset;
		TArray<FW::ObjectPtr<FW::GraphPin>> OverridePins;
		TArray<ShaderOverrideSlot> OverrideSlots;
		FW::Vector2f CurrentViewportSize{ 0, 0 };

	private:
		void BindMaterialDelegates();
		void UnbindMaterialDelegates();
		void OnMaterialChanged();
		void BuildBindGroupFromMaterial(bool bRebuildLayouts = true, bool bRebuildUniformBuffers = true);
		bool BuildPipeline(const TArray<FW::GpuFormat>& ColorFormats, FW::GpuFormat DepthFormat, uint32 SampleCount);
		void UpdateMaterialDrawState(const FMatrix44f& ModelMatrix, const FMatrix44f& ViewMat, const FMatrix44f& ProjMat, const FW::Vector2f& ViewportSize, const FW::Vector2f& MousePos, float Time, const FW::Vector3f& CameraPos, const FW::Vector3f& CameraDir);
		TArray<BindingBuilder> BuildDebugBindingBuilders() const;
		// Returns false if the material has no PS source or compilation fails.
		bool BuildAssertHighlightPs();
		FW::GpuTexture* ResolveBindingTexture(const FW::GpuShaderLayoutBinding& Binding);
		FW::GpuBuffer* ResolveBindingBuffer(const FW::GpuShaderLayoutBinding& Binding);

		TMap<int32, TRefCountPtr<FW::GpuBindGroupLayout>> BindGroupLayouts;
		TMap<int32, TRefCountPtr<FW::GpuBindGroup>> BindGroups;
		TRefCountPtr<FW::GpuRenderPipelineState> Pipeline;
		FW::GpuRenderPipelineStateDesc PipelineDesc;
		TMap<FString, TUniquePtr<FW::UniformBuffer>> UniformBuffers;
		TUniquePtr<FW::PrintBuffer> PrinterBuffer;
		MaterialErrorRenderResources ErrorResources;
		FDelegateHandle MaterialChangedHandle;
		Material* BoundMaterial = nullptr;
		bool bDrawMaterialError = false;
		bool bDebugging = false;

		// Assert-highlight PS that overlays pink on asserting fragments.
		// Reuses the original Pipeline's BindGroups (no extra resources are injected).
		TRefCountPtr<FW::GpuShader> AssertHighlightPs;
	};
}
