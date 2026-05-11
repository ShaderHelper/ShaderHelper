#include "CommonHeader.h"
#include "SceneObject.h"
#include "Common/Util/Math.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/Scene/SSceneView.h"

#include <Serialization/MemoryWriter.h>

using namespace FW;

static void DecomposeMatrix(const FMatrix44f& Mat, Vector3f& OutPos, Vector3f& OutRot, Vector3f& OutScale)
{
	const FVector3f Origin = Mat.GetOrigin();
	OutPos = Vector3f(Origin.X, Origin.Y, Origin.Z);

	const FVector3f ScaleVec = Mat.GetScaleVector();
	OutScale = Vector3f(ScaleVec.X, ScaleVec.Y, ScaleVec.Z);

	// Strip scale and translation, leaving the pure rotation in project space
	FMatrix44f ProjRotMat = Mat.GetMatrixWithoutScale();
	ProjRotMat.SetOrigin(FVector3f::ZeroVector);

	// Convert project-space rotation to UE space and extract Pitch/Yaw/Roll
	const FRotator3f UeRotator = FQuat4f(ProjectToUeBasis * ProjRotMat * UeToProjectBasis).Rotator();
	OutRot = Vector3f(UeRotator.Pitch, UeRotator.Yaw, UeRotator.Roll);
}

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
		Ar << Parent;
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

		if (SceneViewWidget->GetRender())
		{
			SceneViewWidget->GetUndoManager().PushCommand(MakeShared<PropertyChangeCommand>(SceneViewWidget, this, MoveTemp(OldData), MoveTemp(NewData)));
		}
	}

	void SceneObject::SetParent(SceneObject* NewParent)
	{
		if (NewParent == Parent.Get())
		{
			return;
		}

		const FMatrix44f WorldMat = GetWorldMatrix();

		// Detach from old parent
		if (Parent.IsValid())
		{
			Parent->Children.Remove(this);
			Parent.Reset();
		}

		// Attach to new parent
		if (NewParent)
		{
			Parent = NewParent;
			NewParent->Children.Add(this);

			const FMatrix44f NewLocalMat = WorldMat * NewParent->GetWorldMatrix().Inverse();
			DecomposeMatrix(NewLocalMat, Position, Rotation, Scale);
		}
		else
		{
			DecomposeMatrix(WorldMat, Position, Rotation, Scale);
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

		FMatrix44f LocalMatrix = ScaleMat * RotMat * TransMat;
		if (Parent.IsValid())
		{
			return LocalMatrix * Parent->GetWorldMatrix();
		}
		return LocalMatrix;
	}
}
