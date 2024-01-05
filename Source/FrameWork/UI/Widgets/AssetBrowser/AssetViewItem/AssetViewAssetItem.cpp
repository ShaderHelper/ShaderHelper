#include "CommonHeader.h"
#include "AssetViewAssetItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Widgets/Text/SInlineEditableTextBlock.h>
#include <Widgets/SViewport.h>
#include "AssetManager/AssetManager.h"

namespace FRAMEWORK
{

	AssetViewAssetItem::AssetViewAssetItem(const FString& InPath)
		: AssetViewItem(InPath)
	{
		ThumbnailViewport = MakeShared<PreviewViewPort>();
		AssetPtr<AssetObject> Asset = TSingleton<AssetManager>::Get().LoadAssetByPath<AssetObject>(Path);
		AssetThumbnail = Asset->GetThumbnail();
		ImageBrush = Asset->GetImage();
	}

	TSharedRef<ITableRow> AssetViewAssetItem::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = SNew(STableRow<TSharedRef<AssetViewItem>>, OwnerTable)
			.Style(&FAppCommonStyle::Get().GetWidgetStyle<FTableRowStyle>("AssetView.Row"));

		TSharedPtr<SWidget> Display;

		if (AssetThumbnail)
		{
			ThumbnailViewport->SetViewPortRenderTexture(AssetThumbnail);
			Display = SNew(SViewport)
				.ViewportInterface(ThumbnailViewport);
		}
		else if (ImageBrush)
		{
			Display = SNew(SImage)
				.Image(ImageBrush);
		}
		else
		{
			Display = SNew(SImage);
		}

		Row->SetContent(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
				.Padding(FMargin(5.0f, 4.0f, 5.0f, 6.0f))
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						Display.ToSharedRef()
					]
					+SOverlay::Slot()
					[
						SNew(SImage)
						.Image(FAppStyle::Get().GetBrush("Brushes.Select"))
						.ColorAndOpacity_Lambda([Row] {
							if (Row->IsSelected())
							{
								return FSlateColor{ FLinearColor{1.0f, 1.0f, 1.0f, 0.2f} };
							}
							else
							{
								return FSlateColor{ FLinearColor{1.0f, 1.0f, 1.0f, 0.0f} };
							}
						})
					]
					
				]
				
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
                SNew(SBox)
                .Padding(FMargin(2.0f, 0.0f, 2.0f, 0.0f))
                [
                    SAssignNew(FolderEditableTextBlock, SInlineEditableTextBlock)
                    .Font(FAppStyle::Get().GetFontStyle("SmallFont"))
                    .Text(FText::FromString(FPaths::GetBaseFilename(Path)))
                    .OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
                        FString NewFilePath = FPaths::GetPath(Path) / NewText.ToString();
                        IFileManager::Get().Move(*NewFilePath, *Path);
                    })
                    .OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                ]
			
			]
		);

		return Row;
	}

}
