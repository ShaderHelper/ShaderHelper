#pragma once
#include "Editor/AssetEditor/ShAssetEditor.h"
#include "AssetObject/Graph.h"
#include "AssetObject/Texture3D.h"
#include "Editor/PreviewViewPort.h"
#include "AssetManager/AssetManager.h"

namespace SH
{
	class Texture3dNodeOp : public ShPropertyOp
	{
		REFLECTION_TYPE(Texture3dNodeOp)
	public:
		Texture3dNodeOp() = default;

		FW::MetaType* SupportType() override;
	};

	class Texture3dNode : public FW::GraphNode
	{
		REFLECTION_TYPE(Texture3dNode)
	public:
		Texture3dNode();
		~Texture3dNode();
		Texture3dNode(FW::AssetPtr<FW::Texture3D> InTexture);
		void Init() override;

	public:
		void InitTexture();
		void InitPins();
		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		FSlateColor GetNodeColor() const override { return FLinearColor{0.34f, 0.24f, 0.5f}; }
		TSharedPtr<SWidget> ExtraNodeWidget() override;
		FW::ExecRet Exec(FW::GraphExecContext& Context) override;

		void PostPropertyChanged(FW::PropertyData* InProperty) override;

	private:
		void RefreshPreview();
		void RefreshProperty();
		void ClearProperty();

	public:
		FW::AssetPtr<FW::Texture3D> Texture;
		uint32 Width{};
		uint32 Height{};
		uint32 Depth{};
		FW::GpuFormat Format;

	private:
		TSharedPtr<FW::PreviewViewPort> Preview = MakeShared<FW::PreviewViewPort>();
	};
}
