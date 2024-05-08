#include "CommonHeader.h"
#include "SAssetBrowser.h"
#include "SAssetView.h"
#include "AssetManager/AssetManager.h"
#include <DirectoryWatcherModule.h>
#include <IDirectoryWatcher.h>
#include <Widgets/Input/SSlider.h>
#include <Widgets/Input/SSearchBox.h>

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
		OnSelectedDirectoryChanged = InArgs._OnSelectedDirectoryChanged;
		OnExpandedDirectoriesChanged = InArgs._OnExpandedDirectoriesChanged;
		OnAssetViewSizeChanged = InArgs._OnAssetViewSizeChanged;

		FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
		IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

		DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(FPaths::GetPath(ContentPathShowed),
			IDirectoryWatcher::FDirectoryChanged::CreateRaw(this, &SAssetBrowser::OnFileChanged),
			DirectoryWatcherHandle, IDirectoryWatcher::WatchOptions::IncludeDirectoryChanges);

		SAssignNew(AssetView, SAssetView)
			.ContentPathShowed(InArgs._ContentPathShowed)
			.OnFolderDoubleClick([this](const FString& FolderPath) {
				DirectoryTree->SetExpansion(FPaths::GetPath(FolderPath));
				DirectoryTree->SetSelection(FolderPath);
			});
		AssetView->SetAssetViewSize(InArgs._InitAssetViewSize);

		SAssignNew(DirectoryTree, SDirectoryTree)
			.ContentPathShowed(InArgs._ContentPathShowed)
			.InitialSelectedDirectory(InArgs._InitialSelectedDirectory)
			.InitialDirectoriesToExpand(InArgs._InitialDirectoriesToExpand)
			.OnExpandedDirectoriesChanged(OnExpandedDirectoriesChanged)
			.OnSelectedDirectoryChanged_Raw(this, &SAssetBrowser::OnDirectoryTreeSelectionChanged);

		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				[
					SNew(SBox)
					.WidthOverride(200)
					[
						SNew(SSearchBox)
					]
				
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				[
					SNew(SBox)
					.WidthOverride(100)
					[
						SNew(SSlider)
						.Orientation(Orient_Horizontal)
						.Value(InArgs._InitAssetViewSize)
						.MinValue(SAssetView::MinAssetViewSize)
						.MaxValue(SAssetView::MaxAssetViewSize)
						.OnValueChanged_Lambda([this](float NewValue) {
							AssetView->SetAssetViewSize(NewValue); 
							OnAssetViewSizeChanged.ExecuteIfBound(NewValue);
						})
					]
			
				]
			]

			+ SVerticalBox::Slot()
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
					.Padding(0)
					[
						AssetView.ToSharedRef()
					]
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
			if (!FPaths::IsUnderDirectory(FullFileName, ContentPathShowed))
			{
				continue;
			}

			if (Extension.IsEmpty())
			{
				if (FileChange.Action == FFileChangeData::FCA_Added)
				{
                    TArray<FString> FileNames;
                    IFileManager::Get().FindFilesRecursive(FileNames, *FullFileName, TEXT("*"), true, false);
                    for (const FString& FileName : FileNames)
                    {
                        if (TSingleton<AssetManager>::Get().GetManageredExts().Contains(FPaths::GetExtension(FileName)))
                        {
                            TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileReader(*FileName));
                            FGuid AssetId;
                            *Ar << AssetId;
                            TSingleton<AssetManager>::Get().UpdateGuidToPath(AssetId, FileName);
                        }
                    }
                    
					DirectoryTree->AddDirectory(FullFileName);
					AssetView->AddFolder(FullFileName);
				}
				else if (FileChange.Action == FFileChangeData::FCA_Removed)
				{
					DirectoryTree->RemoveDirectory(FullFileName);
					AssetView->RemoveFolder(FullFileName);
				}
			}
			else if(TSingleton<AssetManager>::Get().GetManageredExts().Contains(Extension))
			{
#if PLATFORM_MAC
                if (FileChange.Action == FFileChangeData::FCA_Added || FileChange.Action == FFileChangeData::FCA_Modified)
#else
                if (FileChange.Action == FFileChangeData::FCA_Added)
#endif
                {
                    TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileReader(*FullFileName));
                    FGuid AssetId;
                    *Ar << AssetId;
                    TSingleton<AssetManager>::Get().UpdateGuidToPath(AssetId, FullFileName);
                    
                    AssetView->AddFile(FullFileName);
                }
				else if (FileChange.Action == FFileChangeData::FCA_Removed)
				{
					AssetView->RemoveFile(FullFileName);
				}
			}
		}
	}

	void SAssetBrowser::OnDirectoryTreeSelectionChanged(const FString& SelectedDirectory)
	{
		AssetView->SetNewViewDirectory(SelectedDirectory);
		OnSelectedDirectoryChanged.ExecuteIfBound(SelectedDirectory);
	}
}
