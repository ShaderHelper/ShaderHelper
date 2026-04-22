#include "CommonHeader.h"
#include "CameraSceneObject.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<CameraSceneObject>("CameraSceneObject")
		.BaseClass<SceneObject>()
		.Data<&CameraSceneObject::bOrthographic, MetaInfo::Property>(LOCALIZATION("Orthographic"))
		.Data<&CameraSceneObject::OrthoSize, MetaInfo::Property>(LOCALIZATION("OrthoSize"), {
			.Min = [](const CameraSceneObject*) { return 0.01f; },
			.Visibility = [](const CameraSceneObject* Obj) { return Obj->bOrthographic; }
		})
		.Data<&CameraSceneObject::VerticalFov, MetaInfo::Property>(LOCALIZATION("VerticalFov"), {
			.Min = [](const CameraSceneObject*) { return 1.0f; },
			.Max = [](const CameraSceneObject*) { return 179.0f; },
			.Visibility = [](const CameraSceneObject* Obj) { return !Obj->bOrthographic; }
		})
		.Data<&CameraSceneObject::NearPlane, MetaInfo::Property>(LOCALIZATION("NearPlane"), {
			.Min = [](const CameraSceneObject*) { return 0.001f; }
		})
		.Data<&CameraSceneObject::FarPlane, MetaInfo::Property>(LOCALIZATION("FarPlane"), {
			.Min = [](const CameraSceneObject*) { return 0.01f; }
		})
	)

	CameraSceneObject::CameraSceneObject()
	{
		ObjectName = FText::FromString("Camera");
		Position = {0, 2, -5};
	}

	void CameraSceneObject::Serialize(FArchive& Ar)
	{
		SceneObject::Serialize(Ar);
		Ar << bOrthographic;
		Ar << OrthoSize;
		Ar << VerticalFov;
		Ar << NearPlane;
		Ar << FarPlane;
	}

	Camera CameraSceneObject::ToCamera(float AspectRatio) const
	{
		Camera Cam;
		Cam.Position = Position;
		Cam.Yaw = FMath::DegreesToRadians(Rotation.Y);
		Cam.Pitch = FMath::DegreesToRadians(Rotation.X);
		Cam.Roll = FMath::DegreesToRadians(Rotation.Z);
		Cam.bOrthographic = bOrthographic;
		Cam.OrthoSize = OrthoSize;
		Cam.VerticalFov = FMath::DegreesToRadians(VerticalFov);
		Cam.AspectRatio = AspectRatio;
		Cam.NearPlane = NearPlane;
		Cam.FarPlane = FarPlane;
		return Cam;
	}
}
