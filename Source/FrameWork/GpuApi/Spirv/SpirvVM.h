#pragma once
#include "SpirvInstruction.h"

namespace FW
{
	struct SpvVmContext
	{
		
	};

	class SpvVmVisitor : public SpvVisitor
	{
	public:
		SpvVmVisitor(SpvVmContext& InContext) : Context(InContext)
		{}
		
	public:
		void Visit(SpvOpString* Inst) override;

	private:
		SpvVmContext& Context;
	};
}
