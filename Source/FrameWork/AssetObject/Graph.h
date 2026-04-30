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
		//Check if this input pin can accept a connection from SourcePin (type compatibility only, no value transfer)
		virtual bool CanAccept(GraphPin* SourcePin) { return false; }
		//Called when connected as an input.
		virtual void Accept(GraphPin* SourcePin) {}
        virtual void Refuse() {}
		bool HasLink() const;
		virtual FLinearColor GetPinColor() const { return FLinearColor::White; }
		//The pins connected to the output
        TArray<GraphPin*> GetTargetPins() const;
		GraphNode* GetOwnerNode() const;
		//The output pin relied upon when this pin is an input
		GraphPin* GetSourcePin() const;
		GraphNode* GetSourceNode() const;
		ObserverObjectPtr<GraphPin> SourcePin;
	
		PinDirection Direction = PinDirection::Output;
	};

	class FRAMEWORK_API GraphNode : public ShObject
	{
		REFLECTION_TYPE(GraphNode)
	public:
		GraphNode() = default;

	public:
		virtual TSharedRef<class SGraphNode> CreateNodeWidget(class SGraphPanel* OwnerPanel);
		virtual TSharedPtr<SWidget> ExtraNodeWidget(class SGraphNode* OwnerWidget) { return {}; }
		virtual void Serialize(FArchive& Ar) override;
		virtual FSlateColor GetNodeColor() const;
        virtual ExecRet Exec(GraphExecContext& Context) = 0;
        GraphPin* GetPin(FGuid Id);
        GraphPin* GetPin(const FString& InName);
		GraphPin* GetPin(const FString& InName, PinDirection Direction) const;

		static constexpr float DefaultNodeWidth = 120.0f;
		static constexpr float MinNodeWidth = 120.0f;
		Vector2D Position{0};
		float NodeWidth = DefaultNodeWidth;
		bool IsCollapsed = false;
        TArray<ObjectPtr<GraphPin>> Pins;
		TMultiMap<ObserverObjectPtr<GraphPin>, ObserverObjectPtr<GraphPin>> OutPinToInPin;
        bool AnyError = false;
		bool HasResponse = false;
		bool IsDebugging = false;
		double GpuTimeMs = 0.0;
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
		void PostLoad() override;
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
			GraphNode* OutputNode = Output->GetOwnerNode();
			GraphNode* InputNode = Input->GetOwnerNode();
			check(OutputNode && InputNode);
			OutputNode->OutPinToInPin.AddUnique(Output, Input);
			AddDep(OutputNode, InputNode);
		}

		const TArray<ObjectPtr<GraphNode>>& GetNodes() const { return NodeDatas; }
		void AddDep(GraphNode* Node1, GraphNode* Node2) { NodeDeps.AddUnique(Node1, Node2); }
		void RemoveDep(GraphNode* Node1, GraphNode* Node2) { NodeDeps.Remove(Node1, Node2); }
            
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
		TMultiMap<ObserverObjectPtr<GraphNode>, ObserverObjectPtr<GraphNode>> NodeDeps;
	};

	FRAMEWORK_API extern TMap<FString, TArray<MetaType*>> RegisteredNodes;

#define REGISTER_NODE_TO_GRAPH(NodeType, GraphName)	\
static const int PREPROCESSOR_JOIN(NodeGlobalRegister_,__COUNTER__) = [] {  \
	auto& Nodes = RegisteredNodes.FindOrAdd(GraphName);  \
	Nodes.Add(GetMetaType<NodeType>());	\
	return 0; }();

}
