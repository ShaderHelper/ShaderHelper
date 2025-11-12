#pragma once
#include "SpirvParser.h"

namespace FW
{
	struct SpvExprDebuggerContext : SpvMetaContext
	{
		SpvTypeDesc* ResultTypeDesc{};
	};

	class FRAMEWORK_API SpvExprDebuggerVisitor : public SpvVisitor
	{
	public:
		SpvExprDebuggerVisitor(SpvExprDebuggerContext& InContext) : Context(InContext) {}
	public:
		void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections) override;
		void Visit(const SpvDebugDeclare* Inst) override;

	protected:
		SpvExprDebuggerContext& Context;
	};
}
