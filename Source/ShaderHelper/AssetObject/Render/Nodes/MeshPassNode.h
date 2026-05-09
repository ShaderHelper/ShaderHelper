#pragma once
#include "AssetObject/Graph.h"
#include "AssetObject/Render/Render.h"
#include "AssetObject/Render/CameraSceneObject.h"
#include "Editor/AssetEditor/ShAssetEditor.h"
#include "MeshRenderObject.h"

namespace FW
{
	class PreviewViewPort;
	class GpuQuerySet;
}

namespace SH
{
	enum class MeshPassColorFormat : uint8
	{
		B8G8R8A8_UNORM,
		R16G16B16A16_FLOAT,
		R32G32B32A32_FLOAT,
		R32_FLOAT,
		R16_FLOAT,
		R32G32_FLOAT,
	};

	enum class MeshPassDepthFormat : uint8
	{
		D32_FLOAT,
	};

	struct MeshPassColorRT
	{
		MeshPassColorFormat Format = MeshPassColorFormat::R32G32B32A32_FLOAT;
		FW::Vector4f ClearValue = FW::Vector4f(0, 0, 0, 1);

		bool operator==(const MeshPassColorRT& Other) const
		{
			return Format == Other.Format
				&& ClearValue == Other.ClearValue;
		}

		friend FArchive& operator<<(FArchive& Ar, MeshPassColorRT& ColorRT)
		{
			Ar << ColorRT.Format;
			Ar << ColorRT.ClearValue;
			return Ar;
		}
	};

	class MeshPassNodeOp : public ShPropertyOp
	{
		REFLECTION_TYPE(MeshPassNodeOp)
	public:
		MeshPassNodeOp() = default;
		FW::MetaType* SupportType() override;
		void OnCancelSelect(FW::ShObject* InObject) override;
		void OnSelect(FW::ShObject* InObject) override;
	};

	class MeshPassNode : public FW::GraphNode
	{
		REFLECTION_TYPE(MeshPassNode)
	public:
		MeshPassNode();
		~MeshPassNode();
		void Init() override;

	public:
		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		FW::ExecRet Exec(FW::GraphExecContext& Context) override;
		FSlateColor GetNodeColor() const override { return FLinearColor{ 0.27f, 0.13f, 0.0f }; }
		TSharedPtr<SWidget> ExtraNodeWidget(FW::SGraphNode* OwnerWidget) override;
		TArray<TSharedRef<FW::PropertyData>> GeneratePropertyDatas() override;
		bool CanChangeProperty(FW::PropertyData* InProperty) override;
		void PostPropertyChanged(FW::PropertyData* InProperty) override;

		// Pin management. Preserve existing Guids so links survive where possible.
		void RebuildPins();

		// Add/remove MeshRenderObject helpers (used by drag-drop + combo button).
		MeshRenderObject* AddMeshRenderObject(MeshSceneObject* InMeshSceneObject);
		void RemoveMeshRenderObject(MeshRenderObject* InObject);

		// Called by property panel after Color RT / Depth target / count changed.
		void OnRenderTargetsChanged();
		void RefreshNodeWidget();
		DebugTargetInfo MakeDebugTargetInfo(TRefCountPtr<FW::GpuTexture> CoverageMask) const;
		const TArray<TRefCountPtr<FW::GpuTexture>>& GetLastActiveColorRTs() const { return LastActiveColorRTs; }
		TRefCountPtr<FW::GpuTexture> GetLastActiveDepthRT() const { return LastActiveDepthRT; }
		const TArray<FW::GpuFormat>& GetLastColorFormats() const { return LastColorFormats; }
		FW::GpuFormat GetLastDepthFormat() const { return LastDepthFormat; }
		uint32 GetLastSampleCount() const { return LastSampleCount; }
		FW::GpuViewPortDesc GetLastViewPortDesc() const { return LastViewPortDesc; }
		TOptional<FW::Camera> GetLastCamera() const { return LastCamera; }
		FW::Vector2f GetLastMousePos() const { return LastMousePos; }

	public:
		TArray<MeshPassColorRT> ColorRTs;
		bool bDepthEnabled = false;
		MeshPassDepthFormat DepthFormat = MeshPassDepthFormat::D32_FLOAT;
		FW::ObserverObjectPtr<CameraSceneObject> CameraRef;
		FW::Vector2u RTSize = {0, 0}; // 0,0 = auto from viewport
		TArray<FW::ObjectPtr<MeshRenderObject>> MeshRenderObjects;

	private:
		// Cached textures
		TArray<TRefCountPtr<FW::GpuTexture>> CachedColorRTs;
		TRefCountPtr<FW::GpuTexture> CachedDepthRT;
		TRefCountPtr<FW::GpuTexture> PreviewRT;
		FString PreviewOutputName = TEXT("Color0");
		TArray<TSharedPtr<FString>> PreviewOptions;
		FW::Vector2u CachedRTSize = {0, 0};
		TArray<MeshPassColorRT> CachedColorRTSettings;
		bool bCachedDepthEnabled = false;
		MeshPassDepthFormat CachedDepthFormat = MeshPassDepthFormat::D32_FLOAT;
		TSharedPtr<FW::PreviewViewPort> Preview;
		TRefCountPtr<FW::GpuQuerySet> TimestampQuerySet;
		TArray<TRefCountPtr<FW::GpuTexture>> LastActiveColorRTs;
		TRefCountPtr<FW::GpuTexture> LastActiveDepthRT;
		TArray<FW::GpuFormat> LastColorFormats;
		FW::GpuFormat LastDepthFormat = FW::GpuFormat::NUM;
		uint32 LastSampleCount = 1;
		FW::GpuViewPortDesc LastViewPortDesc;
		FW::Vector2f LastMousePos = FW::Vector2f(0, 0);
		TOptional<FW::Camera> LastCamera;

		TArray<FString> GetPreviewOutputNames() const;
		void NormalizePreviewOutputName();
		static FW::GpuFormat ToGpuFormat(MeshPassColorFormat F);
		static FW::GpuFormat ToGpuFormat(MeshPassDepthFormat F);
		static FString ColorPinName(int32 Idx) { return FString::Printf(TEXT("Color%d"), Idx); }
	};
}
