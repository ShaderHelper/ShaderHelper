#include "CommonHeader.h"
#include "ShaderPassEditor.h"
#include "AssetObject/ShaderPass.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetManager/AssetManager.h"

using namespace FRAMEWORK;

namespace SH
{
	GLOBAL_REFLECTION_REGISTER(AddClass<ShaderPassOp>()
                                .BaseClass<AssetOp>()
	)

    MetaType* ShaderPassOp::SupportAsset()
    {
        return GetMetaType<ShaderPass>();
    }

	void ShaderPassOp::OnOpen(const FString& InAssetPath)
	{
        AssetPtr<ShaderPass> LoadedShaderPassAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<ShaderPass>(InAssetPath);
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->OpenShaderPassTab(MoveTemp(LoadedShaderPassAsset));
	}

    void ShaderPassOp::OnAdd(const FString& InAssetPath)
    {
        AssetPtr<ShaderPass> LoadedShaderPassAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<ShaderPass>(InAssetPath);
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->TryRestoreShaderPassTab(MoveTemp(LoadedShaderPassAsset));
    }

    void ShaderPassOp::OnDelete(const FString& InAssetPath)
    {
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        FName TabId{TSingleton<AssetManager>::Get().GetGuid(InAssetPath).ToString()};
        TSharedPtr<SDockTab> ExistingTab = ShEditor->GetCodeTabManager()->FindExistingLiveTab(TabId);
        if (ExistingTab)
        {
            ExistingTab->RequestCloseTab();
        }
    }
}
