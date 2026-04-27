#include "CommonHeader.h"
#include "Render.h"
#include "MeshSceneObject.h"
#include "CameraSceneObject.h"
#include "AssetObject/Render/Nodes/MeshPassNode.h"
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
	namespace
	{
		void RemoveMeshPassReferencesToSceneObject(Render* OwnerRender, SceneObject* RemovedObject)
		{
			if (!RemovedObject)
			{
				return;
			}

			MeshSceneObject* RemovedMesh = dynamic_cast<MeshSceneObject*>(RemovedObject);
			CameraSceneObject* RemovedCamera = dynamic_cast<CameraSceneObject*>(RemovedObject);
			if (!RemovedMesh && !RemovedCamera)
			{
				return;
			}

			for (const ObjectPtr<GraphNode>& Node : OwnerRender->GetNodes())
			{
				MeshPassNode* MeshPass = dynamic_cast<MeshPassNode*>(Node.Get());
				if (!MeshPass)
				{
					continue;
				}

				bool bChanged = false;
				if (RemovedCamera && MeshPass->CameraRef.Get() == RemovedCamera)
				{
					MeshPass->CameraRef.Reset();
					bChanged = true;
				}

				if (RemovedMesh)
				{
					for (int32 Index = MeshPass->MeshRenderObjects.Num() - 1; Index >= 0; --Index)
					{
						MeshRenderObject* MeshRenderObj = MeshPass->MeshRenderObjects[Index].Get();
						if (MeshRenderObj && MeshRenderObj->MeshSceneObjectRef.Get() == RemovedMesh)
						{
							MeshPass->RemoveMeshRenderObject(MeshRenderObj);
							bChanged = true;
						}
					}
				}

				if (bChanged)
				{
					MeshPass->RefreshNodeWidget();
				}
			}
		}
	}

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

		SerializePolymorphicObjectArray(Ar, SceneObjects, this);
	}

	void Render::PostLoad()
	{
		Graph::PostLoad();

		for (const ObjectPtr<SceneObject>& Obj : SceneObjects)
		{
			if (Obj->Parent.IsValid())
			{
				Obj->Parent->Children.Add(Obj.Get());
			}
		}
	}

	void Render::RemoveSceneObject(SceneObject* InObject)
	{
		RemoveMeshPassReferencesToSceneObject(this, InObject);

		if (InObject->Parent.IsValid())
		{
			InObject->Parent->Children.Remove(InObject);
			InObject->Parent.Reset();
		}

		TArray<SceneObject*> ChildrenCopy = InObject->Children;
		for (SceneObject* Child : ChildrenCopy)
		{
			RemoveSceneObject(Child);
		}

		SceneObjects.RemoveAll([InObject](const ObjectPtr<SceneObject>& Element) {
			return Element.Get() == InObject;
		});
		MarkDirty();
	}

	void Render::InsertSceneObject(int32 Index, ObjectPtr<SceneObject> InObject, SceneObject* InParent, int32 ParentChildIndex)
	{
		if (InParent)
		{
			InObject->Parent = InParent;
			if (ParentChildIndex >= 0 && ParentChildIndex <= InParent->Children.Num())
			{
				InParent->Children.Insert(InObject.Get(), ParentChildIndex);
			}
			else
			{
				InParent->Children.Add(InObject.Get());
			}
		}
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
