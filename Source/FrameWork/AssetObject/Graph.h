#pragma once
#include "AssetObject.h"

namespace FRAMEWORK
{
	enum class PinDirection
	{
		Input,
		Output
	};

	class FRAMEWORK_API GraphPin
	{
		REFLECTION_TYPE(GraphPin)
	public:
		GraphPin() = default;
		GraphPin(const FText& InName, PinDirection InDirection, FLinearColor InColor = FLinearColor::White) 
			: PinName(InName), Direction(InDirection), PinColor(InColor)
		{}
		virtual ~GraphPin() = default;

	public:
		virtual void Serialize(FArchive& Ar);
		virtual void LinkTo(GraphPin* TargetPin) {}

		FGuid Guid = FGuid::NewGuid();
		FText PinName = FText::FromString("Unknown");
		FLinearColor PinColor = FLinearColor::White;
		PinDirection Direction = PinDirection::Output;
	};

	class FRAMEWORK_API GraphNode
	{
		REFLECTION_TYPE(GraphNode)
	public:
		GraphNode() = default;
		virtual ~GraphNode() = default;

	public:
		virtual TSharedRef<class SGraphNode> CreateNodeWidget();
		virtual TSharedPtr<SWidget> ExtraNodeWidget() { return {}; }
		virtual void Serialize(FArchive& Ar);
		virtual FText GetNodeTitle() const { return FText::FromString("Unknown"); }
		virtual FSlateColor GetNodeColor() const;
		virtual TArray<GraphPin*> GetPins() { return {}; }
		virtual void Exec() {}

		FGuid Guid = FGuid::NewGuid();
		TOptional<Vector2D> Position;
		TMultiMap<GraphPin*, GraphPin*> OutPinToInPin;
		TMap<GraphNode*, int> TargetNodeCounts;
	};

	class FRAMEWORK_API Graph : public AssetObject
	{
		REFLECTION_TYPE(Graph)
	public:
		Graph() = default;

	public:
		void Serialize(FArchive& Ar) override;
		const FSlateBrush* GetImage() const override;
		void AddNode(TSharedPtr<GraphNode> InNode) { NodeDatas.Add(MoveTemp(InNode)); }
		const TArray<TSharedPtr<GraphNode>>& GetNodes() const { return NodeDatas; }
	
	protected:
		TArray<TSharedPtr<GraphNode>> NodeDatas;
		TMultiMap<TSharedPtr<GraphNode>, TSharedPtr<GraphNode>> NodeDeps;
	};
}