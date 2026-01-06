#pragma once
#include "SGraphNode.h"
#include "AssetObject/Graph.h"
#include "UI/Widgets/Graph/SGraphPin.h"

namespace FW
{
	class SGraphPanel;

	class GraphDragDropOp : public FDragDropOperation
	{
	public:
		DRAG_DROP_OPERATOR_TYPE(GraphDragDropOp, FDragDropOperation)

		GraphDragDropOp(SGraphPin* InPin) : StartPin(InPin) {}

		SGraphPin* StartPin;
	};

	class FRAMEWORK_API GraphCommand {
	public:
		GraphCommand(SGraphPanel* InGraphPanel) : GraphPanel(InGraphPanel) {}
		virtual ~GraphCommand() = default;
		virtual void Do() {}
		virtual void Undo() {}
	protected:
		SGraphPanel* GraphPanel;
	};

	struct GraphState
	{
		TArray<TSharedPtr<GraphCommand>> Commands;
	};

	class FRAMEWORK_API RenameNodeCommand : public GraphCommand {
	public:
		RenameNodeCommand(SGraphPanel* InGraphPanel, ObjectPtr<GraphNode> InNodeData, const FText& InOldName, const FText& InNewName)
			: GraphCommand(InGraphPanel)
			, NodeData(InNodeData), OldName(InOldName), NewName(InNewName)
		{}
		void Do() override;
		void Undo() override;
	private:
		ObjectPtr<GraphNode> NodeData;
		FText OldName, NewName;
	};

	class FRAMEWORK_API MoveNodeCommand : public GraphCommand {
	public:
		MoveNodeCommand(SGraphPanel* InGraphPanel, ObjectPtr<GraphNode> InNodeData, const Vector2D& InOldPos, const Vector2D& InNewPos)
			: GraphCommand(InGraphPanel)
			, NodeData(InNodeData), OldPos(InOldPos), NewPos(InNewPos)
		{}
		void Do() override;
		void Undo() override;
	private:
		ObjectPtr<GraphNode> NodeData;
		Vector2D OldPos, NewPos;
	};

	class FRAMEWORK_API AddNodeCommand : public GraphCommand {
	public:
		AddNodeCommand(SGraphPanel* InGraphPanel, ObjectPtr<GraphNode> InNodeData, const Vector2D& InPos)
			: GraphCommand(InGraphPanel)
			, NodeData(InNodeData), Pos(InPos)
		{}
		void Do() override;
		void Undo() override;

	private:
		ObjectPtr<GraphNode> NodeData;
		Vector2D Pos;
	};

	class FRAMEWORK_API RemoveNodeCommand : public GraphCommand {
	public:
		RemoveNodeCommand(SGraphPanel* InGraphPanel, ObjectPtr<GraphNode> InNodeData)
			: GraphCommand(InGraphPanel)
			, NodeData(InNodeData)
		{}
		void Do() override;
		void Undo() override;
	private:
		ObjectPtr<GraphNode> NodeData;
	};

	class FRAMEWORK_API AddLinkCommand : public GraphCommand {
	public:
		AddLinkCommand(SGraphPanel* InGraphPanel, GraphPin* Output, GraphPin* Input)
			: GraphCommand(InGraphPanel)
			, Output(Output), Input(Input)
		{}
		void Do() override;
		void Undo() override;

	private:
		GraphPin* Output;
		GraphPin* Input;
	};

	class FRAMEWORK_API RemoveLinkCommand : public GraphCommand {
	public:
		RemoveLinkCommand(SGraphPanel* InGraphPanel, GraphPin* Output, GraphPin* Input)
			: GraphCommand(InGraphPanel)
			, Output(Output), Input(Input)
		{}
		void Do() override;
		void Undo() override;

	private:
		GraphPin* Output;
		GraphPin* Input;
	};

	class FRAMEWORK_API SGraphPanel : public SPanel
	{
		friend SGraphNode;
	public:
		SLATE_BEGIN_ARGS(SGraphPanel) : _GraphData(nullptr)
			{}
			SLATE_ARGUMENT(class Graph*, GraphData)
		SLATE_END_ARGS()

		struct Transaction
		{
			TArray<TSharedPtr<GraphCommand>> Commands;
		};
		struct ScopedTransaction
		{
			ScopedTransaction(SGraphPanel* InGraphPanel) : GraphPanel(InGraphPanel)
			{
				if (!GraphPanel->CurrentTransaction)
				{
					GraphPanel->CurrentTransaction = Transaction{};
				}
			}
			~ScopedTransaction()
			{
				if (GraphPanel->CurrentTransaction.value().Commands.Num() > 0)
				{
					GraphPanel->UndoStack.Emplace(GraphPanel->CurrentTransaction.value().Commands);
					GraphPanel->RedoStack.Empty();
				}
				GraphPanel->CurrentTransaction.reset();
			}

			SGraphPanel* GraphPanel;
		};

		SGraphPanel();
		void Construct(const FArguments& InArgs);
		
	public:
		void DoCommand(TSharedPtr<GraphCommand> Command) {
			if (CurrentTransaction)
			{
				CurrentTransaction.value().Commands.Add(Command);
			}
			Command->Do();
		}

		void SetFocus() { FSlateApplication::Get().SetKeyboardFocus(AsShared(), EFocusCause::SetDirectly); }
		void AddNode(ObjectPtr<GraphNode> NewNodeData, const Vector2D& Pos);
		
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
		SGraphNode* GetNode(GraphNode* NodeData);
		
		SGraphPin* GetGraphPin(FGuid PinId);
		SGraphPin* GetOuputPin(SGraphPin* InputPin) const;
		void AddLink(SGraphPin* Output, SGraphPin* Input);
		void RemoveLink(SGraphPin* Output, SGraphPin* Input);

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

		std::optional<Transaction> CurrentTransaction;
		TArray<GraphState> UndoStack;
		TArray<GraphState> RedoStack;
		TArray<TPair<GraphNode*, Vector2D>> MovingNodes;

		TArray<TSharedPtr<FText>> MenuNodeItems;
		TSharedPtr<SListView<TSharedPtr<FText>>> MenuNodeList;

		//OutputPin to InputPin
		TMultiMap<SGraphPin*, SGraphPin*> Links;
	};
	
}


