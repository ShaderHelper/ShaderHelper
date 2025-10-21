#include "CommonHeader.h"
#include "SDebuggerWatchView.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "Editor/DebuggerViewCommands.h"

namespace SH
{
	const FName ExpColId = "ExpCol";
	const FName ValueColId = "ValueCol";
	const FName TypeColId = "TypeCol";

	void SDebuggerWatchView::Construct( const FArguments& InArgs )
	{
		ExpressionNodeDatas.Add(MakeShared<ExpressionNode>());
		
		UICommandList = MakeShared<FUICommandList>();
		UICommandList->MapAction(
			DebuggerViewCommands::Get().Delete,
			FExecuteAction::CreateLambda([this]{
				ExpressionNodePtr SelectedItem = ExpressionTreeView->GetSelectedItems()[0];
				ExpressionNodeDatas.Remove(SelectedItem);
				ExpressionTreeView->RequestTreeRefresh();
			})
		);
		
		ChildSlot
		[
			SAssignNew(ExpressionTreeView, STreeView<ExpressionNodePtr>)
			.AllowOverscroll(EAllowOverscroll::No)
			.TreeItemsSource(&ExpressionNodeDatas)
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateRow(this, &SDebuggerWatchView::OnGenerateRow)
			.OnGetChildren(this, &SDebuggerWatchView::OnGetChildren)
			.OnExpansionChanged_Lambda([this](ExpressionNodePtr InData, bool bExpanded){
				InData->Expanded = bExpanded;
			})
			.OnContextMenuOpening_Lambda([this]{
				if(ExpressionTreeView->GetSelectedItems().Num() > 0)
				{
					ExpressionNodePtr SelectedItem = ExpressionTreeView->GetSelectedItems()[0];
					if(!SelectedItem->Expr.IsEmpty())
					{
						FMenuBuilder MenuBuilder{ true, UICommandList };
						MenuBuilder.AddMenuEntry(DebuggerViewCommands::Get().Delete, NAME_None, {}, {},
							FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "GenericCommands.Delete" });
						return MenuBuilder.MakeWidget();
					}
				}
				return SNullWidget::NullWidget;
			})
			.HeaderRow
			(
				SNew(SHeaderRow)
				.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FHeaderRowStyle>("TableView.DebuggerHeader"))
				+ SHeaderRow::Column(ExpColId)
				.FillWidth(0.2f)
				.DefaultLabel(LOCALIZATION(ExpColId.ToString()))
				+ SHeaderRow::Column(ValueColId)
				.FillWidth(0.6f)
				.DefaultLabel(LOCALIZATION(ValueColId.ToString()))
				+ SHeaderRow::Column(TypeColId)
				.FillWidth(0.2f)
				.DefaultLabel(LOCALIZATION(TypeColId.ToString()))
			)
		];
	}

	void SWatchViewRow::Construct(const FArguments& InArgs, SDebuggerWatchView* InOwner, ExpressionNodePtr InData, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Data = InData;
		Owner = InOwner;
		SMultiColumnTableRow<ExpressionNodePtr>::Construct(FSuperRowType::FArguments(), OwnerTableView);
	}

	TSharedRef<SWidget> SWatchViewRow::GenerateWidgetForColumn(const FName& ColumnId)
	{
		auto InternalBorder = SNew(SBorder).BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel")).Padding(0);
		auto Border = SNew(SBorder).BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
		[
			InternalBorder
		];
		if(ColumnId == ExpColId)
		{
			Border->SetPadding(FMargin{0, 0, 1, 2});
			auto ExprText = SNew(FW::SShInlineEditableTextBlock)
			.Text_Lambda([this]{ return FText::FromString(Data->Expr); })
			.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
				if(NewText.ToString().IsEmpty() && !Data->Expr.IsEmpty())
				{
					Owner->ExpressionNodeDatas.Remove(Data);
					Owner->ExpressionTreeView->RequestTreeRefresh();
				}
				else if(!NewText.ToString().IsEmpty())
				{
					if(Data->Expr.IsEmpty())
					{
						Owner->ExpressionNodeDatas.Add(MakeShared<ExpressionNode>());
						Owner->ExpressionTreeView->RequestTreeRefresh();
					}
					if(Owner->OnWatch && NewText.ToString() != Data->Expr)
					{
						*Data = Owner->OnWatch(NewText.ToString());
						Owner->ExpressionTreeView->RequestTreeRefresh();
					}
				}
			});
			InternalBorder->SetContent(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SExpanderArrow, SharedThis(this))
				]
				+ SHorizontalBox::Slot()
				[
					ExprText
				]
			);
		}
		else if(ColumnId == ValueColId)
		{
			Border->SetPadding(FMargin{1, 0, 1, 2});

			InternalBorder->SetBorderImage(TAttribute<const FSlateBrush*>::CreateLambda([this]{
				if(Data->Dirty)
				{
					return FAppStyle::Get().GetBrush("Brushes.White");
				}
				return FAppStyle::Get().GetBrush("Brushes.Panel");
			}));
			InternalBorder->SetBorderBackgroundColor(TAttribute<FSlateColor>::CreateLambda([this]{
				if(Data->Dirty)
				{
					return FLinearColor{1,1,1,0.2f};
				}
				return FLinearColor::White;
			}));
			InternalBorder->SetContent(SNew(STextBlock).Text_Lambda([this] {
				return FText::FromString(Data->ValueStr);
			}));
			InternalBorder->SetToolTipText(FText::FromString(Data->ValueStr));
		}
		else
		{
			Border->SetPadding(FMargin{1, 0, 0, 2});
			InternalBorder->SetContent(SNew(STextBlock).Text_Lambda([this] {
				return FText::FromString(Data->TypeName);
			}));
		}
		return Border;
	}

	void SDebuggerWatchView::Refresh()
	{
		for(auto Data : ExpressionNodeDatas)
		{
			if(OnWatch && !Data->Expr.IsEmpty())
			{
				ExpressionNode ExprResult = OnWatch(Data->Expr);
				if(ExprResult.ValueStr != Data->ValueStr)
				{
					*Data = ExprResult;
					Data->Dirty = true;
				}
				else
				{
					Data->Dirty = false;
				}
			}
		}
		ExpressionTreeView->RequestTreeRefresh();
	}

	TSharedRef<ITableRow> SDebuggerWatchView::OnGenerateRow(ExpressionNodePtr InTreeNode, const TSharedRef<STableViewBase>& OwnerTable)
	{
		TSharedRef<SWatchViewRow> TableRow = SNew(SWatchViewRow, this, InTreeNode, OwnerTable);
		return TableRow;
	}

	void SDebuggerWatchView::OnGetChildren(ExpressionNodePtr InTreeNode, TArray<ExpressionNodePtr>& OutChildren)
	{
		OutChildren = InTreeNode->Children;
	}

}
