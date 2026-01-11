#include "CommonHeader.h"
#include "SPreferenceView.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"
#include "UI/Widgets/Misc/MiscWidget.h"

#include <Styling/StyleColors.h>
#include <Widgets/Input/SSearchBox.h>
#include <Widgets/Input/SSpinBox.h>
#include <DesktopPlatformModule.h>

CALL_PRIVATE_FUNCTION(USlateThemeManager_LoadThemeColors, USlateThemeManager, LoadThemeColors, , void, FStyleTheme&)

using namespace FW;

namespace SH
{
	void SPluginView::Construct(const FArguments& InArgs)
	{
		for(auto& Plugin : TSingleton<ShPluginManager>::Get().GetPlugins())
		{
			auto Data = MakeShared<PluginData>(&Plugin);
			PluginDatas.Add(MoveTemp(Data));
		}

		ChildSlot
		[
			SAssignNew(PluginListView, SListView<PluginDataPtr>)
			.ListItemsSource(&PluginDatas)
			.OnGenerateRow(this, &SPluginView::GenerateRowForItem)
		];
	}

	void SPluginView::SavePersistentState()
	{
		TArray<FString> ActivePlugins;
		for(const auto& PluginData : PluginDatas)
		{
			if(PluginData->Data->bActive)
			{
				ActivePlugins.Add(PluginData->Data->Name);
			}
		}
		Editor::GetEditorConfig()->SetArray(TEXT("Common"), TEXT("ActivePlugins"), ActivePlugins);
		Editor::SaveEditorConfig();
	}

	TSharedRef<ITableRow> SPluginView::GenerateRowForItem(PluginDataPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = SNew(STableRow<PluginDataPtr>, OwnerTable)
			.Padding(FMargin{0,0,0,6})
			[
				SNew(SBorder)
				.BorderImage_Lambda([Item] {
					if(Item->Data->bFailed) {
						return FAppStyle::Get().GetBrush("Brushes.Error");
					}
					return FAppStyle::Get().GetBrush("Brushes.Border");
				})
				.Padding(FMargin{1.0f})
				[
					SNew(SExpandableArea)
					.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
					.BodyBorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
					.InitiallyCollapsed(true)
					.HeaderContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(0, 0, 8, 0)
						.AutoWidth()
						[
							SNew(SCheckBox).IsChecked_Lambda([Item] { return Item->Data->bActive ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
							.OnCheckStateChanged_Lambda([this, Item](ECheckBoxState InState) {
								if(InState == ECheckBoxState::Checked) {
									TSingleton<ShPluginManager>::Get().RegisterPlugin(*Item->Data);
									if(!Item->Data->bFailed)
									{
										Item->Data->bActive = true;
									}
								}
								else {
									TSingleton<ShPluginManager>::Get().UnregisterPlugin(*Item->Data);
									Item->Data->bActive = false;
								}
								SavePersistentState();
							})
						]
						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock).OverflowPolicy(ETextOverflowPolicy::Ellipsis).Text(FText::FromString(Item->Data->Name))
							.IsEnabled_Lambda([Item] { return Item->Data->bActive; })
						]
					]
					.BodyContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(0.25)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							[
								SNew(STextBlock).Text_Lambda([] { return FText::FromString(LOCALIZATION("Desc").ToString() + ":"); })
							]
							+ SVerticalBox::Slot()
							[
								SNew(STextBlock).Text_Lambda([] { return FText::FromString(LOCALIZATION("Version").ToString() + ":"); })
							]
							+ SVerticalBox::Slot()
							[
								SNew(STextBlock).Text_Lambda([] { return FText::FromString(LOCALIZATION("Author").ToString() + ":"); })
							]
							+ SVerticalBox::Slot()
							[
								SNew(STextBlock).Text_Lambda([] { return FText::FromString(LOCALIZATION("Location").ToString() + ":"); })
							]
						]
						+ SHorizontalBox::Slot()
						.FillWidth(0.75)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							[
								SNew(STextBlock).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
								.Text_Lambda([=] { return FText::FromString(Item->Data->Desc); })
							]
							+ SVerticalBox::Slot()
							[
								SNew(STextBlock).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
								.Text_Lambda([=] { return FText::FromString(Item->Data->Version); })
							]
							+ SVerticalBox::Slot()
							[
								SNew(STextBlock).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
								.Text_Lambda([=] { return FText::FromString(Item->Data->Author); })
							]
							+ SVerticalBox::Slot()
							[
								SNew(STextBlock).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
									.Text_Lambda([=] { return FText::FromString(Item->Data->Path); })
							]
						]
					]
				]
				
			];
		Row->SetBorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"));
		return Row;
	}

	const FName CommandColId = "Command";
	const FName ShortCutColId = "ShortCut";
	const FName CategoryColId = "Category";

	void SKeymapView::GenerateKeyDatas(const FText& InFilterText)
	{
		KeyDatas.Reset();
		TArray<TSharedPtr<FUICommandInfo>> Commands;
		TArray<TSharedPtr<FUICommandInfo>> DebuggerViewCommands, GraphEditorCommandInfos, CodeEditorCommandInfos, AssetViewCommands;
		FInputBindingManager::Get().GetCommandInfosFromContext("DebuggerViewCommands", DebuggerViewCommands);
		FInputBindingManager::Get().GetCommandInfosFromContext("AssetViewCommands", AssetViewCommands);
		FInputBindingManager::Get().GetCommandInfosFromContext("GraphEditorCommands", GraphEditorCommandInfos);
		FInputBindingManager::Get().GetCommandInfosFromContext("CodeEditorCommands", CodeEditorCommandInfos);
		Commands.Append(MoveTemp(CodeEditorCommandInfos));
		Commands.Append(MoveTemp(AssetViewCommands));
		Commands.Append(MoveTemp(GraphEditorCommandInfos));
		Commands.Append(MoveTemp(DebuggerViewCommands));
		for (const auto& Command : Commands)
		{
			if (!InFilterText.IsEmpty())
			{
				FText Label = Command->GetLabel();
				FText Category = LOCALIZATION(Command->GetBindingContext().ToString());
				FText Keybinding = Command->GetActiveChord(EMultipleKeyBindingIndex::Primary)->GetInputText(false);
				if (Category.ToString().Contains(InFilterText.ToString()) ||
					Keybinding.ToString().Contains(InFilterText.ToString()) ||
					Label.ToString().Contains(InFilterText.ToString()))
				{
					KeyDatas.Add(MakeShared<KeyData>(Command));
				}
			}
			else
			{
				KeyDatas.Add(MakeShared<KeyData>(Command));
			}
		}
	}

	void SKeymapView::Construct(const FArguments& InArgs)
	{
		GenerateKeyDatas();
		ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding(0, 0, 4, 0)
				[
					SNew(SSearchBox).MinDesiredWidth(120)
					.HintText(LOCALIZATION("Search"))
					.OnTextChanged_Lambda([this](const FText& InFilterText) {
						GenerateKeyDatas(InFilterText);
						KeyListView->RequestListRefresh();
					})
					.DelayChangeNotificationsWhileTyping(true)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SIconButton)
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle< FButtonStyle >("Button"))
					.Label(LOCALIZATION("Reset"))
					.Icon(FAppStyle::Get().GetBrush("GenericCommands.Undo"))
					.IconSize(FVector2D{12,12})
					.OnClicked_Lambda([this] {
						FInputBindingManager::Get().RemoveUserDefinedChords();
						GConfig->Flush(false, GEditorKeyBindingsIni);
						for (auto& KeyData : KeyDatas)
						{
							const auto& DefaultChord = KeyData->CommandInfo->GetDefaultChord(EMultipleKeyBindingIndex::Primary);
							KeyData->CommandInfo->SetActiveChord(DefaultChord, EMultipleKeyBindingIndex::Primary);
						}
						return FReply::Handled();
					})
				]
			]
			+SVerticalBox::Slot()
			.Padding(0, 4, 0, 0)
			[
				SAssignNew(KeyListView, SListView<KeyDataPtr>)
				.ListItemsSource(&KeyDatas)
				.OnGenerateRow(this, &SKeymapView::GenerateRowForItem)
				.OnMouseButtonDoubleClick_Lambda([this](KeyDataPtr Item) {
					auto Row = KeyListView->WidgetFromItem(Item);
					if (Row)
					{
						StaticCastSharedPtr<SKeymapViewRow>(Row)->KeybindingText->EnterEditingMode();
					}
				})
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(CommandColId)
					.FillWidth(0.4f)
					.DefaultLabel(LOCALIZATION(CommandColId.ToString()))
					+ SHeaderRow::Column(ShortCutColId)
					.FillWidth(0.35f)
					.DefaultLabel(LOCALIZATION(ShortCutColId.ToString()))
					+ SHeaderRow::Column(CategoryColId)
					.FillWidth(0.25f)
					.DefaultLabel(LOCALIZATION(CategoryColId.ToString()))
				)
			]
			
		];
	}

	TSharedRef<ITableRow> SKeymapView::GenerateRowForItem(KeyDataPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		TSharedRef<SKeymapViewRow> TableRow = SNew(SKeymapViewRow, this, Item, OwnerTable)
			.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FTableRowStyle>("Keybinding.Row"));
		if (Item->CommandInfo->GetActiveChord(EMultipleKeyBindingIndex::Primary)->Key.IsMouseButton())
		{
			TableRow->SetEnabled(false);
		}
		return TableRow;
	}

	void SKeymapViewRow::Construct(const FArguments& InArgs, SKeymapView* InOwner, KeyDataPtr InData, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Owner = InOwner;
		Data = InData;
		SMultiColumnTableRow<KeyDataPtr>::Construct(InArgs, OwnerTableView);
	}

	TSharedRef<SWidget> SKeymapViewRow::GenerateWidgetForColumn(const FName& ColumnId)
	{
		TSharedPtr<SWidget> Content;
		if (ColumnId == CommandColId)
		{
			Content = SNew(STextBlock).Text(Data->CommandInfo->GetLabel());
		}
		else if(ColumnId == CategoryColId)
		{
			Content = SNew(STextBlock).Text(LOCALIZATION(Data->CommandInfo->GetBindingContext().ToString()));
		}
		else
		{
			Content = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(KeybindingText, SKeybindingText)
					.Command(Data->CommandInfo)
			]
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			[
				SNew(SIconButton).ButtonStyle(&FAppCommonStyle::Get().GetWidgetStyle< FButtonStyle >("SuperSimpleButton"))
				.Icon(FAppStyle::Get().GetBrush("Icons.ArrowLeft"))
				.Visibility_Lambda([this] {
					if (*Data->CommandInfo->GetActiveChord(EMultipleKeyBindingIndex::Primary) != Data->CommandInfo->GetDefaultChord(EMultipleKeyBindingIndex::Primary))
					{
						return EVisibility::Visible;
					}
					return EVisibility::Hidden;
				})
				.OnClicked_Lambda([this] {
					const auto& DefaultChord = Data->CommandInfo->GetDefaultChord(EMultipleKeyBindingIndex::Primary);
					Data->CommandInfo->SetActiveChord(DefaultChord, EMultipleKeyBindingIndex::Primary);
					FInputBindingManager::Get().SaveInputBindings();
					GConfig->Flush(false, GEditorPerProjectIni);
					return FReply::Handled();
				})
			];
			 
		}

		return SNew(SBox).VAlign(VAlign_Center)
		[
			Content.ToSharedRef()
		];
	}

	void SAppearanceView::Construct(const FArguments& InArgs)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		auto PreferenceWindow = ShEditor->GetPreferenceWindow().Pin().ToSharedRef();

		auto UserInterfaceGrid = SNew(SGridPanel).FillColumn(0, 0.4f).FillColumn(1, 0.5f).FillColumn(2, 0.1f);
		auto CodeEditorGrid = SNew(SGridPanel).FillColumn(0, 0.4f).FillColumn(1, 0.5f).FillColumn(2, 0.1f);

		Palette.Add(EStyleColor::Foreground);
		Palette.Add(EStyleColor::ForegroundHover);
		Palette.Add(EStyleColor::Border);
		Palette.Add(EStyleColor::Background);
		Palette.Add(EStyleColor::Panel);
		Palette.Add(EStyleColor::Recessed);
		Palette.Add(EStyleColor::Input);
		Palette.Add(EStyleColor::Secondary);
		Palette.Add(EStyleColor::Dropdown);
		Palette.Add(EStyleColor::Hover);
		Palette.Add(EStyleColor::Primary);
		Palette.Add(EStyleColor::Select);
		Palette.Add(EStyleColor::SelectInactive);
		Palette.Add(EStyleColor::SelectHover);
		Palette.Add(EStyleColor::Header);
		Palette.Add(EStyleColor::Title);

		SyntaxPalette.Add(EStyleColor::User1);
		SyntaxPalette.Add(EStyleColor::User2);
		SyntaxPalette.Add(EStyleColor::User3);
		SyntaxPalette.Add(EStyleColor::User4);
		SyntaxPalette.Add(EStyleColor::User5);
		SyntaxPalette.Add(EStyleColor::User6);
		SyntaxPalette.Add(EStyleColor::User7);
		SyntaxPalette.Add(EStyleColor::User8);
		SyntaxPalette.Add(EStyleColor::User9);
		SyntaxPalette.Add(EStyleColor::User10);
		SyntaxPalette.Add(EStyleColor::User11);
		SyntaxPalette.Add(EStyleColor::User12);
		SyntaxPalette.Add(EStyleColor::User13);

		auto AppendItem = [&, ItemNum = 0](auto& InGrid, const FText& InLabel, const FText& InToolTipText = FText::GetEmpty()) mutable -> SHorizontalBox::FScopedWidgetSlotArguments
		{
			InGrid->AddSlot(0, ItemNum)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock).Text(InLabel).ToolTipText(InToolTipText)
			];
			TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox);
			InGrid->AddSlot(1, ItemNum)
			.Padding(12.f, 2.f, 0, 2.f)
			[
				HBox
			];
			InGrid->AddSlot(2, ItemNum);
			ItemNum++;
			return HBox->AddSlot();
		};
		auto AppendUserInterfaceItem = AppendItem;
		auto AppendCodeEditorItem = AppendItem;
		
		for (const auto& StyleColor : Palette)
		{
			AppendUserInterfaceItem(UserInterfaceGrid, USlateThemeManager::Get().GetColorDisplayName(StyleColor))
			[
				SNew(SShColorBlockPicker, PreferenceWindow)
				.Color_Lambda([&] { return USlateThemeManager::Get().GetColor(StyleColor); })
				.OnColorChanged_Lambda([&](const FLinearColor& InColor) {
					FLinearColor& Color = const_cast<FLinearColor&>(USlateThemeManager::Get().GetColor(StyleColor));
					Color = InColor;
				})
			];
		}

		AppendCodeEditorItem(CodeEditorGrid, LOCALIZATION("MouseZoom"), LOCALIZATION("MouseZoomTip"))
		[
			SNew(SCheckBox).IsChecked_Lambda([] {
				return SShaderEditorBox::CanMouseWheelZoom() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;;
			})
			.OnCheckStateChanged_Lambda([](ECheckBoxState InState) {
				Editor::GetEditorConfig()->SetBool(TEXT("CodeEditor"), TEXT("MouseWheelZoom"), InState == ECheckBoxState::Checked);
				Editor::SaveEditorConfig();
			})
		];

		AppendCodeEditorItem(CodeEditorGrid, LOCALIZATION("ShowColorBlock"), LOCALIZATION("ShowColorBlockTip"))
		[
			SNew(SCheckBox).IsChecked_Lambda([] {
				return SShaderEditorBox::CanShowColorBlock() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;;
			})
			.OnCheckStateChanged_Lambda([](ECheckBoxState InState) {
				Editor::GetEditorConfig()->SetBool(TEXT("CodeEditor"), TEXT("ShowColorBlock"), InState == ECheckBoxState::Checked);
				Editor::SaveEditorConfig();
			})
		];

		AppendCodeEditorItem(CodeEditorGrid, LOCALIZATION("RealTimeDiagnosis"))
		[
			SNew(SCheckBox).IsChecked_Lambda([] {
				return SShaderEditorBox::CanRealTimeDiagnosis() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;;
			})
			.OnCheckStateChanged_Lambda([](ECheckBoxState InState) {
				Editor::GetEditorConfig()->SetBool(TEXT("CodeEditor"), TEXT("RealTimeDiagnosis"), InState == ECheckBoxState::Checked);
				Editor::SaveEditorConfig();
				auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
				for (auto ShaderEditor : ShEditor->GetShaderEditors())
				{
					ShaderEditor->ClearDiagInfoEffect();
				}
			})
		];

		AppendCodeEditorItem(CodeEditorGrid, LOCALIZATION("GuideLine"))
		[
			SNew(SCheckBox).IsChecked_Lambda([] {
				return SShaderEditorBox::CanShowGuideLine() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;;
			})
			.OnCheckStateChanged_Lambda([](ECheckBoxState InState) {
				Editor::GetEditorConfig()->SetBool(TEXT("CodeEditor"), TEXT("ShowGuideLine"), InState == ECheckBoxState::Checked);
				Editor::SaveEditorConfig();
			})
		];

		AppendCodeEditorItem(CodeEditorGrid, LOCALIZATION("QuickInfo"))
		[
			SNew(SCheckBox).IsChecked_Lambda([] {
				return SShaderEditorBox::CanShowQuickInfo() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;;
			})
			.OnCheckStateChanged_Lambda([](ECheckBoxState InState) {
				Editor::GetEditorConfig()->SetBool(TEXT("CodeEditor"), TEXT("ShowQuickInfo"), InState == ECheckBoxState::Checked);
				Editor::SaveEditorConfig();
			})
		];

		AppendCodeEditorItem(CodeEditorGrid, LOCALIZATION("HighlightCursorLine"))
		[
			SNew(SCheckBox).IsChecked_Lambda([] {
				return SShaderEditorBox::CanHighlightCursorLine() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;;
			})
			.OnCheckStateChanged_Lambda([](ECheckBoxState InState) {
				Editor::GetEditorConfig()->SetBool(TEXT("CodeEditor"), TEXT("HighlightCursorLine"), InState == ECheckBoxState::Checked);
				Editor::SaveEditorConfig();
			})
		];

		auto FontPathEditBox = SNew(SEditableTextBox).Text_Lambda([] {
				return FText::FromString(SShaderEditorBox::GetFontPath());
			})
			.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
			.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
				Editor::GetEditorConfig()->SetString(TEXT("CodeEditor"), TEXT("Font"), *NewText.ToString());
				Editor::SaveEditorConfig();
				auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
				for (auto ShaderEditor : ShEditor->GetShaderEditors())
				{
					ShaderEditor->RefreshFont();
				}
			});
		FontPathEditBox->SetToolTipText(TAttribute<FText>::CreateLambda([Self = &*FontPathEditBox] { return Self->GetText(); }));
		AppendCodeEditorItem(CodeEditorGrid, LOCALIZATION("Font"))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				FontPathEditBox
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SIconButton).Icon(FAppStyle::Get().GetBrush("Icons.Search"))
				.OnClicked_Lambda([this] {
					static FString CacheSelectDir = FPaths::GetPath(SShaderEditorBox::GetFontPath());
					FString DialogType = "Font(*.ttf)|*.ttf";
					IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
					TArray<FString> OpenedFileNames;
					TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
					void* ParentWindowHandle = (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid()) ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;
					if (DesktopPlatform->OpenFileDialog(ParentWindowHandle, "Select Font", CacheSelectDir, "", MoveTemp(DialogType), EFileDialogFlags::None, OpenedFileNames))
					{
						if (OpenedFileNames.Num() > 0)
						{
							CacheSelectDir = FPaths::GetPath(OpenedFileNames[0]);
							Editor::GetEditorConfig()->SetString(TEXT("CodeEditor"), TEXT("Font"), *FPaths::ConvertRelativePathToFull(OpenedFileNames[0]));
							Editor::SaveEditorConfig();
							auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
							for (auto ShaderEditor : ShEditor->GetShaderEditors())
							{
								ShaderEditor->RefreshFont();
							}
						}
					}
					return FReply::Handled();
				})
			]
		];

		AppendCodeEditorItem(CodeEditorGrid, LOCALIZATION("FontSize"))
		.AutoWidth()
		[
			SNew(SSpinBox<int32>)
			.MinValue(SShaderEditorBox::MinFontSize)
			.OnValueChanged_Lambda([this](int32 NewValue) {
				Editor::GetEditorConfig()->SetInt64(TEXT("CodeEditor"), TEXT("FontSize"), NewValue);
				Editor::SaveEditorConfig();
				auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
				for (auto ShaderEditor : ShEditor->GetShaderEditors())
				{
					ShaderEditor->RefreshFont();
				}
			})
			.Value_Lambda([] { 
				return SShaderEditorBox::GetFontSize();
			})
		];

		for (const auto& StyleColor : SyntaxPalette)
		{
			AppendCodeEditorItem(CodeEditorGrid, USlateThemeManager::Get().GetColorDisplayName(StyleColor))
			[
				SNew(SShColorBlockPicker, PreferenceWindow)
				.Color_Lambda([&] { return USlateThemeManager::Get().GetColor(StyleColor); })
				.OnColorChanged_Lambda([&](const FLinearColor& InColor) {
					FLinearColor& Color = const_cast<FLinearColor&>(USlateThemeManager::Get().GetColor(StyleColor));
					Color = InColor;
				})
			];
		}

		for (const auto& Theme : USlateThemeManager::Get().GetThemes())
		{
			Themes.Add(MakeShared<FGuid>(Theme.Id));
		}

		ChildSlot
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().Padding(0,0,0,8)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(SComboBox<TSharedPtr<FGuid>>)
						.Visibility_Lambda([this] { return IsNaming ? EVisibility::Collapsed : EVisibility::Visible; })
						.OptionsSource(&Themes)
						.OnSelectionChanged_Lambda([this](TSharedPtr<FGuid> InItem, ESelectInfo::Type) {
							Editor::SetTheme(*InItem);
						})
						.OnGenerateWidget_Lambda([this](TSharedPtr<FGuid> InItem) {
							FText ThemeName = Editor::GetTheme(*InItem).DisplayName;
							if (*InItem == Editor::GetDefaultTheme())
							{
								ThemeName = FText::FromString(ThemeName.ToString() + " (" + LOCALIZATION("Default").ToString() + ")");
							}
							return SNew(STextBlock).Text(ThemeName);
						})
						[
							SNew(STextBlock).Text_Lambda([this] {
								FText ThemeName = Editor::GetMutableCurrentTheme().DisplayName;
								if (Editor::GetMutableCurrentTheme() == Editor::GetDefaultTheme())
								{
									return FText::FromString(ThemeName.ToString() + " (" + LOCALIZATION("Default").ToString() + ")");
								}
								return ThemeName;
							})
						]
					]
					+ SHorizontalBox::Slot()
					[
						SAssignNew(NameBox, SEditableTextBox)
						.SelectAllTextWhenFocused(true)
						.Visibility_Lambda([this] { return IsNaming ? EVisibility::Visible : EVisibility::Collapsed; })
						.OnTextCommitted_Lambda([this](const FText& InText, ETextCommit::Type) {
							USlateThemeManager::Get().SetCurrentThemeDisplayName(InText);
							FString ThemeName = Editor::GetMutableCurrentTheme().DisplayName.ToString();
							USlateThemeManager::Get().SaveCurrentThemeAs(PathHelper::ThemeDir() / ThemeName + ".json");
							IsNaming = false;
						})
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(25)
						[
							SNew(SIconButton).ButtonStyle(&FAppStyle::Get().GetWidgetStyle< FButtonStyle >("Button"))
							.Icon(FAppStyle::Get().GetBrush("Icons.Plus"))
							.IconSize(FVector2D{ 10,10 })
							.OnClicked_Lambda([this] {
								FGuid NewThemeId = USlateThemeManager::Get().DuplicateActiveTheme();
								Themes.Add(MakeShared<FGuid>(NewThemeId));
								Editor::SetTheme(NewThemeId);
								IsNaming = true;
								NameBox->SetText(Editor::GetMutableCurrentTheme().DisplayName);
								FSlateApplication::Get().SetKeyboardFocus(NameBox);
								return FReply::Handled();
							})
						]
					
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(25)
						[
							SNew(SIconButton).ButtonStyle(&FAppStyle::Get().GetWidgetStyle< FButtonStyle >("Button"))
							.IsEnabled_Lambda([] {
								return Editor::GetMutableCurrentTheme() != Editor::GetDefaultTheme();
							})
							.Icon(FAppStyle::Get().GetBrush("Icons.Minus"))
							.IconSize(FVector2D{ 10,10 })
							.OnClicked_Lambda([this] {
								FStyleTheme& CurTheme = Editor::GetMutableCurrentTheme();
								Themes.RemoveAll([&](const auto& InItem) { return *InItem == CurTheme.Id; });
								IFileManager::Get().Delete(*CurTheme.Filename);
								USlateThemeManager::Get().RemoveTheme(CurTheme.Id);
								USlateThemeManager::Get().ApplyTheme(Editor::GetDefaultTheme().Id);
								IsNaming = false;
								return FReply::Handled();
							})
						]

					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 0, 0)
					[
						SNew(SIconButton).ButtonStyle(&FAppStyle::Get().GetWidgetStyle< FButtonStyle >("Button"))
						.Label(LOCALIZATION("Reset"))
						.Icon(FAppStyle::Get().GetBrush("GenericCommands.Undo"))
						.IconSize(FVector2D{ 12,12 })
						.OnClicked_Lambda([this] {
							for (auto StyleColor : Palette)
							{
								USlateThemeManager::Get().ResetActiveColorToDefault(StyleColor);
							}
							for (auto StyleColor : SyntaxPalette)
							{
								USlateThemeManager::Get().ResetActiveColorToDefault(StyleColor);
							}
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(4,0,0,0)
					[
						SNew(SIconButton).ButtonStyle(&FAppStyle::Get().GetWidgetStyle< FButtonStyle >("Button"))
						.Label(LOCALIZATION("Save"))
						.Icon(FAppStyle::Get().GetBrush("Icons.Save"))
						.IconSize(FVector2D{ 12,12 })
						.OnClicked_Lambda([this] {
							FString ThemeName = Editor::GetMutableCurrentTheme().DisplayName.ToString();
							USlateThemeManager::Get().SaveCurrentThemeAs(PathHelper::ThemeDir() / ThemeName + ".json");
							CallPrivate_USlateThemeManager_LoadThemeColors(USlateThemeManager::Get(), Editor::GetMutableCurrentTheme());
							return FReply::Handled();
						})
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SExpandableArea)
					.AreaTitle(LOCALIZATION("UserInterface"))
					.AreaTitleFont(FAppStyle::Get().GetFontStyle("NormalFont"))
					.BodyBorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
					.BodyContent()
					[
						UserInterfaceGrid
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SExpandableArea)
					.AreaTitle(LOCALIZATION("CodeEditor"))
					.AreaTitleFont(FAppStyle::Get().GetFontStyle("NormalFont"))
					.BodyBorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
					.BodyContent()
					[
						CodeEditorGrid
					]
				]
			]
		];
	}

	void SDebugView::Construct(const FArguments& InArgs)
	{
		auto DebugGrid = SNew(SGridPanel).FillColumn(0, 0.4f).FillColumn(1, 0.5f).FillColumn(2, 0.1f);
		auto AppendItem = [ItemNum = 0](auto& InGrid, const FText& InLabel, const FText& InToolTipText = FText::GetEmpty()) mutable -> SHorizontalBox::FScopedWidgetSlotArguments
		{
			InGrid->AddSlot(0, ItemNum)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				[
					SNew(STextBlock).Text(InLabel).ToolTipText(InToolTipText)
				];
			TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox);
			InGrid->AddSlot(1, ItemNum)
				.Padding(12.f, 2.f, 0, 2.f)
				[
					HBox
				];
			InGrid->AddSlot(2, ItemNum);
			ItemNum++;
			return HBox->AddSlot();
		};

		AppendItem(DebugGrid, LOCALIZATION("EnableUbsan"), LOCALIZATION("EnableUbsanTip"))
		[
			SNew(SCheckBox).IsChecked_Lambda([] {
				return ShaderDebugger::EnableUbsan() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;;
			})
			.OnCheckStateChanged_Lambda([](ECheckBoxState InState) {
				Editor::GetEditorConfig()->SetBool(TEXT("Debugger"), TEXT("EnableUbsan"), InState == ECheckBoxState::Checked);
				Editor::SaveEditorConfig();
			})
		];

		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
				[
					DebugGrid
				]
			]
	
		];
	}

	void SPreferenceView::Construct(const FArguments& InArgs)
	{
		SAssignNew(KeymapView, SKeymapView);
		SAssignNew(PluginView, SPluginView);
		SAssignNew(AppearanceView, SAppearanceView);
		SAssignNew(DebugView, SDebugView);

		auto PluginPreference = CreatePreference(LOCALIZATION("Plugins"), PluginView.ToSharedRef());
		auto AppearancePreference = CreatePreference(LOCALIZATION("Appearance"), AppearanceView.ToSharedRef());
		auto KeymapPreference = CreatePreference(LOCALIZATION("Keymap"), KeymapView.ToSharedRef());
		auto DebugPreference = CreatePreference(LOCALIZATION("Debug"), DebugView.ToSharedRef());
		CurPreference = &*PluginPreference;

		ChildSlot
		[
			SNew(SBorder)
			.Padding(8)
			[
				SAssignNew(PreferenceBox, SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.Padding(0, 0, 0, 1.5)
						.AutoHeight()
						[
							PluginPreference
						]
						+ SVerticalBox::Slot()
						.Padding(0, 0, 0, 1.5)
						.AutoHeight()
						[
							AppearancePreference
						]
						+ SVerticalBox::Slot()
						.Padding(0, 0, 0, 1.5)
						.AutoHeight()
						[
							KeymapPreference
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							DebugPreference
						]
					]
				]
				+SHorizontalBox::Slot()
				.Padding(16, 0, 0, 0)
				[
					PluginView.ToSharedRef()
				]
			]
		];
		

	}

	TSharedRef<SWidget> SPreferenceView::CreatePreference(const FText& InText, TSharedRef<SWidget> InView)
	{
		TSharedPtr<SBorder> Preference = SNew(SBorder).Padding(FMargin{ 4.0f, 2.0f }).HAlign(HAlign_Left).VAlign(VAlign_Center).DesiredSizeScale(FVector2D{ 2.5, 1.2 });
		auto TextBlock = SNew(STextBlock).Text(InText).ColorAndOpacity_Lambda([this, Self = &*Preference] {
			if (CurPreference == Self) { return FStyleColors::White; }
			return FStyleColors::Foreground;
		});
		Preference->SetContent(TextBlock);
		Preference->SetBorderImage(TAttribute<const FSlateBrush*>::CreateLambda([this, Self = &*Preference] {
			if (CurPreference == Self)
			{
				return FAppStyle::Get().GetBrush("Brushes.Select");
			}
			return FAppStyle::Get().GetBrush("Brushes.Secondary");
		}));
		Preference->SetOnMouseButtonDown(FPointerEventHandler::CreateLambda([this, InView, Self = &*Preference](auto&&, auto&&) {
			CurPreference = Self;
			PreferenceBox->GetSlot(1).AttachWidget(InView);
			return FReply::Handled();
		}));
		return Preference.ToSharedRef();
	}
}
