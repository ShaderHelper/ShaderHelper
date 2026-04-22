#include "CommonHeader.h"
#include "SceneObject.h"
#include "Common/Util/Math.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/Scene/SSceneView.h"

#include <Serialization/MemoryWriter.h>

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<SceneObject>("SceneObject")
		.BaseClass<ShObject>()
		.Data<&SceneObject::Position, MetaInfo::Property>(LOCALIZATION("Position"))
		.Data<&SceneObject::Rotation, MetaInfo::Property>(LOCALIZATION("Rotation"))
		.Data<&SceneObject::Scale, MetaInfo::Property>(LOCALIZATION("Scale"))
	)

	SceneObject::SceneObject()
	{
		ObjectName = FText::FromString("SceneObject");
	}

	void SceneObject::Serialize(FArchive& Ar)
	{
		ShObject::Serialize(Ar);
		Ar << Position.X << Position.Y << Position.Z;
		Ar << Rotation.X << Rotation.Y << Rotation.Z;
		Ar << Scale.X << Scale.Y << Scale.Z;
	}

	void SceneObject::PostPropertyEdit(PropertyData* InProperty, TArray<uint8>&& OldData)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SSceneView* SceneViewWidget = ShEditor->GetSceneView();
		
		TArray<uint8> NewData;
		FMemoryWriter Ar(NewData);
		Serialize(Ar);
		if (OldData == NewData)
		{
			return;
		}

		if (auto* Mgr = SceneViewWidget->GetUndoManager())
		{
			Mgr->PushCommand(MakeShared<PropertyChangeCommand>(SceneViewWidget, this, MoveTemp(OldData), MoveTemp(NewData)));
		}
	}

	FMatrix44f SceneObject::GetWorldMatrix() const
	{
		float PitchRad = FMath::DegreesToRadians(Rotation.X);
		float YawRad = FMath::DegreesToRadians(Rotation.Y);
		float RollRad = FMath::DegreesToRadians(Rotation.Z);

		FMatrix44f RotMat = RotationMatrix(YawRad, PitchRad, RollRad);
		FMatrix44f ScaleMat = FScaleMatrix44f(FVector3f(Scale.X, Scale.Y, Scale.Z));
		FMatrix44f TransMat = FTranslationMatrix44f(FVector3f(Position.X, Position.Y, Position.Z));

		return ScaleMat * RotMat * TransMat;
	}
}
