#include "CommonHeader.h"
#include "PixelShaderEditor.h"
#include "AssetManager/AssetManager.h"
#include "AssetObject/PixelShader.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<PixelShaderOp>()
		.BaseClass<ShAssetOp>()
	)

	MetaType* PixelShaderOp::SupportType()
	{
		return GetMetaType<PixelShader>();
	}

	void PixelShaderOp::OnOpen(const FString& InAssetPath)
	{
		AssetPtr<PixelShader> LoadedPixelShader = TSingleton<AssetManager>::Get().LoadAssetByPath<PixelShader>(InAssetPath);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->OpenShaderTab(MoveTemp(LoadedPixelShader));
	}

	void PixelShaderOp::OnRename(const FString& OldPath, const FString& NewPath)
	{
		AssetOp::OnRename(OldPath, NewPath);
		OnMove(OldPath, NewPath);
	}

	void PixelShaderOp::OnMove(const FString& OldPath, const FString& NewPath)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->UpdateShaderPath(NewPath);
	}

	void PixelShaderOp::OnDelete(const FString& InAssetPath)
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
