#pragma once

#include "Common/Util/Math.h"

namespace FW
{
	struct Camera
	{
		Vector3f Position = { 0.0f, 0.0f, 0.0f };
		float Yaw = 0.0f;
		float Pitch = 0.0f;
		float Roll = 0.0f;
		float VerticalFov = FMath::DegreesToRadians(60.0f);
		float AspectRatio = 1.0f;
		float NearPlane = 0.1f;
		float FarPlane = 1000.0f;

		FMatrix44f GetWorldRotationMatrix() const
		{
			return RotationMatrix(Yaw, Pitch, Roll);
		}

		FMatrix44f GetWorldMatrix() const
		{
			return GetWorldRotationMatrix() * FTranslationMatrix44f(Position);
		}

		FMatrix44f GetViewMatrix() const
		{
			return GetWorldMatrix().Inverse();
		}

		FMatrix44f GetProjectionMatrix() const
		{
			const float SafeVerticalFov = FMath::Clamp(VerticalFov, 0.01f, PI - 0.01f);
			const float SafeAspectRatio = FMath::Max(AspectRatio, 0.01f);
			const float HalfFovY = SafeVerticalFov * 0.5f;

			return FPerspectiveMatrix44f(
				HalfFovY,
				HalfFovY,
				1.0f / SafeAspectRatio,
				1.0f,
				NearPlane,
				FarPlane
			);
		}

		FMatrix44f GetViewProjectionMatrix() const
		{
			return GetViewMatrix() * GetProjectionMatrix();
		}
	};
}
