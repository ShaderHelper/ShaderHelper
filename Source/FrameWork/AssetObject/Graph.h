#pragma once
#include "AssetObject.h"

namespace FW
{
	struct GraphExecContext {
	};

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
		GraphPin(const FText& InName, PinDirection InDirection) 
			: PinName(InName), Direction(InDirection)
		{}
		virtual ~GraphPin() = default;

	public:
		virtual void Serialize(FArchive& Ar);
		virtual bool Accept(GraphPin* TargetPin) { return false; }
		virtual FLinearColor GetPinColor() const { return FLinearColor::White; }

		FGuid Guid = FGuid::NewGuid();
		FText PinName = FText::FromString("Unknown");
		PinDirection Direction = PinDirection::Output;
	};

	class FRAMEWORK_API GraphNode
	{
		REFLECTION_TYPE(GraphNode)
	public:
		GraphNode() = default;
		virtual ~GraphNode() = default;

	public:
		virtual TSharedRef<class SGraphNode> CreateNodeWidget(class SGraphPanel* OwnerPanel);
		virtual TSharedPtr<SWidget> ExtraNodeWidget() { return {}; }
		virtual void Serialize(FArchive& Ar);
		virtual FSlateColor GetNodeColor() const;
		virtual TArray<GraphPin*> GetPins() { return {}; }
		virtual void Exec(GraphExecContext& Context) {}

		FGuid Guid = FGuid::NewGuid();
		Vector2D Position{0};
		TMultiMap<FGuid, FGuid> OutPinToInPin;
		FText NodeTitle = FText::FromString("Unknown");
	};

	class FRAMEWORK_API Graph : public AssetObject
	{
		REFLECTION_TYPE(Graph)
	public:
		Graph() = default;

	public:
		void AddNode(TSharedPtr<GraphNode> InNode) { NodeDatas.Add(MoveTemp(InNode)); }
		void RemoveNode(FGuid Id) {
			NodeDatas.RemoveAll([Id](const TSharedPtr<GraphNode>& Element) {
				return Element->Guid == Id;
				});
		}
		TSharedPtr<GraphNode> GetNode(FGuid Id) const {
			return (*NodeDatas.FindByPredicate([Id](const TSharedPtr<GraphNode>& Element) {
				return Element->Guid == Id;
				}));
		}
		const TArray<TSharedPtr<GraphNode>>& GetNodes() const { return NodeDatas; }
		void AddDep(GraphNode* Node1, GraphNode* Node2) { NodeDeps.Add(Node1->Guid, Node2->Guid); }
		void RemoveDep(GraphNode* Node1, GraphNode* Node2) { NodeDeps.Remove(Node1->Guid, Node2->Guid); }

	public:
		void Serialize(FArchive& Ar) override;
		const FSlateBrush* GetImage() const override;
		virtual TArray<MetaType*> SupportNodes() const { return {}; }
		virtual void Exec(GraphExecContext& Context);
	
	protected:
		//Keep layer order
		TArray<TSharedPtr<GraphNode>> NodeDatas;

		TMultiMap<FGuid, FGuid> NodeDeps;
	};
}