#pragma once
#include "PluginManager/ShPluginManager.h"

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

	class SKeymapView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SKeymapView) {}
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs);
		void SavePersistentState();
	};

	class SThemeView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SThemeView) {}
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs);
		void SavePersistentState();
	};

	class SPreferenceView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPreferenceView) {}
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs);
		void SavePersistentState();
	private:
		TSharedRef<SWidget> CreatePreference(const FText& InText, TSharedRef<SWidget> InView);	

	private:
		TSharedPtr<SHorizontalBox> PreferenceBox;
		SWidget* CurPreference{};

		TSharedPtr<SPluginView> PluginView;
		TSharedPtr<SKeymapView> KeymapView;
		TSharedPtr<SThemeView>  ThemeView;
	};
}
