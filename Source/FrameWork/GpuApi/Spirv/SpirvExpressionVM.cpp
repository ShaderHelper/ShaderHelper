#include "CommonHeader.h"
#include "SpirvExpressionVM.h"

namespace FW
{
	SpvVmPixelExprVisitor::SpvVmPixelExprVisitor(SpvVmPixelExprContext& InPixelExprContext)
	: SpvVmPixelVisitor(InPixelExprContext)
	, PixelExprContext(InPixelExprContext)
	{}

	void SpvVmPixelExprVisitor::Visit(SpvDebugDeclare* Inst)
	{
		SpvVmVisitor::Visit(Inst);
		
		SpvVmContext& Context = GetActiveContext();
		SpvVariableDesc* VarDesc = &Context.VariableDescs[Inst->GetVarDesc()];
		SpvPointer* Pointer = GetPointer(Inst->GetPointer());
		if(VarDesc->Name == "__Expression_Result")
		{
			PixelExprContext.ResultTypeDesc = VarDesc->TypeDesc;
			PixelExprContext.ResultValue = GetPointerValue(&Context, Pointer);
			bTerminate = true;
		}
	}

	void SpvVmPixelExprVisitor::Visit(SpvOpStore* Inst)
	{
		SpvPointer* Pointer = GetPointer(Inst->GetPointer());
		if(Pointer->Pointee->Initialized)
		{
			PixelExprContext.HasSideEffect = true;
		}
		
		SpvVmVisitor::Visit(Inst);
	}
}
