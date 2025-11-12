#include "CommonHeader.h"
#include "SpirvExprDebugger.h"

namespace FW
{
	void SpvExprDebuggerVisitor::Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections)
	{
		int InstIndex = 0;
		while (InstIndex < Insts.Num())
		{
			Insts[InstIndex]->Accept(this);
			InstIndex++;
		}
	}

	void SpvExprDebuggerVisitor::Visit(const SpvDebugDeclare* Inst)
	{
		SpvVariableDesc* VarDesc = &Context.VariableDescs[Inst->GetVarDesc()];
		if (VarDesc->Name == "__Expression_Result")
		{
			Context.ResultTypeDesc = VarDesc->TypeDesc;
		}
	}
}
