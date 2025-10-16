#pragma once
#include "PluginManager/ShPluginManager.h"
#include "SKeybindingText.h"

namespace SH
{
	struct PluginData
	{
		FW::ShPlugin* Data;
	};
	using PluginDataPtr = TSharedPtr<PluginData>;
	class SPluginView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPluginView) {}
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs);
		void SavePersistentState();
		TSharedRef<ITableRow> GenerateRowForItem(PluginDataPtr Item, const TSharedRef<STableViewBase>& OwnerTable);

	private:
		TSharedPtr<SListView<PluginDataPtr>> PluginListView;
		TArray<PluginDataPtr> PluginDatas;
	};

	struct KeyData
	{
		TSharedPtr<FUICommandInfo> CommandInfo;
	};
	using KeyDataPtr = TSharedPtr<KeyData>;
	class SKeymapView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SKeymapView) {}
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs);
		void GenerateKeyDatas(const FText& InFilterText = FText::GetEmpty());
		TSharedRef<ITableRow> GenerateRowForItem(KeyDataPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
	private:
		TSharedPtr<SListView<KeyDataPtr>> KeyListView;
		TArray<KeyDataPtr> KeyDatas;
	};
	class SKeymapViewRow : public SMultiColumnTableRow<KeyDataPtr>
	{
	public:
		void Construct(const FArguments& InArgs, SKeymapView* InOwner, KeyDataPtr InData, const TSharedRef<STableViewBase>& OwnerTableView);
		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnId) override;

		TSharedPtr<SKeybindingText> KeybindingText;

	private:
		SKeymapView* Owner{};
		KeyDataPtr Data;
	};

	class SAppearanceView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SAppearanceView) {}
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs);
	};

	class SPreferenceView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPreferenceView) {}
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs);
	private:
		TSharedRef<SWidget> CreatePreference(const FText& InText, TSharedRef<SWidget> InView);	

	private:
		TSharedPtr<SHorizontalBox> PreferenceBox;
		SWidget* CurPreference{};

		TSharedPtr<SPluginView> PluginView;
		TSharedPtr<SKeymapView> KeymapView;
		TSharedPtr<SAppearanceView> AppearanceView;
	};
}
