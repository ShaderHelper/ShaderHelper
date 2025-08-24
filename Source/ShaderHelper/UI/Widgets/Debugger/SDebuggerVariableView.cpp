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
			.OnExpansionChanged_Lambda([this](VariableNodePtr InData, bool bExpanded){
				InData->Expanded = bExpanded;
			})
			.HeaderRow
			(
				SNew(SHeaderRow)
				.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FHeaderRowStyle>("TableView.DebuggerHeader"))
				+ SHeaderRow::Column(VariableColId)
				.FillWidth(0.2f)
				.DefaultLabel(LOCALIZATION(VariableColId.ToString()))
				+ SHeaderRow::Column(ValueColId)
				.FillWidth(0.6f)
				.DefaultLabel(LOCALIZATION(ValueColId.ToString()))
				+ SHeaderRow::Column(TypeColId)
				.FillWidth(0.2f)
				.DefaultLabel(LOCALIZATION(TypeColId.ToString()))
			)
		];
		
		RefreshExpansions();
	}

	void SDebuggerVariableView::RefreshExpansions()
	{
		auto ExpandRecursive = [this](auto&& Self, VariableNodePtr Node) -> void
		{
			VariableTreeView->SetItemExpansion(Node, Node->Expanded);
			for (auto& Child : Node->Children)
			{
				Self(Self, Child);
			}
		};
		for (auto& Node : VariableNodeDatas)
		{
			ExpandRecursive(ExpandRecursive, Node);
		}
	}

	void SVariableViewRow::Construct(const FArguments& InArgs, VariableNodePtr InData, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Data = InData;
		SMultiColumnTableRow<VariableNodePtr>::Construct(FSuperRowType::FArguments(), OwnerTableView);
	}

	TSharedRef<SWidget> SVariableViewRow::GenerateWidgetForColumn(const FName& ColumnId)
	{
		auto InternalBorder = SNew(SBorder).BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel")).Padding(0);
		auto Border = SNew(SBorder).BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
		[
			InternalBorder
		];
		if(ColumnId == VariableColId)
		{
			Border->SetPadding(FMargin{0, 0, 1, 2});
			InternalBorder->SetContent(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SExpanderArrow, SharedThis(this))
				]
				+ SHorizontalBox::Slot()
				[
				   SNew(STextBlock).Text(FText::FromString(Data->VarName))
				]
			);
		}
		else if(ColumnId == ValueColId)
		{
			Border->SetPadding(FMargin{1, 0, 1, 2});
			if(Data->Dirty)
			{
				InternalBorder->SetBorderImage(FAppStyle::Get().GetBrush("Brushes.White"));
				InternalBorder->SetBorderBackgroundColor(FLinearColor{1,1,1,0.2f});
			}
			InternalBorder->SetContent(SNew(STextBlock).Text(FText::FromString(Data->ValueStr)));
		}
		else
		{
			Border->SetPadding(FMargin{1, 0, 0, 2});
			InternalBorder->SetContent(SNew(STextBlock).Text(FText::FromString(Data->TypeName)));
		}
		return Border;
	}

	void SDebuggerVariableView::SetVariableNodeDatas(const TArray<VariableNodePtr>& InDatas)
	{
		for(const auto& Data: VariableNodeDatas)
		{
			if(const VariableNodePtr* InData = InDatas.FindByPredicate([&](const VariableNodePtr& InItem){ return InItem->VarName == Data->VarName; }))
			{
				(*InData)->Expanded = Data->Expanded;
			}
		}
		VariableNodeDatas = InDatas;
		RefreshExpansions();
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
