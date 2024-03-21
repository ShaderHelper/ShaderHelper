#include "CommonHeader.h"
#include "SAssetView.h"
#include <DirectoryWatcherModule.h>
#include <IDirectoryWatcher.h>
#include <DesktopPlatformModule.h>
#include "Common/Util/Reflection.h"
#include "AssetManager/AssetImporter/AssetImporter.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "AssetManager/AssetManager.h"
#include "UI/Widgets/AssetBrowser/AssetViewItem/AssetViewFolderItem.h"
#include "UI/Widgets/AssetBrowser/AssetViewItem/AssetViewAssetItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Framework/Commands/GenericCommands.h>

namespace FRAMEWORK
{

	void SAssetView::Construct(const FArguments& InArgs)
	{
		UICommandList = MakeShared<FUICommandList>();
		UICommandList->MapAction(
			FGenericCommands::Get().Delete,
			FExecuteAction::CreateRaw(this, &SAssetView::OnHandleDeleteAction)
		);
		UICommandList->MapAction(
			FGenericCommands::Get().Rename,
			FExecuteAction::CreateRaw(this, &SAssetView::OnHandleRenameAction)
		);

		ContentPathShowed = InArgs._ContentPathShowed;
		OnFolderDoubleClick = InArgs._OnFolderDoubleClick;

		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SAssignNew(AssetTileView, STileView<TSharedRef<AssetViewItem>>)
					.SelectionMode(ESelectionMode::Single)
					.ListItemsSource(&AssetViewItems)
					.ItemWidth(58)
					.ItemHeight(58)
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
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				[
					SNew(STextBlock)
					.Font(FAppStyle::Get().GetFontStyle("SmallFont"))
					.Text_Lambda([this] {
						TArray<TSharedRef<AssetViewItem>> SelectedItems = AssetTileView->GetSelectedItems();
						if (SelectedItems.Num() > 0)
						{
							FString RelativeName = SelectedItems[0]->GetPath();
							FPaths::MakePathRelativeTo(RelativeName, *ContentPathShowed);
							return FText::FromString(MoveTemp(RelativeName));
						}
						else
						{
							FString RelativeName = CurViewDirectory;
							FPaths::MakePathRelativeTo(RelativeName, *ContentPathShowed);
							return FText::FromString(MoveTemp(RelativeName));
						}
					})
				]
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
            FString Ext = FPaths::GetExtension(FileOrFolderName);
			if (Ext.IsEmpty())
			{
				AssetViewItems.Add(MakeShared<AssetViewFolderItem>(ViewDirectory / FileOrFolderName));
			}
			else if(TSingleton<AssetManager>::Get().GetManageredExts().Contains(Ext))
			{
				AssetViewItems.Add(MakeShared<AssetViewAssetItem>(ViewDirectory / FileOrFolderName));
			}
		}

		SortViewItems();
		AssetTileView->RequestListRefresh();
	}

	TSharedPtr<SWidget> SAssetView::CreateContextMenu()
	{
		TArray<TSharedRef<AssetViewItem>> SelectedItems = AssetTileView->GetSelectedItems();
		if (SelectedItems.Num() > 0)
		{
			return CreateItemContextMenu(SelectedItems[0]);
		}

		FMenuBuilder MenuBuilder{ true, TSharedPtr<FUICommandList>() };
		MenuBuilder.AddMenuEntry(
			FText::FromString("Show in Explorer"),
			FText::GetEmpty(),
			FSlateIcon{},
			FUIAction{ FExecuteAction::CreateLambda([this] {
				FPlatformProcess::ExploreFolder(*CurViewDirectory);
			}) });

		MenuBuilder.BeginSection("Asset", FText::FromString("Asset"));
		{
			MenuBuilder.AddMenuEntry(
				FText::FromString("Import"),
				FText::GetEmpty(),
				FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "Icons.Import" },
				FUIAction{ FExecuteAction::CreateRaw(this, &SAssetView::ImportAsset) });
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("Folder", FText::FromString("Folder"));
		{
			MenuBuilder.AddMenuEntry(
				FText::FromString("New Folder"),
				FText::GetEmpty(),
				FSlateIcon{ FAppCommonStyle::Get().GetStyleSetName(), "Icons.FolderPlus" },
				FUIAction{ FExecuteAction::CreateLambda([this] {
					int32 Number = 0;
					FString NewDirectoryPath = *(CurViewDirectory / TEXT("New Folder"));
					while(IFileManager::Get().DirectoryExists(*NewDirectoryPath))
					{
						NewDirectoryPath = *(CurViewDirectory / FString::Format(TEXT("New Folder {0}"), { Number++ }));
					}
					IFileManager::Get().MakeDirectory(*NewDirectoryPath);
				})});
		}
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}

	TSharedPtr<SWidget> SAssetView::CreateItemContextMenu(TSharedRef<AssetViewItem> ViewItem)
	{
		FMenuBuilder MenuBuilder{ true, UICommandList };
		MenuBuilder.AddMenuEntry(
			FText::FromString("Show in Explorer"),
			FText::GetEmpty(),
			FSlateIcon{},
			FUIAction{ FExecuteAction::CreateLambda([ViewItem] {
				FPlatformProcess::ExploreFolder(*ViewItem->GetPath());
			}) });
		MenuBuilder.BeginSection("Control", FText::FromString("Control"));
		{
			MenuBuilder.AddMenuEntry(
				FText::FromString("Open"),
				FText::GetEmpty(),
				FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "Icons.FolderOpen" },
				FUIAction{ FExecuteAction::CreateRaw(this, &SAssetView::OnHandleOpenAction) });
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Rename);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);
		}
		MenuBuilder.EndSection();
		return MenuBuilder.MakeWidget();
	}

	void SAssetView::AddFolder(const FString& InFolderName)
	{
		if (InFolderName == CurViewDirectory || FPaths::GetPath(InFolderName) != CurViewDirectory)
		{
			return;
		}

		TSharedRef<AssetViewFolderItem> NewFolderItem = MakeShared<AssetViewFolderItem>(InFolderName);
		AssetViewItems.Add(MoveTemp(NewFolderItem));
		SortViewItems();
		AssetTileView->RequestListRefresh();
	}

	void SAssetView::RemoveFolder(const FString& InFolderName)
	{
		if (InFolderName == CurViewDirectory)
		{
			if (OnFolderDoubleClick)
			{
				OnFolderDoubleClick(FPaths::GetPath(InFolderName));
			}
			return;
		}

		if (FPaths::GetPath(InFolderName) != CurViewDirectory)
		{
			return;
		}

		AssetViewItems.RemoveAll([&InFolderName](const TSharedRef<AssetViewItem>& Element) {
			return Element->GetPath() == InFolderName;
		});
		AssetTileView->RequestListRefresh();
	}

	void SAssetView::AddFile(const FString& InFileName)
	{
		if (FPaths::GetPath(InFileName) != CurViewDirectory)
		{
			return;
		}

		TSharedRef<AssetViewAssetItem> NewAssetItem = MakeShared<AssetViewAssetItem>(InFileName);
		AssetViewItems.Add(MoveTemp(NewAssetItem));
		SortViewItems();
		AssetTileView->RequestListRefresh();
	}

	void SAssetView::RemoveFile(const FString& InFileName)
	{
		if (FPaths::GetPath(InFileName) != CurViewDirectory)
		{
			return;
		}

		AssetViewItems.RemoveAll([&InFileName](const TSharedRef<AssetViewItem>& Element) {
			return Element->GetPath() == InFileName;
		});
		SortViewItems();
		AssetTileView->RequestListRefresh();
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
			
			FString DialogType = "All files ({0})|";
            FString CanImportExts;
			for (const FString& FileExt : FileExts)
			{
                CanImportExts += FString::Format(TEXT("*.{0};"), {FileExt});
			}
            DialogType = FString::Format(*DialogType, {CanImportExts}) + CanImportExts;

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
						MessageDialog::Open(MessageDialog::Ok, "Failed to import asset.");
					}
				}
			}
		}
	}

	void SAssetView::SortViewItems()
	{
		AssetViewItems.Sort([](const TSharedRef<AssetViewItem>& A, const TSharedRef<AssetViewItem>& B) {
			if (A->IsOfType<AssetViewFolderItem>() && B->IsOfType<AssetViewAssetItem>())
			{
				return true;
			}
			else if (A->IsOfType<AssetViewAssetItem>() && B->IsOfType<AssetViewFolderItem>())
			{
				return false;
			}

			return A->GetPath() < B->GetPath();
		});
	}

	FReply SAssetView::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
	{
		if (UICommandList.IsValid() && UICommandList->ProcessCommandBindings(InKeyEvent))
		{
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	void SAssetView::OnMouseButtonDoubleClick(TSharedRef<AssetViewItem> ViewItem)
	{
		if (ViewItem->IsOfType<AssetViewFolderItem>())
		{
			if (OnFolderDoubleClick)
			{
				OnFolderDoubleClick(ViewItem->GetPath());
			}
		}
	}

	void SAssetView::OnHandleDeleteAction()
	{
		if (MessageDialog::Open(MessageDialog::OkCancel, "Delete selected Item?"))
		{
			TArray<TSharedRef<AssetViewItem>> SelectedItems = AssetTileView->GetSelectedItems();
			if (SelectedItems[0]->IsOfType<AssetViewFolderItem>())
			{
				IFileManager::Get().DeleteDirectory(*SelectedItems[0]->GetPath(), false, true);
			}
			else if (SelectedItems[0]->IsOfType<AssetViewAssetItem>())
			{
				IFileManager::Get().Delete(*SelectedItems[0]->GetPath());
			}
		}
	
	}

	void SAssetView::OnHandleRenameAction()
	{
		TArray<TSharedRef<AssetViewItem>> SelectedItems = AssetTileView->GetSelectedItems();
		if (SelectedItems[0]->IsOfType<AssetViewFolderItem>())
		{
			TSharedRef<AssetViewFolderItem> SelectedFolderItem = StaticCastSharedRef<AssetViewFolderItem>(SelectedItems[0]);
			SelectedFolderItem->EnterRenameState();
		}
        else if (SelectedItems[0]->IsOfType<AssetViewAssetItem>())
        {
            TSharedRef<AssetViewAssetItem> SelectedAssetItem = StaticCastSharedRef<AssetViewAssetItem>(SelectedItems[0]);
            SelectedAssetItem->EnterRenameState();
        }
	}

	void SAssetView::OnHandleOpenAction()
	{
		TArray<TSharedRef<AssetViewItem>> SelectedItems = AssetTileView->GetSelectedItems();
		OnMouseButtonDoubleClick(SelectedItems[0]);
	}

}
