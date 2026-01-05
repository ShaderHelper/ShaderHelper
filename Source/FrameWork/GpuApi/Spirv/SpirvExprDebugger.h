#pragma once
#include "SpirvPixelDebugger.h"

namespace FW
{
	struct SpvPixelExprDebuggerContext : SpvPixelDebuggerContext
	{
		SpvPixelExprDebuggerContext(const SpvDebugState& InStopDebugState, int32 InStopDebugStateIndex, const Vector2u& InCoord, const TArray<SpvBinding>& InBindings)
		: SpvPixelDebuggerContext{ InCoord, InBindings }
		, StopDebugState(InStopDebugState), StopDebugStateIndex(InStopDebugStateIndex)
		{}

		SpvDebugState StopDebugState;
		int32 StopDebugStateIndex;
	};

	class FRAMEWORK_API SpvPixelExprDebuggerVisitor : public SpvPixelDebuggerVisitor
	{
	public:
		SpvPixelExprDebuggerVisitor(SpvPixelExprDebuggerContext& InContext, GpuShaderLanguage InLanguage, bool InEnableUbsan) : SpvPixelDebuggerVisitor(InContext, InLanguage, InEnableUbsan) {}
		void Visit(const SpvDebugDeclare* Inst);

	protected:
		void ParseInternal() override;
		void PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList) override;
		void PatchBaseTypeAppendExprFunc();
		void PatchAppendExprFunc(const SpvType* Type, const SpvTypeDesc* TypeDesc);
		void PatchAppendExprDummyFunc();
		void AppendExprDummy(const TFunction<int32()>& OffsetEval);

	private:
		TMap<const SpvType*, SpvId> AppendExprFuncIds;
		SpvId DebugStateNum;
		SpvId DoAppendExpr;
		SpvId AppendExprDummyFuncId;
		const SpvDebugLine* StopInst{};
	};
}
