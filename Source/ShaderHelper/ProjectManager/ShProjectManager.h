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

	class ShProject : public FW::Project
	{
	public:
		ShProject(FString InPath);

		virtual void Open(const FString& ProjectPath) override;
		virtual void SaveAs(const FString& InPath) override;
		void Serialize(FArchive& Ar) override;

		FW::AssetBrowserPersistentState AssetBrowserState;
		FW::AssetPtr<FW::Graph> Graph;
		TMap<FW::AssetPtr<StShader>, TSharedPtr<class SDockTab>> OpenedStShaders;
		TSharedPtr<FTabManager::FLayout> CodeTabLayout;
	};

	using ShProjectManager = FW::ProjectManager<ShProject>;
}
