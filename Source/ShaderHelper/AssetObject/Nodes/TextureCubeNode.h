#pragma once
#include "Editor/AssetEditor/ShAssetEditor.h"
#include "AssetObject/Graph.h"
#include "AssetObject/TextureCube.h"
#include "Editor/PreviewViewPort.h"
#include "AssetManager/AssetManager.h"

namespace SH
{
	class TextureCubeNodeOp : public ShPropertyOp
	{
		REFLECTION_TYPE(TextureCubeNodeOp)
	public:
		TextureCubeNodeOp() = default;

		FW::MetaType* SupportType() override;
	};

	class TextureCubeNode : public FW::GraphNode
	{
		REFLECTION_TYPE(TextureCubeNode)
	public:
		TextureCubeNode();
		~TextureCubeNode();
		TextureCubeNode(FW::AssetPtr<FW::TextureCube> InTexture);
		void Init() override;

	public:
		void InitTexture();
		void InitPins();
		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		FSlateColor GetNodeColor() const override { return FLinearColor{0.19f, 0.05f, 0.19f}; }
		TSharedPtr<SWidget> ExtraNodeWidget() override;
		FW::ExecRet Exec(FW::GraphExecContext& Context) override;

		void PostPropertyChanged(FW::PropertyData* InProperty) override;

	private:
		void RefreshPreview();
		void RefreshProperty();
		void ClearProperty();

	public:
		FW::AssetPtr<FW::TextureCube> Texture;
		uint32 Size{};
		FW::GpuTextureFormat Format;

	private:
		TSharedPtr<FW::PreviewViewPort> Preview = MakeShared<FW::PreviewViewPort>();
	};
}
