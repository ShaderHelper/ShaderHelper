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
		void SetOwner(class SGraphPanel* InOwner) { Owner = InOwner; }
		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		TSharedRef<SWidget> CreateContextMenu();
		void OnHandleDeleteAction();
		virtual bool SupportsKeyboardFocus() const override { return true; }
		FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	public:
		class GraphNode* NodeData;
		SGraphPanel* Owner;
		TSharedPtr<FUICommandList> UICommandList;
	};
}
