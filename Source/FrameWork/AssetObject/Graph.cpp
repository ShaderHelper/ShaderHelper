#include "CommonHeader.h"
#include "AssetManager/AssetManager.h"
#include "Graph.h"
#include "UI/Widgets/Graph/SGraphNode.h"
#include "UI/Widgets/Graph/SGraphPanel.h"

#include <Styling/StyleColors.h>
#include <Algo/TopologicalSort.h>

namespace FW
{
	REFLECTION_REGISTER(AddClass<GraphNode>("GraphNode")
								.BaseClass<ShObject>()
	)

	REFLECTION_REGISTER(AddClass<GraphPin>("GraphPin")
								.BaseClass<ShObject>()
	)

	REFLECTION_REGISTER(AddClass<Graph>("Graph")
								.BaseClass<AssetObject>()
	)

	TMap<FString, TArray<MetaType*>> RegisteredNodes;

	void Graph::AddLink(GraphPin* OutputPin, GraphPin* InputPin)
	{
		check(OutputPin && InputPin);
		GraphNode* OutputNode = OutputPin->GetOwnerNode();
		GraphNode* InputNode = InputPin->GetOwnerNode();
		check(OutputNode && InputNode);
		InputPin->Accept(OutputPin);
		InputPin->SourcePin = OutputPin;
		OutputNode->OutPinToInPin.AddUnique(OutputPin, InputPin);
		AddDep(InputNode, OutputNode);
	}

	void Graph::RemoveLink(GraphPin* OutputPin, GraphPin* InputPin)
	{
		check(OutputPin && InputPin);
		GraphNode* OutputNode = OutputPin->GetOwnerNode();
		GraphNode* InputNode = InputPin->GetOwnerNode();
		check(OutputNode && InputNode);
		OutputNode->OutPinToInPin.Remove(OutputPin, InputPin);
		InputPin->SourcePin.Reset();
		InputPin->Refuse();
		RemoveDep(InputNode, OutputNode);
	}

	void Graph::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);

		SerializePolymorphicObjectArray(Ar, NodeDatas, this);

		Ar << NodeDeps;
	}

	const FSlateBrush* Graph::GetImage() const
	{
		return FAppStyle::Get().GetBrush("Icons.Blueprints");
	}

    GraphPin* Graph::GetPin(FGuid Id)
    {
        for(auto Node : NodeDatas)
        {
            GraphPin* Pin = Node->GetPin(Id);
            if(Pin) {
                return Pin;
            }
        }
        return nullptr;
    }

    ExecRet Graph::Exec(GraphExecContext& Context)
	{
		TArray<ObjectPtr<GraphNode>> ExecNodes = NodeDatas;

        AnyError = !Algo::TopologicalSort(ExecNodes, [this](const ObjectPtr<GraphNode>& Element) {
			TArray<ObserverObjectPtr<GraphNode>> DependencyNodePtrs;
			NodeDeps.MultiFind(Element, DependencyNodePtrs);
			TArray<ObjectPtr<GraphNode>> DependencyNodes;
			for (const ObserverObjectPtr<GraphNode>& DependencyNodePtr : DependencyNodePtrs)
			{
				if (GraphNode* Node = DependencyNodePtr.Get())
				{
					DependencyNodes.Add(Node);
				}
			}
			return DependencyNodes;
		});
        
        if(AnyError)
        {
            SH_LOG(LogGraph, Error, TEXT("Cycle not allowed."));
            return {true, true};
        }

		for (auto Node : ExecNodes)
		{
            ExecRet NodeRet = Node->Exec(Context);;
            Node->AnyError = NodeRet.AnyError;
            if(Node->AnyError)
            {
                AnyError = true;
                if(NodeRet.Terminate)
                {
                    return {true ,true};
                }
            }
		}
    
        return {AnyError, false};
	}

	TSharedRef<SGraphNode> GraphNode::CreateNodeWidget(SGraphPanel* OwnerPanel)
	{
		return SNew(SGraphNode, OwnerPanel).NodeData(this);
	}

	void GraphNode::Serialize(FArchive& Ar)
	{
		ShObject::Serialize(Ar);
		Ar << Position;
		Ar << NodeWidth;
		Ar << IsCollapsed;

		SerializePolymorphicObjectArray(Ar, Pins, this);

        
		Ar << OutPinToInPin;
	}

	FSlateColor GraphNode::GetNodeColor() const
	{
		return FStyleColors::Header;
	}

    GraphPin* GraphNode::GetPin(FGuid Id)
    {
        for(GraphPin* Pin : Pins)
        {
            if(Pin->GetGuid() == Id)
                return Pin;
        }
        return nullptr;
    }

    GraphPin* GraphNode::GetPin(const FString& InName)
    {
        for(GraphPin* Pin : Pins)
        {
            if(Pin->ObjectName.ToString() == InName)
                return Pin;
        }
        return nullptr;
    }

	GraphPin* GraphNode::GetPin(const FString& InName, PinDirection Direction) const
	{
		for (GraphPin* Pin : Pins)
		{
			if (Pin && Pin->Direction == Direction && Pin->ObjectName.ToString() == InName)
			{
				return Pin;
			}
		}
		return nullptr;
	}

	bool GraphPin::HasLink() const
	{
		return GetSourceNode() != nullptr || !GetTargetPins().IsEmpty();
	}

    TArray<GraphPin*> GraphPin::GetTargetPins() const
    {
		GraphNode* Owner = GetOwnerNode();
		if (!Owner)
		{
			return {};
		}
        
		TArray<ObserverObjectPtr<GraphPin>> TargetPinPtrs;
		ObserverObjectPtr<GraphPin> ThisPin(const_cast<GraphPin*>(this));
		Owner->OutPinToInPin.MultiFind(ThisPin, TargetPinPtrs);
        
        TArray<GraphPin*> TargetPins;
		for (const ObserverObjectPtr<GraphPin>& TargetPinPtr : TargetPinPtrs)
        {
			if (GraphPin* TargetPin = TargetPinPtr.Get())
			{
				TargetPins.Add(TargetPin);
			}
        }
        
        return TargetPins;
    }

	GraphNode* GraphPin::GetOwnerNode() const
	{
		ShObject* CurOuter = GetOuter();
		while (CurOuter)
		{
			if (CurOuter->DynamicMetaType()->IsDerivedFrom<GraphNode>())
			{
				return static_cast<GraphNode*>(CurOuter);
			}
			CurOuter = CurOuter->GetOuter();
		}
		return nullptr;
	}

	GraphPin* GraphPin::GetSourcePin() const
	{
		return SourcePin.Get();
	}

	GraphNode* GraphPin::GetSourceNode() const
	{
		if(GraphPin* SrcPin = GetSourcePin())
		{
			return SrcPin->GetOwnerNode();
		}
		return nullptr;
	}

	void GraphPin::Serialize(FArchive& Ar)
	{
		ShObject::Serialize(Ar);
		Ar << SourcePin;
		Ar << Direction;
	}

}
