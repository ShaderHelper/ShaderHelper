#pragma once
#include "SpirvExprDebuggerBase.h"

namespace FW
{
	class SpvPixelExprDebuggerVisitor
		: public SpvExprDebuggerVisitorImpl<SpvPixelDebuggerVisitor, SpvPixelExprDebuggerContext>
	{
	public:
		using SpvExprDebuggerVisitorImpl::SpvExprDebuggerVisitorImpl;

        void PatchInvocationGate(TArray<TUniquePtr<SpvInstruction>>& InstList) override
		{
			SpvPixelDebuggerVisitor::PatchActiveCondition(InstList);
		}
	};
}
