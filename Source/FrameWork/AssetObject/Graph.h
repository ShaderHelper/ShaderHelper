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
		GraphPin(const FText& InName, PinDirection InDirection) 
			: PinName(InName), Direction(InDirection)
		{}
		virtual ~GraphPin() = default;

	public:
		virtual void Serialize(FArchive& Ar);
		virtual void LinkTo(GraphPin* TargetPin) {}
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
		virtual TSharedRef<class SGraphNode> CreateNodeWidget();
		virtual TSharedPtr<SWidget> ExtraNodeWidget() { return {}; }
		virtual void Serialize(FArchive& Ar);
		virtual FText GetNodeTitle() const { return FText::FromString("Unknown"); }
		virtual FSlateColor GetNodeColor() const;
		virtual TArray<GraphPin*> GetPins() { return {}; }
		virtual void Exec() {}

		FGuid Guid = FGuid::NewGuid();
		Vector2D Position{0};
		TOptional<int32> Layer;
		TMultiMap<FGuid, FGuid> OutPinToInPin;
		TMap<FGuid, int> TargetNodeCounts;
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
		void RemoveNode(FGuid Id) {
			NodeDatas.RemoveAll([Id](const TSharedPtr<GraphNode>& Element) {
				return Element->Guid == Id;
			});
		}

		GraphNode* GetNode(FGuid Id) const {
			return (*NodeDatas.FindByPredicate([Id](const TSharedPtr<GraphNode>& Element) {
				return Element->Guid == Id;
			})).Get();
		}
		const TArray<TSharedPtr<GraphNode>>& GetNodes() const { return NodeDatas; }

		virtual TArray<MetaType*> SupportNodes() const { return {}; }
	
	protected:
		TArray<TSharedPtr<GraphNode>> NodeDatas;
		TMultiMap<FGuid, FGuid> NodeDeps;
	};
}