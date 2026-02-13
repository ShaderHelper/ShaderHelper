#include "CommonHeader.h"
#include "ShAssetEditor.h"
#include "Editor/ShaderHelperEditor.h"
#include "App/App.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<ShPropertyOp>()
		.BaseClass<ShObjectOp>()
	)
	REFLECTION_REGISTER(AddClass<ShAssetOp>()
		.BaseClass<AssetOp>()
	)

	void ShPropertyOp::OnCancelSelect(FW::ShObject* InObject)
	{
		if (auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor()))
		{
			if (ShEditor->GetCurPropertyObject() == InObject && !ShEditor->IsPropertyLocked())
			{
				ShEditor->RefreshProperty(true);
			}
		}
	}

	void ShPropertyOp::OnSelect(FW::ShObject* InObject)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ShowProperty(InObject);
	}

	void ShAssetOp::OnNavigate(const FString& InAssetPath)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->GetTabManager()->TryInvokeTab(AssetTabId);
		auto AssetBrowser = ShEditor->GetAssetBrowser();
		AssetBrowser->SetCurrentDisplyPath(FPaths::GetPath(InAssetPath));
		AssetBrowser->GetAssetView()->SetSelectedItem(InAssetPath);
	}

	void ShAssetOp::OnCancelSelect(FW::ShObject* InObject)
	{
		if (auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor()))
		{
			if (ShEditor->GetCurPropertyObject() == InObject && !ShEditor->IsPropertyLocked())
			{
				ShEditor->RefreshProperty(true);
			}
		}
	}

	void ShAssetOp::OnSelect(FW::ShObject* InObject)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ShowProperty(InObject);
	}
}
