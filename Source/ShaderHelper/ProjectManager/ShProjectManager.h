#pragma once
#include "ProjectManager/ProjectManager.h"
#include "UI/Widgets/AssetBrowser/SAssetBrowser.h"
#include "AssetObject/StShader.h"
#include "AssetObject/Graph.h"

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
		virtual void Save(const FString& InPath) override;
		void Serialize(FArchive& Ar);

		Version Ver = Version::Initial;
		FRAMEWORK::AssetBrowserPersistentState AssetBrowserState;
		FRAMEWORK::AssetPtr<FRAMEWORK::Graph> Graph;
		TMap<FRAMEWORK::AssetPtr<StShader>, TSharedPtr<class SDockTab>> OpenedStShaders;
		TSharedPtr<FTabManager::FLayout> CodeTabLayout;
	};

	using ShProjectManager = FRAMEWORK::ProjectManager<ShProject>;

}
