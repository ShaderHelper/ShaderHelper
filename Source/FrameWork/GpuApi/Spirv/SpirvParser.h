#pragma once
#include "SpirvInstruction.h"

namespace FW
{
	//Info before logical layout section 10
	struct SpvMetaContext
	{
		Vector3u ThreadGroupSize{};
		
		SpvId EntryPoint;
		TMap<SpvId, FString> DebugStrs;
		TMap<SpvId, TUniquePtr<SpvType>> Types;
		TMap<SpvId, SpvObject> Constants;
		TMap<SpvId, SpvPointer> GlobalPointers;
		TMap<SpvId, SpvVariable> GlobalVariables;
		TMultiMap<SpvId, SpvDecoration> Decorations;		//GlobalVarId -> Decoration
		TMap<SpvId, SpvTypeDesc> TypeDescs;
		TMap<SpvId, SpvVariableDesc> VariableDescs;
		TMap<SpvId, SpvVariableDesc*> GlobalVariableDescMap;   //GlobalVarId -> Desc
		TMap<SpvId, TUniquePtr<SpvLexicalScope>> LexicalScopes;
	};

	class FRAMEWORK_API SpvMetaVisitor : public SpvVisitor
	{
	public:
		SpvMetaVisitor(SpvMetaContext& InContext) : Context(InContext)
		{}
		
	public:
		void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts) override;
		
		void Visit(SpvOpVariable* Inst) override;
		void Visit(SpvOpTypeVoid* Inst) override;
		void Visit(SpvOpTypeFloat* Inst) override;
		void Visit(SpvOpTypeInt* Inst) override;
		void Visit(SpvOpTypeVector* Inst) override;
		void Visit(SpvOpTypeBool* Inst) override;
		void Visit(SpvOpTypePointer* Inst) override;
		void Visit(SpvOpTypeStruct* Inst) override;
 
		void Visit(SpvOpDecorate* Inst) override;
		void Visit(SpvOpExecutionMode* Inst) override;
		void Visit(SpvOpString* Inst) override;
		void Visit(SpvOpConstant* Inst) override;
		void Visit(SpvOpConstantTrue* Inst) override;
		void Visit(SpvOpConstantFalse* Inst) override;
		void Visit(SpvOpConstantComposite* Inst) override;
		
		void Visit(SpvDebugCompilationUnit* Inst) override;
		void Visit(SpvDebugFunction* Inst) override;
		void Visit(SpvDebugLocalVariable* Inst) override;
		void Visit(SpvDebugGlobalVariable* Inst) override;
 
	private:
		SpvMetaContext& Context;
	};

	class FRAMEWORK_API SpirvParser
	{
	public:
		void Parse(const TArray<uint32>& SpvCode);
		void Accept(SpvVisitor* Visitor);
		
	protected:
		TMap<SpvId, SpvExtSet> ExtSets;
		TArray<TUniquePtr<SpvInstruction>> Insts;
	};
	
}
