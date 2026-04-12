#include "CommonHeader.h"
#include "MaterialEditor.h"
#include "AssetObject/Material.h"
#include "AssetManager/AssetManager.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/SMaterialPreviewer.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<MaterialOp>()
		.BaseClass<ShAssetOp>()
	)

	MetaType* MaterialOp::SupportType()
	{
		return GetMetaType<Material>();
	}

	void MaterialOp::OnOpen(const FString& InAssetPath)
	{
		AssetPtr<Material> Asset = TSingleton<AssetManager>::Get().LoadAssetByPath<Material>(InAssetPath);
		if (Asset)
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			ShEditor->ShowProperty(Asset.Get());
			SMaterialPreviewer::OpenMaterialPreviewer(Asset, FPointerEventHandler::CreateLambda([=](const FGeometry&, const FPointerEvent&) {
				ShEditor->ShowProperty(Asset.Get());
				return FReply::Handled();
			}), ShEditor->GetMainWindow());
		}
		
	}
}
