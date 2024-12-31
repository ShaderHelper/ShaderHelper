#pragma once
#include "SGraphNode.h"
#include "AssetObject/Graph.h"
#include "UI/Widgets/Graph/SGraphPin.h"

namespace FRAMEWORK
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
		void Clear();
		void SetGraphData(Graph* InGraphData);
		void SetSelectedNode(TSharedPtr<SGraphNode> InNode);

		virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
		virtual FVector2D ComputeDesiredSize(float) const override;
		virtual FChildren* GetChildren() override;

		virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
		virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
		virtual bool SupportsKeyboardFocus() const override { return true; }
		FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
		void DrawConnection(const FPaintGeometry& PaintGeometry, FSlateWindowElementList& OutDrawElements, int32 Layer, PinDirection InStartDir, const Vector2D& Start, const Vector2D& End) const;
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

		TSharedRef<SWidget> CreateContextMenu();
		TSharedRef<ITableRow> GenerateNodeItems(TSharedPtr<FText> Item, const TSharedRef<STableViewBase>& OwnerTable);
		void OnMenuItemSelected(TSharedPtr<FText> InSelectedItem, ESelectInfo::Type SelectInfo);

		void DeleteNode(TSharedPtr<SGraphNode> InNode);
		
		TSharedPtr<SGraphNode> AddNodeFromData(GraphNode* InNodeData);
		void AddLink(SGraphPin* Output, SGraphPin* Input);
		void RemoveInputLink(SGraphPin* Input);

		//GraphCoord center is (0,0) 
		Vector2D PanelCoordToGraphCoord(const Vector2D& InCoord) const;
		Vector2D GraphCoordToPanelCoord(const Vector2D& InCoord) const;

	public:
		PinDirection PreviewStartDir;
		TOptional<Vector2D> PreviewStart;
		Vector2D PreviewEnd;

		TOptional<Vector2D> CutLineStart;
		Vector2D CutLineEnd;

	protected:
		Graph* GraphData;
		TSlotlessChildren<SGraphNode> Nodes;
		Vector2D MousePos;
		Vector2D ViewOffset;
		float ZoomValue;
		mutable int32 CurMaxNodeLayer;
		TWeakPtr<SGraphNode> SelectedNode;
		TSharedPtr<FUICommandList> UICommandList;
		TArray<TSharedPtr<FText>> MenuNodeItems;

		//OutputPin to InputPin
		TMultiMap<SGraphPin*, SGraphPin*> Links;
	};
	
}


