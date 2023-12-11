#include "CommonHeader.h"
#include "SAssetView.h"
#include <Slate/Widgets/Views/STileView.h>
#include <DirectoryWatcher/DirectoryWatcherModule.h>
#include <DirectoryWatcher/IDirectoryWatcher.h>
#include <DesktopPlatform/DesktopPlatformModule.h>
#include "Common/Util/Reflection.h"
#include "AssetManager/AssetImporter/AssetImporter.h"

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
				.OnContextMenuOpening(this, &SAssetView::CreateContextMenu)
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
				.Visibility_Lambda([this] {	return AssetViewItems.Num() != 0 ? EVisibility::Collapsed : EVisibility::Visible; })
			]
		];
	}

	void SAssetView::SetNewViewDirectory(const FString& NewViewDirectory)
	{
		AssetViewItems.Empty();
		ImportAssetPath = NewViewDirectory;
	}

	TSharedPtr<SWidget> SAssetView::CreateContextMenu()
	{
		FMenuBuilder MenuBuilder{ true, TSharedPtr<FUICommandList>() };

		MenuBuilder.BeginSection("NewAsset", FText::FromString("New Asset"));
		{
			MenuBuilder.AddMenuEntry(
				FText::FromString("Import"),
				FText::GetEmpty(),
				FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "Icons.Import" },
				FUIAction{ FExecuteAction::CreateRaw(this, &SAssetView::ImportAsset) });
		}
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}

	void SAssetView::ImportAsset()
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (DesktopPlatform)
		{
			TArray<FString> FileExts;
			TArray<ShReflectToy::MetaType*> AssetImporterMetaTypes = ShReflectToy::GetMetaTypes<AssetImporter>();
			for (auto MetaTypePtr : AssetImporterMetaTypes)
			{
				void* Importer = MetaTypePtr->GetDefaultObject();
				if (Importer)
				{
					FileExts = static_cast<AssetImporter*>(Importer)->SupportFileExts();
				}
			}
			
			FString DialogType = "All files|";
			for (const FString& FileExt : FileExts)
			{
				DialogType += FString::Format(TEXT("*.{0};"), {FileExt});
			}

			TArray<FString> OpenedFileNames;
			if (DesktopPlatform->OpenFileDialog(nullptr, "Import Asset", ImportAssetPath, "", MoveTemp(DialogType), EFileDialogFlags::None, OpenedFileNames))
			{
				if (OpenedFileNames.Num() > 0)
				{
					ImportAssetPath = OpenedFileNames[0];
				}
			}
		}
	}

}