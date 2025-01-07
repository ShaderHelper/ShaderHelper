#include "CommonHeader.h"
#include "Graph.h"
#include "UI/Widgets/Graph/SGraphNode.h"
#include "UI/Widgets/Graph/SGraphPanel.h"
#include <Styling/StyleColors.h>
#include <Algo/TopologicalSort.h>

namespace FW
{
	GLOBAL_REFLECTION_REGISTER(AddClass<GraphNode>("GraphNode")
								.BaseClass<ShObject>()
	)

	GLOBAL_REFLECTION_REGISTER(AddClass<GraphPin>()
								.BaseClass<ShObject>()
	)

	GLOBAL_REFLECTION_REGISTER(AddClass<Graph>()
								.BaseClass<AssetObject>()
	)

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

	void Graph::Exec(GraphExecContext& Context)
	{
		TArray<TSharedPtr<GraphNode>> ExecNodes = NodeDatas;

		bool bHasSucceeded = Algo::TopologicalSort(ExecNodes, [this](const TSharedPtr<GraphNode>& Element) {
			TArray<FGuid> DepIds;
			NodeDeps.MultiFind(Element->GetGuid(), DepIds);
			TArray<TSharedPtr<GraphNode>> DepNodes;
			for (FGuid Id : DepIds)
			{
				DepNodes.Add(GetNode(Id));
			}
			return DepNodes;
		});

		for (auto Node : ExecNodes)
		{
			Node->Exec(Context);
		}
	}

	TSharedRef<SGraphNode> GraphNode::CreateNodeWidget(SGraphPanel* OwnerPanel)
	{
		return SNew(SGraphNode, OwnerPanel).NodeData(this);
	}

	void GraphNode::Serialize(FArchive& Ar)
	{
		ShObject::Serialize(Ar);
		Ar << Position;
		Ar << OutPinToInPin;
	}

	FSlateColor GraphNode::GetNodeColor() const
	{
		return FStyleColors::Title;
	}

	GraphPin::GraphPin(const FText& InName, PinDirection InDirection)
		: Direction(InDirection)
	{
		ObjectName = InName;
	}

	void GraphPin::Serialize(FArchive& Ar)
	{
		ShObject::Serialize(Ar);
		Ar << Direction;
	}

}