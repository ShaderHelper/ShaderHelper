#include "CommonHeader.h"
#include "SAssetView.h"
#include "SAssetBrowser.h"
#include "AssetManager/AssetImporter/AssetImporter.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "AssetManager/AssetManager.h"
#include "UI/Widgets/AssetBrowser/AssetViewItem/AssetViewFolderItem.h"
#include "UI/Widgets/AssetBrowser/AssetViewItem/AssetViewAssetItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "Editor/AssetEditor/AssetEditor.h"
#include "ProjectManager/ProjectManager.h"
#include "Editor/AssetViewCommands.h"
#include "App/App.h"
#include "PluginManager/PluginManager.h"

#include <DirectoryWatcherModule.h>
#include <IDirectoryWatcher.h>
#include <DesktopPlatformModule.h>

namespace FW
{

	void SAssetView::Construct(const FArguments& InArgs, SAssetBrowser* InBrowser)
	{
		Browser = InBrowser;
		UICommandList = MakeShared<FUICommandList>();
		UICommandList->MapAction(
			AssetViewCommands::Get().Delete,
			FExecuteAction::CreateRaw(this, &SAssetView::OnHandleDeleteAction)
		);
		UICommandList->MapAction(
			AssetViewCommands::Get().Rename,
			FExecuteAction::CreateRaw(this, &SAssetView::OnHandleRenameAction),
			FCanExecuteAction::CreateLambda([this] { return AssetTileView->GetSelectedItems().Num() == 1; })
		);
		UICommandList->MapAction(
			AssetViewCommands::Get().Save,
			FExecuteAction::CreateRaw(this, &SAssetView::OnHandleSaveAction)
		);

		ContentPathShowed = InArgs._ContentPathShowed;
		BuiltInDir = InArgs._BuiltInDir;
        OnFolderOpen = InArgs._OnFolderOpen;
        State = InArgs._State;

		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SAssignNew(AssetTileView, STileView<TSharedRef<AssetViewItem>>)
					.SelectionMode(ESelectionMode::Multi)
					.ListItemsSource(&AssetViewItems)
					.ItemWidth(TAttribute<float>::CreateLambda([this] { return State->AssetViewSize; }))
					.ItemHeight(TAttribute<float>::CreateLambda([this] { return State->AssetViewSize; }))
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
					.Text(LOCALIZATION("EmptyFolderTip"))
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
        if(CurViewDirectory != NewViewDirectory)
        {
            AssetViewItems.Empty();
            CacheImportAssetPath = NewViewDirectory;
            CurViewDirectory = NewViewDirectory;
            PopulateAssetView(CurViewDirectory);
        }
	}

	void SAssetView::PopulateAssetView(const FString& ViewDirectory, const FText& InFilterText)
	{
		AssetViewItems.Reset();

		TArray<FString> FileOrFolderNames;
		IFileManager::Get().FindFiles(FileOrFolderNames, *(ViewDirectory / TEXT("*")), true, true);
		for (const FString& FileOrFolderName : FileOrFolderNames)
		{
            FString Ext = FPaths::GetExtension(FileOrFolderName);
			FString Path = ViewDirectory / FileOrFolderName;
			if (Ext.IsEmpty())
			{
				if (!InFilterText.IsEmpty())
				{
					if (FileOrFolderName.Contains(InFilterText.ToString()))
					{
						AssetViewItems.Add(MakeShared<AssetViewFolderItem>(this, Path));
					}
				}
				else
				{
					AssetViewItems.Add(MakeShared<AssetViewFolderItem>(this, Path));
				}
			}
			else if(TSingleton<AssetManager>::Get().GetManageredExts().Contains(Ext))
			{
				if (!InFilterText.IsEmpty())
				{
					if (FileOrFolderName.Contains(InFilterText.ToString()))
					{
						auto Item = MakeShared<AssetViewAssetItem>(this, Path);
						SetAssetIcon(Item);
						AssetViewItems.Add(Item);
					}
				}
				else
				{
					auto Item = MakeShared<AssetViewAssetItem>(this, Path);
					SetAssetIcon(Item);
					AssetViewItems.Add(Item);
				}
			}
		}

		SortViewItems();
		AssetTileView->RequestListRefresh();
	}
    
    static bool IsImportedAsset(const MetaType* InAssetMetaType)
    {
        return GetDefaultObject<AssetImporter>([=](AssetImporter* CurImporter) { return InAssetMetaType == CurImporter->SupportAsset(); } ) != nullptr;
    }

	TSharedPtr<SWidget> SAssetView::CreateContextMenu()
	{
		if (FPaths::IsUnderDirectory(CurViewDirectory,BuiltInDir))
		{
			return SNullWidget::NullWidget;
		}

		TArray<TSharedRef<AssetViewItem>> SelectedItems = AssetTileView->GetSelectedItems();
		if (SelectedItems.Num() > 0)
		{
			return CreateItemContextMenu(SelectedItems);
		}

		FMenuBuilder MenuBuilder{ true, TSharedPtr<FUICommandList>() };
		MenuBuilder.AddMenuEntry(
			LOCALIZATION("ShowInExplorer"),
			FText::GetEmpty(),
			FSlateIcon{},
			FUIAction{ FExecuteAction::CreateLambda([this] {
				FPlatformProcess::ExploreFolder(*CurViewDirectory);
			}) });

		MenuBuilder.BeginSection("Asset", FText::FromString("Asset"));
		{
            MenuBuilder.AddSubMenu(LOCALIZATION("Create"), FText::GetEmpty(), FNewMenuDelegate::CreateLambda(
                [this](FMenuBuilder& MenuBuilder) {
                    TArray<MetaType*> AssetMetaTypes = GetMetaTypes<AssetObject>();
                    for(auto MetaTypePtr : AssetMetaTypes)
                    {
                        if(!IsImportedAsset(MetaTypePtr) && MetaTypePtr->Constructor)
                        {
							AssetObject* NewAsset = static_cast<AssetObject*>(MetaTypePtr->Construct());
							MenuBuilder.AddMenuEntry(
								LOCALIZATION(NewAsset->FileExtension()),
								FText::GetEmpty(),
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateLambda([NewAsset, this] {
									int32 Number = 1;
									FString GeneratedFileName = "New" + NewAsset->FileExtension();
									FString SavedFileName = CurViewDirectory / GeneratedFileName + "." + NewAsset->FileExtension();
									while (IFileManager::Get().FileExists(*SavedFileName))
									{
										GeneratedFileName = FString::Format(TEXT("New{0}{1}"), { NewAsset->FileExtension(), Number++ });
										SavedFileName = CurViewDirectory / GeneratedFileName + "." + NewAsset->FileExtension();
									}

									if (AssetOp* AssetOp_ = GetAssetOp(NewAsset->DynamicMetaType()))
									{
										AssetOp_->OnCreate(NewAsset);
									}

									TSharedRef<AssetViewAssetItem> NewAssetItem = MakeShared<AssetViewAssetItem>(this, SavedFileName);
									if (auto* Thumbnail = NewAsset->GetThumbnail())
									{
										NewAssetItem->SetAssetThumbnail(Thumbnail);
									}
									else
									{
										NewAssetItem->SetAssetImage(NewAsset->GetImage());
									}
									
									if (!AssetViewItems.ContainsByPredicate([&](const TSharedRef<AssetViewItem>& InItem) {
										return InItem->GetPath() == SavedFileName;
										}))
									{
										FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=](float) {
											NewAssetItem->EnterRenameState();
											return false;
										}));
										AssetViewItems.Add(MoveTemp(NewAssetItem));
										AssetTileView->RequestListRefresh();
									}

									TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*SavedFileName));
									NewAsset->ObjectName = FText::FromString(FPaths::GetBaseFilename(SavedFileName));
									NewAsset->Serialize(*Ar);
								})),
								NAME_None,
								EUserInterfaceActionType::Button
							);
                        }
                    }
                }),
               false, FSlateIcon{FAppStyle::Get().GetStyleSetName(), "Icons.Plus"});
            
			MenuBuilder.AddMenuEntry(
				LOCALIZATION("Import"),
				FText::GetEmpty(),
				FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "Icons.Import" },
				FUIAction{ FExecuteAction::CreateRaw(this, &SAssetView::ImportAsset) }
            );
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("Folder", FText::FromString("Folder"));
		{
			MenuBuilder.AddMenuEntry(
				LOCALIZATION("NewFolder"),
				FText::GetEmpty(),
				FSlateIcon{ FAppCommonStyle::Get().GetStyleSetName(), "Icons.FolderPlus" },
				FUIAction{ FExecuteAction::CreateLambda([this] {
					int32 Number = 1;
					FString NewDirectoryPath = *(CurViewDirectory / TEXT("NewFolder"));
					while(IFileManager::Get().DirectoryExists(*NewDirectoryPath))
					{
						NewDirectoryPath = *(CurViewDirectory / FString::Format(TEXT("NewFolder {0}"), { Number++ }));
					}

					if (!AssetViewItems.ContainsByPredicate([&](const TSharedRef<AssetViewItem>& InItem) {
						return InItem->GetPath() == NewDirectoryPath;
					}))
					{
						TSharedRef<AssetViewFolderItem> NewFolderItem = MakeShared<AssetViewFolderItem>(this, NewDirectoryPath);
						// Setting the focus directly here will not take effect.
						// The widget related to this item will only be added to STileView at the next tick
						FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, NewFolderItem](float) {
							NewFolderItem->EnterRenameState();
							NewFolderItem->OnExitRenameState = [this] {
								Browser->GetAssetSearchBox()->SetText(FText::GetEmpty());
							};
							return false;
						}));
						AssetViewItems.Add(MoveTemp(NewFolderItem));
						AssetTileView->RequestListRefresh();
					}

					IFileManager::Get().MakeDirectory(*NewDirectoryPath);
				})});
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("Other", FText::FromString("Other"));
		for (MenuEntryExt* Ext : MenuEntryExts)
		{
			if (Ext->TargetMenu == "Asset")
			{
				MenuBuilder.AddMenuEntry(FText::FromString(Ext->Label), FText::GetEmpty(), FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateLambda([Ext] { Ext->OnExecute();  }),
						FCanExecuteAction::CreateLambda([Ext] { return Ext->CanExecute(); })
				));
			}
		}
		MenuBuilder.EndSection();
		return MenuBuilder.MakeWidget();
	}

	TSharedPtr<SWidget> SAssetView::CreateItemContextMenu(const TArray<TSharedRef<AssetViewItem>>& ViewItems)
	{
		FMenuBuilder MenuBuilder{ true, UICommandList };
		MenuBuilder.AddMenuEntry(
			LOCALIZATION("ShowInExplorer"),
			FText::GetEmpty(),
			FSlateIcon{},
			FUIAction{ 
				FExecuteAction::CreateLambda([ViewItems] {
					FPlatformProcess::ExploreFolder(*ViewItems[0]->GetPath());
				}),
				FCanExecuteAction::CreateLambda([ViewItems] { return ViewItems.Num() == 1; })
			});
		MenuBuilder.BeginSection("Control", FText::FromString("Control"));
		{
			MenuBuilder.AddMenuEntry(
				LOCALIZATION("Open"),
				FText::GetEmpty(),
				FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "Icons.FolderOpen" },
				FUIAction{ 
					FExecuteAction::CreateRaw(this, &SAssetView::OnHandleOpenAction, ViewItems[0]),
					FCanExecuteAction::CreateLambda([ViewItems] { return ViewItems.Num() == 1; })
				}
			); 
			MenuBuilder.AddMenuEntry(AssetViewCommands::Get().Save, NAME_None, {}, {},
				FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "Icons.Save" });
			MenuBuilder.AddMenuEntry(AssetViewCommands::Get().Rename, NAME_None, {}, {},
				FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "GenericCommands.Rename" });
			MenuBuilder.AddMenuEntry(AssetViewCommands::Get().Delete, NAME_None, {}, {},
				FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "GenericCommands.Delete" });
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

        if(!AssetViewItems.ContainsByPredicate([&](const TSharedRef<AssetViewItem>& InItem){
            return InItem->GetPath() == InFolderName;
        }))
        {
			TSharedRef<AssetViewFolderItem> NewFolderItem = MakeShared<AssetViewFolderItem>(this, InFolderName);
            AssetViewItems.Add(MoveTemp(NewFolderItem));
            SortViewItems();
            AssetTileView->RequestListRefresh();
        }
	}

	void SAssetView::RemoveFolder(const FString& InFolderName)
	{
		if (InFolderName == CurViewDirectory)
		{
			if (OnFolderOpen)
			{
                OnFolderOpen(FPaths::GetPath(InFolderName));
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

        if(!AssetViewItems.ContainsByPredicate([&](const TSharedRef<AssetViewItem>& InItem){
            return InItem->GetPath() == InFileName;
        }))
        {
			TSharedRef<AssetViewAssetItem> NewAssetItem = MakeShared<AssetViewAssetItem>(this, InFileName);
			SetAssetIcon(NewAssetItem);
            AssetViewItems.Add(MoveTemp(NewAssetItem));
            SortViewItems();
            AssetTileView->RequestListRefresh();
        }
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

		TArray<FString> FileExts;
        TArray<AssetImporter*> AssetImporters;
        ForEachDefaultObject<AssetImporter>([&](AssetImporter* CurImporter){
            FileExts.Append(CurImporter->SupportFileExts());
            AssetImporters.Add(CurImporter);
        });
			
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
		if (DesktopPlatform->OpenFileDialog(ParentWindowHandle, "Import Asset", CacheImportAssetPath, "", MoveTemp(DialogType), EFileDialogFlags::Multiple, OpenedFileNames))
		{
			for(const auto& OpenedFileName : OpenedFileNames)
			{
				CacheImportAssetPath = FPaths::GetPath(OpenedFileName);
				AssetImporter* FinalImporter = nullptr;
				FString OpenedFileExt = FPaths::GetExtension(OpenedFileName);
				for (AssetImporter* AssetImporterPtr : AssetImporters)
				{
					if(AssetImporterPtr->SupportFileExts().Contains(OpenedFileExt))
					{
						FinalImporter = AssetImporterPtr;
						break;
					}
				}

				check(FinalImporter);
				TUniquePtr<AssetObject> ImportedAssetObject = FinalImporter->CreateAssetObject(OpenedFileName);
				if (ImportedAssetObject)
				{
					FString SavedFilePath = CurViewDirectory / FPaths::GetBaseFilename(OpenedFileName) + "." + ImportedAssetObject->FileExtension();
					TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*SavedFilePath));
					ImportedAssetObject->ObjectName = FText::FromString(FPaths::GetBaseFilename(SavedFilePath));
					ImportedAssetObject->Serialize(*Ar);
				}
				else
				{
					MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, GApp->GetEditor()->GetMainWindow(), LOCALIZATION("AssetImportFailure"));
					break;
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

	FReply SAssetView::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		if (auto ExternalDragOp = DragDropEvent.GetOperationAs<FExternalDragOperation>())
		{
			for(const FString& File : ExternalDragOp->GetFiles())
			{
				if (GetAssetImporter(File))
				{
					return FReply::Handled();
				}
			}
		}
		return FReply::Unhandled();
	}

	FReply SAssetView::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		if (auto ExternalDragOp = DragDropEvent.GetOperationAs<FExternalDragOperation>())
		{
			for (const FString& File : ExternalDragOp->GetFiles())
			{
				if (AssetImporter* Importer = GetAssetImporter(File))
				{
					TUniquePtr<AssetObject> ImportedAssetObject = Importer->CreateAssetObject(File);
					if (ImportedAssetObject)
					{
						FString SavedFilePath = CurViewDirectory / FPaths::GetBaseFilename(File) + "." + ImportedAssetObject->FileExtension();
						TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*SavedFilePath));
						ImportedAssetObject->ObjectName = FText::FromString(FPaths::GetBaseFilename(SavedFilePath));
						ImportedAssetObject->Serialize(*Ar);
					}
				}
			}
			return FReply::Handled();
		}
		return FReply::Unhandled();
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
        OnHandleOpenAction(MoveTemp(ViewItem));
	}

	void SAssetView::OnHandleDeleteAction()
	{
		TArray<TSharedRef<AssetViewItem>> SelectedItems = AssetTileView->GetSelectedItems();
		if (SelectedItems.IsEmpty())
		{
			return;
		}

		if (FPaths::IsUnderDirectory(SelectedItems[0]->GetPath(), BuiltInDir))
		{
			return;
		}

		auto Ret = MessageDialog::Open(MessageDialog::OkCancel, MessageDialog::Shocked, GApp->GetEditor()->GetMainWindow(), LOCALIZATION("DeleteItemTip"));
		if (Ret == MessageDialog::MessageRet::Ok)
		{
			for (const auto& Item : SelectedItems)
			{
				if (Item->IsOfType<AssetViewFolderItem>())
				{
					IFileManager::Get().DeleteDirectory(*Item->GetPath(), false, true);
				}
				else if (Item->IsOfType<AssetViewAssetItem>())
				{
					IFileManager::Get().Delete(*Item->GetPath());
				}
			}
			
		}
	
	}

	void SAssetView::OnHandleRenameAction()
	{
		TArray<TSharedRef<AssetViewItem>> SelectedItems = AssetTileView->GetSelectedItems();
		if (SelectedItems.IsEmpty())
		{
			return;
		}

		if (FPaths::IsUnderDirectory(SelectedItems[0]->GetPath(), BuiltInDir))
		{
			return;
		}

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

	void SAssetView::OnHandleSaveAction()
	{
		TArray<TSharedRef<AssetViewItem>> SelectedItems = AssetTileView->GetSelectedItems();
		if (SelectedItems.IsEmpty())
		{
			return;
		}

		if (FPaths::IsUnderDirectory(SelectedItems[0]->GetPath(), BuiltInDir))
		{
			return;
		}

		for (const auto& Item : SelectedItems)
		{
			if (Item->IsOfType<AssetViewAssetItem>())
			{
				TSharedRef<AssetViewAssetItem> SelectedAssetItem = StaticCastSharedRef<AssetViewAssetItem>(Item);
				FGuid AssetId = TSingleton<AssetManager>::Get().GetGuid(SelectedAssetItem->GetPath());
				AssetObject* AssetObj = TSingleton<AssetManager>::Get().FindLoadedAsset(AssetId);
				if (AssetObj && AssetObj->IsDirty())
				{
					AssetObj->Save();
				}
			}
		}
		
	}

	void SAssetView::OnHandleOpenAction(TSharedRef<AssetViewItem> ViewItem)
	{
        if (ViewItem->IsOfType<AssetViewFolderItem>())
        {
            if (OnFolderOpen)
            {
                OnFolderOpen(ViewItem->GetPath());
            }
        }
        else if(ViewItem->IsOfType<AssetViewAssetItem>())
        {
            if(AssetOp* AssetOp_ = GetAssetOp(ViewItem->GetPath()))
            {
                AssetOp_->OnOpen(ViewItem->GetPath());
            }
            
        }
	}

	void SAssetView::SetSelectedItem(const FString& InItem)
	{
		for (auto Item : AssetViewItems)
		{
			if (InItem == Item->GetPath())
			{
				AssetTileView->ClearSelection();
				AssetTileView->SetSelection(Item);
				break;
			}
		}
	}

	void SAssetView::SetAssetIcon(TSharedRef<AssetViewAssetItem> ViewItem)
	{
		TOptional<FGuid> Id = TSingleton<AssetManager>::Get().TryGetGuid(ViewItem->GetPath());
		if (Id; auto* Thumbnail = TSingleton<AssetManager>::Get().FindAssetThumbnail(*Id))
		{
			ViewItem->SetAssetThumbnail(Thumbnail);
		}
		else
		{
			TSingleton<AssetManager>::Get().AsyncLoadAssetByPath<AssetObject>(ViewItem->GetPath(), [WeakItem = TWeakPtr<AssetViewAssetItem>{ ViewItem }](AssetPtr<AssetObject> Asset) {
				if (WeakItem.IsValid())
				{
					if (auto* Thumbnail = Asset->GetThumbnail())
					{
						WeakItem.Pin()->SetAssetThumbnail(Thumbnail);
					}
					else
					{
						WeakItem.Pin()->SetAssetImage(Asset->GetImage());
					}
				}
			});
		}
	}

}
