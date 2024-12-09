#include "CommonHeader.h"
#include "SGraphNode.h"
#include "AssetObject/Graph.h"

namespace FRAMEWORK
{
	
	void SGraphNode::Construct(const FArguments& InArgs)
	{
		NodeData = InArgs._NodeData;
	}

	Vector2D SGraphNode::GetPosition() const
	{
		return NodeData->Position;
	}

}
