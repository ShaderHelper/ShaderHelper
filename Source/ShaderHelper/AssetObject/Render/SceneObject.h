#pragma once
#include "Common/FrameWorkCore.h"

namespace SH
{
	class SceneObject : public FW::ShObject
	{
		REFLECTION_TYPE(SceneObject)
	public:
		SceneObject();

		void Serialize(FArchive& Ar) override;
		void PostPropertyEdit(FW::PropertyData* InProperty, TArray<uint8>&& OldData) override;
		FMatrix44f GetWorldMatrix() const;

		SceneObject* GetParent() const { return Parent.Get(); }
		const TArray<SceneObject*>& GetChildren() const { return Children; }
		void SetParent(SceneObject* NewParent);

		FW::Vector3f Position = {0, 0, 0};
		FW::Vector3f Rotation = {0, 0, 0};  // Euler degrees: Pitch, Yaw, Roll
		FW::Vector3f Scale = {1, 1, 1};

		FW::ObserverObjectPtr<SceneObject> Parent;
		TArray<SceneObject*> Children;
	};
}
