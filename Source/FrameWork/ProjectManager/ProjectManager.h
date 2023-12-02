#pragma once

namespace FRAMEWORK
{
	struct Project
	{
		FString Path;
	};

	class FRAMEWORK_API ProjectManager
	{
	public:
		void New();
		void Load(const FString& ProjectPath);
		void Save(const FString& SaveDirectory);
		FString GetActiveProjectDirectory() const;
		FString GetActiveContentDirectory() const;

	private:
		TUniquePtr<Project> ActiveProject;
	};
}