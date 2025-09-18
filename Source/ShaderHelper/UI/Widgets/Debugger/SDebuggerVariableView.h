#pragma once
#include "SDebuggerWatchView.h"

namespace SH
{
	class SVariableViewRow : public SMultiColumnTableRow<ExpressionNodePtr>
	{
	public:
		void Construct(const FArguments& InArgs, ExpressionNodePtr InData, const TSharedRef<STableViewBase>& OwnerTableView);
		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnId) override;
		ExpressionNodePtr Data;
	};

	class SDebuggerVariableView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDebuggerVariableView)
		: _HasHeaderRow(true)
		, _AutoWidth(false)
		{}
			SLATE_ARGUMENT(bool, HasHeaderRow)
			SLATE_ARGUMENT(bool, AutoWidth)
		SLATE_END_ARGS()
		void Construct( const FArguments& InArgs );
		
	public:
		void SaveExpansionStates(const TArray<ExpressionNodePtr>& NodeDatas, TMap<FString, bool>& OutStates);
		void RestoreExpansionStates(const TArray<ExpressionNodePtr>& NodeDatas, const TMap<FString, bool>& ExpansionStates);
		void RefreshExpansions();
		void SetOnShowUninitialized(const TFunction<void(bool)>& InHandler) { OnShowUninitialized = InHandler; }
		void SetVariableNodeDatas(const TArray<ExpressionNodePtr>& InDatas);
		const TArray<ExpressionNodePtr>& GetVariableNodeDatas() const { return VariableNodeDatas; }
		TSharedRef<ITableRow> OnGenerateRow(ExpressionNodePtr InTreeNode, const TSharedRef<STableViewBase>& OwnerTable);
		void OnGetChildren(ExpressionNodePtr InTreeNode, TArray<ExpressionNodePtr>& OutChildren);

		static inline bool bShowUninitialized = false;
	private:
		TSharedPtr<STreeView<ExpressionNodePtr>> VariableTreeView;
		TArray<ExpressionNodePtr> VariableNodeDatas;
		TFunction<void(bool)> OnShowUninitialized;
	};
}
