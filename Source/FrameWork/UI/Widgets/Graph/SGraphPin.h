#pragma once

namespace FW
{
	class SGraphPin : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SGraphPin) : _PinData(nullptr)
			{}
			SLATE_ARGUMENT(class GraphPin*, PinData)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, class SGraphNode* InOwner);
		virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
		virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

		GraphPin* PinData;
		SGraphNode* Owner;
		class SGraphPanel* OwnerPanel;
	};
}