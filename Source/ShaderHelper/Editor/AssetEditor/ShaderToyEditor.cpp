#include "CommonHeader.h"
#include "ShaderToyEditor.h"
#include "AssetObject/ShaderToy/ShaderToy.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetManager/AssetManager.h"
#include "App/App.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyOutputNode.h"

using namespace FRAMEWORK;

namespace SH
{
	GLOBAL_REFLECTION_REGISTER(AddClass<ShaderToyOp>()
		.BaseClass<AssetOp>()
	)

	MetaType* ShaderToyOp::SupportAsset()
	{
		return GetMetaType<ShaderToy>();
	}

	void ShaderToyOp::OnOpen(const FString& InAssetPath)
	{
		AssetPtr<ShaderToy> LoadedShaderToy = TSingleton<AssetManager>::Get().LoadAssetByPath<ShaderToy>(InAssetPath);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->OpenGraph(LoadedShaderToy);
	}

	void ShaderToyOp::OnCreate(AssetObject* InAsset)
	{
		static_cast<ShaderToy*>(InAsset)->AddNode(MakeShared<ShaderToyOuputNode>());
	}

	void ShaderToyOp::OnDelete(const FString& InAssetPath)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->OpenGraph(nullptr);
	}

}