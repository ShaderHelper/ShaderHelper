#pragma once
#include "Editor/AssetEditor/ShAssetEditor.h"
#include "AssetObject/Graph.h"
#include "AssetObject/Texture2D.h"
#include "Editor/PreviewViewPort.h"
#include "AssetManager/AssetManager.h"

namespace SH
{
	class Texture2dNodeOp : public ShPropertyOp
	{
		REFLECTION_TYPE(Texture2dNodeOp)
	public:
		Texture2dNodeOp() = default;
		
		FW::MetaType* SupportType() override;
	};

	enum class TextureChannelFilter
	{
		None,
		R,
		G,
		B,
		A
	};

	class Texture2dNode : public FW::GraphNode
	{
		REFLECTION_TYPE(Texture2dNode)
	public:
		Texture2dNode();
		~Texture2dNode();
		Texture2dNode(FW::AssetPtr<FW::Texture2D> InTexture);
		void Init() override;
		
	public:
		void InitTexture();
		void InitPins();
		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		FSlateColor GetNodeColor() const override { return FLinearColor{ 0.05f, 0.19f, 0.0f }; }
		TSharedPtr<SWidget> ExtraNodeWidget() override;
		FW::ExecRet Exec(FW::GraphExecContext& Context) override;
		
		void PostPropertyChanged(FW::PropertyData* InProperty) override;
		
	private:
		void RefershPreview();
		void RefreshProprety();
		void ClearProperty();
		
	public:
		FW::AssetPtr<FW::Texture2D> Texture;
		FW::GpuTextureFormat Format;
		uint32 Width{};
		uint32 Height{};
		
	private:
		TextureChannelFilter ChannelFilter = TextureChannelFilter::None;
		TSharedPtr<FW::PreviewViewPort> Preview = MakeShared<FW::PreviewViewPort>();
		
	};
}
