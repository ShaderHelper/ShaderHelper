#include "CommonHeader.h"
#include "StShaderEditor.h"
#include "AssetObject/StShader.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetManager/AssetManager.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<StShaderOp>()
                                .BaseClass<ShAssetOp>()
	)

    MetaType* StShaderOp::SupportType()
    {
        return GetMetaType<StShader>();
    }

    void StShaderOp::OnSelect(ShObject* InObject)
    {
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->ShowProperty(InObject);
    }

	void StShaderOp::OnOpen(const FString& InAssetPath)
	{
        AssetPtr<StShader> LoadedStShaderAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<StShader>(InAssetPath);
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->AddNavigationInfo(LoadedStShaderAsset->GetGuid(), {});
        ShEditor->OpenShaderTab(MoveTemp(LoadedStShaderAsset));
	}

    void StShaderOp::OnRename(const FString& OldPath, const FString& NewPath)
    {
        AssetOp::OnRename(OldPath, NewPath);
        OnMove(OldPath, NewPath);
    }

    void StShaderOp::OnMove(const FString& OldPath, const FString& NewPath)
    {
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->UpdateShaderPath(NewPath);
    }

    void StShaderOp::OnAdd(const FString& InAssetPath)
    {

    }

    void StShaderOp::OnDelete(const FString& InAssetPath)
    {
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		FName TabId{ TSingleton<AssetManager>::Get().GetGuid(InAssetPath).ToString() };
		TSharedPtr<SDockTab> ExistingTab = ShEditor->GetCodeTabManager()->FindExistingLiveTab(TabId);
		if (ExistingTab)
		{
			ExistingTab->RequestCloseTab();
		}

		AssetOp::OnDelete(InAssetPath);
    }
}
