#include "CommonHeader.h"
#include "Render.h"
#include "MeshSceneObject.h"
#include "CameraSceneObject.h"
#include "App/App.h"
#include "AssetManager/AssetManager.h"
#include "AssetObject/Nodes/Texture2dNode.h"
#include "AssetObject/Nodes/Texture3dNode.h"
#include "AssetObject/Nodes/TextureCubeNode.h"
#include "AssetObject/Texture2D.h"
#include "AssetObject/Texture3D.h"
#include "AssetObject/TextureCube.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/Graph/SGraphPanel.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<Render>("Render")
		.BaseClass<Graph>()
	)

	Render::~Render()
	{
	}

	FString Render::FileExtension() const
	{
		return "render";
	}

	void Render::Serialize(FArchive& Ar)
	{
		Graph::Serialize(Ar);

		int SceneObjNum = SceneObjects.Num();
		Ar << SceneObjNum;
		if (Ar.IsSaving())
		{
			for (int Index = 0; Index < SceneObjNum; Index++)
			{
				FString TypeName = GetRegisteredName(SceneObjects[Index]->DynamicMetaType());
				Ar << TypeName;
				SceneObjects[Index]->Serialize(Ar);
			}
		}
		else
		{
			SceneObjects.Reserve(SceneObjNum);
			for (int Index = 0; Index < SceneObjNum; Index++)
			{
				FString TypeName;
				Ar << TypeName;
				auto LoadedObj = NewShObject<SceneObject>(GetMetaType(TypeName), this);
				LoadedObj->Serialize(Ar);
				SceneObjects.Emplace(LoadedObj);
			}
		}
	}

	void Render::RemoveSceneObject(SceneObject* InObject)
	{
		SceneObjects.RemoveAll([InObject](const ObjectPtr<SceneObject>& Element) {
			return Element.Get() == InObject;
		});
		MarkDirty();
	}

	void Render::InsertSceneObject(int32 Index, ObjectPtr<SceneObject> InObject)
	{
		if (Index >= 0 && Index <= SceneObjects.Num())
		{
			SceneObjects.Insert(MoveTemp(InObject), Index);
		}
		else
		{
			SceneObjects.Add(MoveTemp(InObject));
		}
		MarkDirty();
	}

	void Render::OnDragEnter(TSharedPtr<FDragDropOperation> DragDropOp)
	{
		if (DragDropOp->IsOfType<AssetViewItemDragDropOp>())
		{
			TArray<FString> DropFilePaths = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Paths;
			for (const auto& DropFilePath : DropFilePaths)
			{
				MetaType* DropAssetMetaType = GetAssetMetaType(DropFilePath);
				if (DropAssetMetaType->IsType<Texture2D>() || DropAssetMetaType->IsType<Texture3D>() || DropAssetMetaType->IsType<TextureCube>())
				{
					DragDropOp->SetCursorOverride(EMouseCursor::GrabHand);
					return;
				}
			}
			DragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);
		}
	}

	void Render::OnDrop(TSharedPtr<FDragDropOperation> DragDropOp, const Vector2D& Pos)
	{
		if (DragDropOp->IsOfType<AssetViewItemDragDropOp>())
		{
			TArray<FString> DropFilePaths = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Paths;
			for (const auto& DropFilePath : DropFilePaths)
			{
				MetaType* DropAssetMetaType = GetAssetMetaType(DropFilePath);
				auto GraphPanel = static_cast<ShaderHelperEditor*>(GApp->GetEditor())->GetGraphPanel();

				if (DropAssetMetaType->IsType<Texture2D>())
				{
					AssetPtr<Texture2D> TextureAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<Texture2D>(DropFilePath);
					SGraphPanel::ScopedTransaction Transaction(GraphPanel);
					auto NewTextureNode = NewShObject<Texture2dNode>(this, MoveTemp(TextureAsset));
					GraphPanel->DoCommand(MakeShared<AddNodeCommand>(GraphPanel, NewTextureNode, Pos));
					GraphPanel->SetFocus();
				}
				else if (DropAssetMetaType->IsType<Texture3D>())
				{
					AssetPtr<Texture3D> TextureAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<Texture3D>(DropFilePath);
					SGraphPanel::ScopedTransaction Transaction(GraphPanel);
					auto NewTextureNode = NewShObject<Texture3dNode>(this, MoveTemp(TextureAsset));
					GraphPanel->DoCommand(MakeShared<AddNodeCommand>(GraphPanel, NewTextureNode, Pos));
					GraphPanel->SetFocus();
				}
				else if (DropAssetMetaType->IsType<TextureCube>())
				{
					AssetPtr<TextureCube> TextureAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<TextureCube>(DropFilePath);
					SGraphPanel::ScopedTransaction Transaction(GraphPanel);
					auto NewTextureNode = NewShObject<TextureCubeNode>(this, MoveTemp(TextureAsset));
					GraphPanel->DoCommand(MakeShared<AddNodeCommand>(GraphPanel, NewTextureNode, Pos));
					GraphPanel->SetFocus();
				}
			}
		}
	}
}
