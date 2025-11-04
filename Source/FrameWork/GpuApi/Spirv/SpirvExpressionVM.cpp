#include "CommonHeader.h"
#include "SpirvExpressionVM.h"

namespace FW
{
	SpvVmPixelExprVisitor::SpvVmPixelExprVisitor(SpvVmPixelExprContext& InPixelExprContext)
	: SpvVmPixelVisitor(InPixelExprContext)
	, PixelExprContext(InPixelExprContext)
	{}

	void SpvVmPixelExprVisitor::Visit(const SpvDebugDeclare* Inst)
	{
		SpvVmVisitor::Visit(Inst);
		
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();

		SpvVariableDesc* VarDesc = &Context.VariableDescs[Inst->GetVarDesc()];
		if(VarDesc->Name == "__Expression_Result")
		{
			PixelExprContext.ResultTypeDesc = VarDesc->TypeDesc;
			PixelExprContext.ResultValue = GetPointerValue(&Context, CurStackFrame.Arguments[0]);
			bTerminate = true;
		}
	}

	void SpvVmPixelExprVisitor::Visit(const SpvOpStore* Inst)
	{
		SpvPointer* Pointer = GetPointer(Inst->GetPointer());
		if(!Pointer->Pointee->InitializedRanges.IsEmpty())
		{
			PixelExprContext.HasSideEffect = true;
		}
		
		SpvVmVisitor::Visit(Inst);
	}
}
