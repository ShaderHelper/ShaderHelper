#include "CommonHeader.h"
#include "AssetViewAssetItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "AssetManager/AssetManager.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "ProjectManager/ProjectManager.h"
#include "App/App.h"

#include <Widgets/Text/SInlineEditableTextBlock.h>
#include <Widgets/SViewport.h>

namespace FW
{
	AssetViewAssetItem::AssetViewAssetItem(STileView<TSharedRef<AssetViewItem>>* InOwner, const FString& InPath)
		: AssetViewItem(InOwner, InPath)
	{
	}

    FReply AssetViewAssetItem::HandleOnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
    {
		TArray<FString> SelectedPaths;
		for (const auto& Item : Owner->GetSelectedItems())
		{
			SelectedPaths.Add(Item->GetPath());
		}
        return FReply::Handled().BeginDragDrop(AssetViewItemDragDropOp::New(SelectedPaths));
    }

	TSharedRef<ITableRow> AssetViewAssetItem::GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable)
	{
		auto Row = SNew(STableRow<TSharedRef<AssetViewItem>>, OwnerTable)
			.Style(&FAppCommonStyle::Get().GetWidgetStyle<FTableRowStyle>("AssetView.Row"))
            .OnDragDetected_Raw(this, &AssetViewAssetItem::HandleOnDragDetected);

		TSharedPtr<SWidget> Display;
        
        SAssignNew(PreviewBox, SBorder)
        .BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
        .Padding(FMargin(5.0f, 4.0f, 5.0f, 6.0f));

		Display =
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SNew(SImage).Image(ImageBrush)
				.Visibility_Lambda([this] {
					return AssetThumbnail ? EVisibility::Collapsed : EVisibility::Visible;
				})
			]
			+SOverlay::Slot()
			[
				SNew(SViewport)
				.Visibility_Lambda([this] {
					return AssetThumbnail ? EVisibility::Visible : EVisibility::Collapsed;
				})
				.ViewportInterface(ThumbnailViewport)
				.ViewportSize_Lambda([this] { 
						float PreviewBoxWidth = (float)PreviewBox->GetCachedGeometry().GetLocalSize().X;
						float PreviewBoxHeight = (float)PreviewBox->GetCachedGeometry().GetLocalSize().Y;
						float ThumbnailWidth = (float)AssetThumbnail->GetWidth();
						float ThumbnailHeight = (float)AssetThumbnail->GetHeight();
						 
						float ScaleFactor = FMath::Clamp(ThumbnailWidth > ThumbnailHeight ? PreviewBoxWidth / ThumbnailWidth : PreviewBoxHeight / ThumbnailHeight, 0 , 1);
						return FVector2D(ThumbnailWidth, ThumbnailHeight) * ScaleFactor;
				})
			];
        
        PreviewBox->SetContent(
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
        );

		Row->SetContent(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					PreviewBox.ToSharedRef()
				]
                +SOverlay::Slot()
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Right)
				[
					SNew(STextBlock).Text(FText::FromString("*"))
					.Visibility_Lambda([this] {
						AssetObject* Asset = TSingleton<AssetManager>::Get().FindLoadedAsset(Path);

						if (Asset && Asset->IsDirty()) {
							return EVisibility::Visible;
						}
						return EVisibility::Collapsed;
					})
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
                            if(IFileManager::Get().FileExists(*NewFilePath))
                            {
                                MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, GApp->GetEditor()->GetMainWindow(), LOCALIZATION("AssetRenameFailure"));
                            }
                            else
                            {
                                IFileManager::Get().Move(*NewFilePath, *Path);
                            }
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
