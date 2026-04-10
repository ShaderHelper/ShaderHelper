#include "CommonHeader.h"
#include "VertexShaderEditor.h"
#include "AssetManager/AssetManager.h"
#include "AssetObject/VertexShader.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<VertexShaderOp>()
		.BaseClass<ShAssetOp>()
	)

	MetaType* VertexShaderOp::SupportType()
	{
		return GetMetaType<VertexShader>();
	}

	void VertexShaderOp::OnOpen(const FString& InAssetPath)
	{
		AssetPtr<VertexShader> LoadedVertexShader = TSingleton<AssetManager>::Get().LoadAssetByPath<VertexShader>(InAssetPath);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->OpenShaderTab(MoveTemp(LoadedVertexShader));
	}

	void VertexShaderOp::OnRename(const FString& OldPath, const FString& NewPath)
	{
		AssetOp::OnRename(OldPath, NewPath);
		OnMove(OldPath, NewPath);
	}

	void VertexShaderOp::OnMove(const FString& OldPath, const FString& NewPath)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->UpdateShaderPath(NewPath);
	}

	void VertexShaderOp::OnDelete(const FString& InAssetPath)
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
