#pragma once
#include "SceneObject.h"
#include "RenderResource/Camera.h"

namespace SH
{
	class CameraSceneObject : public SceneObject
	{
		REFLECTION_TYPE(CameraSceneObject)
	public:
		CameraSceneObject();

		void Serialize(FArchive& Ar) override;

		FW::Camera ToCamera(float AspectRatio) const;

		bool bOrthographic = false;
		float OrthoSize = 10.0f;
		float VerticalFov = 60.0f;
		float NearPlane = 0.1f;
		float FarPlane = 1000.0f;
	};
}
