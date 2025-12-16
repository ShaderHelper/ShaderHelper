#include "CommonHeader.h"
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

	void Graph::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);
		
		int NodeNum = NodeDatas.Num();
		Ar << NodeNum;
		if (Ar.IsSaving())
		{
			for (int Index = 0; Index < NodeNum; Index++)
			{
				//Serialize polymorphic pointers
				FString TypeName = GetRegisteredName(NodeDatas[Index]->DynamicMetaType());
				Ar << TypeName;
				NodeDatas[Index]->Serialize(Ar);
			}
		}
		else
		{
			NodeDatas.Reserve(NodeNum);
			for (int Index = 0; Index < NodeNum; Index++)
			{
				FString TypeName;
				Ar << TypeName;
				GraphNode* LoadedNodeData = static_cast<GraphNode*>(GetMetaType(TypeName)->Construct());
                LoadedNodeData->SetOuter(this);
				LoadedNodeData->Serialize(Ar);
				NodeDatas.Emplace(LoadedNodeData);
			}
		}

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
			TArray<FGuid> DepIds;
			NodeDeps.MultiFind(Element->GetGuid(), DepIds);
			TArray<ObjectPtr<GraphNode>> DepNodes;
			for (FGuid Id : DepIds)
			{
				DepNodes.Add(GetNode(Id));
			}
			return DepNodes;
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
        
        int PinNum = Pins.Num();
        Ar << PinNum;
        if (Ar.IsSaving())
        {
            for (int Index = 0; Index < PinNum; Index++)
            {
                //Serialize polymorphic pointers
                FString TypeName = GetRegisteredName(Pins[Index]->DynamicMetaType());
                Ar << TypeName;
                Pins[Index]->Serialize(Ar);
            }
        }
        else
        {
            Pins.Reserve(PinNum);
            for (int Index = 0; Index < PinNum; Index++)
            {
                FString TypeName;
                Ar << TypeName;
                GraphPin* LoadedPinData = static_cast<GraphPin*>(GetMetaType(TypeName)->Construct());
                LoadedPinData->SetOuter(this);
                LoadedPinData->Serialize(Ar);
                Pins.Emplace(LoadedPinData);
            }
        }

        
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

    TArray<GraphPin*> GraphPin::GetTargetPins() const
    {
        GraphNode* Owner = static_cast<GraphNode*>(GetOuter());
        Graph* OwnerGraph = static_cast<Graph*>(Owner->GetOuter());
        
        TArray<FGuid> TargetPinGuids;
        Owner->OutPinToInPin.MultiFind(GetGuid(), TargetPinGuids);
        
        TArray<GraphPin*> TargetPins;
        for(FGuid PinGuid :TargetPinGuids)
        {
            TargetPins.Add(OwnerGraph->GetPin(PinGuid));
        }
        
        return TargetPins;
    }

	GraphPin* GraphPin::GetSourcePin() const
	{
		GraphNode* Owner = static_cast<GraphNode*>(GetOuter());
		Graph* OwnerGraph = static_cast<Graph*>(Owner->GetOuter());
		return OwnerGraph->GetPin(SourcePin);
	}

	GraphNode* GraphPin::GetSourceNode() const
	{
		if(GraphPin* SrcPin = GetSourcePin())
		{
			return static_cast<GraphNode*>(SrcPin->GetOuter());
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
