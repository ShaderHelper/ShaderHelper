#include "CommonHeader.h"
#include "AssetViewAssetItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Widgets/Text/SInlineEditableTextBlock.h>
#include <Widgets/SViewport.h>
#include "AssetManager/AssetManager.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "ProjectManager/ProjectManager.h"
#include "App/App.h"

namespace FW
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
        return FReply::Handled().BeginDragDrop(AssetViewItemDragDropOp::New(Path));
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

		if (AssetThumbnail)
		{
            PreviewBox->SetHAlign(HAlign_Center);
            PreviewBox->SetVAlign(VAlign_Center);
            
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
		else
		{
			Display = SNew(SImage)
				.Image(ImageBrush);
		}
        
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
						FGuid Id = TSingleton<AssetManager>::Get().GetGuid(Path);
						AssetObject* Asset = TSingleton<AssetManager>::Get().FindLoadedAsset(Id);

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
                                MessageDialog::Open(MessageDialog::Ok, GApp->GetEditor()->GetMainWindow(), LOCALIZATION("AssetRenameFailure"));
                            }
                            else
                            {
								if (GProject->IsPendingAsset(Path))
								{
									if (MessageDialog::Open(MessageDialog::OkCancel, GApp->GetEditor()->GetMainWindow(), LOCALIZATION("OpPendingAssetTip")))
									{
										IFileManager::Get().Move(*NewFilePath, *Path);
									}
								}
								else
								{
									IFileManager::Get().Move(*NewFilePath, *Path);
								}
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
