#pragma once

namespace FW
{
	enum class FrameWorkVer
	{
		Initial,
	};

	inline FrameWorkVer GFrameWorkVer = FrameWorkVer::Initial;

	class FRAMEWORK_API ShObject
	{
		REFLECTION_TYPE(ShObject)
	public:
		ShObject();
		virtual ~ShObject() = default;

		FGuid GetGuid() const { return Guid; }

		//Use the serialization system from unreal engine
		virtual void Serialize(FArchive& Ar);

	public:
		FText ObjectName;

	protected:
		FGuid Guid;
	};

	class FRAMEWORK_API ShObjectOp
	{
		REFLECTION_TYPE(ShObjectOp)
	public:
		ShObjectOp() = default;
		virtual ~ShObjectOp() = default;

		virtual void OnSelect(ShObject* InObject) {}
	};
}