#include "CommonHeader.h"
#include "SAssetBrowser.h"
#include "SAssetView.h"
#include "SDirectoryTree.h"
#include <DirectoryWatcher/DirectoryWatcherModule.h>
#include <DirectoryWatcher/IDirectoryWatcher.h>

namespace FRAMEWORK
{

	SAssetBrowser::~SAssetBrowser()
	{
		if (FDirectoryWatcherModule* Module = FModuleManager::GetModulePtr<FDirectoryWatcherModule>(TEXT("DirectoryWatcher")))
		{
			if (IDirectoryWatcher* DirectoryWatcher = Module->Get())
			{
				DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(FPaths::GetPath(ContentPathShowed), DirectoryWatcherHandle);
			}
		}
	}

	void SAssetBrowser::Construct(const FArguments& InArgs)
	{
		ContentPathShowed = InArgs._ContentPathShowed;
		OnDirectoryChanged = InArgs._OnDirectoryChanged;
		SelectedDirectory = InArgs._InitialDirectory;

		FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
		IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

		DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(FPaths::GetPath(ContentPathShowed),
			IDirectoryWatcher::FDirectoryChanged::CreateRaw(this, &SAssetBrowser::OnFileChanged),
			DirectoryWatcherHandle, IDirectoryWatcher::WatchOptions::IncludeDirectoryChanges);

		SAssignNew(AssetView, SAssetView);

		SAssignNew(DirectoryTree, SDirectoryTree)
			.ContentPathShowed(InArgs._ContentPathShowed)
			.InitialDirectory(InArgs._InitialDirectory)
			.OnSelectionChanged_Raw(this, &SAssetBrowser::OnDirectoryTreeSelectionChanged);

		ChildSlot
		[
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(2.0f)
			+ SSplitter::Slot()
			.Value(0.25f)
			[
				SNew(SBorder)
				[
					DirectoryTree.ToSharedRef()
				]
				
			]
			+ SSplitter::Slot()
			.Value(0.75f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
				[
					AssetView.ToSharedRef()
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

	void SAssetBrowser::OnDirectoryTreeSelectionChanged(const FString& SelectedDirectory)
	{
		AssetView->SetNewViewDirectory(SelectedDirectory);
		OnDirectoryChanged.ExecuteIfBound(SelectedDirectory);
	}
}
