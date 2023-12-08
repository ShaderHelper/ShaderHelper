#include "CommonHeader.h"
#include "SAssetView.h"
#include <Slate/Widgets/Views/STileView.h>

namespace FRAMEWORK
{

	void SAssetView::Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(STileView<TSharedRef<AssetViewItem>>)
				.ListItemsSource(&AssetViewItems)
				.OnGenerateTile_Lambda([](TSharedRef<AssetViewItem> InTileItem, const TSharedRef<STableViewBase>& OwnerTable) {
					return InTileItem->GenerateWidgetForTableView(OwnerTable);
				})
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString("The folder is empty"))
				.Visibility_Lambda([this] {	return EVisibility::Visible; })
			]
		];
	}

}