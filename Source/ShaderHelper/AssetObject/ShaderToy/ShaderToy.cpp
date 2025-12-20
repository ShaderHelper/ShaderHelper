#include "CommonHeader.h"
#include "ShaderToy.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetObject/Texture2D.h"
#include "AssetObject/Nodes/Texture2dNode.h"
#include "AssetObject/StShader.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyPassNode.h"
#include "UI/Widgets/Graph/SGraphPanel.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<ShaderToy>("ShaderToy Graph")
								.BaseClass<Graph>()
	)

    ShaderToy::~ShaderToy()
    {

    }

	void ShaderToy::Serialize(FArchive& Ar)
	{
		Graph::Serialize(Ar);
	}

	FString ShaderToy::FileExtension() const
	{
		return "shaderToy";
	}

	void ShaderToy::OnDragEnter(TSharedPtr<FDragDropOperation> DragDropOp)
	{
		if(DragDropOp->IsOfType<AssetViewItemDragDropOp>())
		{
			TArray<FString> DropFilePaths = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Paths;
			for (const auto& DropFilePath : DropFilePaths)
			{
				MetaType* DropAssetMetaType = GetAssetMetaType(DropFilePath);
				if (DropAssetMetaType->IsType<Texture2D>() || DropAssetMetaType->IsType<StShader>())
				{
					DragDropOp->SetCursorOverride(EMouseCursor::GrabHand);
					return;
				}
			}
		}
		DragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);
	}

	void ShaderToy::OnDrop(TSharedPtr<FDragDropOperation> DragDropOp)
	{
		if(DragDropOp->IsOfType<AssetViewItemDragDropOp>())
		{
			TArray<FString> DropFilePaths = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Paths;
			for (const auto& DropFilePath : DropFilePaths)
			{
				MetaType* DropAssetMetaType = GetAssetMetaType(DropFilePath);

				auto GraphPanel = static_cast<ShaderHelperEditor*>(GApp->GetEditor())->GetGraphPanel();
				if (DropAssetMetaType->IsType<Texture2D>())
				{
					AssetPtr<Texture2D> TexAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<Texture2D>(DropFilePath);
					auto NewTexNode = NewShObject<Texture2dNode>(this, MoveTemp(TexAsset));
					GraphPanel->AddNode(MoveTemp(NewTexNode));
				}
				else if (DropAssetMetaType->IsType<StShader>())
				{
					AssetPtr<StShader> ShaderAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<StShader>(DropFilePath);
					auto NewPassNode = NewShObject<ShaderToyPassNode>(this, MoveTemp(ShaderAsset));
					GraphPanel->AddNode(MoveTemp(NewPassNode));
				}
			}
			
		}
	}

}
