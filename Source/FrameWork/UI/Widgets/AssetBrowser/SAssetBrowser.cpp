#include "CommonHeader.h"
#include "SAssetBrowser.h"
#include "SAssetView.h"
#include "AssetManager/AssetManager.h"
#include <DirectoryWatcherModule.h>
#include <IDirectoryWatcher.h>
#include <Widgets/Input/SSlider.h>
#include <Widgets/Input/SSearchBox.h>
#include "Editor/AssetEditor/AssetEditor.h"

namespace FW
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

    void SAssetBrowser::SetCurrentDisplyPath(const FString& DisplayDir)
    {
        AssetView->SetNewViewDirectory(DisplayDir);
        DirectoryTree->SetExpansionRecursive(FPaths::GetPath(DisplayDir));
        DirectoryTree->SetSelection(DisplayDir);
    }

    void SAssetBrowser::InitDirectory(AssetBrowserDirectory& OutDirectory, const FString& Path)
    {
        OutDirectory.Path = Path;
        
        TArray<FString> FileNames;
        IFileManager::Get().FindFiles(FileNames, *(Path / TEXT("*")), true, false);
        for (const FString& FileName : FileNames)
        {
            if (TSingleton<AssetManager>::Get().GetManageredExts().Contains(FPaths::GetExtension(FileName)))
            {
                OutDirectory.Files.Add(Path / FileName);
            }
        }
        
        TArray<FString> FolderNames;
        IFileManager::Get().FindFiles(FolderNames, *(Path / TEXT("*")), false, true);
        for(const FString& FolderName : FolderNames)
        {
            InitDirectory(OutDirectory.Directories.AddDefaulted_GetRef(), Path / FolderName);
        }
    }

    AssetBrowserDirectory* SAssetBrowser::FindBrowserDirectory(AssetBrowserDirectory& CurDirectory, const FString& SearchDir)
    {
        if(CurDirectory.Path == SearchDir)
        {
            return &CurDirectory;
        }
        
        for(int32 i = 0 ; i < CurDirectory.Directories.Num(); i++)
        {
            if(CurDirectory.Directories[i].Path == SearchDir)
            {
                return &CurDirectory.Directories[i];
            }
            else if(auto* Directory = FindBrowserDirectory(CurDirectory.Directories[i], SearchDir))
            {
                return Directory;
            }
        }
        
        return nullptr;
    }

    void SAssetBrowser::AddFile(const FString& File)
    {
        AssetBrowserDirectory& ParDirectory = *FindBrowserDirectory(ContentDirectory, FPaths::GetPath(File));
        ParDirectory.Files.Add(File);
    }

    void SAssetBrowser::RemoveFile(const FString& File)
    {
        AssetBrowserDirectory& ParDirectory = *FindBrowserDirectory(ContentDirectory, FPaths::GetPath(File));
        ParDirectory.Files.Remove(File);
    }

    void SAssetBrowser::AddBrowserDirectory(const FString& DirName)
    {
        AssetBrowserDirectory NewDirectory;
        InitDirectory(NewDirectory, DirName);
        AssetBrowserDirectory& ParDirectory = *FindBrowserDirectory(ContentDirectory, FPaths::GetPath(DirName));
        ParDirectory.Directories.Add(MoveTemp(NewDirectory));
    }

    void SAssetBrowser::RemoveBrowserDirectory(const FString& DirName)
    {
        AssetBrowserDirectory& ParDirectory = *FindBrowserDirectory(ContentDirectory, FPaths::GetPath(DirName));
        ParDirectory.Directories.Remove(AssetBrowserDirectory{DirName});
    }

    TArray<FString> SAssetBrowser::FindFilesUnderBrowserDirectory(const FString& DirName)
    {
        AssetBrowserDirectory& CurDirectory = *FindBrowserDirectory(ContentDirectory, DirName);
        TArray<FString> Files = CurDirectory.Files;
        for(const AssetBrowserDirectory& Directory : CurDirectory.Directories)
        {
            Files.Append(FindFilesUnderBrowserDirectory(Directory.Path));
        }
        return Files;
    }

	void SAssetBrowser::Construct(const FArguments& InArgs)
	{
		ContentPathShowed = InArgs._ContentPathShowed;
        InitDirectory(ContentDirectory, ContentPathShowed);
        State = InArgs._State;

		FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
		IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

		DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(FPaths::GetPath(ContentPathShowed),
			IDirectoryWatcher::FDirectoryChanged::CreateRaw(this, &SAssetBrowser::OnFileChanged),
			DirectoryWatcherHandle, IDirectoryWatcher::WatchOptions::IncludeDirectoryChanges);

		SAssignNew(AssetView, SAssetView)
            .State(&State->AssetViewState)
			.ContentPathShowed(InArgs._ContentPathShowed)
			.OnFolderOpen([this](const FString& FolderPath) {
				DirectoryTree->SetExpansion(FPaths::GetPath(FolderPath));
				DirectoryTree->SetSelection(FolderPath);
			});

		SAssignNew(DirectoryTree, SDirectoryTree)
            .State(&State->DirectoryTreeState)
			.ContentPathShowed(InArgs._ContentPathShowed)
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
						.Value(State->AssetViewState.AssetViewSize)
						.MinValue(SAssetView::MinAssetViewSize)
						.MaxValue(SAssetView::MaxAssetViewSize)
						.OnValueChanged_Lambda([this](float NewValue) {
							AssetView->SetAssetViewSize(NewValue);
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
                            TSingleton<AssetManager>::Get().UpdateGuidToPath(FileName);
                            
                            if(AssetOp* AssetOp_ = GetAssetOp(FileName))
                            {
                                AssetOp_->OnAdd(FileName);
                            }
                        }
                    }
                    
					DirectoryTree->AddDirectory(FullFileName);
					AssetView->AddFolder(FullFileName);
                    AddBrowserDirectory(FullFileName);
				}
				else if (FileChange.Action == FFileChangeData::FCA_Removed)
				{
                    DirectoryTree->RemoveDirectory(FullFileName);
                    AssetView->RemoveFolder(FullFileName);
                    SetCurrentDisplyPath(FPaths::GetPath(FullFileName));
                    
                    TArray<FString> FileNames = FindFilesUnderBrowserDirectory(FullFileName);
                    for(const FString& FileName : FileNames)
                    {
                        if(AssetOp* AssetOp_ = GetAssetOp(FileName))
                        {
                            AssetOp_->OnDelete(FileName);
                        }
                        TSingleton<AssetManager>::Get().RemoveGuidToPath(FileName);
                    }
                    
                    RemoveBrowserDirectory(FullFileName);
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
                    TSingleton<AssetManager>::Get().UpdateGuidToPath(FullFileName);
                    
                    if(AssetOp* AssetOp_ = GetAssetOp(FullFileName))
                    {
                        AssetOp_->OnAdd(FullFileName);
                    }
                    
                    AssetView->AddFile(FullFileName);
                    AddFile(FullFileName);
                }
				else if (FileChange.Action == FFileChangeData::FCA_Removed)
				{
                    
					AssetView->RemoveFile(FullFileName);
                    RemoveFile(FullFileName);
                
                    if(AssetOp* AssetOp_ = GetAssetOp(FullFileName))
                    {
                        AssetOp_->OnDelete(FullFileName);
                    }
                    
                    TSingleton<AssetManager>::Get().RemoveGuidToPath(FullFileName);
				}
			}
		}
	}

	void SAssetBrowser::OnDirectoryTreeSelectionChanged(const FString& SelectedDirectory)
	{
		AssetView->SetNewViewDirectory(SelectedDirectory);
	}
}
