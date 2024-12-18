#include "CommonHeader.h"
#include "Graph.h"
#include "UI/Widgets/Graph/SGraphNode.h"

namespace FRAMEWORK
{
	GLOBAL_REFLECTION_REGISTER(AddClass<GraphNode>("GraphNode"))

	GLOBAL_REFLECTION_REGISTER(AddClass<Graph>()
								.BaseClass<AssetObject>()
	)

	Graph::Graph()
	{

	}

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
				FString TypeName = GetRegisteredName(NodeDatas[Index]->MetaType());
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
	}

	const FSlateBrush* Graph::GetImage() const
	{
		return FAppStyle::Get().GetBrush("Icons.Blueprints");
	}

	TSharedRef<SGraphNode> GraphNode::CreateNodeWidget()
	{
		return SNew(SGraphNode).NodeData(this);
	}

	void GraphNode::Serialize(FArchive& Ar)
	{

	}

}