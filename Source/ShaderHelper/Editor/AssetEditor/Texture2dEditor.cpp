#include "CommonHeader.h"
#include "Texture2dEditor.h"
#include "AssetObject/Texture2D.h"
#include "Editor/ShaderHelperEditor.h"
#include "App/App.h"
#include "UI/Widgets/STexturePreviewer.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<Texture2dOp>()
		.BaseClass<ShAssetOp>()
	)

	MetaType* Texture2dOp::SupportType()
	{
		return GetMetaType<Texture2D>();
	}

	void Texture2dOp::OnOpen(const FString& InAssetPath)
	{
		AssetPtr<Texture2D> Asset = TSingleton<AssetManager>::Get().LoadAssetByPath<Texture2D>(InAssetPath);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ShowProperty(Asset.Get());
		STexturePreviewer::OpenTexturePreviewer(Asset, FPointerEventHandler::CreateLambda([=](const FGeometry&, const FPointerEvent&) {
			ShEditor->ShowProperty(Asset.Get());
			return FReply::Handled();
		}), ShEditor->GetMainWindow());
	}

}
