#pragma once
#include "AssetObject.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGraph, Log, All);
inline DEFINE_LOG_CATEGORY(LogGraph);

namespace FW
{
	struct GraphExecContext {
	};

    struct ExecRet
    {
        bool AnyError;
        bool Terminate;
    };

	enum class PinDirection
	{
		Input,
		Output
	};

    class GraphNode;

	class FRAMEWORK_API GraphPin : public ShObject
	{
		REFLECTION_TYPE(GraphPin)
	public:
		GraphPin() = default;

	public:
		virtual void Serialize(FArchive& Ar) override;
		//Called when connected as an input.
		virtual bool Accept(GraphPin* SourcePin) { return false; }
        virtual void Refuse() {}
		bool HasLink() const;
		virtual FLinearColor GetPinColor() const { return FLinearColor::White; }
		//The pins connected to the output
        TArray<GraphPin*> GetTargetPins() const;
		//The output pin relied upon when this pin is an input
		GraphPin* GetSourcePin() const;
		GraphNode* GetSourceNode() const;
		FGuid SourcePin;
	
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
        virtual ExecRet Exec(GraphExecContext& Context) = 0;
        GraphPin* GetPin(FGuid Id);
        GraphPin* GetPin(const FString& InName);

		Vector2D Position{0};
        TArray<ObjectPtr<GraphPin>> Pins;
		TMultiMap<FGuid, FGuid> OutPinToInPin;
        bool AnyError = false;
		bool HasResponse = false;
		bool IsDebugging = false;
	};

	DECLARE_MULTICAST_DELEGATE_OneParam(OnAddNodeDelegate, ObjectPtr<GraphNode>)
	DECLARE_MULTICAST_DELEGATE_OneParam(OnRemoveNodeDelegate, FGuid)

	class FRAMEWORK_API Graph : public AssetObject
	{
		REFLECTION_TYPE(Graph)
	public:
		Graph() = default;

	public:
		void AddNode(ObjectPtr<GraphNode> InNode) { 
			NodeDatas.Add(InNode);
			AddNodeHandler.Broadcast(MoveTemp(InNode));
		}
		void RemoveNode(FGuid Id) {
			NodeDatas.RemoveAll([Id](const ObjectPtr<GraphNode>& Element) {
				return Element->GetGuid() == Id;
			});
			RemoveNodeHandler.Broadcast(Id);
		}
        ObjectPtr<GraphNode> GetNode(FGuid Id) const {
			auto* ResultPtr = NodeDatas.FindByPredicate([Id](const ObjectPtr<GraphNode>& Element) {
				return Element->GetGuid() == Id;
			});
			if (ResultPtr)
			{
				return *ResultPtr;
			}
			return nullptr;
		}

		void AddLink(GraphPin* Output, GraphPin* Input)
		{
			GraphNode* OutputNode = static_cast<GraphNode*>(Output->GetOuter());
			GraphNode* InputNode = static_cast<GraphNode*>(Input->GetOuter());
			OutputNode->OutPinToInPin.AddUnique(Output->GetGuid(), Input->GetGuid());
			AddDep(OutputNode, InputNode);
		}

		const TArray<ObjectPtr<GraphNode>>& GetNodes() const { return NodeDatas; }
		void AddDep(GraphNode* Node1, GraphNode* Node2) { NodeDeps.Add(Node1->GetGuid(), Node2->GetGuid()); }
		void RemoveDep(GraphNode* Node1, GraphNode* Node2) { NodeDeps.Remove(Node1->GetGuid(), Node2->GetGuid()); }
            
        GraphPin* GetPin(FGuid Id);
	public:
		void Serialize(FArchive& Ar) override;
		const FSlateBrush* GetImage() const override;
		virtual void OnDragEnter(TSharedPtr<FDragDropOperation> DragDropOp) {}
		virtual void OnDrop(TSharedPtr<FDragDropOperation> DragDropOp, const Vector2D& Pos) {}
		virtual ExecRet Exec(GraphExecContext& Context);
		
    public:
        bool AnyError = false;
		OnAddNodeDelegate AddNodeHandler;
		OnRemoveNodeDelegate RemoveNodeHandler;
	
	protected:
		//Keep layer order
		TArray<ObjectPtr<GraphNode>> NodeDatas;
		TMultiMap<FGuid, FGuid> NodeDeps;
	};

	FRAMEWORK_API extern TMap<FString, TArray<MetaType*>> RegisteredNodes;

#define REGISTER_NODE_TO_GRAPH(NodeType, GraphName)	\
static const int PREPROCESSOR_JOIN(NodeGlobalRegister_,__COUNTER__) = [] {  \
	auto& Nodes = RegisteredNodes.FindOrAdd(GraphName);  \
	Nodes.Add(GetMetaType<NodeType>());	\
	return 0; }();

}
