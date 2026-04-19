#pragma once
#include "AssetObject/Graph.h"
#include "SceneObject.h"

namespace SH
{
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
		~Render();

	public:
		FString FileExtension() const override;
		void Serialize(FArchive& Ar) override;
		void OnDragEnter(TSharedPtr<FDragDropOperation> DragDropOp) override;
		void OnDrop(TSharedPtr<FDragDropOperation> DragDropOp, const FW::Vector2D& Pos) override;

		template<typename T>
		FW::ObjectPtr<T> AddSceneObject()
		{
			auto Obj = FW::NewShObject<T>(this);
			SceneObjects.Add(Obj);
			MarkDirty();
			return Obj;
		}

		void RemoveSceneObject(SceneObject* InObject);

		TArray<FW::ObjectPtr<SceneObject>> SceneObjects;
	};
}
