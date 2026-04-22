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

		FW::AssetPtr<FW::Model> ModelAsset;
	};
}
