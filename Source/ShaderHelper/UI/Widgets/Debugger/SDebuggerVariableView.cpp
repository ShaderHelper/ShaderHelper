#include "CommonHeader.h"
#include "SDebuggerVariableView.h"
#include "UI/Styles/FShaderHelperStyle.h"

namespace SH
{
	const FName VariableColId = "VariableCol";
	const FName ValueColId = "ValueCol";
	const FName TypeColId = "TypeCol";

	void SDebuggerVariableView::Construct( const FArguments& InArgs )
	{
		ChildSlot
		[
			SAssignNew(VariableTreeView, STreeView<VariableNodePtr>)
			.TreeItemsSource(&VariableNodeDatas)
			.OnGenerateRow(this, &SDebuggerVariableView::OnGenerateRow)
			.OnGetChildren(this, &SDebuggerVariableView::OnGetChildren)
			.HeaderRow
			(
				SNew(SHeaderRow)
				+ SHeaderRow::Column(VariableColId)
				.DefaultLabel(LOCALIZATION(VariableColId.ToString()))
				+ SHeaderRow::Column(ValueColId)
				.DefaultLabel(LOCALIZATION(ValueColId.ToString()))
				+ SHeaderRow::Column(TypeColId)
				.DefaultLabel(LOCALIZATION(TypeColId.ToString()))
			)
		];
	}

	void SVariableViewRow::Construct(const FArguments& InArgs, VariableNodePtr InData, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Data = InData;
		SMultiColumnTableRow<VariableNodePtr>::Construct(FSuperRowType::FArguments(), OwnerTableView);
	}

	TSharedRef<SWidget> SVariableViewRow::GenerateWidgetForColumn(const FName& ColumnId)
	{
		if(ColumnId == VariableColId)
		{
			return SNew(STextBlock).Text(FText::FromString(Data->VarName));
		}
		else if(ColumnId == ValueColId)
		{
			return SNew(STextBlock).Text(FText::FromString(Data->ValueStr));
		}
		else
		{
			return SNew(STextBlock).Text(FText::FromString(Data->TypeName));
		}
	}

	void SDebuggerVariableView::SetVariableNodeDatas(const TArray<VariableNodePtr>& InDatas)
	{
		VariableNodeDatas = InDatas;
		VariableTreeView->RequestTreeRefresh();
	}

	TSharedRef<ITableRow> SDebuggerVariableView::OnGenerateRow(VariableNodePtr InTreeNode, const TSharedRef<STableViewBase>& OwnerTable)
	{
		TSharedRef<SVariableViewRow> TableRow = SNew(SVariableViewRow, InTreeNode, OwnerTable);
		return TableRow;
	}

	void SDebuggerVariableView::OnGetChildren(VariableNodePtr InTreeNode, TArray<VariableNodePtr>& OutChildren)
	{
		OutChildren = InTreeNode->Children;
	}

}
