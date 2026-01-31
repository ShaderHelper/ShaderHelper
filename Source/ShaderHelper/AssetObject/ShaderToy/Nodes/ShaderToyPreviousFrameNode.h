#pragma once
#include "AssetObject/Graph.h"
#include "AssetObject/ShaderToy/ShaderToy.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyPassNode.h"

namespace SH
{
	class ShaderToyPreviousFrameNodeOp : public FW::ShObjectOp
	{
		REFLECTION_TYPE(ShaderToyPreviousFrameNodeOp)
	public:
		ShaderToyPreviousFrameNodeOp() = default;

		FW::MetaType* SupportType() override;
	};

	class ShaderToyPreviousFrameNode : public FW::GraphNode
	{
		REFLECTION_TYPE(ShaderToyPreviousFrameNode)
	public:
		ShaderToyPreviousFrameNode();
		ShaderToyPreviousFrameNode(ShaderToyPassNode* InPassNode);
		~ShaderToyPreviousFrameNode();
		void Init() override;

	public:
		void InitPins();
		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		TSharedPtr<SWidget> ExtraNodeWidget() override;
		TArray<TSharedRef<FW::PropertyData>>* GetPropertyDatas() override;
		FSlateColor GetNodeColor() const override { return FLinearColor{ 0.27f, 0.13f, 0.0f }; }
		FW::ExecRet Exec(FW::GraphExecContext& Context) override;
		ShaderToyPassNode* GetPassNode();

	private:
		void UpdatePassNodes();

	private:
		FGuid PassNode;
		TArray<TSharedPtr<FGuid>> AllPassNodes;
		TRefCountPtr<FW::GpuTexture> PreFramePassTex;
	};
}
