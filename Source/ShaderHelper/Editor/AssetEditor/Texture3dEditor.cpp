#include "CommonHeader.h"
#include "Texture3dEditor.h"
#include "AssetObject/Texture3D.h"
#include "Editor/ShaderHelperEditor.h"
#include "App/App.h"
#include "UI/Widgets/STexturePreviewer.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<Texture3dOp>()
		.BaseClass<ShAssetOp>()
	)

	MetaType* Texture3dOp::SupportType()
	{
		return GetMetaType<Texture3D>();
	}

	void Texture3dOp::OnOpen(const FString& InAssetPath)
	{
		AssetPtr<Texture3D> Asset = TSingleton<AssetManager>::Get().LoadAssetByPath<Texture3D>(InAssetPath);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ShowProperty(Asset.Get());
		STexturePreviewer::OpenTexturePreviewer(Asset, FPointerEventHandler::CreateLambda([=](const FGeometry&, const FPointerEvent&) {
			ShEditor->ShowProperty(Asset.Get());
			return FReply::Handled();
		}), ShEditor->GetMainWindow());
	}
}
