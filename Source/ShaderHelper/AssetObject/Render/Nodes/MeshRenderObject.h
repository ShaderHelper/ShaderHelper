#pragma once
#include "AssetObject/Graph.h"
#include "AssetObject/Material.h"
#include "AssetObject/Render/MeshSceneObject.h"
#include "RenderResource/UniformBuffer.h"
#include "RenderResource/Camera.h"
#include "GpuApi/GpuBindGroupLayout.h"
#include "GpuApi/GpuBindGroup.h"
#include "Renderer/MaterialRenderResources.h"

namespace SH
{
	// Metadata for a single material-binding-member override exposed as a row pin.
	struct MaterialOverrideSlot
	{
		FString BindingName;
		FString MemberName;        // For UB members. Empty for whole-binding resource overrides.
		FString Type;              // Type string of the member/resource (e.g. "float4", "Texture2D").
		FW::BindingShaderStage Stage = FW::BindingShaderStage::All;
		bool bIsResource = false;
		FGuid PinGuid;
		TArray<uint8> Bytes;
		FW::AssetPtr<FW::AssetObject> TextureAsset;

		friend FArchive& operator<<(FArchive& Ar, MaterialOverrideSlot& S)
		{
			Ar << S.BindingName << S.MemberName << S.Type << S.Stage << S.bIsResource << S.PinGuid << S.Bytes << S.TextureAsset;
			return Ar;
		}
	};
	class MeshRenderObject : public FW::ShObject
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
		void Draw(FW::GpuRenderPassRecorder* Recorder, const FW::Camera* InCamera, const FMatrix44f& ModelMatrix);

		// Called by property panel: add/remove override entries.
		void AddOverride(const FString& BindingName, const FString& MemberName, const FString& Type, FW::BindingShaderStage Stage, bool bIsResource);
		void RemoveOverride(int32 SlotIndex);

		// Invalidate material-derived render resources.
		void InvalidateRenderResources();

	public:
		FW::ObserverObjectPtr<MeshSceneObject> MeshSceneObjectRef;
		FW::AssetPtr<Material> MaterialAsset;
		TArray<FW::ObjectPtr<FW::GraphPin>> OverridePins;
		TArray<MaterialOverrideSlot> OverrideSlots;

	private:
		void BindMaterialDelegates();
		void UnbindMaterialDelegates();
		void OnMaterialChanged();
		void BuildBindGroupFromMaterial(bool bRebuildLayouts = true, bool bRebuildUniformBuffers = true);
		bool BuildPipeline(const TArray<FW::GpuFormat>& ColorFormats, FW::GpuFormat DepthFormat, uint32 SampleCount);
		FW::GraphPin* FindOverridePin(const FString& BindingName, const FString& MemberName, FW::BindingShaderStage Stage) const;
		void EnsureOverridePins();
		void SyncOverridePinsFromSlots();
		static FW::ObjectPtr<FW::GraphPin> CreateOverridePin(FW::ShObject* Outer, const FString& Type, bool bIsResource);

	private:
		TMap<int32, TRefCountPtr<FW::GpuBindGroupLayout>> BindGroupLayouts;
		TMap<int32, TRefCountPtr<FW::GpuBindGroup>> BindGroups;
		TRefCountPtr<FW::GpuRenderPipelineState> Pipeline;
		TMap<FString, TUniquePtr<FW::UniformBuffer>> UniformBuffers;
		MaterialErrorRenderResources ErrorResources;
		FDelegateHandle MaterialChangedHandle;
		Material* BoundMaterial = nullptr;
		bool bDrawMaterialError = false;
	};
}
