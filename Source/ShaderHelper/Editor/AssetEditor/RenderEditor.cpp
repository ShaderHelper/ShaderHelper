#include "CommonHeader.h"
#include "RenderEditor.h"
#include "App/App.h"
#include "AssetManager/AssetManager.h"
#include "AssetObject/Render/Nodes/RenderOutputNode.h"
#include "AssetObject/Render/Render.h"
#include "Editor/ShaderHelperEditor.h"
#include "Renderer/RenderRenderComp.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<RenderOp>()
		.BaseClass<ShAssetOp>()
	)

	MetaType* RenderOp::SupportType()
	{
		return GetMetaType<Render>();
	}

	void RenderOp::OnOpen(const FString& InAssetPath)
	{
		AssetPtr<Render> LoadedRender = TSingleton<AssetManager>::Get().LoadAssetByPath<Render>(InAssetPath);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if (ShEditor->OpenGraph(LoadedRender))
		{
			ShEditor->SetGraphRenderComp(nullptr);
			ShEditor->SetGraphRenderComp(MakeShared<RenderRenderComp>(LoadedRender, ShEditor->GetViewPort()));
		}
	}

	bool RenderOp::OnCreate(AssetObject* InAsset)
	{
		auto OutputNode = NewShObject<RenderOutputNode>(InAsset);
		static_cast<Render*>(InAsset)->AddNode(MoveTemp(OutputNode));
		return true;
	}

	void RenderOp::OnDelete(const FString& InAssetPath)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		auto Graph = TSingleton<ShProjectManager>::Get().GetProject()->Graph;
		if (Graph && Graph->GetPath() == InAssetPath)
		{
			ShEditor->SetGraphRenderComp(nullptr);
			ShEditor->OpenGraph(nullptr);
		}

		AssetOp::OnDelete(InAssetPath);
	}
}
