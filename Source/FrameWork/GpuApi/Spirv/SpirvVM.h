#pragma once
#include "SpirvParser.h"

namespace FW
{
	TArray<uint8> GetPointerValue(SpvPointer* InPointer, SpvVariableDesc* PointeeDesc);
	void WritePointerValue(SpvPointer* InPointer, SpvVariableDesc* PointeeDesc, const TArray<uint8>& ValueToStore);

	struct SpvVariableChange
	{
		
	};

	struct SpvLexicalScopeChange
	{
		SpvLexicalScope* PreScope{};
		SpvLexicalScope* NewScope{};
	};

	struct SpvDebugState
	{
		uint32 LineNumber;
		SpvLexicalScopeChange ScopeChange;
		TArray<SpvVariableChange> VarChanges;
		FString UbError;
	};

	struct SpvRecordedInfo
	{
		TMap<SpvId, SpvVariable> AllVariables;
		TMap<SpvId, SpvVariableDesc*> AllVariableDescMap;
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
		TMap<SpvId, SpvVariableDesc*> VariableDescMap;
	};

	struct ThreadState
	{
		int32 InstIndex;
		int32 NextInstIndex;
		TArray<SpvVmFrame> StackFrames;
		
		//Finalized information for external use
		SpvRecordedInfo RecordedInfo;
	};

	struct PixelThreadState : ThreadState
	{
		Vector4f FragCoord;
	};

	struct SpvVmPixelContext : SpvMetaContext
	{
		int32 DebugIndex;
		TArray<SpvVmBinding> Bindings;
		std::array<PixelThreadState,4> Quad;
		int32 CurActiveIndex;
	};

	class FRAMEWORK_API SpvVmPixelVisitor : public SpvVisitor
	{
	public:
		SpvVmPixelVisitor(SpvVmPixelContext& InContext);
		
	public:
		void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts) override;
		void ParseQuad(int32 QuadIndex, int32 StopIndex = INDEX_NONE);
		
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

	private:
		bool AnyError{};
		const TArray<TUniquePtr<SpvInstruction>>* Insts;
		SpvVmPixelContext& Context;
	};
}
