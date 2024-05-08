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

    FReply AssetViewAssetItem::HandleOnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
    {
        if(MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
        {
            return FReply::Handled().BeginDragDrop(AssetViewItemDragDropOp::New(Path));
        }
        return FReply::Unhandled();
    }

	TSharedRef<ITableRow> AssetViewAssetItem::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = SNew(STableRow<TSharedRef<AssetViewItem>>, OwnerTable)
			.Style(&FAppCommonStyle::Get().GetWidgetStyle<FTableRowStyle>("AssetView.Row"))
            .OnDragDetected_Raw(this, &AssetViewAssetItem::HandleOnDragDetected);

		TSharedPtr<SWidget> Display;

		if (AssetThumbnail)
		{
			ThumbnailViewport->SetViewPortRenderTexture(AssetThumbnail);
			Display =
				SNew(SViewport)
				.ViewportInterface(ThumbnailViewport)
				.ViewportSize(TAttribute<FVector2D>::CreateLambda(
					[this] { 
						float PreviewBoxWidth = (float)PreviewBox->GetCachedGeometry().GetLocalSize().X;
						float PreviewBoxHeight = (float)PreviewBox->GetCachedGeometry().GetLocalSize().Y;
						float ThumbnailWidth = (float)AssetThumbnail->GetWidth();
						float ThumbnailHeight = (float)AssetThumbnail->GetHeight();
						 
						float ScaleFactor = FMath::Clamp(ThumbnailWidth > ThumbnailHeight ? PreviewBoxWidth / ThumbnailWidth : PreviewBoxHeight / ThumbnailHeight, 0 , 1);
						return FVector2D(ThumbnailWidth, ThumbnailHeight) * ScaleFactor;
					}
				));
	
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
				SAssignNew(PreviewBox, SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
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
                    SAssignNew(AssetEditableTextBlock, SInlineEditableTextBlock)
                    .Font(FAppStyle::Get().GetFontStyle("SmallFont"))
                    .Text(FText::FromString(FPaths::GetBaseFilename(Path)))
                    .OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
                        FString NewFilePath = FPaths::GetPath(Path) / NewText.ToString() + "."
                            + FPaths::GetExtension(Path);
                        if(NewFilePath != Path)
                        {
                            IFileManager::Get().Move(*NewFilePath, *Path);
                        }
                    })
                    .OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                ]
			
			]
		);

		return Row;
	}

    void AssetViewAssetItem::EnterRenameState()
    {
        AssetEditableTextBlock->EnterEditingMode();
    }

}
