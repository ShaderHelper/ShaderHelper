#include "CommonHeader.h"
#include "MeshSceneObject.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"

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
		Ar << VertexCount;
	}

	TArray<TSharedRef<PropertyData>> MeshSceneObject::GeneratePropertyDatas()
	{
		TArray<TSharedRef<PropertyData>> Result = ShObject::GeneratePropertyDatas();
		if (!ModelAsset)
		{
			Result.Add(MakeShared<PropertyScalarItem<uint32>>(this, LOCALIZATION("VertexCount"), &VertexCount));
		}
		return Result;
	}

	void MeshSceneObject::PostPropertyChanged(PropertyData* InProperty)
	{
		SceneObject::PostPropertyChanged(InProperty);
		if (InProperty->GetDisplayName().EqualTo(LOCALIZATION("Model")))
		{
			GApp->GetEditor()->RefreshProperty();
		}
	}
}
