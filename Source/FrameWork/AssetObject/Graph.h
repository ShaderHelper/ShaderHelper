#pragma once
#include "AssetObject.h"

namespace FRAMEWORK
{
	class FRAMEWORK_API GraphNode
	{
		REFLECTION_TYPE(GraphNode)
	public:
		GraphNode() = default;
		virtual ~GraphNode() = default;

	public:
		virtual TSharedRef<class SGraphNode> CreateNodeWidget();
		virtual void Serialize(FArchive& Ar);

		Vector2D Position;
	};

	class FRAMEWORK_API Graph : public AssetObject
	{
		REFLECTION_TYPE(Graph)
	public:
		Graph();

	public:
		void Serialize(FArchive& Ar) override;
		const FSlateBrush* GetImage() const override;

		TArray<TSharedPtr<GraphNode>> NodeDatas;
	};
}