#include "CommonHeader.h"
#include "SDebuggerWatchView.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "Editor/DebuggerViewCommands.h"
#include <Widgets/Colors/SColorBlock.h>
#include <regex>

using namespace FW;

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
				for (ExpressionNodePtr SelectedItem : ExpressionTreeView->GetSelectedItems())
				{
					if (!SelectedItem->Expr.IsEmpty())
					{
						ExpressionNodeDatas.Remove(SelectedItem);
					}
				}
				ExpressionTreeView->RequestTreeRefresh();
			})
		);
		
		ChildSlot
		[
			SAssignNew(ExpressionTreeView, STreeView<ExpressionNodePtr>)
			.AllowOverscroll(EAllowOverscroll::No)
			.TreeItemsSource(&ExpressionNodeDatas)
			.OnGenerateRow(this, &SDebuggerWatchView::OnGenerateRow)
			.OnGetChildren(this, &SDebuggerWatchView::OnGetChildren)
			.OnExpansionChanged_Lambda([this](ExpressionNodePtr InData, bool bExpanded){
				InData->PersistantState.Expanded = bExpanded;
			})
			.OnContextMenuOpening_Lambda([this]{
				if(ExpressionTreeView->GetSelectedItems().Num() > 0)
				{
					FMenuBuilder MenuBuilder{ true, UICommandList };
					MenuBuilder.AddMenuEntry(DebuggerViewCommands::Get().Delete, NAME_None, {}, {},
						FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "GenericCommands.Delete" });
					return MenuBuilder.MakeWidget();
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
					.AutoWidth()
					[
						SNew(SShToggleButton).Icon(FAppStyle::Get().GetBrush("ColorPicker.Mode")).IconColorAndOpacity(FLinearColor::White)
						.ToolTipText(LOCALIZATION("DisplayColorBlock"))
						.IsChecked_Lambda([this] { return DebuggerViewDisplayColorBlock ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) {
							DebuggerViewDisplayColorBlock = InState == ECheckBoxState::Checked ? true : false;
						})
					]
				]
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
						Owner->ExpressionTreeView->RebuildList();
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
			InternalBorder->SetBorderImage(FAppStyle::Get().GetBrush("Brushes.White"));
			InternalBorder->SetBorderBackgroundColor(TAttribute<FSlateColor>::CreateLambda([this] {
				if (Data->Dirty)
				{
					return FLinearColor{ 1,1,1,0.2f };
				}
				return FAppStyle::Get().GetBrush("Brushes.Panel")->TintColor.GetSpecifiedColor();
			}));

			if (Data->TypeName == "float3" || Data->TypeName == "float4")
			{
				std::string Str = TCHAR_TO_UTF8(*Data->ValueStr);
				std::regex Pattern(
					R"(\{\s*)"                                                  // {
					R"(([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?[fF]?)\s*,\s*)"          // x
					R"(([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?[fF]?)\s*,\s*)"          // y
					R"(([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?[fF]?)\s*)"              // z
					R"((?:\s*,\s*([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?[fF]?))?)"     // optional w
					R"(\})"                                                     // }
				);
				std::smatch Match;
				float X = 1.0f, Y = 1.0f, Z = 1.0f, W = 1.0f;
				if (std::regex_search(Str, Match, Pattern) && Match.size() >= 4)
				{
					X = std::stof(Match[1].str());
					Y = std::stof(Match[2].str());
					Z = std::stof(Match[3].str());
					if (Match.size() > 4 && Match[4].matched && Match[4].length() > 0)
					{
						W = std::stof(Match[4].str());
					}
					X = FMath::Clamp(X, 0.0f, 1.0f);
					Y = FMath::Clamp(Y, 0.0f, 1.0f);
					Z = FMath::Clamp(Z, 0.0f, 1.0f);
					W = FMath::Clamp(W, 0.0f, 1.0f);
				}
				InternalBorder->SetContent(
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SColorBlock)
						.AlphaDisplayMode(EColorBlockAlphaDisplayMode::Separate)
						.Visibility_Lambda([] { return DebuggerViewDisplayColorBlock ? EVisibility::Visible : EVisibility::Hidden; })
						.ShowBackgroundForAlpha(true)
						.Color(FLinearColor{ X, Y, Z, W })
						.UseSRGB(false)
					]
					+ SOverlay::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(Data->ValueStr))
					]
				);
			}
			else
			{
				InternalBorder->SetContent(SNew(STextBlock).Text(FText::FromString(Data->ValueStr)));
			}

			InternalBorder->SetToolTipText(FText::FromString(Data->ValueStr));
		}
		else
		{
			Border->SetPadding(FMargin{1, 0, 0, 2});
			InternalBorder->SetContent(SNew(STextBlock).Text(FText::FromString(Data->TypeName)));
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
		ExpressionTreeView->RebuildList();
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
