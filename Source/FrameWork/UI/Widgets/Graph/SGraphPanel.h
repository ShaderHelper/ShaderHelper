#pragma once
#include "SGraphNode.h"
#include "AssetObject/Graph.h"
#include "UI/Widgets/Graph/SGraphPin.h"

namespace FW
{
	class GraphDragDropOp : public FDragDropOperation
	{
	public:
		DRAG_DROP_OPERATOR_TYPE(GraphDragDropOp, FDragDropOperation)

		GraphDragDropOp(SGraphPin* InPin) : StartPin(InPin) {}

		SGraphPin* StartPin;
	};

	class FRAMEWORK_API SGraphPanel : public SPanel
	{
	public:
		SLATE_BEGIN_ARGS(SGraphPanel) : _GraphData(nullptr)
			{}
			SLATE_ARGUMENT(class Graph*, GraphData)
		SLATE_END_ARGS()

		SGraphPanel();
		void Construct(const FArguments& InArgs);
		
	public:
		void AddNode(ObjectPtr<GraphNode> NewNodeData);
		
		void Clear();
		void SetGraphData(Graph* InGraphData);
		Graph* GetGraphData() const { return GraphData; }
		void AddSelectedNode(TSharedRef<SGraphNode> InNode);
		void ClearSelectedNode();
		bool IsSelectedNode(SGraphNode* InNode) const { return SelectedNodes.Contains(InNode); }
		bool IsMultiSelect() const { return SelectedNodes.Num() > 1; }
		Vector2D GetMousePos() const { return MousePos; }

		virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
		virtual FVector2D ComputeDesiredSize(float) const override;
		virtual FChildren* GetChildren() override;

		virtual FReply OnPreviewMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual void OnDragEnter(FGeometry const& MyGeometry, FDragDropEvent const& DragDropEvent) override;
		void OnDragLeave(FDragDropEvent const& DragDropEvent) override;
		virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
		virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
		virtual bool SupportsKeyboardFocus() const override { return true; }
		FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
		void DrawConnection(const FPaintGeometry& PaintGeometry, FSlateWindowElementList& OutDrawElements, int32 Layer, PinDirection InStartDir, const Vector2D& Start, const Vector2D& End) const;
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

		void GenerateMenuNodeItems(const FText& InFilterText = FText::GetEmpty());
		TSharedRef<SWidget> CreateContextMenu();
		TSharedRef<ITableRow> GenerateNodeItems(TSharedPtr<FText> Item, const TSharedRef<STableViewBase>& OwnerTable);
		void OnMenuItemSelected(TSharedPtr<FText> InSelectedItem, ESelectInfo::Type SelectInfo);

		void DeleteSelectedNodes();
		void DeleteNode(SGraphNode* Node);
		
		SGraphPin* GetGraphPin(FGuid PinId);
		SGraphPin* GetOuputPinInLink(SGraphPin* InputPin) const;
		void AddLink(SGraphPin* Output, SGraphPin* Input);
		void RemoveLink(SGraphPin* Input);

		//GraphCoord center is (0,0) 
		Vector2D PanelCoordToGraphCoord(const Vector2D& InCoord) const;
		Vector2D GraphCoordToPanelCoord(const Vector2D& InCoord) const;

	public:
		PinDirection PreviewStartDir;
		TOptional<Vector2D> PreviewStart;
		Vector2D PreviewEnd;

		TOptional<Vector2D> CutLineStart;
		Vector2D CutLineEnd;

		TOptional<Vector2D> MarqueeStart;
		Vector2D MarqueeEnd;

	protected:
		Graph* GraphData;
		TSlotlessChildren<SGraphNode> Nodes;
		Vector2D MousePos;
		Vector2D ViewOffset;
		float ZoomValue;
		TArray<SGraphNode*> SelectedNodes;
		TSharedPtr<FUICommandList> UICommandList;
		TArray<TSharedPtr<FText>> MenuNodeItems;
		TSharedPtr<SListView<TSharedPtr<FText>>> MenuNodeList;

		//OutputPin to InputPin
		TMultiMap<SGraphPin*, SGraphPin*> Links;
	};
	
}


