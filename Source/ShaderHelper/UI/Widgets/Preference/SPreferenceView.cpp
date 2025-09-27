#include "CommonHeader.h"
#include "SPreferenceView.h"
#include <Styling/StyleColors.h>
#include "App/App.h"

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
					return FAppStyle::Get().GetBrush("Brushes.AccentGray");
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
							.OnCheckStateChanged_Lambda([Item](ECheckBoxState InState) {
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

	void SKeymapView::Construct(const FArguments& InArgs)
	{

	}

	void SKeymapView::SavePersistentState()
	{

	}

	void SThemeView::Construct(const FArguments& InArgs)
	{

	}

	void SThemeView::SavePersistentState()
	{

	}

	void SPreferenceView::Construct(const FArguments& InArgs)
	{
		SAssignNew(KeymapView, SKeymapView);
		SAssignNew(PluginView, SPluginView);
		SAssignNew(ThemeView, SThemeView);

		auto PluginPreference = CreatePreference(LOCALIZATION("Plugins"), PluginView.ToSharedRef());
		auto ThemePreference = CreatePreference(LOCALIZATION("Theme"), ThemeView.ToSharedRef());
		auto KeymapPreference = CreatePreference(LOCALIZATION("Keymap"), KeymapView.ToSharedRef());
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
						ThemePreference
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						KeymapPreference
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

	void SPreferenceView::SavePersistentState()
	{
		PluginView->SavePersistentState();
		KeymapView->SavePersistentState();
		ThemeView->SavePersistentState();
	}

	TSharedRef<SWidget> SPreferenceView::CreatePreference(const FText& InText, TSharedRef<SWidget> InView)
	{
		TSharedPtr<SBorder> Preference = SNew(SBorder).Padding(FMargin{ 4.0f, 2.0f }).HAlign(HAlign_Left).VAlign(VAlign_Center).DesiredSizeScale(FVector2D{ 2.5, 1.2 })[
			SNew(STextBlock).Text(InText)
		];
		Preference->SetBorderImage(TAttribute<const FSlateBrush*>::CreateLambda([this, Self = &*Preference] {
			if (CurPreference == Self)
			{
				return FAppStyle::Get().GetBrush("Brushes.Select");
			}
			return FAppStyle::Get().GetBrush("Brushes.Header");
			}));
		Preference->SetOnMouseButtonDown(FPointerEventHandler::CreateLambda([this, InView, Self = &*Preference](auto&&, auto&&) {
			CurPreference = Self;
			PreferenceBox->GetSlot(1).AttachWidget(InView);
			return FReply::Handled();
		}));
		return Preference.ToSharedRef();
	}
}
