#pragma once

namespace SH
{
	struct CallStackData
	{
		FString Name;
		FString Location;
	};

	using CallStackDataPtr = TSharedPtr<CallStackData>;

	class SDebuggerCallStackView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDebuggerCallStackView){}
		SLATE_END_ARGS()
		void Construct( const FArguments& InArgs );
	public:
		void SetCallStackDatas(const TArray<CallStackDataPtr>& InDatas);
		TSharedRef<ITableRow> GenerateRowForItem(CallStackDataPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
		CallStackDataPtr GetActiveData() const { return ActiveData; }
		
	private:
		TSharedPtr<SListView<CallStackDataPtr>> CallStackView;
		TArray<CallStackDataPtr> CallStackDatas;
		CallStackDataPtr ActiveData;
	};

	class SCallStackViewRow : public SMultiColumnTableRow<CallStackDataPtr>
	{
	public:
		void Construct(const FArguments& InArgs, SDebuggerCallStackView* InOwner, CallStackDataPtr InData, const TSharedRef<STableViewBase>& OwnerTableView);
		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnId) override;
		
	private:
		SDebuggerCallStackView* Owner{};
		CallStackDataPtr Data;
	};
}
