#pragma once
#include "AssetManager/AssetManager.h"
#include <Serialization/JsonSerializer.h>
#include "Common/Path/PathHelper.h"
#include <Misc/FileHelper.h>
namespace FW
{
	FRAMEWORK_API extern int GProjectVer;
	FRAMEWORK_API extern class Project* GProject;

	class FRAMEWORK_API Project
	{
	public:
		Project(FString InPath) : Path(MoveTemp(InPath)) 
		{}
		virtual ~Project() = default;

		virtual void Open(const FString& ProjectPath) = 0;
		virtual void SaveAs(const FString& InPath) = 0;
		virtual void Save();
		virtual void SavePendingAssets();
		virtual void Serialize(FArchive& Ar)
		{
			Ar << GProjectVer;
		}
		void AddPendingAsset(AssetPtr<AssetObject> InAsset) {
            PendingAssets.AddUnique(MoveTemp(InAsset));
        }
		void RemovePendingAsset(AssetObject* InAsset) {
			PendingAssets.Remove(InAsset);
        }

		bool IsPendingAsset(AssetObject* InAsset) const { return PendingAssets.Contains(InAsset); }
		bool IsPendingAsset(const FString& InPath) {
			return PendingAssets.ContainsByPredicate([InPath](const AssetPtr<AssetObject>& Element) {
				return Element->GetPath() == InPath;
			});
		}
        bool AnyPendingAsset() const;
		const FString& GetFilePath() const
		{
			return Path;
		}

	protected:
		TArray<AssetPtr<AssetObject>> PendingAssets;
		FString Path;
	};

	template<typename ProjectDataType, 
		typename = std::enable_if_t<std::is_base_of_v<Project, ProjectDataType>>>
	class ProjectManager
	{
	public:
		ProjectManager()
		{
			FString JsonContent;
			if (FFileHelper::LoadFileToString(JsonContent, *(PathHelper::SavedDir() / TEXT("ProjMgmt.json"))))
			{
				TSharedPtr<FJsonObject> JsonObject;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
				FJsonSerializer::Deserialize(Reader, JsonObject);
				TArray<TSharedPtr<FJsonValue>> JsonDirectories = JsonObject->GetArrayField("RecentProjects");
				for (const auto& JsonDirectory : JsonDirectories)
				{
					RecentProjcetPaths.Add(JsonDirectory->AsString());
				}
			}
		}

		virtual ~ProjectManager() = default;

	public:
		ProjectDataType* NewProject(const FString& ProjectName, const FString& ProjectDir)
		{
			IFileManager::Get().MakeDirectory(*(ProjectDir / ProjectName), true);
			IFileManager::Get().MakeDirectory(*(ProjectDir / ProjectName / "Content"), true);
			FString ProjectPath = ProjectDir / ProjectName / ProjectName + ".shprj";
			ActiveProject = MakeShared<ProjectDataType>(ProjectPath);
			GProject = ActiveProject.Get();

			AddToProjMgmt(ProjectPath);
			ActiveProject->Save();
			return ActiveProject.Get();
		}

		ProjectDataType* OpenProject(const FString& ProjectPath)
		{
			ActiveProject = MakeShared<ProjectDataType>(ProjectPath);
			GProject = ActiveProject.Get();

			TSingleton<AssetManager>::Get().Clear();
			TSingleton<AssetManager>::Get().MountProject(GetActiveContentDirectory());
			ActiveProject->Open(ProjectPath);
			AddToProjMgmt(ProjectPath);
			return ActiveProject.Get();
		}

		void RemoveFromProjMgmt(const FString& InProjectPath)
		{
			RecentProjcetPaths.Remove(InProjectPath);
			SaveProjMgmt();
		}

		void AddToProjMgmt(const FString& InProjectPath)
		{
			if (RecentProjcetPaths.Contains(InProjectPath))
			{
				RemoveFromProjMgmt(InProjectPath);
			}
			RecentProjcetPaths.Insert(InProjectPath, 0);
			SaveProjMgmt();
		}

		void SaveProjMgmt()
		{
			TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
			TArray<TSharedPtr<FJsonValue>> JsonDirectories;
			for (const FString& Directory : RecentProjcetPaths)
			{
				TSharedPtr<FJsonValue> JsonDriectory = MakeShared<FJsonValueString>(Directory);
				JsonDirectories.Add(MoveTemp(JsonDriectory));
			}
			JsonObject->SetArrayField("RecentProjects", JsonDirectories);
			FString NewJsonContents;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NewJsonContents);
			FJsonSerializer::Serialize(JsonObject, Writer);
			FFileHelper::SaveStringToFile(NewJsonContents, *(PathHelper::SavedDir() / TEXT("ProjMgmt.json")));
		}

		TSharedPtr<ProjectDataType> GetProject() const
		{
			return ActiveProject;
		}

		FString GetActiveProjectDirectory() const
		{
			check(ActiveProject);
			return FPaths::GetPath(ActiveProject->GetFilePath());
		}

		FString GetActiveContentDirectory() const
		{
			return GetActiveProjectDirectory() / TEXT("Content");
		}

		//FullPath and ProjectPath need be on the same drive.
		FString GetRelativePathToProject(const FString& FullPath) const
		{
			FString RelPath = FullPath;
			FString SlashDirectory = GetActiveProjectDirectory() + TEXT("/");
			FPaths::MakePathRelativeTo(RelPath, *SlashDirectory);
			return RelPath;
		}

		FString ConvertRelativePathToFull(const FString& RelativePathToProject) const
		{
			return FPaths::ConvertRelativePathToFull(GetActiveProjectDirectory(), RelativePathToProject);
		}

		const TArray<FString>& GetRecentProjcetPaths() const { return RecentProjcetPaths; }

	protected:
		TSharedPtr<ProjectDataType> ActiveProject;
		TArray<FString> RecentProjcetPaths;
	};


	FRAMEWORK_API void AddProjectAssociation();
}
