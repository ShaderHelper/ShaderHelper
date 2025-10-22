#pragma once

namespace SH
{
	struct ExpressionNodePersistantState
	{
		bool Expanded{};
	};

	struct ExpressionNode
	{
		FString Expr;
		FString ValueStr;
		FString TypeName;
		bool Dirty{};
		TArray<TSharedPtr<ExpressionNode>> Children;
		ExpressionNodePersistantState PersistantState;
	};

	using ExpressionNodePtr = TSharedPtr<ExpressionNode>;

	inline bool DebuggerViewDisplayColorBlock;

	class SDebuggerWatchView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDebuggerWatchView){}
		SLATE_END_ARGS()
		void Construct( const FArguments& InArgs );
	public:
		void Refresh();
		TSharedRef<ITableRow> OnGenerateRow(ExpressionNodePtr InTreeNode, const TSharedRef<STableViewBase>& OwnerTable);
		void OnGetChildren(ExpressionNodePtr InTreeNode, TArray<ExpressionNodePtr>& OutChildren);
		
		TSharedPtr<FUICommandList> UICommandList;
		TSharedPtr<STreeView<ExpressionNodePtr>> ExpressionTreeView;
		TArray<ExpressionNodePtr> ExpressionNodeDatas;
		TFunction<ExpressionNode(const FString&)> OnWatch;
	};

	class SWatchViewRow : public SMultiColumnTableRow<ExpressionNodePtr>
	{
	public:
		void Construct(const FArguments& InArgs, SDebuggerWatchView* InOwner, ExpressionNodePtr InData, const TSharedRef<STableViewBase>& OwnerTableView);
		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnId) override;
		SDebuggerWatchView* Owner{};
		ExpressionNodePtr Data;
	};
}
