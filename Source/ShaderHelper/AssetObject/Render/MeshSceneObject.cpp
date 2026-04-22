#include "CommonHeader.h"
#include "MeshSceneObject.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<MeshSceneObject>("MeshSceneObject")
		.BaseClass<SceneObject>()
		.Data<&MeshSceneObject::ModelAsset, MetaInfo::Property>(LOCALIZATION("Model"))
	)

	MeshSceneObject::MeshSceneObject()
	{
		ObjectName = FText::FromString("Mesh");
	}

	void MeshSceneObject::Serialize(FArchive& Ar)
	{
		SceneObject::Serialize(Ar);
		Ar << ModelAsset;
	}
}
