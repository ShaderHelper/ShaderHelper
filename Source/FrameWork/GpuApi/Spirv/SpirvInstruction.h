#pragma once
#include "SpirvCore.h"
#include "SpirvExt.h"

namespace FW
{
	class SpvInstruction;

	class SpvVisitor
	{
	public:
		virtual void Visit(SpvInstruction*) {}
		
		virtual void Visit(class SpvOpExecutionMode* Inst) {}
		virtual void Visit(class SpvOpString* Inst) {}
		
		virtual void Visit(class SpvDebugLine* ExtInst) {}
	};

	class SpvInstruction
	{
	public:
		SpvInstruction() = default;
		virtual ~SpvInstruction() = default;
		
	public:
		void SetId(SpvId Id) { ResultId = Id; }
		void SetSourceLocation(const Vector2u& InSrcLoc) { SrcLoc = InSrcLoc; }
		virtual void Accpet(const TArray<SpvVisitor*>& Visitors) = 0;
		
	private:
		std::optional<SpvId> ResultId;
		std::optional<Vector2u> SrcLoc;
	};

	template<typename T>
	class SpvInstructionBase : public SpvInstruction
	{
	public:
		void Accpet(const TArray<SpvVisitor*>& Visitors) override
		{
			for(SpvVisitor* Visitor : Visitors)
			{
				Visitor->Visit(static_cast<T*>(this));
			}
		}
	};

	using ExtraOperands = TArray<uint32, TInlineAllocator<4>>;

	class SpvOpExecutionMode : public SpvInstructionBase<SpvOpExecutionMode>
	{
	public:
		SpvOpExecutionMode(SpvId InEntryPoint, SpvExecutionMode InMode,
						   const ExtraOperands& InOperands)
			: EntryPoint(InEntryPoint)
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

	class SpvOpString : public SpvInstructionBase<SpvOpString>
	{
	public:
		SpvOpString()
		{}
	};

	class SpvOpExtInst : public SpvInstructionBase<SpvOpExtInst>
	{
	public:
		SpvOpExtInst()
		{}
	};

	class SpvDebugLine : public SpvInstructionBase<SpvDebugLine>
	{
	public:
		SpvDebugLine()
		{}
	};
}
