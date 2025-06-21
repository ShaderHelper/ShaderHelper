#pragma once
#include "SpirvParser.h"

namespace FW
{
	struct SpvVariableChange
	{
		
	};

	struct SpvLexicalScopeChange
	{
		
	};

	struct SpvDebugState
	{
		uint32 LineNumber;
		SpvLexicalScope* Scope = nullptr;
		SpvId PreviousBB;
		TArray<SpvFunctionDesc*> CallStack;
		SpvLexicalScopeChange ScopeChange;
		TArray<SpvVariableChange> VarChanges;
	};

	struct ThreadState
	{
		TArray<SpvDebugState> DebugStates;
		TMap<SpvVariableDesc*, SpvVariable*> VariableDescMap;
		TMap<SpvFunctionDesc*, SpvFunction*> FunctionDescMap;
	};

	struct SpvVmContext : SpvMetaContext
	{
		const int32 CurActiveIndex;
		ThreadState Quad[4];
	};

	class SpvVmVisitor : public SpvVisitor
	{
	public:
		SpvVmVisitor(SpvVmContext& InContext) : Context(InContext)
		{}
		
	public:
		void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts) override;
		
		void Visit(SpvDebugLine* Inst) override;

	private:
		SpvVmContext& Context;
	};
}
