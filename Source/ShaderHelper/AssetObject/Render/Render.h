#pragma once
#include "AssetObject/Graph.h"
#include "SceneObject.h"

namespace SH
{
	class CameraSceneObject;

	enum class RenderFormat
	{
		B8G8R8A8_UNORM = (int)FW::GpuFormat::B8G8R8A8_UNORM,
		R32G32B32A32_FLOAT = (int)FW::GpuFormat::R32G32B32A32_FLOAT
	};

	class Render : public FW::Graph
	{
		REFLECTION_TYPE(Render)
	public:
		Render() = default;

	public:
		FString FileExtension() const override;
		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		void OnDragEnter(TSharedPtr<FDragDropOperation> DragDropOp) override;
		void OnDrop(TSharedPtr<FDragDropOperation> DragDropOp, const FW::Vector2D& Pos) override;

		template<typename T>
		FW::ObjectPtr<T> AddSceneObject(SceneObject* InParent = nullptr)
		{
			auto Obj = FW::NewShObject<T>(this);
			if (InParent)
			{
				Obj->Parent = InParent;
				InParent->Children.Add(Obj.Get());
			}
			SceneObjects.Add(Obj);
			MarkDirty();
			return Obj;
		}

		void RemoveSceneObject(SceneObject* InObject);
		void InsertSceneObject(int32 Index, FW::ObjectPtr<SceneObject> InObject, SceneObject* InParent = nullptr, int32 ParentChildIndex = INDEX_NONE);

		TArray<FW::ObjectPtr<SceneObject>> SceneObjects;
		FW::ObserverObjectPtr<FW::ShObject> SelectedMeshRenderObject;
		FW::ObserverObjectPtr<CameraSceneObject> PreviewCamera;
	};
}
