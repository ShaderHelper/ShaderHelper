#pragma once

namespace FRAMEWORK
{
	class Project
	{
	public:
		Project(FString InPath) : Path(MoveTemp(InPath)) 
		{}
		virtual ~Project() = default;

		virtual void Open(const FString& ProjectPath) = 0;
		virtual void Save() = 0;

		const FString& GetFilePath() const
		{
			return Path;
		}

	protected:
		FString Path;
	};

	template<typename ProjectDataType, 
		typename = std::enable_if_t<std::is_base_of_v<Project, ProjectDataType>>>
	class ProjectManager
	{
	public:
		virtual ~ProjectManager() = default;

		virtual void NewProject()
		{

		}

		virtual void OpenProject(const FString& ProjectPath)
		{
			ActiveProject = MakeUnique<ProjectDataType>(ProjectPath);
			ActiveProject->Open(ProjectPath);
		}

		virtual void SaveProject()
		{
			ActiveProject->Save();
		}

		ProjectDataType& GetProject()
		{
			return *ActiveProject;
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

		FString GetActiveSavedDirectory() const
		{
			return GetActiveProjectDirectory() / TEXT("Saved");
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

	protected:
		TUniquePtr<ProjectDataType> ActiveProject;
	};
}