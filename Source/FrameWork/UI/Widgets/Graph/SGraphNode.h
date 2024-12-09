#pragma once

namespace FRAMEWORK
{
	class SGraphNode : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SGraphNode) : _NodeData(nullptr)
			{}
			SLATE_ARGUMENT(class GraphNode*, NodeData)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

	public:
		Vector2D GetPosition() const;

	private:
		class GraphNode* NodeData;
	};
}
