#include "CommonHeader.h"
#include "SAssetBrowser.h"
#include "SAssetView.h"
#include "SDirectoryTree.h"
#include <DirectoryWatcher/DirectoryWatcherModule.h>
#include <DirectoryWatcher/IDirectoryWatcher.h>
#include <DesktopPlatform/DesktopPlatformModule.h>

namespace FRAMEWORK
{

	SAssetBrowser::~SAssetBrowser()
	{
		if (FDirectoryWatcherModule* Module = FModuleManager::GetModulePtr<FDirectoryWatcherModule>(TEXT("DirectoryWatcher")))
		{
			if (IDirectoryWatcher* DirectoryWatcher = Module->Get())
			{
				DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(FPaths::GetPath(DirectoryShowed), DirectoryWatcherHandle);
			}
		}
	}

	void SAssetBrowser::Construct(const FArguments& InArgs)
	{
		DirectoryShowed = InArgs._DirectoryShowed;

		FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
		IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

		DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(FPaths::GetPath(DirectoryShowed),
			IDirectoryWatcher::FDirectoryChanged::CreateRaw(this, &SAssetBrowser::OnFileChanged),
			DirectoryWatcherHandle, IDirectoryWatcher::WatchOptions::IncludeDirectoryChanges);

		ChildSlot
		[
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(2.0f)
			+ SSplitter::Slot()
			.Value(0.25f)
			[
				SNew(SBorder)
				[
					SAssignNew(DirectoryTree, SDirectoryTree)
					.DirectoryShowed(InArgs._DirectoryShowed)
				]
				
			]
			+ SSplitter::Slot()
			.Value(0.75f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
				[
					SAssignNew(AssetView, SAssetView)
					.OnContextMenuOpening(this, &SAssetBrowser::CreateContextMenu)
				]
			]
		];
	}

	void SAssetBrowser::OnFileChanged(const TArray<FFileChangeData>& InFileChanges)
	{
		for (const FFileChangeData& FileChange : InFileChanges)
		{
			FString Extension = FPaths::GetExtension(FileChange.Filename);
			FString FullFileName = FPaths::ConvertRelativePathToFull(FileChange.Filename);
			if (Extension.IsEmpty())
			{
				if (FileChange.Action == FFileChangeData::FCA_Added)
				{
					DirectoryTree->AddDirectory(FullFileName);
				}
				else if (FileChange.Action == FFileChangeData::FCA_Removed)
				{
					DirectoryTree->RemoveDirectory(FullFileName);
				}
			}
			else
			{

			}
		}
	}

	TSharedPtr<SWidget> SAssetBrowser::CreateContextMenu()
	{
		FMenuBuilder MenuBuilder{ true, TSharedPtr<FUICommandList>() };

		MenuBuilder.BeginSection("NewAsset", FText::FromString("New Asset"));
		{
			MenuBuilder.AddMenuEntry(
				FText::FromString("Import"),
				FText::GetEmpty(),
				FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "Icons.Import" },
				FUIAction{ FExecuteAction::CreateRaw(this, &SAssetBrowser::ImportAsset) });
		}
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}

	void SAssetBrowser::ImportAsset()
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (DesktopPlatform)
		{
			TArray<FString> OpenedFileNames;
			//DesktopPlatform->OpenFileDialog(nullptr, "Import Asset", )
		}
	}

}
