#include "CommonHeader.h"
#include "SAssetView.h"
#include <DirectoryWatcher/DirectoryWatcherModule.h>
#include <DirectoryWatcher/IDirectoryWatcher.h>
#include <DesktopPlatform/DesktopPlatformModule.h>
#include "Common/Util/Reflection.h"
#include "AssetManager/AssetImporter/AssetImporter.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "AssetManager/AssetManager.h"
#include "UI/Widgets/AssetBrowser/AssetViewItem/AssetViewFolderItem.h"
#include "UI/Widgets/AssetBrowser/AssetViewItem/AssetViewAssetItem.h"

namespace FRAMEWORK
{

	void SAssetView::Construct(const FArguments& InArgs)
	{
		OnFolderDoubleClick = InArgs._OnFolderDoubleClick;

		ChildSlot
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(AssetTileView, STileView<TSharedRef<AssetViewItem>>)
				.ListItemsSource(&AssetViewItems)
				.ItemWidth(64)
				.ItemHeight(64)
				.OnContextMenuOpening(this, &SAssetView::CreateContextMenu)
				.OnGenerateTile_Lambda([](TSharedRef<AssetViewItem> InTileItem, const TSharedRef<STableViewBase>& OwnerTable) {
					return InTileItem->GenerateWidgetForTableView(OwnerTable);
				})
				.OnMouseButtonDoubleClick(this, &SAssetView::OnMouseButtonDoubleClick)
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
		CacheImportAssetPath = NewViewDirectory;
		CurViewDirectory = NewViewDirectory;
		PopulateAssetView(CurViewDirectory);
	}

	void SAssetView::PopulateAssetView(const FString& ViewDirectory)
	{
		AssetViewItems.Reset();

		TArray<FString> FileOrFolderNames;
		IFileManager::Get().FindFiles(FileOrFolderNames, *(ViewDirectory / TEXT("*")), true, true);
		for (const FString& FileOrFolderName : FileOrFolderNames)
		{
			if (FPaths::GetExtension(FileOrFolderName).IsEmpty())
			{
				AssetViewItems.Add(MakeShared<AssetViewFolderItem>(ViewDirectory / FileOrFolderName));
			}
			else
			{

			}
		}

		AssetTileView->RequestListRefresh();
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

	void SAssetView::AddFolder(const FString& InFolderName)
	{

	}

	void SAssetView::RemoveFolder(const FString& InFolderName)
	{

	}

	void SAssetView::AddFile(const FString& InFileName)
	{

	}

	void SAssetView::RemoveFile(const FString& InFileName)
	{

	}

	void SAssetView::ImportAsset()
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (DesktopPlatform)
		{
			TArray<FString> FileExts;
			TArray<ShReflectToy::MetaType*> AssetImporterMetaTypes = ShReflectToy::GetMetaTypes<AssetImporter>();
			TArray<AssetImporter*> AssetImporters;
			for (auto MetaTypePtr : AssetImporterMetaTypes)
			{
				void* Importer = MetaTypePtr->GetDefaultObject();
				if (Importer)
				{
					AssetImporter* CurImporter = static_cast<AssetImporter*>(Importer);
					FileExts = CurImporter->SupportFileExts();
					AssetImporters.Add(CurImporter);
				}
			}
			
			FString DialogType = "All files|";
			for (const FString& FileExt : FileExts)
			{
				DialogType += FString::Format(TEXT("*.{0};"), {FileExt});
			}

			TArray<FString> OpenedFileNames;
			TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
			void* ParentWindowHandle = (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid()) ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;
			if (DesktopPlatform->OpenFileDialog(ParentWindowHandle, "Import Asset", CacheImportAssetPath, "", MoveTemp(DialogType), EFileDialogFlags::None, OpenedFileNames))
			{
				if (OpenedFileNames.Num() > 0)
				{
					CacheImportAssetPath = OpenedFileNames[0];
					AssetImporter* FinalImporter = nullptr;
					FString OpenedFileExt = FPaths::GetExtension(OpenedFileNames[0]);
					for (AssetImporter* AssetImporterPtr : AssetImporters)
					{
						if(AssetImporterPtr->SupportFileExts().Contains(OpenedFileExt))
						{
							FinalImporter = AssetImporterPtr;
							break;
						}
					}

					check(FinalImporter);
					TUniquePtr<AssetObject> ImportedAssetObject = FinalImporter->CreateAssetObject(OpenedFileNames[0]);
					if (ImportedAssetObject)
					{
						FString SavedFileName = CurViewDirectory / FPaths::GetBaseFilename(OpenedFileNames[0]) + "." + ImportedAssetObject->FileExtension();
						TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*SavedFileName));
						ImportedAssetObject->Serialize(*Ar);
					}
					else
					{
						MessageDialog::Open("Failed to import asset.");
					}
				}
			}
		}
	}

	void SAssetView::OnMouseButtonDoubleClick(TSharedRef<AssetViewItem> ViewItem)
	{
		if (ViewItem->IsOfType<AssetViewFolderItem>())
		{
			TSharedRef<AssetViewFolderItem> FolderViewItem = StaticCastSharedRef<AssetViewFolderItem>(ViewItem);
			if (OnFolderDoubleClick)
			{
				OnFolderDoubleClick(FolderViewItem->GetFolderPath());
			}
		}
	}

}