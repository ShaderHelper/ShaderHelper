#include "CommonHeader.h"
#include "ShaderToyEditor.h"
#include "AssetObject/ShaderToy/ShaderToy.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetManager/AssetManager.h"
#include "App/App.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyOutputNode.h"
#include "Renderer/ShaderToyRenderComp.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<ShaderToyOp>()
		.BaseClass<AssetOp>()
	)

	MetaType* ShaderToyOp::SupportType()
	{
		return GetMetaType<ShaderToy>();
	}

	void ShaderToyOp::OnOpen(const FString& InAssetPath)
	{
		AssetPtr<ShaderToy> LoadedShaderToy = TSingleton<AssetManager>::Get().LoadAssetByPath<ShaderToy>(InAssetPath);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->OpenGraph(LoadedShaderToy, MakeShared<ShaderToyRenderComp>(LoadedShaderToy, ShEditor->GetViewPort()));
	}

	void ShaderToyOp::OnCreate(AssetObject* InAsset)
	{
        auto OutputNode = NewShObject<ShaderToyOuputNode>(InAsset);
        OutputNode->InitPins();
		static_cast<ShaderToy*>(InAsset)->AddNode(MoveTemp(OutputNode));
	}

	void ShaderToyOp::OnDelete(const FString& InAssetPath)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        auto Graph = TSingleton<ShProjectManager>::Get().GetProject()->Graph;
        if(Graph && Graph->GetPath() == InAssetPath)
        {
            ShEditor->OpenGraph(nullptr, nullptr);
        }
        
        AssetOp::OnDelete(InAssetPath);
	}

}
