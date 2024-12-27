#pragma once
#include "ProjectManager/ProjectManager.h"
#include "UI/Widgets/AssetBrowser/SAssetBrowser.h"
#include "AssetObject/StShader.h"
#include "AssetObject/Graph.h"

namespace SH
{
	enum class ShProjectVersion
	{
		Initial
	};

	class ShProject : public FRAMEWORK::Project
	{
	public:
		ShProject(FString InPath);

		virtual void Open(const FString& ProjectPath) override;
		virtual void Save(const FString& InPath) override;
		void Serialize(FArchive& Ar) override;

		FRAMEWORK::AssetBrowserPersistentState AssetBrowserState;
		FRAMEWORK::AssetPtr<FRAMEWORK::Graph> Graph;
		TMap<FRAMEWORK::AssetPtr<StShader>, TSharedPtr<class SDockTab>> OpenedStShaders;
		TSharedPtr<FTabManager::FLayout> CodeTabLayout;
	};

	using ShProjectManager = FRAMEWORK::ProjectManager<ShProject>;
}
