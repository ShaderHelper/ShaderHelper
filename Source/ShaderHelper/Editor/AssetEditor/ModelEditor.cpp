#include "CommonHeader.h"
#include "ModelEditor.h"
#include "AssetObject/Model.h"
#include "Editor/ShaderHelperEditor.h"
#include "App/App.h"
#include "UI/Widgets/SModelPreviewer.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<ModelOp>()
		.BaseClass<ShAssetOp>()
	)

	MetaType* ModelOp::SupportType()
	{
		return GetMetaType<Model>();
	}

	void ModelOp::OnOpen(const FString& InAssetPath)
	{
		AssetPtr<Model> Asset = TSingleton<AssetManager>::Get().LoadAssetByPath<Model>(InAssetPath);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ShowProperty(Asset.Get());
		SModelPreviewer::OpenModelPreviewer(Asset, FPointerEventHandler::CreateLambda([=](const FGeometry&, const FPointerEvent&) {
			ShEditor->ShowProperty(Asset.Get());
			return FReply::Handled();
		}), ShEditor->GetMainWindow());
	}
}
