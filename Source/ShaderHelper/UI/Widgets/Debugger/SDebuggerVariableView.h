#pragma once

namespace SH
{
	struct VariableNode
	{
		FString VarName;
		FString ValueStr;
		FString TypeName;
		TArray<TSharedPtr<VariableNode>> Children;
	};

	using VariableNodePtr = TSharedPtr<VariableNode>;

	class SVariableViewRow : public SMultiColumnTableRow<VariableNodePtr>
	{
	public:
		void Construct(const FArguments& InArgs, VariableNodePtr InData, const TSharedRef<STableViewBase>& OwnerTableView);
		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnId) override;
		VariableNodePtr Data;
	};

	class SDebuggerVariableView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDebuggerVariableView){}
		SLATE_END_ARGS()
		void Construct( const FArguments& InArgs );
	public:
		void SetVariableNodeDatas(const TArray<VariableNodePtr>& InDatas);
		TSharedRef<ITableRow> OnGenerateRow(VariableNodePtr InTreeNode, const TSharedRef<STableViewBase>& OwnerTable);
		void OnGetChildren(VariableNodePtr InTreeNode, TArray<VariableNodePtr>& OutChildren);
		
	private:
		TSharedPtr<STreeView<VariableNodePtr>> VariableTreeView;
		TArray<VariableNodePtr> VariableNodeDatas;
	};
}
