#pragma once
#include "ProjectManager/ProjectManager.h"

namespace SH
{
	class ShProject : public FRAMEWORK::Project
	{
	public:
		enum class Version
		{
			Initial
		};

		using Project::Project;

		virtual void Open(const FString& ProjectPath) override;
		virtual void Save() override;
		void Serialize(FArchive& Ar);

		Version Ver = Version::Initial;
	};

	using ShProjectManager = FRAMEWORK::ProjectManager<ShProject>;

}
