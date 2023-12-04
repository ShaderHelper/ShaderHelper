#pragma once

namespace FRAMEWORK
{
	class Project
	{
	public:
		Project(FString InPath) : Path(MoveTemp(InPath)) 
		{}
		virtual ~Project() = default;

		virtual void Load(const FString& ProjectPath) = 0;
		virtual void Save(const FString& ProjectPath) = 0;

		const FString& GetFilePath() const
		{
			return Path;
		}

	private:
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
			ActiveProject->Load(ProjectPath);
		}

		virtual void SaveProject(const FString& ProjectPath)
		{
			ActiveProject->Save(ProjectPath);
		}

		FString GetActiveProjectDirectory() const
		{
			check(ActiveProject);
			return FPaths::GetPath(ActiveProject->GetFilePath());
		}

		FString GetActiveContentDirectory() const
		{
			check(ActiveProject);
			return GetActiveProjectDirectory() / TEXT("Content");
		}

	protected:
		TUniquePtr<ProjectDataType> ActiveProject;
	};
}