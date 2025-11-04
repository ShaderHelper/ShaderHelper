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
		std::unordered_map<SpvId, FString> Names; //VarId/TypeId etc. -> Name
		std::unordered_map<SpvId, TUniquePtr<SpvType>> Types;
		std::unordered_map<SpvId, SpvObject> Constants;
		std::unordered_map<SpvId, SpvPointer> GlobalPointers;
		std::unordered_map<SpvId, SpvVariable> GlobalVariables;
		TMultiMap<SpvId, SpvDecoration> Decorations;		//GlobalVarId/TypeId -> Decoration
		std::unordered_map<SpvId, TUniquePtr<SpvTypeDesc>> TypeDescs;
		std::unordered_map<SpvId, SpvVariableDesc> VariableDescs;
		std::unordered_map<SpvId, SpvVariableDesc*> VariableDescMap;   //VarId -> Desc
		std::unordered_map<SpvId, TUniquePtr<SpvLexicalScope>> LexicalScopes;

		TMap<SpvSectionKind, SpvSection> Sections;
	};

	class FRAMEWORK_API SpvMetaVisitor : public SpvVisitor
	{
	public:
		SpvMetaVisitor(SpvMetaContext& InContext) : Context(InContext)
		{}
		SpvMetaContext& GetContext() { return Context; }
		
	public:
		void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections) override;
		
		void Visit(const SpvOpVariable* Inst) override;
		void Visit(const SpvOpTypeVoid* Inst) override;
		void Visit(const SpvOpTypeFloat* Inst) override;
		void Visit(const SpvOpTypeInt* Inst) override;
		void Visit(const SpvOpTypeVector* Inst) override;
		void Visit(const SpvOpTypeMatrix* Inst) override;
		void Visit(const SpvOpTypeBool* Inst) override;
		void Visit(const SpvOpTypePointer* Inst) override;
		void Visit(const SpvOpTypeStruct* Inst) override;
		void Visit(const SpvOpTypeImage* Inst) override;
		void Visit(const SpvOpTypeSampler* Inst) override;
		void Visit(const SpvOpTypeArray* Inst) override;
		void Visit(const SpvOpTypeRuntimeArray* Inst) override;
		void Visit(const SpvOpDecorate* Inst) override;
		void Visit(const SpvOpMemberDecorate* Inst) override;
		void Visit(const SpvOpExecutionMode* Inst) override;
		void Visit(const SpvOpName* Inst) override;
		void Visit(const SpvOpString* Inst) override;
		void Visit(const SpvOpConstant* Inst) override;
		void Visit(const SpvOpConstantTrue* Inst) override;
		void Visit(const SpvOpConstantFalse* Inst) override;
		void Visit(const SpvOpConstantComposite* Inst) override;
		void Visit(const SpvOpConstantNull* Inst) override;
		
		void Visit(const SpvDebugTypeBasic* Inst) override;
		void Visit(const SpvDebugTypeVector* Inst) override;
		void Visit(const SpvDebugTypeMatrix* Inst) override;
		void Visit(const SpvDebugTypeComposite* Inst) override;
		void Visit(const SpvDebugTypeMember* Inst) override;
		void Visit(const SpvDebugTypeArray* Inst) override;
		void Visit(const SpvDebugTypeFunction* Inst) override;
		void Visit(const SpvDebugCompilationUnit* Inst) override;
		void Visit(const SpvDebugLexicalBlock* Inst) override;
		void Visit(const SpvDebugFunction* Inst) override;
		void Visit(const SpvDebugInlinedAt* Inst) override;
		void Visit(const SpvDebugLocalVariable* Inst) override;
		void Visit(const SpvDebugGlobalVariable* Inst) override;
 
	private:
		SpvMetaContext& Context;
	};

	class FRAMEWORK_API SpirvParser : FNoncopyable
	{
	public:
		void Parse(const TArray<uint32>& InSpvCode);
		void Accept(SpvVisitor* Visitor);
		
	protected:
		TMap<SpvId, SpvExtSet> ExtSets;
		TArray<TUniquePtr<SpvInstruction>> Insts;
		TArray<uint32> SpvCode;
		TMap<SpvSectionKind, SpvSection> Sections;
	};
	
}
