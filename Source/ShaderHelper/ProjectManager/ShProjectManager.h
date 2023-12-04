#pragma once
#include "ProjectManager/ProjectManager.h"

namespace SH
{
	class ShProject : public Project
	{
	public:
		using Project::Project;

		virtual void Load(const FString& ProjectPath) override;
		virtual void Save(const FString& ProjectPath) override;
	};

	using ShProjectManager = ProjectManager<ShProject>;

}