#include "CommonHeader.h"
#include "SDebuggerVariableView.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/Misc/MiscWidget.h"

using namespace FW;

namespace SH
{
	const FName VariableColId = "VariableCol";
	const FName ValueColId = "ValueCol";
	const FName TypeColId = "TypeCol";

	void SDebuggerVariableView::Construct( const FArguments& InArgs )
	{
		EVisibility HeaderRowVisibility = InArgs._HasHeaderRow ? EVisibility::Visible : EVisibility::Collapsed;

		TSharedPtr<SHeaderRow> HeaderRow = SNew(SHeaderRow)
			.Visibility(HeaderRowVisibility)
			.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FHeaderRowStyle>("TableView.DebuggerHeader"));
		if (InArgs._AutoWidth)
		{
			HeaderRow->AddColumn(SHeaderRow::Column(VariableColId)
				.ManualWidth_Lambda([this] {
					float MaxWidth{};
					for (ExpressionNodePtr Data : VariableNodeDatas)
					{
						auto Temp = SNew(STextBlock).Text(FText::FromString(Data->Expr));
						float DpiScale = static_cast<ShaderHelperEditor*>(GApp->GetEditor())->GetDebuggerTipWindow()->GetDPIScaleFactor();
						MaxWidth = FMath::Max(MaxWidth, (float)Temp->ComputeDesiredSize(DpiScale).X + 60);
					}
					return MaxWidth;
				}));
			HeaderRow->AddColumn(SHeaderRow::Column(ValueColId)
				.ManualWidth_Lambda([this] {
					float MaxWidth{};
					for (ExpressionNodePtr Data : VariableNodeDatas)
					{
						auto Temp = SNew(STextBlock).Text(FText::FromString(Data->ValueStr));
						float DpiScale = static_cast<ShaderHelperEditor*>(GApp->GetEditor())->GetDebuggerTipWindow()->GetDPIScaleFactor();
						MaxWidth = FMath::Max(MaxWidth, (float)Temp->ComputeDesiredSize(DpiScale).X + 40);
					}
					return MaxWidth;
				}));
			HeaderRow->AddColumn(SHeaderRow::Column(TypeColId)
				.ManualWidth_Lambda([this] {
					float MaxWidth{};
					for (ExpressionNodePtr Data : VariableNodeDatas)
					{
						auto Temp = SNew(STextBlock).Text(FText::FromString(Data->TypeName));
						float DpiScale = static_cast<ShaderHelperEditor*>(GApp->GetEditor())->GetDebuggerTipWindow()->GetDPIScaleFactor();
						MaxWidth = FMath::Max(MaxWidth, (float)Temp->ComputeDesiredSize(DpiScale).X + 40);
					}
					return MaxWidth;
				}));
		}
		else
		{
			HeaderRow->AddColumn(SHeaderRow::Column(VariableColId)
				.FillWidth(0.2f)
				.DefaultLabel(LOCALIZATION(VariableColId.ToString())));
			HeaderRow->AddColumn(SHeaderRow::Column(ValueColId)
				.FillWidth(0.6f)
				.HeaderContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(LOCALIZATION(ValueColId.ToString()))
					]
					+ SHorizontalBox::Slot()
					.Padding(0, 0, 4, 0)
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					[
						SNew(SShToggleButton).Icon(FShaderHelperStyle::Get().GetBrush("Icons.Uninitialized"))
						.ToolTipText(LOCALIZATION("ShowUninitialized"))
						.IsChecked_Lambda([this] { return bShowUninitialized ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) {
							bShowUninitialized = InState == ECheckBoxState::Checked ? true : false;
							if (OnShowUninitialized)
							{
								OnShowUninitialized(bShowUninitialized);
							}
						})
					]
				]);
			HeaderRow->AddColumn(SHeaderRow::Column(TypeColId)
				.FillWidth(0.2f)
				.DefaultLabel(LOCALIZATION(TypeColId.ToString())));
		}

		ChildSlot
		[
			SAssignNew(VariableTreeView, STreeView<ExpressionNodePtr>)
			.AllowOverscroll(EAllowOverscroll::No)
			.TreeItemsSource(&VariableNodeDatas)
			.OnGenerateRow(this, &SDebuggerVariableView::OnGenerateRow)
			.OnGetChildren(this, &SDebuggerVariableView::OnGetChildren)
			.OnExpansionChanged_Lambda([this](ExpressionNodePtr InData, bool bExpanded){
				InData->Expanded = bExpanded;
			})
			.HeaderRow
			(
				HeaderRow.ToSharedRef()
			)
		];
	
		RefreshExpansions();
	}

	void SDebuggerVariableView::SaveExpansionStates(const TArray<ExpressionNodePtr>& NodeDatas, TMap<FString, bool>& OutStates)
	{
		auto SaveRecursive = [&OutStates](auto&& Self, const TArray<ExpressionNodePtr>& Nodes, const FString& Prefix) -> void {
			for (const auto& Node : Nodes)
			{
				if (Node.IsValid())
				{
					FString NodeKey = Prefix.IsEmpty() ? Node->Expr : (Prefix + TEXT(".") + Node->Expr);
					OutStates.Add(NodeKey, Node->Expanded);

					if (!Node->Children.IsEmpty())
					{
						Self(Self, Node->Children, NodeKey);
					}
				}
			}
			};

		SaveRecursive(SaveRecursive, NodeDatas, TEXT(""));
	}

	void SDebuggerVariableView::RestoreExpansionStates(const TArray<ExpressionNodePtr>& NodeDatas, const TMap<FString, bool>& ExpansionStates)
	{
		auto RestoreRecursive = [&ExpansionStates](auto&& Self, const TArray<ExpressionNodePtr>& Nodes, const FString& Prefix) -> void {
			for (const auto& Node : Nodes)
			{
				if (Node.IsValid())
				{
					FString NodeKey = Prefix.IsEmpty() ? Node->Expr : (Prefix + TEXT(".") + Node->Expr);

					if (const bool* bExpanded = ExpansionStates.Find(NodeKey))
					{
						Node->Expanded = *bExpanded;
					}

					if (!Node->Children.IsEmpty())
					{
						Self(Self, Node->Children, NodeKey);
					}
				}
			}
			};

		RestoreRecursive(RestoreRecursive, NodeDatas, TEXT(""));
	}

	void SDebuggerVariableView::RefreshExpansions()
	{
		auto ExpandRecursive = [this](auto&& Self, ExpressionNodePtr Node) -> void
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

	void SVariableViewRow::Construct(const FArguments& InArgs, ExpressionNodePtr InData, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Data = InData;
		SMultiColumnTableRow<ExpressionNodePtr>::Construct(FSuperRowType::FArguments(), OwnerTableView);
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
				   SNew(STextBlock).Text(FText::FromString(Data->Expr))
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

	void SDebuggerVariableView::SetVariableNodeDatas(const TArray<ExpressionNodePtr>& InDatas)
	{
		TMap<FString, bool> ExpansionStates;
		SaveExpansionStates(VariableNodeDatas, ExpansionStates);

		VariableNodeDatas = InDatas;

		RestoreExpansionStates(VariableNodeDatas, ExpansionStates);
		RefreshExpansions();
		VariableTreeView->RequestTreeRefresh();
	}

	TSharedRef<ITableRow> SDebuggerVariableView::OnGenerateRow(ExpressionNodePtr InTreeNode, const TSharedRef<STableViewBase>& OwnerTable)
	{
		TSharedRef<SVariableViewRow> TableRow = SNew(SVariableViewRow, InTreeNode, OwnerTable);
		return TableRow;
	}

	void SDebuggerVariableView::OnGetChildren(ExpressionNodePtr InTreeNode, TArray<ExpressionNodePtr>& OutChildren)
	{
		OutChildren = InTreeNode->Children;
	}

}
