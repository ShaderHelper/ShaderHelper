#pragma once
#include "SpirvDebugger.h"

namespace FW
{
	struct SpvComputeDebuggerContext : SpvDebuggerContext
	{
		SpvComputeDebuggerContext(const Vector3u& InTargetGroupId, const Vector3u& InThreadGroupSize, const TArray<SpvBinding>& InBindings)
			: SpvDebuggerContext{ .Bindings = InBindings }
			, TargetWorkGroupId(InTargetGroupId)
			, ThreadGroupSize(InThreadGroupSize)
		{}

		Vector3u TargetWorkGroupId;
		Vector3u ThreadGroupSize;
	};

	class FRAMEWORK_API SpvComputeDebuggerVisitor : public SpvDebuggerVisitor
	{
	public:
		SpvComputeDebuggerVisitor(SpvComputeDebuggerContext& InComputeContext, GpuShaderLanguage InLanguage, bool InEnableUbsan);

	public:
		void Visit(const SpvOpControlBarrier* Inst) override;

	protected:
		bool PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList) override;
		void ParseInternal() override;

		// Atomically reserve a slot in the shared debug buffer and emit a per-record prefix
		// uvec4(LocalInvocationIndex, PayloadVec4Count, 0, 0) so the CPU can demux records from
		// every thread of the target workgroup, followed by the payload uvec4s.
		void BatchStoreToDebugBuffer(const TArray<SpvId>& UIntValues, TArray<TUniquePtr<SpvInstruction>>& InstList) override;

	protected:
		SpvId DebuggerParams = 0;
		SpvId WorkgroupIdVar = 0;
		SpvId LocalInvocationIndexVar = 0;
	};
}
