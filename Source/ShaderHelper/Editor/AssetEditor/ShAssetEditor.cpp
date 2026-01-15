#include "CommonHeader.h"
#include "ShAssetEditor.h"
#include "Editor/ShaderHelperEditor.h"
#include "App/App.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<ShAssetOp>()
		.BaseClass<AssetOp>()
	)

	void ShAssetOp::OnNavigate(const FString& InAssetPath)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->GetTabManager()->TryInvokeTab(AssetTabId);
		auto AssetBrowser = ShEditor->GetAssetBrowser();
		AssetBrowser->SetCurrentDisplyPath(FPaths::GetPath(InAssetPath));
		AssetBrowser->GetAssetView()->SetSelectedItem(InAssetPath);
	}
}
