#pragma once
#include "SpirvParser.h"

namespace FW
{
	struct SpvVariableChange
	{
		SpvId VarId;
		TArray<uint8> PreValue;
		TArray<uint8> NewValue;
		struct DirtyRange
		{
			int32 OffsetBytes;
			int32 ByteSize;
		} Range ;
	};

	struct SpvLexicalScopeChange
	{
		SpvLexicalScope* PreScope{};
		SpvLexicalScope* NewScope{};
	};

	struct SpvDebugState
	{
		int32 LineNumber{};
		SpvLexicalScopeChange ScopeChange;
		TArray<SpvVariableChange> VarChanges;
		FString UbError;
	};

	struct SpvRecordedInfo
	{
		TMap<SpvId, SpvVariable> AllVariables;
		TArray<SpvFunctionDesc*> CallStack;
		SpvLexicalScope* Scope = nullptr;
		
		TArray<SpvDebugState> LineDebugStates;
	};

	struct SpvVmBinding
	{
		int32 DescriptorSet;
		int32 Binding;
		
		GpuResource* Resource = nullptr;
	};

	struct SpvVmFrame
	{
		int32 ReturnPointIndex{};
		TArray<SpvPointer*> Arguments;
		SpvObject* ReturnObject = nullptr;
		SpvId CurBasicBlock{};
		SpvId PreBasicBlock{};
		SpvLexicalScope* CurScope = nullptr;
		int32 CurLineNumber{};
		TMap<SpvId, SpvObject> IntermediateObjects;
		TMap<SpvId, SpvPointer> Pointers;
		TMap<SpvId, SpvVariable> Variables;
	};

	struct SpvThreadState
	{
		int32 InstIndex;
		int32 NextInstIndex;
		TArray<SpvVmFrame> StackFrames;
		
		TMap<SpvBuiltIn, TArray<uint8>> BuiltInInput;
		TMap<uint32, TArray<uint8>> LocationInput;
		
		//Finalized information for external use
		SpvRecordedInfo RecordedInfo;
	};

	struct SpvVmContext : SpvMetaContext
	{
		SpvThreadState ThreadState;
	};

	class FRAMEWORK_API SpvVmVisitor : public SpvVisitor
	{
	public:
		SpvVmVisitor() = default;
		virtual SpvVmContext& GetActiveContext() = 0;
		
	public:
		void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts) override;
		int32 GetInstIndex(SpvId Inst) const;
		
		void Visit(SpvDebugLine* Inst) override;
		void Visit(SpvDebugScope* Inst) override;
		void Visit(SpvDebugDeclare* Inst) override;
		
		void Visit(SpvOpFunctionParameter* Inst) override;
		void Visit(SpvOpFunctionCall* Inst) override;
		void Visit(SpvOpVariable* Inst) override;
		void Visit(SpvOpLabel* Inst) override;
		void Visit(SpvOpLoad* Inst) override;
		void Visit(SpvOpStore* Inst) override;
		void Visit(SpvOpCompositeConstruct* Inst) override;
		void Visit(SpvOpAccessChain* Inst) override;
		void Visit(SpvOpIEqual* Inst) override;
		void Visit(SpvOpINotEqual* Inst) override;

	protected:
		bool AnyError{};
		const TArray<TUniquePtr<SpvInstruction>>* Insts;
	};

	TArray<uint8> GetPointerValue(SpvPointer* InPointer, SpvVariableDesc* PointeeDesc);
	void WritePointerValue(SpvPointer* InPointer, SpvVariableDesc* PointeeDesc, const TArray<uint8>& ValueToStore, SpvVariableChange* OutVariableChange = nullptr);
}
