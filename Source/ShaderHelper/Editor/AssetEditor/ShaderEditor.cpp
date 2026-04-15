#include "CommonHeader.h"
#include "ShaderEditor.h"
#include "AssetManager/AssetManager.h"
#include "AssetObject/Shader.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<ShaderOp>()
		.BaseClass<ShAssetOp>()
	)

	MetaType* ShaderOp::SupportType()
	{
		return GetMetaType<Shader>();
	}

	void ShaderOp::OnOpen(const FString& InAssetPath)
	{
		AssetPtr<Shader> LoadedShader = TSingleton<AssetManager>::Get().LoadAssetByPath<Shader>(InAssetPath);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->OpenShaderTab(MoveTemp(LoadedShader));
	}

	void ShaderOp::OnRename(const FString& OldPath, const FString& NewPath)
	{
		AssetOp::OnRename(OldPath, NewPath);
		OnMove(OldPath, NewPath);
	}

	void ShaderOp::OnMove(const FString& OldPath, const FString& NewPath)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->UpdateShaderPath(NewPath);
	}

	void ShaderOp::OnDelete(const FString& InAssetPath)
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
