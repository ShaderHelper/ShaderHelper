#pragma once
#include "AssetObject/Graph.h"
#include "AssetObject/Render/Render.h"
#include "AssetObject/Render/CameraSceneObject.h"
#include "Editor/AssetEditor/ShAssetEditor.h"
#include "AssetObject/Render/MeshRenderObject.h"

namespace FW
{
	class PreviewViewPort;
	class GpuQuerySet;
	class RenderGraph;
	struct GpuRenderPassDesc;
	struct RGRenderPass;
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

	struct MeshPassDebugFrameState
	{
		TOptional<FW::Camera> Camera;
		FW::Vector2f MousePos = FW::Vector2f(0, 0);
		float Time = 0.0f;
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

		void RebuildPins();
		// Add/remove MeshRenderObject helpers (used by drag-drop + combo button).
		MeshRenderObject* AddMeshRenderObject(MeshSceneObject* InMeshSceneObject);
		void RemoveMeshRenderObject(MeshRenderObject* InObject);

		// Called by property panel after Color RT / Depth target / count changed.
		void OnRenderTargetsChanged();
		void RefreshNodeWidget();
		DebugTargetInfo MakeDebugTargetInfo(MeshRenderObject* StopObject);
		const TArray<TRefCountPtr<FW::GpuTexture>>& GetOutputColorRTs() const { return CachedColorRTs; }
		TRefCountPtr<FW::GpuTexture> GetOutputDepthRT() const { return CachedDepthRT; }
		FW::GpuViewPortDesc GetOutputViewPortDesc() const { return FW::GpuViewPortDesc{ (float)CachedRTSize.X, (float)CachedRTSize.Y }; }
		const MeshPassDebugFrameState& GetDebugFrameState() const { return DebugFrameState; }

	public:
		TArray<MeshPassColorRT> ColorRTs;
		bool bDepthEnabled = false;
		MeshPassDepthFormat DepthFormat = MeshPassDepthFormat::D32_FLOAT;
		FW::ObserverObjectPtr<CameraSceneObject> CameraRef;
		FW::Vector2u RTSize = {1, 1};
		TArray<FW::ObjectPtr<MeshRenderObject>> MeshRenderObjects;

	private:
		struct MeshPassInputState
		{
			TArray<uint8> ColorInputConnected;
			TArray<TRefCountPtr<FW::GpuTexture>> ColorInputSources;
			TRefCountPtr<FW::GpuTexture> DepthInputSource;
			bool bHasDepthInput = false;
		};

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
		MeshPassDebugFrameState DebugFrameState;

		TArray<FString> GetPreviewOutputNames() const;
		void NormalizePreviewOutputName();
		DebugTargetInfo MakeDebugTargetInfoFromRTs(const TArray<TRefCountPtr<FW::GpuTexture>>& ColorRTs, TRefCountPtr<FW::GpuTexture> DepthRT, TRefCountPtr<FW::GpuTexture> CoverageMask) const;
		MeshPassInputState CollectInputState(uint32 Width, uint32 Height, bool& bOutValid) const;
		void AddInputBlitPasses(FW::RenderGraph& RG, const MeshPassInputState& InputState, const TArray<TRefCountPtr<FW::GpuTexture>>& TargetColorRTs, TRefCountPtr<FW::GpuTexture> TargetDepthRT) const;
		FW::GpuRenderPassDesc MakeRenderPassDesc(const TArray<TRefCountPtr<FW::GpuTexture>>& TargetColorRTs, TRefCountPtr<FW::GpuTexture> TargetDepthRT, const TArray<uint8>& ColorInputConnected, bool bHasDepthInput) const;
		TArray<FW::ObjectPtr<MeshRenderObject>> CollectMeshRenderObjectsThrough(MeshRenderObject* StopObject) const;
		TArray<FW::GpuFormat> MakeColorFormatKey() const;
		TRefCountPtr<FW::GpuTexture> CreateColorRT(int32 ColorIndex, uint32 Width, uint32 Height) const;
		TRefCountPtr<FW::GpuTexture> CreateDepthRT(uint32 Width, uint32 Height) const;
		FW::RGRenderPass& AddMeshDrawPass(FW::RenderGraph& RG, const FString& Name, FW::GpuRenderPassDesc PassDesc, const TArray<FW::ObjectPtr<MeshRenderObject>>& MROs, const FW::Vector2f& ViewportSize) const;
		static FW::GpuFormat ToGpuFormat(MeshPassColorFormat F);
		static FW::GpuFormat ToGpuFormat(MeshPassDepthFormat F);
		static FString ColorPinName(int32 Idx) { return FString::Printf(TEXT("Color%d"), Idx); }
	};
}
