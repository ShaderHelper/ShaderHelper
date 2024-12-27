#pragma once
#include "UI/Widgets/Misc/SIconButton.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "Common/Path/PathHelper.h"
#include <DesktopPlatformModule.h>
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "ProjectManager/ProjectManager.h"
#include "Editor.h"

namespace FRAMEWORK
{
	template<typename T>
	class ProjectLauncher : Editor
	{
	public:
		ProjectLauncher(TFunction<void()> InLaunchProjectFunc) : LaunchProjectFunc(MoveTemp(InLaunchProjectFunc))
		{
			AddProjectAssociation();

			const float space = 4.0f;
			TSharedRef<SVerticalBox> LeftContent =
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(0.0f, 0.0f, 0.0f, space)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCALIZATION("ProjectName"))
				]
				+ SVerticalBox::Slot()
				.Padding(0.0f, 0.0f, 0.0f, space)
				.VAlign(VAlign_Center)
				[
					SNew(SEditableTextBox).Text(FText::FromString(ProjectName))
					.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
						ProjectName = NewText.ToString();
					})
				]
				+ SVerticalBox::Slot()
				.Padding(0.0f, 0.0f, 0.0f, space)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(STextBlock).Text(LOCALIZATION("ProjectLocation"))
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SIconButton).Icon(FAppStyle::Get().GetBrush("Icons.Search"))
						.OnClicked_Lambda([this] {
							IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
							FString OutDir;
							void* WindowHandle = (Window.IsValid() && Window->GetNativeWindow().IsValid()) ? Window->GetNativeWindow()->GetOSWindowHandle() : nullptr;
							DesktopPlatform->OpenDirectoryDialog(WindowHandle, "Select Dir", CacheSelectDir, OutDir);
							if (!OutDir.IsEmpty())
							{
								CacheSelectDir = OutDir;
								ProjectDir = OutDir;
							}
							return FReply::Handled();
						})
					]
				]
				+ SVerticalBox::Slot()
				.Padding(0.0f, 0.0f, 0.0f, space)
				[
					SNew(SEditableTextBox).Text_Lambda([this] { return FText::FromString(ProjectDir); })
					.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
						ProjectDir = NewText.ToString();
					})
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Right)
				[
					SNew(SIconButton)
						.Icon(FAppCommonStyle::Get().GetBrush("Icons.File"))
						.Label(LOCALIZATION("New"))
						.OnClicked_Lambda([this] {
							FString NewProjectPath = ProjectDir / ProjectName;
							if (IFileManager::Get().DirectoryExists(*NewProjectPath))
							{
								MessageDialog::Open(MessageDialog::Ok, FText::FromString("The project path already exists."));
							}
							else
							{
								TSingleton<ProjectManager<T>>::Get().NewProject(ProjectName, ProjectDir);
								LaunchProjectFunc();
								Window->RequestDestroyWindow();
							}
							return FReply::Handled();
						})
				];
			

			RightContent =
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(0.0f, 0.0f, 0.0f, space)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCALIZATION("RecentProjects"))
				];

			auto InsertDummy = [this](int32 SlotIndex = -1) {
				RightContent->InsertSlot(SlotIndex)
					.Padding(0.0f, 0.0f, 0.0f, space)
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SNew(SIconButton)
							.Visibility(EVisibility::Hidden)
							.Icon(FAppStyle::Get().GetBrush("AppIcon.Small"))
							.IconSize(FVector2D{ 16,16 })

					];
			};

			const TArray<FString>& RecentProjcetPaths = TSingleton<ProjectManager<T>>::Get().GetRecentProjcetPaths();
			for (int i = 0; i < 3; i++)
			{
				if (RecentProjcetPaths.IsValidIndex(i))
				{
					FString RecentProjcetPath = RecentProjcetPaths[i];
					TSharedRef<SIconButton> RecentProjcetButton = SNew(SIconButton)
						.Label(FText::FromString(FPaths::GetCleanFilename(RecentProjcetPath)))
						.ToolTipText(FText::FromString(RecentProjcetPath))
						.Icon(FAppStyle::Get().GetBrush("AppIcon.Small"))
						.IconSize(FVector2D{ 16,16 });

					RecentProjcetButton->SetOnClicked(FOnClicked::CreateLambda(
						[RecentProjcetButton = TWeakPtr<SIconButton>{ RecentProjcetButton }, RecentProjcetPath, InsertDummy, this] {
							if (IFileManager::Get().FileExists(*RecentProjcetPath))
							{
								TSingleton<ProjectManager<T>>::Get().OpenProject(RecentProjcetPath);
								LaunchProjectFunc();
								Window->RequestDestroyWindow();
							}
							else
							{
								MessageDialog::Open(MessageDialog::Ok, FText::FromString("The project file does not exist."));
								TSingleton<ProjectManager<T>>::Get().RemoveFromProjMgmt(RecentProjcetPath);
								InsertDummy(RightContent->NumSlots() - 1);
								RightContent->RemoveSlot(RecentProjcetButton.Pin().ToSharedRef());
							}
							return FReply::Handled();
						}));
					RightContent->AddSlot()
						.Padding(0.0f, 0.0f, 0.0f, space)
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						[
							RecentProjcetButton
						];
				}
				else
				{
					InsertDummy();
				}
			}

			RightContent->AddSlot()
				.HAlign(HAlign_Left)
				[
					SNew(SIconButton)
						.Icon(FAppCommonStyle::Get().GetBrush("Icons.Folder"))
						.Label(LOCALIZATION("OpenEx"))
						.OnClicked_Lambda([this] {
							IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
							FString DialogType = "Project(*.shprj)|*.shprj";
							TArray<FString> OpenedFileNames;
							void* WindowHandle = (Window.IsValid() && Window->GetNativeWindow().IsValid()) ? Window->GetNativeWindow()->GetOSWindowHandle() : nullptr;
							if (DesktopPlatform->OpenFileDialog(WindowHandle, "Open Project", CacheOpenProjectDir, "", MoveTemp(DialogType), EFileDialogFlags::None, OpenedFileNames))
							{
								if (OpenedFileNames.Num() > 0)
								{
									CacheOpenProjectDir = FPaths::GetPath(OpenedFileNames[0]);
									TSingleton<ProjectManager<T>>::Get().OpenProject(FPaths::ConvertRelativePathToFull(OpenedFileNames[0]));
									LaunchProjectFunc();
									Window->RequestDestroyWindow();
								}
							}
							return FReply::Handled();
						})
				];


			SAssignNew(Window, SWindow)
				.Title(FText::FromString("Launcher"))
				.CreateTitleBar(false)
				.SizingRule(ESizingRule::Autosized)
				.bDragAnywhere(true)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SOverlay)
					+SOverlay::Slot()
					[
						SNew(SImage)
							.Image(FAppCommonStyle::Get().GetBrush("ProjectLauncher.Background"))
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Bottom)
					[
						SNew(SImage)
							.Image(FAppCommonStyle::Get().GetBrush("ProjectLauncher.Logo"))
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Top)
					[
						SNew(SButton)
							.ButtonStyle(&FAppCommonStyle::Get().GetWidgetStyle< FButtonStyle >("CloseButton"))
							.OnClicked_Lambda([this] {
								Window->RequestDestroyWindow();
								return FReply::Handled();
							})
					]
				]
				+ SVerticalBox::Slot()
				[
					SNew(SBorder)
					.Padding(FMargin{ 5.0f })
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.FillWidth(0.7f)
						[
							LeftContent
						]
						+SHorizontalBox::Slot()
						.FillWidth(0.3f)
						.Padding(25.0f,0,0,0)
						[

							RightContent.ToSharedRef()
						]
					]
				]
			];
			FSlateApplication::Get().AddWindow(Window.ToSharedRef());
		}

	~ProjectLauncher()
	{
		if (Window.IsValid())
		{
			FSlateApplication::Get().DestroyWindowImmediately(Window.ToSharedRef());
		}
	}

	private:
		TFunction<void()> LaunchProjectFunc;
		TSharedPtr<SWindow> Window;
		TSharedPtr<SVerticalBox> RightContent;
		FString ProjectName = "Project1";
		FString ProjectDir = PathHelper::WorkspaceDir() / "Projects";
		FString CacheSelectDir = PathHelper::WorkspaceDir();
		FString CacheOpenProjectDir = PathHelper::WorkspaceDir();
	};
}