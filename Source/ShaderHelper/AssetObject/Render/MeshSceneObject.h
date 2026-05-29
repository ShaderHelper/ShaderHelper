#pragma once
#include "SceneObject.h"
#include "AssetObject/Model.h"
#include "AssetManager/AssetManager.h"

namespace SH
{
	class MeshSceneObject : public SceneObject
	{
		REFLECTION_TYPE(MeshSceneObject)
	public:
		MeshSceneObject();

		void Serialize(FArchive& Ar) override;
		TArray<TSharedRef<FW::PropertyData>> GeneratePropertyDatas() override;
		void PostPropertyChanged(FW::PropertyData* InProperty) override;

		FW::AssetPtr<FW::Model> ModelAsset;
		uint32 VertexCount = 3;
		uint32 InstanceCount = 1;
	};
}
