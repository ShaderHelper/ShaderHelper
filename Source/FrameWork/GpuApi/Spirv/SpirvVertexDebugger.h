#pragma once
#include "SpirvDebugger.h"

namespace FW
{
	struct SpvVertexCaptureContext : SpvDebuggerContext
	{
		SpvVertexCaptureContext(const TArray<SpvBinding>& InBindings)
			: SpvDebuggerContext{ .Bindings = InBindings }
		{}
	};

	// Patches a vertex shader so that, for every (instance, vertex), its clip-space SV_Position
	// (BuiltIn Position) is written into the debugger storage buffer at the flat index
	// gl_InstanceIndex * VertexCount + gl_VertexIndex (one uvec4 per entry).
	class FRAMEWORK_API SpvVertexCaptureVisitor : public SpvVisitor
	{
	public:
		SpvVertexCaptureVisitor(SpvVertexCaptureContext& InContext, uint32 InVertexCount)
			: Context(InContext)
			, VertexCount(InVertexCount)
		{}

		const SpvPatcher& GetPatcher() const { return Patcher; }
		void Parse(TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>&) override;

	protected:
		SpvVertexCaptureContext& Context;
		uint32 VertexCount = 0;   // per-instance vertex count, used to flatten (instance, vertex) indices
		SpvPatcher Patcher;
	};

	struct SpvVertexDebuggerContext : SpvDebuggerContext
	{
		SpvVertexDebuggerContext(uint32 InTargetVertexIndex, uint32 InTargetInstanceIndex, const TArray<SpvBinding>& InBindings)
			: SpvDebuggerContext{ .Bindings = InBindings }
			, TargetVertexIndex(InTargetVertexIndex)
			, TargetInstanceIndex(InTargetInstanceIndex)
		{}

		uint32 TargetVertexIndex = 0;
		uint32 TargetInstanceIndex = 0;
	};

	// Full step instrumentation that records the VS execution of a single target vertex of a single
	// instance (gl_VertexIndex == TargetVertexIndex && gl_InstanceIndex == TargetInstanceIndex); every
	// other invocation returns early.
	class FRAMEWORK_API SpvVertexDebuggerVisitor : public SpvDebuggerVisitor
	{
	public:
		SpvVertexDebuggerVisitor(SpvVertexDebuggerContext& InContext, GpuShaderLanguage InLanguage, bool InEnableUbsan);

	protected:
		void ParseInternal() override;
		bool PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList) override;

	protected:
		SpvId VertexIndexVar = 0;
		SpvId InstanceIndexVar = 0;
		SpvId DebuggerParams = 0;
	};
}
