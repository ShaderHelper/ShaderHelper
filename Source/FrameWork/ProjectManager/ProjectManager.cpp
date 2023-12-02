#include "CommonHeader.h"
#include "ProjectManager.h"

namespace FRAMEWORK
{

	void ProjectManager::New()
	{

	}

	void ProjectManager::Load(const FString& ProjectPath)
	{
		ActiveProject = MakeUnique<Project>();
		ActiveProject->Path = ProjectPath;
	}

	void ProjectManager::Save(const FString& SaveDirectory)
	{

	}

	FString ProjectManager::GetActiveProjectDirectory() const
	{
		check(ActiveProject);
		return FPaths::GetPath(ActiveProject->Path);
	}

	FString ProjectManager::GetActiveContentDirectory() const
	{
		check(ActiveProject);
		return GetActiveProjectDirectory() / TEXT("Content");
	}

}