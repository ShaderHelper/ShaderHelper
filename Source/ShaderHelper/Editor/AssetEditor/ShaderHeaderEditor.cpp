#include "CommonHeader.h"
#include "ShaderHeaderEditor.h"
#include "AssetObject/ShaderHeader.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetManager/AssetManager.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<ShaderHeaderOp>()
                                .BaseClass<ShAssetOp>()
	)

    MetaType* ShaderHeaderOp::SupportType()
    {
        return GetMetaType<ShaderHeader>();
    }

	void ShaderHeaderOp::OnSelect(FW::ShObject* InObject)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ShowProperty(InObject);
	}

	void ShaderHeaderOp::OnOpen(const FString& InAssetPath)
	{
		AssetPtr<ShaderHeader> LoadedShaderHeader = TSingleton<AssetManager>::Get().LoadAssetByPath<ShaderHeader>(InAssetPath);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->AddNavigationInfo(LoadedShaderHeader->GetGuid(), {});
		ShEditor->OpenShaderTab(MoveTemp(LoadedShaderHeader));
	}

}
