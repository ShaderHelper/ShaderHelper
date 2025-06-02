#include "CommonHeader.h"
#include "SpirvParser.h"

namespace FW
{

	void SpvMetaVisitor::Visit(SpvOpTypeFloat* Inst)
	{
		
	}

	void SpvMetaVisitor::Visit(SpvOpExecutionMode* Inst)
	{
		if(Inst->GetMode() == SpvExecutionMode::LocalSize)
		{
			const ExtraOperands& Operands = Inst->GetExtraOperands();
			Context.ThreadGroupSize = {Operands[0], Operands[1], Operands[2]};
		}
	}

	void SpvMetaVisitor::Visit(SpvOpString* Inst)
	{
		Context.DebugStrs.Add(Inst->GetId().value(), Inst->GetStr());
	}

	void SpvMetaVisitor::Visit(SpvOpConstant* Inst)
	{
		
	}

	void ParseSpv(const TArray<uint32>& SpvCode, const TArray<SpvVisitor*>& Visitors, FString& OutErrorInfo)
	{
		int32 WordOffset = 5;
		while(WordOffset < SpvCode.Num())
		{
			uint32 Inst = SpvCode[WordOffset];
			int32 InstLen = int32(Inst >> 16);
			SpvOp OpCode = static_cast<SpvOp>(Inst & 0xffff);
			if(OpCode == SpvOp::ExecutionMode)
			{
				SpvId EntryPoint = SpvCode[WordOffset + 1];
				SpvExecutionMode Mode = static_cast<SpvExecutionMode>(SpvCode[WordOffset + 2]);
				ExtraOperands Operands = {&SpvCode[WordOffset + 3], InstLen - 3};
				SpvOpExecutionMode DecodedInst = {EntryPoint, Mode, Operands};
				DecodedInst.Accpet(Visitors);
			}
			else if(OpCode == SpvOp::String)
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				char* Str = (char*)&SpvCode[WordOffset + 2];
				int32 StrWordLen = InstLen - 2;
				SpvOpString DecodedInst = FString(StrWordLen * 4, Str);
				DecodedInst.SetId(ResultId);
				DecodedInst.Accpet(Visitors);
			}
			else if(OpCode == SpvOp::Constant)
			{
				SpvOpConstant DecodedInst;
			}
			else if(OpCode == SpvOp::ExtInst)
			{
				
			}
			WordOffset += InstLen;
		}
	}
}
