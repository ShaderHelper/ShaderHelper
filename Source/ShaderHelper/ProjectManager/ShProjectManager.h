#pragma once
#include "ProjectManager/ProjectManager.h"
#include "UI/Widgets/AssetBrowser/SAssetBrowser.h"
#include "AssetObject/ShaderAsset.h"
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
        ~ShProject();

		virtual void Open(const FString& ProjectPath) override;
		virtual void SaveAs(const FString& InPath) override;
		void Serialize(FArchive& Ar) override;

		FW::AssetBrowserPersistentState AssetBrowserState;
		FW::AssetPtr<FW::Graph> Graph;
		TMap<FW::AssetPtr<ShaderAsset>, TSharedPtr<class SDockTab>> OpenedShaders;
		TSharedPtr<FTabManager::FLayout> CodeTabLayout;
        bool TimelineStop = true;
        float TimelineCurTime = 0;
        float TimelineMaxTime = 100;
	};

	using ShProjectManager = FW::ProjectManager<ShProject>;
}
