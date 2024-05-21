#include "CommonHeader.h"
#include "ShaderPassEditor.h"
#include "AssetObject/ShaderPass.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetManager/AssetManager.h"

using namespace FRAMEWORK;

namespace SH
{
	GLOBAL_REFLECTION_REGISTER(
		ShReflectToy::AddClass<ShaderPassOp>()
						.BaseClass<AssetOp>();
	)

    ShReflectToy::MetaType* ShaderPassOp::SupportAsset()
    {
        return ShReflectToy::GetMetaType<ShaderPass>();
    }

	void ShaderPassOp::Open(const FString& InAssetPath)
	{
        AssetPtr<ShaderPass> LoadedShaderPassAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<ShaderPass>(InAssetPath);
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->OpenShaderPassTab(MoveTemp(LoadedShaderPassAsset));
	}
}
