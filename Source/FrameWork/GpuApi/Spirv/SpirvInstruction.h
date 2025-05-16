#pragma once
#include "SpirvCommon.h"

namespace FW
{
	class SpvInstruction;

	class SpvVisitor
	{
	public:
		virtual void Visit(SpvInstruction*) {}
		virtual void Visit(class SpvExecutionModeOp* Inst) {}
	};

	class SpvInstruction
	{
	public:
		SpvInstruction(SpvOp InOpCode)
			: OpCode(InOpCode)
		{}
		virtual ~SpvInstruction() = default;
		
	public:
		void SetId(SpvId Id) { ResultId = Id; }
		void SetSourceLocation(const Vector2u& InSrcLoc) { SrcLoc = InSrcLoc; }
		virtual void Accpet(const TArray<SpvVisitor*>& Visitors) = 0;
		
	private:
		SpvOp OpCode;
		SpvId ResultId = 0;
		Vector2u SrcLoc{};
	};

	template<typename T>
	class SpvInstructionBase : public SpvInstruction
	{
	public:
		using SpvInstruction::SpvInstruction;
		void Accpet(const TArray<SpvVisitor*>& Visitors) override
		{
			for(SpvVisitor* Visitor : Visitors)
			{
				Visitor->Visit(static_cast<T*>(this));
			}
		}
	};

	using ExtraOperands = TArray<uint32, TInlineAllocator<4>>;

	class SpvExecutionModeOp : public SpvInstructionBase<SpvExecutionModeOp>
	{
	public:
		SpvExecutionModeOp(SpvId InEntryPoint, SpvExecutionMode InMode,
						   const ExtraOperands& InOperands)
			: SpvInstructionBase(SpvOp::ExecutionMode)
			, EntryPoint(InEntryPoint)
			, Mode(InMode)
			, Operands(InOperands)
		{}
	public:
		SpvExecutionMode GetMode() const { return Mode; }
		const ExtraOperands& GetExtraOperands() const { return Operands; }
		
	private:
		SpvId EntryPoint;
		SpvExecutionMode Mode;
		ExtraOperands Operands;
	};
}
