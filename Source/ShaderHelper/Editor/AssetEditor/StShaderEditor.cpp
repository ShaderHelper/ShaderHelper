#include "CommonHeader.h"
#include "StShaderEditor.h"
#include "AssetObject/StShader.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetManager/AssetManager.h"

using namespace FRAMEWORK;

namespace SH
{
	GLOBAL_REFLECTION_REGISTER(AddClass<StShaderOp>()
                                .BaseClass<AssetOp>()
	)

    MetaType* StShaderOp::SupportAsset()
    {
        return GetMetaType<StShader>();
    }

	void StShaderOp::OnOpen(const FString& InAssetPath)
	{
        AssetPtr<StShader> LoadedStShaderAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<StShader>(InAssetPath);
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->OpenStShaderTab(MoveTemp(LoadedStShaderAsset));
	}

    void StShaderOp::OnAdd(const FString& InAssetPath)
    {
		/*AssetPtr<StShader> LoadedStShaderAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<StShader>(InAssetPath);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->TryRestoreStShaderTab(MoveTemp(LoadedStShaderAsset));*/
    }

    void StShaderOp::OnDelete(const FString& InAssetPath)
    {
		AssetOp::OnDelete(InAssetPath);

        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        FName TabId{TSingleton<AssetManager>::Get().GetGuid(InAssetPath).ToString()};
        TSharedPtr<SDockTab> ExistingTab = ShEditor->GetCodeTabManager()->FindExistingLiveTab(TabId);
        if (ExistingTab)
        {
            ExistingTab->RequestCloseTab();
        }
    }
}
