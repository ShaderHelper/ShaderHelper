#include "CommonHeader.h"
#include "SDebuggerCallStackView.h"
#include "UI/Styles/FShaderHelperStyle.h"

namespace SH
{

	const FName CallPointColId = "CallPointCol";
	const FName FuncNameColId = "FuncNameCol";
	const FName LocationColId = "LocationCol";

	void SDebuggerCallStackView::Construct( const FArguments& InArgs )
	{
		ChildSlot
		[
			SAssignNew(CallStackView, SListView<CallStackDataPtr>)
			.AllowOverscroll(EAllowOverscroll::No)
			.ListItemsSource(&CallStackDatas)
			.OnGenerateRow(this, &SDebuggerCallStackView::GenerateRowForItem)
			.SelectionMode(ESelectionMode::Single)
			.OnSelectionChanged_Lambda([this](CallStackDataPtr SelectedData, ESelectInfo::Type) {
				if(SelectedData && ActiveData != SelectedData)
				{
					ActiveData = SelectedData;
					if(OnSelectionChanged)
					{
						OnSelectionChanged(SelectedData->Name);
					}
				}
			})
			.HeaderRow
			(
				SNew(SHeaderRow)
				.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FHeaderRowStyle>("TableView.DebuggerHeader"))
				+ SHeaderRow::Column(CallPointColId)
				.FillWidth(0.1f)
				.DefaultLabel(LOCALIZATION(CallPointColId.ToString()))
				+ SHeaderRow::Column(FuncNameColId)
				.FillWidth(0.6f)
				.DefaultLabel(LOCALIZATION(FuncNameColId.ToString()))
				+ SHeaderRow::Column(LocationColId)
				.FillWidth(0.3f)
				.DefaultLabel(LOCALIZATION(LocationColId.ToString()))
			)
		];
	}

	void SCallStackViewRow::Construct(const FArguments& InArgs, SDebuggerCallStackView* InOwner, CallStackDataPtr InData, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Owner = InOwner;
		Data = InData;
		SMultiColumnTableRow<CallStackDataPtr>::Construct(FSuperRowType::FArguments(), OwnerTableView);
	}

	TSharedRef<SWidget> SCallStackViewRow::GenerateWidgetForColumn(const FName& ColumnId)
	{
		auto InternalBorder = SNew(SBorder).BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel")).Padding(0);
		auto Border = SNew(SBorder).BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
		[
			InternalBorder
		];
		if(ColumnId == FuncNameColId)
		{
			Border->SetPadding(FMargin{1, 0, 1, 2});
			InternalBorder->SetContent(SNew(STextBlock).Text(FText::FromString(Data->Name)));
		}
		else if(ColumnId == LocationColId)
		{
			Border->SetPadding(FMargin{1, 0, 0, 2});
			InternalBorder->SetContent(SNew(STextBlock).Text(FText::FromString(Data->Location)));
		}
		else
		{
			Border->SetPadding(FMargin{0, 0, 1, 2});
			InternalBorder->SetVAlign(VAlign_Center);
			InternalBorder->SetHAlign(HAlign_Center);
			InternalBorder->SetContent(
				   SNew(SImage)
				   .Visibility_Lambda([this]{
					   if(Owner->GetActiveData() == Data)
					   {
						   return EVisibility::Visible;
					   }
					   return EVisibility::Hidden;
				   })
				   .Image(FShaderHelperStyle::Get().GetBrush("Icons.ArrowBoldRight"))
		   );
				
		}
		return Border;
	}

	void SDebuggerCallStackView::SetCallStackDatas(const TArray<CallStackDataPtr>& InDatas)
	{
		CallStackDatas = InDatas;
		if(!CallStackDatas.IsEmpty())
		{
			ActiveData = CallStackDatas[0];
		}
		CallStackView->RequestListRefresh();
	}
	
	TSharedRef<ITableRow> SDebuggerCallStackView::GenerateRowForItem(CallStackDataPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		TSharedRef<SCallStackViewRow> TableRow = SNew(SCallStackViewRow, this, Item, OwnerTable);
		return TableRow;
	}
}
