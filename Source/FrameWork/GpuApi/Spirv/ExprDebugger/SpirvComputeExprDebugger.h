#pragma once
#include "SpirvExprDebuggerBase.h"

namespace FW
{
	class SpvComputeExprDebuggerVisitor
		: public SpvExprDebuggerVisitorImpl<SpvComputeDebuggerVisitor, SpvComputeExprDebuggerContext>
	{
	public:
		using SpvExprDebuggerVisitorImpl::SpvExprDebuggerVisitorImpl;

	protected:
		void PatchInvocationGate(TArray<TUniquePtr<SpvInstruction>>& InstList) override
		{
			SpvComputeDebuggerVisitor::PatchActiveCondition(InstList);
			PatchSelectedThreadCheck(InstList);
		}

		void PatchSelectedThreadCheck(TArray<TUniquePtr<SpvInstruction>>& InstList)
		{
			auto& ExprContext = static_cast<SpvComputeExprDebuggerContext&>(Context);
			SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());

			SpvId LoadedLocalIndex = Patcher.NewId();
			{
				auto Op = MakeUnique<SpvOpLoad>(UIntType, LocalInvocationIndexVar);
				Op->SetId(LoadedLocalIndex);
				InstList.Add(MoveTemp(Op));
			}

			SpvId IEqual = Patcher.NewId();
			{
				auto Op = MakeUnique<SpvOpIEqual>(BoolType, LoadedLocalIndex,
					Patcher.FindOrAddConstant(ExprContext.SelectedLocalInvocationIndex));
				Op->SetId(IEqual);
				InstList.Add(MoveTemp(Op));
			}

			auto TrueLabel = Patcher.NewId();
			auto FalseLabel = Patcher.NewId();
			InstList.Add(MakeUnique<SpvOpSelectionMerge>(TrueLabel, SpvSelectionControl::None));
			InstList.Add(MakeUnique<SpvOpBranchConditional>(IEqual, TrueLabel, FalseLabel));
			auto FalseLabelOp = MakeUnique<SpvOpLabel>();
			FalseLabelOp->SetId(FalseLabel);
			InstList.Add(MoveTemp(FalseLabelOp));
			InstList.Add(MakeUnique<SpvOpReturn>());
			auto TrueLabelOp = MakeUnique<SpvOpLabel>();
			TrueLabelOp->SetId(TrueLabel);
			InstList.Add(MoveTemp(TrueLabelOp));
		}
	};
}
