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

	class FRAMEWORK_API GraphPin : public ShObject
	{
		REFLECTION_TYPE(GraphPin)
	public:
		GraphPin() = default;
		GraphPin(const FText& InName, PinDirection InDirection);

	public:
		virtual void Serialize(FArchive& Ar) override;
		virtual bool Accept(GraphPin* TargetPin) { return false; }
		virtual FLinearColor GetPinColor() const { return FLinearColor::White; }

		PinDirection Direction = PinDirection::Output;
	};

	class FRAMEWORK_API GraphNode : public ShObject
	{
		REFLECTION_TYPE(GraphNode)
	public:
		GraphNode() = default;

	public:
		virtual TSharedRef<class SGraphNode> CreateNodeWidget(class SGraphPanel* OwnerPanel);
		virtual TSharedPtr<SWidget> ExtraNodeWidget() { return {}; }
		virtual void Serialize(FArchive& Ar) override;
		virtual FSlateColor GetNodeColor() const;
		virtual TArray<GraphPin*> GetPins() { return {}; }
		virtual void Exec(GraphExecContext& Context) {}

		Vector2D Position{0};
		TMultiMap<FGuid, FGuid> OutPinToInPin;
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
				return Element->GetGuid() == Id;
				});
		}
		TSharedPtr<GraphNode> GetNode(FGuid Id) const {
			return (*NodeDatas.FindByPredicate([Id](const TSharedPtr<GraphNode>& Element) {
				return Element->GetGuid() == Id;
				}));
		}
		const TArray<TSharedPtr<GraphNode>>& GetNodes() const { return NodeDatas; }
		void AddDep(GraphNode* Node1, GraphNode* Node2) { NodeDeps.Add(Node1->GetGuid(), Node2->GetGuid()); }
		void RemoveDep(GraphNode* Node1, GraphNode* Node2) { NodeDeps.Remove(Node1->GetGuid(), Node2->GetGuid()); }

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