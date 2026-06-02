#pragma once
#include "../SpirvVertexDebugger.h"
#include "SpirvExprDebuggerBase.h"

namespace FW
{
	struct SpvVertexExprDebuggerContext : SpvVertexDebuggerContext
	{
		SpvVertexExprDebuggerContext(const SpvDebugState& InStopDebugState, int32 InStopDebugStateIndex,
			uint32 InTargetVertexIndex, uint32 InTargetInstanceIndex, const TArray<SpvBinding>& InBindings)
			: SpvVertexDebuggerContext{ InTargetVertexIndex, InTargetInstanceIndex, InBindings }
			, StopDebugState(InStopDebugState), StopDebugStateIndex(InStopDebugStateIndex)
		{}

		SpvDebugState StopDebugState;
		int32 StopDebugStateIndex;
	};

	class SpvVertexExprDebuggerVisitor
		: public SpvExprDebuggerVisitorImpl<SpvVertexDebuggerVisitor, SpvVertexExprDebuggerContext>
	{
	public:
		using SpvExprDebuggerVisitorImpl::SpvExprDebuggerVisitorImpl;

	protected:
		void PatchInvocationGate(TArray<TUniquePtr<SpvInstruction>>& InstList) override
		{
			SpvVertexDebuggerVisitor::PatchActiveCondition(InstList);
		}
	};
}
