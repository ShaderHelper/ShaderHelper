#pragma once
#include "SpirvInstruction.h"

namespace FW
{
	//Info before logical layout section 10
	struct SpvMetaContext
	{
		Vector3u ThreadGroupSize{};
		
		SpvId EntryPoint;
		//Can not use Tmap, because its underlying impl relies on TArray,
		//which may ivalidate references to existing elements upon insertion.
		//Therefore, we replace it with std::unordered_map
		std::unordered_map<SpvId, FString> DebugStrs;
		std::unordered_map<SpvId, TUniquePtr<SpvType>> Types;
		std::unordered_map<SpvId, SpvObject> Constants;
		std::unordered_map<SpvId, SpvPointer> GlobalPointers;
		std::unordered_map<SpvId, SpvVariable> GlobalVariables;
		TMultiMap<SpvId, SpvDecoration> Decorations;		//GlobalVarId -> Decoration
		std::unordered_map<SpvId, TUniquePtr<SpvTypeDesc>> TypeDescs;
		std::unordered_map<SpvId, SpvVariableDesc> VariableDescs;
		std::unordered_map<SpvId, SpvVariableDesc*> VariableDescMap;   //VarId -> Desc
		std::unordered_map<SpvId, TUniquePtr<SpvLexicalScope>> LexicalScopes;
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
		void Visit(SpvOpTypeArray* Inst) override;
		void Visit(SpvOpDecorate* Inst) override;
		void Visit(SpvOpExecutionMode* Inst) override;
		void Visit(SpvOpString* Inst) override;
		void Visit(SpvOpConstant* Inst) override;
		void Visit(SpvOpConstantTrue* Inst) override;
		void Visit(SpvOpConstantFalse* Inst) override;
		void Visit(SpvOpConstantComposite* Inst) override;
		void Visit(SpvOpConstantNull* Inst) override;
		
		void Visit(SpvDebugTypeBasic* Inst) override;
		void Visit(SpvDebugTypeVector* Inst) override;
		void Visit(SpvDebugTypeComposite* Inst) override;
		void Visit(SpvDebugTypeMember* Inst) override;
		void Visit(SpvDebugTypeArray* Inst) override;
		void Visit(SpvDebugTypeFunction* Inst) override;
		void Visit(SpvDebugCompilationUnit* Inst) override;
		void Visit(SpvDebugLexicalBlock* Inst) override;
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
